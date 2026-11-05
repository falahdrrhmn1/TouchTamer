// TouchTamer: touchpad gestures and window hotkeys for Windows.
// Plain Win32, no .NET or scripting runtime.
//
// Gestures work in every application because they are read directly from the touchpad
// through Raw Input:
//   Ctrl+Shift + two-finger swipe on the left half of the touchpad  -> brightness
//   Ctrl+Shift + two-finger swipe on the right half of the touchpad -> volume
//   Five-finger swipe                                               -> move the window under the cursor
// Hotkeys:
//   Alt+Up / Alt+Down    -> step window size (full screen, 95%, 90%, 75%, 40%)
//   Alt+Right / Alt+Left -> widen / narrow the active window
// Settings live in TouchTamer.ini, next to the executable.

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <objidl.h>
#include <propidl.h>
#include <gdiplus.h>
#include <wbemidl.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <dwmapi.h>
#include "gesture.h"
#include <stdio.h>
#include <stdarg.h>

using namespace Gdiplus;

#ifdef TOUCHTAMER_TEST
#define APP_MUTEX     L"Local\\TouchTamer_test_single"
#define APP_WNDCLASS  L"TouchTamerHudTest"
#else
#define APP_MUTEX     L"Local\\TouchTamer_single"
#define APP_WNDCLASS  L"TouchTamerHud"
#endif

// ---------------------------------------------------------------- settings
static int   g_levelEnabled  = 1;      // 1 = modifier + two-finger swipe changes brightness and volume
static DWORD g_gestureMod    = 5;      // bit0=Ctrl, bit1=Alt, bit2=Shift
static int   g_fullHeightPct = 60;     // percent change for a swipe across the full touchpad height
static int   g_moveEnabled   = 1;      // 1 = five-finger swipe moves the window under the cursor
static int   g_moveSpeedPct  = 150;    // 100 = a swipe across the touchpad width moves one screen width
static int   g_sizeHotkeys   = 1;      // 1 = Alt+Up and Alt+Down
static int   g_widthHotkeys  = 1;      // 1 = Alt+Right and Alt+Left
static int   g_hideAfterMs   = 850;
static int   g_hudAlpha      = 245;
static int   g_volNativeOsd  = 0;      // 1 = use the Windows volume overlay instead of the HUD

// ---------------------------------------------------------------- log (only when debug=1)
static int g_debug = 0;

static void Log(const char* fmt, ...)
{
    if (!g_debug) return;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf) - 2, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    buf[n++] = '\n';

    static wchar_t path[MAX_PATH] = {0};
    if (!path[0]) {
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        wchar_t* dot = wcsrchr(path, L'.');
        if (dot) wcscpy(dot, L".log");
    }
    HANDLE h = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD wrote = 0;
    WriteFile(h, buf, (DWORD)n, &wrote, nullptr);
    CloseHandle(h);
}

static void LoadConfig(const wchar_t* iniPath)
{
    wchar_t buf[128];
    auto readMod = [&](const wchar_t* key, DWORD def) -> DWORD {
        GetPrivateProfileStringW(L"touchtamer", key, L"", buf, 128, iniPath);
        if (!buf[0]) return def;
        DWORD m = 0;
        if (wcsstr(buf, L"ctrl"))  m |= 1;
        if (wcsstr(buf, L"alt"))   m |= 2;
        if (wcsstr(buf, L"shift")) m |= 4;
        return m ? m : def;
    };
    g_levelEnabled  = GetPrivateProfileIntW(L"touchtamer", L"level_gesture", 1, iniPath);
    g_gestureMod    = readMod(L"gesture_mod", 5);
    g_sizeHotkeys   = GetPrivateProfileIntW(L"touchtamer", L"size_hotkeys", 1, iniPath);
    g_widthHotkeys  = GetPrivateProfileIntW(L"touchtamer", L"width_hotkeys", 1, iniPath);
    g_fullHeightPct = GetPrivateProfileIntW(L"touchtamer", L"full_height_pct", 60, iniPath);
    g_moveEnabled   = GetPrivateProfileIntW(L"touchtamer", L"move_gesture", 1, iniPath);
    g_moveSpeedPct  = GetPrivateProfileIntW(L"touchtamer", L"move_speed_pct", 150, iniPath);
    g_hideAfterMs  = GetPrivateProfileIntW(L"touchtamer", L"hide_after_ms", 850, iniPath);
    g_hudAlpha     = GetPrivateProfileIntW(L"touchtamer", L"alpha", 245, iniPath);
    g_debug        = GetPrivateProfileIntW(L"touchtamer", L"debug", 0, iniPath);
    g_volNativeOsd = GetPrivateProfileIntW(L"touchtamer", L"volume_native_osd", 0, iniPath);
    if (g_hudAlpha < 40)  g_hudAlpha = 40;
    if (g_hudAlpha > 255) g_hudAlpha = 255;
    if (g_moveSpeedPct < 10) g_moveSpeedPct = 10;
}

// ---------------------------------------------------------------- global state
#define WM_APP_HUD   (WM_APP + 1)

static HWND      g_hud;
static HINSTANCE g_hinst;
static int       g_dpi = 96;
static int       g_target = 1;         // 1 = brightness, 2 = volume
static int       g_percent = 50;
static float     g_fade = 1.0f;
static bool      g_visible = false;
static ULONGLONG g_lastShown = 0;

static CRITICAL_SECTION g_cs;
static double    g_accBright = 0;       // percent not applied yet
static double    g_accVol = 0;
static HANDLE    g_workEvent = NULL;

// ---------------------------------------------------------------- touchpad gestures
// Physical touchpad scrolling does not reach WH_MOUSE_LL while some applications are in front,
// such as Firefox, but Raw Input touchpad reports always arrive. Gestures are read from those.
static PadFrame    g_padFrame;
static PadGesture  g_pad;
static MoveGesture g_moveGesture;
static HANDLE g_padDev = nullptr;
static BYTE   g_padPrepBuf[16384];
static PHIDP_PREPARSED_DATA g_padPrep = nullptr;
static USHORT g_padLinks[PadFrame::MAX_CONTACTS];
static int    g_padLinkN = 0;
static USHORT g_padCountLink = 0;
static long   g_padXMax = 0;
static long   g_padYMax = 0;

static bool GestureModsDown()
{
    if ((g_gestureMod & 1) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) return false;
    if ((g_gestureMod & 2) && !(GetAsyncKeyState(VK_MENU) & 0x8000))    return false;
    if ((g_gestureMod & 4) && !(GetAsyncKeyState(VK_SHIFT) & 0x8000))   return false;
    return true;
}

static void AddGestureChange(int target, double pct)
{
    EnterCriticalSection(&g_cs);
    if (target == 1) g_accBright += pct;
    else             g_accVol += pct;
    LeaveCriticalSection(&g_cs);
    SetEvent(g_workEvent);
}

static bool LoadPadDevice(HANDLE dev)
{
    if (dev == g_padDev && g_padPrep) return true;
    g_padPrep = nullptr;
    g_padLinkN = 0;

    UINT size = 0;
    GetRawInputDeviceInfoW(dev, RIDI_PREPARSEDDATA, nullptr, &size);
    if (!size || size > sizeof(g_padPrepBuf)) return false;
    if (GetRawInputDeviceInfoW(dev, RIDI_PREPARSEDDATA, g_padPrepBuf, &size) == (UINT)-1) return false;
    PHIDP_PREPARSED_DATA prep = (PHIDP_PREPARSED_DATA)g_padPrepBuf;

    HIDP_CAPS caps;
    if (HidP_GetCaps(prep, &caps) != HIDP_STATUS_SUCCESS) return false;
    HIDP_VALUE_CAPS vc[64];
    USHORT nv = caps.NumberInputValueCaps;
    if (nv > 64) nv = 64;
    if (HidP_GetValueCaps(HidP_Input, vc, &nv, prep) != HIDP_STATUS_SUCCESS) return false;

    long xMax = 0, yMax = 0;
    g_padCountLink = 0;
    for (USHORT i = 0; i < nv; ++i) {
        USAGE u = vc[i].IsRange ? vc[i].Range.UsageMin : vc[i].NotRange.Usage;
        if (vc[i].UsagePage == 0x0D && u == 0x54) g_padCountLink = vc[i].LinkCollection;
        if (vc[i].UsagePage != 0x01) continue;
        if (u == 0x30 && g_padLinkN < PadFrame::MAX_CONTACTS) {
            g_padLinks[g_padLinkN++] = vc[i].LinkCollection;
            xMax = vc[i].LogicalMax;
        } else if (u == 0x31) {
            yMax = vc[i].LogicalMax;
        }
    }
    if (!g_padLinkN || xMax <= 0 || yMax <= 0) return false;

    g_pad.SetXMax(xMax);
    g_padXMax = xMax;
    g_padYMax = yMax;
    g_padPrep = prep;
    g_padDev = dev;
    Log("touchpad links=%d xMax=%ld yMax=%ld", g_padLinkN, xMax, yMax);
    return true;
}

// ---------------------------------------------------------------- window operations
// Touchpad reports are read on the UI thread and windows are changed on this thread. Moves use
// asynchronous SetWindowPos, at most once per display frame, so a slow application cannot hold
// up the touchpad. The thread sleeps completely between gestures.
static HANDLE g_moveEvent = NULL;
static bool   g_moveBegin = false, g_moveEnd = false;
static double g_moveDX = 0, g_moveDY = 0;
static double g_movePxPerCount = 1;

enum { HOTKEY_UP = 1, HOTKEY_DOWN, HOTKEY_RIGHT, HOTKEY_LEFT };
static int g_hotkeyQueue[16];
static int g_hotkeyCount = 0;

static void QueueHotkey(int id)
{
    EnterCriticalSection(&g_cs);
    if (g_hotkeyCount < 16) g_hotkeyQueue[g_hotkeyCount++] = id;
    LeaveCriticalSection(&g_cs);
    SetEvent(g_moveEvent);
}

static void MovePost(bool begin, bool end, double dx, double dy)
{
    EnterCriticalSection(&g_cs);
    if (begin) {               // leftovers of the previous gesture must not carry over
        g_moveDX = g_moveDY = 0;
        g_moveEnd = false;
        g_moveBegin = true;
    }
    if (end) g_moveEnd = true;
    g_moveDX += dx;
    g_moveDY += dy;
    LeaveCriticalSection(&g_cs);
    SetEvent(g_moveEvent);
}

static bool GetMonitorRects(POINT p, RECT* monitor, RECT* work)
{
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(MonitorFromPoint(p, MONITOR_DEFAULTTONEAREST), &mi)) return false;
    if (monitor) *monitor = mi.rcMonitor;
    if (work) *work = mi.rcWork;
    return true;
}

static bool IsShellWindow(HWND h, wchar_t* cls, int clsLen)
{
    cls[0] = 0;
    GetClassNameW(h, cls, clsLen);
    static const wchar_t* const shell[] = {
        L"Progman", L"WorkerW", L"Shell_TrayWnd", L"Shell_SecondaryTrayWnd",
        L"Windows.UI.Core.CoreWindow", L"XamlExplorerHostIslandWindow"
    };
    for (const wchar_t* name : shell)
        if (!wcscmp(cls, name)) return true;
    return false;
}

// Only ordinary application windows: not the desktop, the taskbar, or borderless full screen.
static HWND PickMoveTarget(POINT cur)
{
    HWND h = WindowFromPoint(cur);
    if (h) h = GetAncestor(h, GA_ROOT);
    if (!h || h == g_hud || !IsWindowVisible(h) || IsIconic(h) || IsHungAppWindow(h)) return nullptr;

    wchar_t cls[64];
    if (IsShellWindow(h, cls, 64)) { Log("move ignored, shell window: %ls", cls); return nullptr; }

    LONG_PTR style = GetWindowLongPtrW(h, GWL_STYLE);
    if ((style & WS_CAPTION) != WS_CAPTION && !(style & WS_THICKFRAME)) {
        Log("move ignored, no frame: %ls", cls);
        return nullptr;
    }
    RECT r, monitor;
    if (!GetWindowRect(h, &r) || !GetMonitorRects(cur, &monitor, nullptr)) return nullptr;
    if (EqualRect(&r, &monitor)) {     // F11, video, games. A maximized window overshoots the monitor by a few pixels.
        Log("move ignored, full screen: %ls", cls);
        return nullptr;
    }
    Log("move target: %ls", cls);
    return h;
}

static void RaiseWindow(HWND h)
{
    if (GetForegroundWindow() == h) return;
    INPUT none = {};                   // an empty input makes Windows allow SetForegroundWindow
    none.type = INPUT_MOUSE;
    SendInput(1, &none, sizeof(none));
    SetForegroundWindow(h);
    if (GetForegroundWindow() != h)
        SetWindowPos(h, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
}

// Leave the maximized state and go straight to the target rectangle. Resizing a maximized window
// without this keeps it flagged as maximized, which renders it broken.
static void LeaveMaximized(HWND h, const RECT& work, int x, int y, int w, int height)
{
    WINDOWPLACEMENT wp = {};
    wp.length = sizeof(wp);
    if (GetWindowPlacement(h, &wp)) {
        wp.flags = 0;
        wp.showCmd = SW_RESTORE;
        wp.rcNormalPosition.left = x - work.left;      // WINDOWPLACEMENT is relative to the work area
        wp.rcNormalPosition.top = y - work.top;
        wp.rcNormalPosition.right = x - work.left + w;
        wp.rcNormalPosition.bottom = y - work.top + height;
        SetWindowPlacement(h, &wp);
    }
    SetWindowPos(h, nullptr, x, y, w, height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

// Return a maximized window to its normal size, keeping the cursor inside the window.
static void RestoreForMove(HWND h, POINT cur)
{
    RECT work;
    WINDOWPLACEMENT wp = {};
    wp.length = sizeof(wp);
    if (!GetMonitorRects(cur, nullptr, &work) || !GetWindowPlacement(h, &wp)) return;

    int workW = work.right - work.left, workH = work.bottom - work.top;
    int w = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
    int height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
    if (w >= workW || height >= workH) {   // its normal size is as large as the screen
        w = workW * 3 / 4;
        height = workH * 3 / 4;
    }
    int x = cur.x - MulDiv(cur.x - work.left, w, workW);
    int y = cur.y - MulDiv(cur.y - work.top, height, workH);
    LeaveMaximized(h, work, x, y, w, height);
}

// The top of the window stays visible and reachable, so a window can never get lost off screen.
static void ClampToWork(POINT cursor, int winW, double* x, double* y)
{
    RECT work;
    if (!GetMonitorRects(cursor, nullptr, &work)) return;
    const int keep = 120;
    double minX = work.left + keep - winW, maxX = work.right - keep;
    double minY = work.top, maxY = work.bottom - 40;
    if (*x < minX) *x = minX;
    if (*x > maxX) *x = maxX;
    if (*y < minY) *y = minY;
    if (*y > maxY) *y = maxY;
}

// ---------------------------------------------------------------- window hotkeys
static bool CanResizeActiveWindow(HWND h)
{
    wchar_t cls[64];
    if (!h || IsShellWindow(h, cls, 64) || IsIconic(h) || IsHungAppWindow(h)) return false;
    return (GetWindowLongPtrW(h, GWL_STYLE) & WS_THICKFRAME) != 0;
}

static bool GetWindowWorkArea(HWND h, RECT* work)
{
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &mi)) return false;
    *work = mi.rcWork;
    return true;
}

// Levels: full screen (a real maximize), then a share of the work area, centered.
// Alt+Up at the top goes back to the lowest level. Alt+Down at the lowest level stays there.
static void StepWindowSize(HWND h, int step)
{
    static const double levels[] = { 0, 0.95, 0.90, 0.75, 0.40 };
    const int last = 4;
    RECT work;
    if (!GetWindowWorkArea(h, &work)) return;
    int workW = work.right - work.left, workH = work.bottom - work.top;
    bool zoomed = IsZoomed(h) != 0;

    int current = 0;
    if (!zoomed) {
        RECT r;
        if (!GetWindowRect(h, &r)) return;
        double best = 1e12;
        for (int i = 1; i <= last; ++i) {
            double diff = (r.right - r.left) - workW * levels[i];
            if (diff < 0) diff = -diff;
            if (diff < best) { best = diff; current = i; }
        }
    }

    int target = current + step;
    if (target < 0) target = last;
    if (target > last) target = last;
    if (target == 0) {
        ShowWindowAsync(h, SW_MAXIMIZE);
        return;
    }

    int w = (int)(workW * levels[target] + 0.5);
    int height = (int)(workH * levels[target] + 0.5);
    int x = work.left + (workW - w) / 2;
    int y = work.top + (workH - height) / 2;
    if (zoomed) LeaveMaximized(h, work, x, y, w, height);
    else SetWindowPos(h, nullptr, x, y, w, height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

static void AdjustWindowWidth(HWND h, int delta)
{
    const int minWidth = 300;
    RECT work;
    if (!GetWindowWorkArea(h, &work)) return;
    if (IsZoomed(h)) {
        if (delta > 0) return;                     // already as wide as the screen
        LeaveMaximized(h, work, work.left, work.top, work.right - work.left + delta, work.bottom - work.top);
        return;
    }
    RECT r;
    if (!GetWindowRect(h, &r)) return;
    int w = r.right - r.left;
    int newW = w + delta;
    if (delta > 0) {
        int room = work.right - r.left;
        if (newW > room) newW = room > w ? room : w;   // a window near the edge must not shrink
    } else if (newW < minWidth) {
        newW = w < minWidth ? w : minWidth;
    }
    SetWindowPos(h, nullptr, r.left, r.top, newW, r.bottom - r.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

static void RunHotkey(int id)
{
    HWND h = GetForegroundWindow();
    if (!CanResizeActiveWindow(h)) return;
    switch (id) {
    case HOTKEY_UP:    StepWindowSize(h, -1); break;
    case HOTKEY_DOWN:  StepWindowSize(h, 1); break;
    case HOTKEY_RIGHT: AdjustWindowWidth(h, 40); break;
    case HOTKEY_LEFT:  AdjustWindowWidth(h, -40); break;
    }
}

static DWORD WINAPI MoverProc(LPVOID)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    const UINT moveFlags = SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_ASYNCWINDOWPOS;
    HWND t = nullptr;
    double wantX = 0, wantY = 0;
    int appliedX = 0, appliedY = 0, seenX = 0, seenY = 0, winW = 0;
    ULONGLONG sentAt = 0;
    bool ending = false;

    for (;;) {
        WaitForSingleObject(g_moveEvent, INFINITE);
        for (;;) {
            EnterCriticalSection(&g_cs);
            bool begin = g_moveBegin, end = g_moveEnd;
            double dx = g_moveDX, dy = g_moveDY;
            g_moveBegin = g_moveEnd = false;
            g_moveDX = g_moveDY = 0;
            int hotkeys[16];
            int hotkeyN = g_hotkeyCount;
            for (int i = 0; i < hotkeyN; ++i) hotkeys[i] = g_hotkeyQueue[i];
            g_hotkeyCount = 0;
            LeaveCriticalSection(&g_cs);

            for (int i = 0; i < hotkeyN; ++i) RunHotkey(hotkeys[i]);

            if (begin) {
                ending = false;
                POINT cur;
                t = GetCursorPos(&cur) ? PickMoveTarget(cur) : nullptr;
                if (t) {
                    RaiseWindow(t);
                    if (IsZoomed(t)) RestoreForMove(t, cur);
                    RECT r0;
                    if (GetWindowRect(t, &r0)) {
                        wantX = appliedX = seenX = r0.left;
                        wantY = appliedY = seenY = r0.top;
                        winW = r0.right - r0.left;
                    } else {
                        t = nullptr;
                    }
                }
            }
            if (end) ending = true;
            if (!t) break;

            RECT r;
            if (!GetWindowRect(t, &r)) { t = nullptr; break; }   // the window was closed

            // The cursor follows the position the window actually reached.
            POINT p;
            if (GetCursorPos(&p) && (r.left != seenX || r.top != seenY)) {
                p.x += r.left - seenX;
                p.y += r.top - seenY;
                SetCursorPos(p.x, p.y);
                seenX = r.left;
                seenY = r.top;
            }

            // Send a new request only after the previous one finished, so a slow application
            // does not build up a queue and the window does not lag behind after fingers lift.
            bool pending = r.left != appliedX || r.top != appliedY;
            if (pending && GetTickCount64() - sentAt > 500) {    // it will not move, e.g. an administrator window
                Log("move stopped: window does not respond");
                t = nullptr;
                break;
            }

            wantX += dx;
            wantY += dy;
            ClampToWork(p, winW, &wantX, &wantY);
            int x = (int)(wantX < 0 ? wantX - 0.5 : wantX + 0.5);
            int y = (int)(wantY < 0 ? wantY - 0.5 : wantY + 0.5);
            if (!pending && (x != appliedX || y != appliedY)) {
                if (!SetWindowPos(t, nullptr, x, y, 0, 0, moveFlags)) { t = nullptr; break; }
                appliedX = x;
                appliedY = y;
                sentAt = GetTickCount64();
                pending = true;
            }

            if (ending && !pending) { t = nullptr; break; }
            if (!pending && dx == 0 && dy == 0 && !begin && !end) break;
            DwmFlush();                // follow the display rate
            if (pending) Sleep(1);     // DwmFlush can return at once while the screen is idle
        }
    }
    return 0;
}

static void HandlePadReport(BYTE* rep, ULONG len)
{
    ULONG count = 0;
    if (HidP_GetUsageValue(HidP_Input, 0x0D, g_padCountLink, 0x54, &count, g_padPrep, (PCHAR)rep, len) != HIDP_STATUS_SUCCESS)
        count = 0;

    PadContact c[PadFrame::MAX_CONTACTS];
    int n = 0;
    for (int i = 0; i < g_padLinkN; ++i) {
        USHORT link = g_padLinks[i];
        ULONG x = 0, y = 0, id = 0;
        if (HidP_GetUsageValue(HidP_Input, 0x01, link, 0x30, &x, g_padPrep, (PCHAR)rep, len) != HIDP_STATUS_SUCCESS) continue;
        if (HidP_GetUsageValue(HidP_Input, 0x01, link, 0x31, &y, g_padPrep, (PCHAR)rep, len) != HIDP_STATUS_SUCCESS) continue;
        HidP_GetUsageValue(HidP_Input, 0x0D, link, 0x51, &id, g_padPrep, (PCHAR)rep, len);
        USAGE usages[16];
        ULONG nu = 16;
        bool tip = false;
        if (HidP_GetUsages(HidP_Input, 0x0D, link, usages, &nu, g_padPrep, (PCHAR)rep, len) == HIDP_STATUS_SUCCESS)
            for (ULONG k = 0; k < nu; ++k) if (usages[k] == 0x42) tip = true;
        c[n].id = id;
        c[n].tip = tip;
        c[n].x = (long)x;
        c[n].y = (long)y;
        ++n;
    }

    if (!g_padFrame.Push(count, c, n)) return;
    const PadContact* frame = g_padFrame.Contacts();
    int frameN = g_padFrame.Count();

    if (g_levelEnabled) {
        int target = 0;
        long dy = 0;
        int before = g_pad.ActiveTarget();
        bool changed = g_pad.OnFrame(frame, frameN, GestureModsDown, &target, &dy);
        int after = g_pad.ActiveTarget();
        if (after != before) Log("gesture target=%d", after);
        if (changed) AddGestureChange(target, (double)dy * g_fullHeightPct / g_padYMax);
    }

    if (g_moveEnabled) {
        double mx, my;
        MoveGesture::Result res = g_moveGesture.OnFrame(frame, frameN, GetTickCount64(), &mx, &my);
        if (res == MoveGesture::STARTED)
            g_movePxPerCount = (double)GetSystemMetrics(SM_CXSCREEN) * g_moveSpeedPct / 100.0 / g_padXMax;
        if (res != MoveGesture::NONE)
            MovePost(res == MoveGesture::STARTED, res == MoveGesture::ENDED,
                     mx * g_movePxPerCount, my * g_movePxPerCount);
    }
}

static void HandlePadInput(HRAWINPUT h)
{
    alignas(8) static BYTE buf[4096];
    UINT size = sizeof(buf);
    if (GetRawInputData(h, RID_INPUT, buf, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1) return;
    RAWINPUT* ri = (RAWINPUT*)buf;
    if (ri->header.dwType != RIM_TYPEHID || !LoadPadDevice(ri->header.hDevice)) return;
    for (DWORD i = 0; i < ri->data.hid.dwCount; ++i)
        HandlePadReport(ri->data.hid.bRawData + i * ri->data.hid.dwSizeHid, ri->data.hid.dwSizeHid);
}

// ---------------------------------------------------------------- hook
// Swallows the wheel while the gesture keys are held, in windows whose wheel events reach the
// hook, so the application underneath does not zoom. It is not used to detect gestures.
static HHOOK g_hook = NULL;

static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && (wParam == WM_MOUSEWHEEL || wParam == 0x020E /*MOUSEHWHEEL*/) && GestureModsDown())
        return 1;
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

static DWORD WINAPI HookThreadProc(LPVOID)
{
    g_hook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, nullptr, 0);
    Log("mouse hook %s", g_hook ? "ok" : "failed");
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (g_hook) UnhookWindowsHookEx(g_hook);
    return 0;
}

// ---------------------------------------------------------------- WMI brightness
class Brightness {
public:
    IWbemServices* svc = nullptr;
    IWbemClassObject* inst = nullptr;
    IWbemClassObject* inSig = nullptr;
    int current = -1;

    // External monitors and desktops have no WMI brightness: the gesture then does nothing.
    bool Available(ULONGLONG now)
    {
        static ULONGLONG lastTry = 0;
        if (inSig) return true;
        if (lastTry && now - lastTry < 10000) return false;
        lastTry = now;
        return Connect();
    }

    bool Connect()
    {
        Release();
        IWbemLocator* loc = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_IWbemLocator, (void**)&loc);
        if (FAILED(hr)) return false;

        BSTR ns = SysAllocString(L"ROOT\\WMI");
        hr = loc->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &svc);
        SysFreeString(ns);
        loc->Release();
        if (FAILED(hr) || !svc) return false;

        CoSetProxyBlanket(svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                          RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                          nullptr, EOAC_NONE);

        IEnumWbemClassObject* en = nullptr;
        BSTR lang = SysAllocString(L"WQL");
        BSTR q = SysAllocString(L"SELECT * FROM WmiMonitorBrightnessMethods");
        hr = svc->ExecQuery(lang, q, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                            nullptr, &en);
        SysFreeString(lang);
        SysFreeString(q);
        if (FAILED(hr) || !en) return false;

        ULONG got = 0;
        hr = en->Next(WBEM_INFINITE, 1, &inst, &got);
        en->Release();
        if (FAILED(hr) || got == 0) { inst = nullptr; return false; }

        // method parameters
        BSTR cls = SysAllocString(L"WmiMonitorBrightnessMethods");
        IWbemClassObject* klass = nullptr;
        hr = svc->GetObject(cls, 0, nullptr, &klass, nullptr);
        SysFreeString(cls);
        if (FAILED(hr) || !klass) return false;
        BSTR meth = SysAllocString(L"WmiSetBrightness");
        hr = klass->GetMethod(meth, 0, &inSig, nullptr);
        SysFreeString(meth);
        Log("GetMethod hr=0x%08lX inSig=%p", (unsigned long)hr, (void*)inSig);
        if (inSig) {
            SAFEARRAY* names = nullptr;
            if (SUCCEEDED(inSig->GetNames(nullptr, WBEM_FLAG_NONSYSTEM_ONLY, nullptr, &names))
                && names) {
                LONG lo = 0, hi = -1;
                SafeArrayGetLBound(names, 1, &lo);
                SafeArrayGetUBound(names, 1, &hi);
                for (LONG i = lo; i <= hi; ++i) {
                    BSTR nm = nullptr;
                    SafeArrayGetElement(names, &i, &nm);
                    VARIANT v; VariantInit(&v);
                    CIMTYPE ct = 0;
                    inSig->Get(nm, 0, &v, &ct, nullptr);
                    Log("   param %ls cimtype=%d vt=%d", nm, (int)ct, v.vt);
                    VariantClear(&v);
                    SysFreeString(nm);
                }
                SafeArrayDestroy(names);
            }
        }
        klass->Release();

        ReadCurrent();
        Log("Connect ok, current=%d inSig=%p", current, (void*)inSig);
        return inSig != nullptr;
    }

    void Release()
    {
        if (inSig) { inSig->Release(); inSig = nullptr; }
        if (inst)  { inst->Release();  inst = nullptr; }
        if (svc)   { svc->Release();   svc = nullptr; }
    }

    void ReadCurrent()
    {
        if (!svc) return;
        IEnumWbemClassObject* en = nullptr;
        BSTR lang = SysAllocString(L"WQL");
        BSTR q = SysAllocString(L"SELECT CurrentBrightness FROM WmiMonitorBrightness");
        if (SUCCEEDED(svc->ExecQuery(lang, q, WBEM_FLAG_FORWARD_ONLY |
                                     WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &en)) && en) {
            IWbemClassObject* o = nullptr;
            ULONG got = 0;
            if (SUCCEEDED(en->Next(WBEM_INFINITE, 1, &o, &got)) && got) {
                VARIANT v; VariantInit(&v);
                if (SUCCEEDED(o->Get(L"CurrentBrightness", 0, &v, nullptr, nullptr)))
                    current = (v.vt == VT_UI1) ? v.bVal : (int)v.lVal;
                VariantClear(&v);
                o->Release();
            }
            en->Release();
        }
        SysFreeString(lang);
        SysFreeString(q);
    }

    bool Set(int percent)
    {
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        if (!inSig || !svc) { if (!Connect()) return false; }
        IWbemClassObject* in = nullptr;
        if (FAILED(inSig->SpawnInstance(0, &in)) || !in) return false;

        VARIANT v; VariantInit(&v);
        v.vt = VT_I4; v.lVal = 1;          // WMI expects VT_I4 here even though the parameter is uint32
        HRESULT hrPut1 = in->Put(L"Timeout", 0, &v, 0);
        VariantInit(&v);
        v.vt = VT_UI1; v.bVal = (BYTE)percent;
        HRESULT hrPut2 = in->Put(L"Brightness", 0, &v, 0);
        Log("Put hr: Timeout=0x%08lX Brightness=0x%08lX",
            (unsigned long)hrPut1, (unsigned long)hrPut2);

        VARIANT path; VariantInit(&path);
        bool ok = false;
        if (SUCCEEDED(inst->Get(L"__PATH", 0, &path, nullptr, nullptr)) && path.vt == VT_BSTR) {
            BSTR meth = SysAllocString(L"WmiSetBrightness");
            HRESULT hr = svc->ExecMethod(path.bstrVal, meth, 0, nullptr, in, nullptr, nullptr);
            Log("ExecMethod(full) hr=0x%08lX", (unsigned long)hr);
            if (FAILED(hr)) {
                // try the relative path (without \server\ROOT\WMI:)
                const wchar_t* p = path.bstrVal;
                const wchar_t* colon = wcschr(p, L':');
                if (colon) {
                    hr = svc->ExecMethod(const_cast<wchar_t*>(colon + 1), meth, 0, nullptr,
                                         in, nullptr, nullptr);
                    Log("ExecMethod(rel) hr=0x%08lX path=%ls",
                        (unsigned long)hr, colon + 1);
                }
            }
            SysFreeString(meth);
            ok = SUCCEEDED(hr);
        }
        VariantClear(&path);
        in->Release();
        if (!ok) Connect();   // the WMI connection can die after being idle
        else current = percent;
        return ok;
    }
};

// ---------------------------------------------------------------- volume
class Volume {
public:
    IAudioEndpointVolume* ev = nullptr;

    void Reset()
    {
        if (ev) { ev->Release(); ev = nullptr; }
    }

    bool Init()
    {
        if (ev) return true;
        IMMDeviceEnumerator* en = nullptr;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                      CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&en);
        if (FAILED(hr)) return false;
        IMMDevice* dev = nullptr;
        hr = en->GetDefaultAudioEndpoint(eRender, eConsole, &dev);
        if (SUCCEEDED(hr) && dev) {
            hr = dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&ev);
            dev->Release();
            if (FAILED(hr)) ev = nullptr;
        }
        en->Release();
        return ev != nullptr;
    }

    int Get()
    {
        if (!Init()) return 0;
        float f = 0;
        if (FAILED(ev->GetMasterVolumeLevelScalar(&f))) return 0;
        return (int)(f * 100.0f + 0.5f);
    }

    void Set(int percent)
    {
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        if (!Init()) return;
        ev->SetMasterVolumeLevelScalar(percent / 100.0f, nullptr);
        BOOL mute = FALSE;
        ev->GetMute(&mute);
        if (percent == 0 && !mute)  ev->SetMute(TRUE, nullptr);
        if (percent > 0 && mute)    ev->SetMute(FALSE, nullptr);
    }
};

// ---------------------------------------------------------------- worker
static Brightness g_brightness;
static Volume     g_volume;
static double     g_level[2] = { -1, -1 };   // [0]=brightness, [1]=volume
static ULONGLONG  g_lastApply = 0;

static int ReadLevel(int target)
{
    if (target == 1) {
        if (g_brightness.current < 0) g_brightness.ReadCurrent();
        return g_brightness.current < 0 ? 50 : g_brightness.current;
    }
    return g_volume.Get();
}

static DWORD WINAPI WorkerProc(LPVOID)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
                         RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
    g_brightness.Connect();
    g_volume.Init();

    ULONGLONG lastApplyMs = 0;
    for (;;) {
        WaitForSingleObject(g_workEvent, INFINITE);

        // merge events that arrive in a row: at most one change per 30 ms
        ULONGLONG t = GetTickCount64();
        if (lastApplyMs && t - lastApplyMs < 30) Sleep((DWORD)(30 - (t - lastApplyMs)));
        lastApplyMs = GetTickCount64();

        double bd = 0, vd = 0;
        EnterCriticalSection(&g_cs);
        bd = g_accBright; g_accBright = 0;
        vd = g_accVol;    g_accVol = 0;
        LeaveCriticalSection(&g_cs);
        if (bd == 0 && vd == 0) continue;

        ULONGLONG now = GetTickCount64();
        int target = bd != 0 ? 1 : 2;
        double change = bd != 0 ? bd : vd;
        if (target == 1 && !g_brightness.Available(now)) continue;
        if (target == 2 && !g_volume.Init()) continue;
        static int lastTarget = 0;
        bool newGesture = now - g_lastApply > 500 || target != lastTarget;
        lastTarget = target;
        if (newGesture) {
            g_level[target - 1] = -1;
            // Read the device again only after a long idle period (over 10 seconds). Right after
            // use, trust the internal value, because WMI often reports the change late.
            if (now - g_lastApply > 10000) {
                if (target == 1) g_brightness.ReadCurrent();
                else             g_volume.Reset();
            }
        }
        g_lastApply = now;

        if (g_level[target - 1] < 0) g_level[target - 1] = ReadLevel(target);
        double lvl = g_level[target - 1] + change;
        if (lvl < 0) lvl = 0;
        if (lvl > 100) lvl = 100;
        g_level[target - 1] = lvl;

        int pct = (int)(lvl + 0.5);
        static int lastPct[2] = {-1, -1};
        if (newGesture) lastPct[target - 1] = -1;
        if (pct == lastPct[target - 1]) continue;      // not a full 1% yet: do not call WMI or COM
        lastPct[target - 1] = pct;

        if (target == 1) {
            if (!g_brightness.Set(pct)) continue;
        } else if (g_volNativeOsd) {
            // change the volume with media keys so the Windows overlay appears
            static double lastOsd = -1;
            if (lastOsd < 0 || newGesture) lastOsd = g_volume.Get();
            int steps = (int)((lvl - lastOsd) / 2.0);
            if (steps != 0) {
                WORD vk = steps > 0 ? VK_VOLUME_UP : VK_VOLUME_DOWN;
                for (int i = 0; i < (steps > 0 ? steps : -steps); ++i) {
                    keybd_event((BYTE)vk, 0, 0, 0);
                    keybd_event((BYTE)vk, 0, KEYEVENTF_KEYUP, 0);
                }
                lastOsd += steps * 2.0;
                pct = (int)(lastOsd + 0.5);
            } else {
                pct = (int)(lastOsd + 0.5);
            }
        } else {
            g_volume.Set(pct);
            int check = g_volume.Get();
            if (check != pct && pct > 0) {   // stale endpoint: get it again
                g_volume.Reset();
                g_volume.Set(pct);
            }
        }

        Log("apply target=%d change=%.2f -> %d%%", target, change, pct);
        if (!(target == 2 && g_volNativeOsd))
            PostMessageW(g_hud, WM_APP_HUD, (WPARAM)target, (LPARAM)pct);
    }
    return 0;
}

// ---------------------------------------------------------------- HUD
static int S(int v) { return MulDiv(v, g_dpi, 96); }

static void AddRoundRect(GraphicsPath& p, RectF r, float radius)
{
    float d = radius * 2;
    p.AddArc(r.X, r.Y, d, d, 180, 90);
    p.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
    p.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
    p.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90);
    p.CloseFigure();
}

static void DrawHud(int target, int pct, float alpha)
{
    int W = S(138), H = S(196);
    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = W;
    bi.bmiHeader.biHeight = -H;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bmp = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ old = SelectObject(mem, bmp);

    {
        Graphics g(mem);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintAntiAlias);
        g.Clear(Color(0, 0, 0, 0));

        BYTE A = (BYTE)(g_hudAlpha * alpha);
        Color panel(A, 20, 22, 28);
        Color barBg(A, 43, 47, 58);
        bool isBright = (target == 1);
        Color accent(A, isBright ? 255 : 90, isBright ? 204 : 169, isBright ? 102 : 255);
        Color label(A, 141, 148, 163);
        Color text(A, 233, 236, 242);

        GraphicsPath panelPath;
        AddRoundRect(panelPath, RectF((REAL)S(6), (REAL)S(6),
                                      (REAL)(W - S(12)), (REAL)(H - S(12))), (REAL)S(22));
        SolidBrush panelBrush(panel);
        g.FillPath(&panelBrush, &panelPath);

        // icon (Segoe MDL2 Assets)
        const wchar_t* glyph = isBright ? L"\xE706" : L"\xE767";
        FontFamily iconFam(L"Segoe MDL2 Assets");
        Font iconFont(&iconFam, (REAL)S(22), FontStyleRegular, UnitPixel);
        SolidBrush accentBrush(accent);
        StringFormat sfCenter;
        sfCenter.SetAlignment(StringAlignmentCenter);
        sfCenter.SetLineAlignment(StringAlignmentCenter);
        RectF iconRect(0, (REAL)S(28), (REAL)W, (REAL)S(30));
        g.DrawString(glyph, -1, &iconFont, iconRect, &sfCenter, &accentBrush);

        // label
        FontFamily uiFam(L"Segoe UI");
        Font labelFont(&uiFam, (REAL)S(9), FontStyleBold, UnitPixel);
        SolidBrush labelBrush(label);
        RectF labelRect(0, (REAL)S(56), (REAL)W, (REAL)S(16));
        g.DrawString(isBright ? L"BRIGHT" : L"VOLUME", -1, &labelFont, labelRect,
                     &sfCenter, &labelBrush);

        // bar
        REAL bx = (W - S(34)) / 2.0f;
        REAL bw = (REAL)S(34);
        REAL top = (REAL)S(78), bottom = (REAL)(H - S(52));
        GraphicsPath barPath;
        AddRoundRect(barPath, RectF(bx, top, bw, bottom - top), (REAL)S(11));
        SolidBrush barBrush(barBg);
        g.FillPath(&barBrush, &barPath);

        REAL innerH = (bottom - top) - S(8);
        REAL fillH = innerH * (pct / 100.0f);
        if (fillH > 1) {
            GraphicsPath fillPath;
            AddRoundRect(fillPath, RectF(bx + S(4), bottom - S(4) - fillH,
                                         bw - S(8), fillH), (REAL)S(8));
            g.FillPath(&accentBrush, &fillPath);
        }

        // percent
        wchar_t txt[16];
        swprintf(txt, 16, L"%d%%", pct);
        Font pctFont(&uiFam, (REAL)S(17), FontStyleBold, UnitPixel);
        SolidBrush textBrush(text);
        RectF pctRect(0, (REAL)(H - S(42)), (REAL)W, (REAL)S(26));
        g.DrawString(txt, -1, &pctFont, pctRect, &sfCenter, &textBrush);
    }

    POINT ptDst;
    RECT wa; SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    ptDst.x = (wa.left + wa.right) / 2 - W / 2;
    ptDst.y = wa.bottom - H - S(24);
    SIZE sz = { W, H };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(g_hud, screen, &ptDst, &sz, mem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
}

static void ShowHud(int target, int pct)
{
    g_target = target;
    g_percent = pct;
    g_fade = 1.0f;
    g_lastShown = GetTickCount64();
    DrawHud(target, pct, 1.0f);
    SetWindowPos(g_hud, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(g_hud, SW_SHOWNA);
    g_visible = true;
    SetTimer(g_hud, 1, 30, nullptr);
}

static LRESULT CALLBACK HudProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INPUT:
        HandlePadInput((HRAWINPUT)lp);
        return DefWindowProcW(hwnd, msg, wp, lp);
    case WM_APP_HUD:
        ShowHud((int)wp, (int)lp);
        return 0;
    case WM_HOTKEY:
        QueueHotkey((int)wp);
        return 0;
    case WM_TIMER:
        if (wp == 1) {
            ULONGLONG idle = GetTickCount64() - g_lastShown;
            if (idle < (ULONGLONG)g_hideAfterMs) return 0;
            g_fade -= 0.14f;
            if (g_fade <= 0.02f) {
                KillTimer(hwnd, 1);
                ShowWindow(hwnd, SW_HIDE);
                g_visible = false;
            } else {
                DrawHud(g_target, g_percent, g_fade);
            }
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

#ifdef TOUCHTAMER_TEST
// Exercises the worker without fingers: down then up again, so the final value is unchanged.
static DWORD WINAPI ApplyTestProc(LPVOID)
{
    Sleep(1500);
    AddGestureChange(1, -10.0); Sleep(700);
    AddGestureChange(1, +10.0); Sleep(700);
    AddGestureChange(2, -4.0);  Sleep(700);
    AddGestureChange(2, +4.0);  Sleep(700);
    PostMessageW(g_hud, WM_CLOSE, 0, 0);
    return 0;
}

// Exercises window moving without fingers, only on the window titled TOUCHTAMER_TEST_WINDOW.
// Usage: TouchTamer_test.exe --move-test=DX,DY[,STEPS]
// With many steps the direction flips every 120 steps, which is useful for measuring load.
static DWORD WINAPI MoveTestProc(LPVOID)
{
    int dx = 300, dy = 180, steps = 30;
    const wchar_t* arg = wcsstr(GetCommandLineW(), L"--move-test=");
    if (arg) {
        wchar_t* next = nullptr;
        dx = (int)wcstol(arg + 12, &next, 10);
        if (next && *next == L',') dy = (int)wcstol(next + 1, &next, 10);
        if (next && *next == L',') steps = (int)wcstol(next + 1, nullptr, 10);
    }
    Sleep(500);
    HWND t = FindWindowW(nullptr, L"TOUCHTAMER_TEST_WINDOW");
    if (t) {
        RECT r;
        GetWindowRect(t, &r);
        SetCursorPos((r.left + r.right) / 2, (r.top + r.bottom) / 2);
        Sleep(50);
        for (int i = 0; i < steps; ++i) {
            double sign = (i / 120) % 2 ? -1.0 : 1.0;
            MovePost(i == 0, false, sign * dx / steps, sign * dy / steps);
            Sleep(8);
        }
        MovePost(false, true, 0, 0);
        Sleep(500);
    }
    PostMessageW(g_hud, WM_CLOSE, 0, 0);
    return 0;
}
#endif

static void RegisterWindowHotkeys(HWND hwnd, UINT extraMods)
{
    const struct { int id; UINT vk; int enabled; } keys[] = {
        { HOTKEY_UP,    VK_UP,    g_sizeHotkeys },
        { HOTKEY_DOWN,  VK_DOWN,  g_sizeHotkeys },
        { HOTKEY_RIGHT, VK_RIGHT, g_widthHotkeys },
        { HOTKEY_LEFT,  VK_LEFT,  g_widthHotkeys },
    };
    for (const auto& k : keys) {
        if (k.enabled && !RegisterHotKey(hwnd, k.id, MOD_ALT | extraMods, k.vk))
            Log("hotkey %d not available, err=%lu", k.id, GetLastError());
    }
}

// ---------------------------------------------------------------- main
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    g_hinst = hInst;

    // `TouchTamer.exe --exit` asks the running copy to close.
    if (wcsstr(GetCommandLineW(), L"--exit")) {
        HWND running = FindWindowW(APP_WNDCLASS, nullptr);
        if (running) PostMessageW(running, WM_CLOSE, 0, 0);
        return 0;
    }

    // Window and cursor coordinates must be real screen coordinates at any display scale.
    typedef BOOL (WINAPI *SetDpiContext)(HANDLE);
    SetDpiContext setDpi = (SetDpiContext)(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"),
                                                                "SetProcessDpiAwarenessContext");
    if (setDpi) setDpi((HANDLE)-4);    // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2

    // only one running copy
    HANDLE mtx = CreateMutexW(nullptr, FALSE, APP_MUTEX);
    if (mtx && GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* dot = wcsrchr(exePath, L'.');
    if (dot) wcscpy(dot, L".ini");
    LoadConfig(exePath);

    if (g_debug) {
        wchar_t lp[MAX_PATH];
        GetModuleFileNameW(nullptr, lp, MAX_PATH);
        wchar_t* d2 = wcsrchr(lp, L'.');
        if (d2) wcscpy(d2, L".log");
        DeleteFileW(lp);
    }

    HDC dc = GetDC(nullptr);
    g_dpi = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(nullptr, dc);

    GdiplusStartupInput gsi;
    ULONG_PTR gdipToken = 0;
    GdiplusStartup(&gdipToken, &gsi, nullptr);

    InitializeCriticalSection(&g_cs);
    g_workEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    g_moveEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = HudProc;
    wc.hInstance = hInst;
    wc.lpszClassName = APP_WNDCLASS;
    RegisterClassExW(&wc);

    int W = S(138), H = S(196);
    g_hud = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT |
        WS_EX_TOPMOST,
        APP_WNDCLASS, L"TouchTamer", WS_POPUP, 0, 0, W, H,
        nullptr, nullptr, hInst, nullptr);
    if (!g_hud) return 1;

    if (g_levelEnabled || g_moveEnabled) {
        RAWINPUTDEVICE rid = { 0x0D, 0x05, RIDEV_INPUTSINK, g_hud };   // Digitizer / Touch Pad
        BOOL ok = RegisterRawInputDevices(&rid, 1, sizeof(rid));
        Log("raw touchpad %s err=%lu", ok ? "ok" : "failed", ok ? 0UL : GetLastError());
    }

    CreateThread(nullptr, 0, WorkerProc, nullptr, 0, nullptr);
    if (g_levelEnabled)
        CreateThread(nullptr, 0, HookThreadProc, nullptr, 0, nullptr);
    if (g_moveEnabled || g_sizeHotkeys || g_widthHotkeys)
        CreateThread(nullptr, 0, MoverProc, nullptr, 0, nullptr);

    UINT hotkeyExtraMods = 0;
#ifdef TOUCHTAMER_TEST
    // Uses different keys so the test can run next to another program that owns Alt+arrows.
    if (wcsstr(GetCommandLineW(), L"--test-hotkeys")) hotkeyExtraMods = MOD_CONTROL | MOD_SHIFT;
    if (wcsstr(GetCommandLineW(), L"--apply-test"))
        CreateThread(nullptr, 0, ApplyTestProc, nullptr, 0, nullptr);
    if (wcsstr(GetCommandLineW(), L"--move-test"))
        CreateThread(nullptr, 0, MoveTestProc, nullptr, 0, nullptr);
#endif
    RegisterWindowHotkeys(g_hud, hotkeyExtraMods);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    GdiplusShutdown(gdipToken);
    return 0;
}
