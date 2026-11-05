"""End-to-end test of the window hotkeys and window moving, on a test window only.

Build first with `build.cmd test`, then run from the repository root:
    python tests/e2e_windows.py

The test build uses Ctrl+Alt+Shift+arrow instead of Alt+arrow, so it can run next to another
program that owns Alt+arrow. Keys are sent only while the test window is confirmed active.
Windows are real and the cursor moves briefly, so leave the keyboard and touchpad alone.
"""
import ctypes as C
import ctypes.wintypes as W
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TESTWIN = os.path.join(ROOT, 'build', 'testwin.exe')
APP = os.path.join(ROOT, 'build', 'TouchTamer_test.exe')

u = C.WinDLL('user32', use_last_error=True)
u.FindWindowW.restype = W.HWND
u.FindWindowW.argtypes = [W.LPCWSTR, W.LPCWSTR]
u.GetForegroundWindow.restype = W.HWND
u.SetForegroundWindow.argtypes = [W.HWND]
u.IsZoomed.argtypes = [W.HWND]
u.GetWindowRect.argtypes = [W.HWND, C.POINTER(W.RECT)]
u.GetClientRect.argtypes = [W.HWND, C.POINTER(W.RECT)]
u.ShowWindow.argtypes = [W.HWND, C.c_int]
u.PostMessageW.argtypes = [W.HWND, W.UINT, W.WPARAM, W.LPARAM]
u.MonitorFromWindow.restype = W.HMONITOR
u.MonitorFromWindow.argtypes = [W.HWND, W.DWORD]
u.GetMonitorInfoW.argtypes = [W.HMONITOR, C.c_void_p]
u.SetWindowPos.argtypes = [W.HWND, W.HWND, C.c_int, C.c_int, C.c_int, C.c_int, W.UINT]
u.keybd_event.argtypes = [C.c_ubyte, C.c_ubyte, W.DWORD, C.c_size_t]
u.GetWindowLongPtrW.restype = C.c_ssize_t
u.GetWindowLongPtrW.argtypes = [W.HWND, C.c_int]
u.SetWindowLongPtrW.restype = C.c_ssize_t
u.SetWindowLongPtrW.argtypes = [W.HWND, C.c_int, C.c_ssize_t]
u.SetProcessDPIAware()

LEFT, UP, RIGHT, DOWN = 0x25, 0x26, 0x27, 0x28
VK_SHIFT, VK_CONTROL, VK_MENU = 0x10, 0x11, 0x12
SWP_NOSIZE, SWP_NOMOVE, SWP_NOZORDER, SWP_NOACTIVATE, SWP_FRAMECHANGED = 0x1, 0x2, 0x4, 0x10, 0x20
FAIL = []


class MI(C.Structure):
    _fields_ = [('cb', W.DWORD), ('mon', W.RECT), ('work', W.RECT), ('flags', W.DWORD)]


class MOUSE(C.Structure):
    _fields_ = [('dx', W.LONG), ('dy', W.LONG), ('data', W.DWORD), ('flags', W.DWORD),
                ('time', W.DWORD), ('extra', C.c_size_t)]


class INPUT(C.Structure):
    _fields_ = [('type', W.DWORD), ('mi', MOUSE), ('pad', C.c_byte * 8)]


def check(name, cond, got):
    print(f"  {'OK    ' if cond else 'FAILED'} {name}: {got}")
    if not cond:
        FAIL.append(name)


def near(a, b):
    return abs(a - b) <= 2


def activate(h):
    for _ in range(10):
        if u.GetForegroundWindow() == h:
            return True
        i = INPUT()
        u.SendInput(1, C.byref(i), C.sizeof(INPUT))
        u.SetForegroundWindow(h)
        time.sleep(0.1)
    return u.GetForegroundWindow() == h


def press(h, vk):
    if not activate(h):
        raise RuntimeError('the test window could not be activated, no keys were sent')
    u.keybd_event(VK_CONTROL, 0, 0, 0)
    u.keybd_event(VK_SHIFT, 0, 0, 0)
    u.keybd_event(VK_MENU, 0, 0, 0)
    u.keybd_event(vk, 0, 1, 0)
    u.keybd_event(vk, 0, 3, 0)
    u.keybd_event(VK_MENU, 0, 2, 0)
    u.keybd_event(VK_SHIFT, 0, 2, 0)
    u.keybd_event(VK_CONTROL, 0, 2, 0)
    time.sleep(0.45)


def state(h):
    r = W.RECT()
    u.GetWindowRect(h, C.byref(r))
    c = W.RECT()
    u.GetClientRect(h, C.byref(c))
    return {'zoom': bool(u.IsZoomed(h)), 'x': r.left, 'y': r.top, 'w': r.right - r.left,
            'h': r.bottom - r.top, 'cw': c.right}


def place(h, x, y, w, height):
    u.ShowWindow(h, 9)
    time.sleep(0.25)
    u.SetWindowPos(h, None, x, y, w, height, SWP_NOZORDER | SWP_NOACTIVATE)
    time.sleep(0.25)


def maximize(h):
    u.ShowWindow(h, 3)
    time.sleep(0.5)


def cursor():
    p = W.POINT()
    u.GetCursorPos(C.byref(p))
    return p.x, p.y


def start_hotkeys():
    p = subprocess.Popen([APP, '--test-hotkeys'])
    time.sleep(1.0)
    return p


def stop_app(p):
    subprocess.run([APP, '--exit'], timeout=10)
    p.wait(timeout=10)


def test_size_hotkeys(h, wa):
    ww, wh = wa.right - wa.left, wa.bottom - wa.top
    print('window size')
    place(h, 300, 200, 800, 600)
    press(h, UP)
    s = state(h)
    check('800 wide -> 75%', not s['zoom'] and near(s['w'], ww * .75), s)
    press(h, UP)
    s = state(h)
    check('-> 90%', not s['zoom'] and near(s['w'], ww * .90), s)
    press(h, UP)
    s = state(h)
    check('-> 95%', not s['zoom'] and near(s['w'], ww * .95) and near(s['h'], wh * .95), s)
    check('95% is centered and inside the work area',
          near(s['x'], wa.left + (ww - s['w']) / 2) and near(s['y'], wa.top + (wh - s['h']) / 2)
          and s['y'] + s['h'] <= wa.bottom, s)
    press(h, UP)
    s = state(h)
    check('-> real full screen', s['zoom'], s)
    press(h, UP)
    s = state(h)
    check('up again -> back to the lowest level, 40%', not s['zoom'] and near(s['w'], ww * .40), s)
    press(h, DOWN)
    s = state(h)
    check('down at 40% stays at 40%', not s['zoom'] and near(s['w'], ww * .40), s)
    maximize(h)
    check('maximized by hand', state(h)['zoom'], state(h))
    press(h, DOWN)
    s = state(h)
    check('maximized + down leaves full screen at 95%',
          not s['zoom'] and near(s['w'], ww * .95) and near(s['h'], wh * .95), s)
    check('the content follows the size', s['w'] - 40 < s['cw'] < s['w'], s)
    press(h, UP)
    check('95% -> full screen again', state(h)['zoom'], state(h))


def test_width_hotkeys(h, wa):
    ww, wh = wa.right - wa.left, wa.bottom - wa.top
    print('window width')
    place(h, 300, 200, 800, 600)
    press(h, RIGHT)
    s = state(h)
    check('right +40', s['w'] == 840 and s['x'] == 300 and s['y'] == 200 and s['h'] == 600, s)
    press(h, LEFT)
    s = state(h)
    check('left -40', s['w'] == 800 and s['x'] == 300 and not s['zoom'], s)

    place(h, 300, 200, 320, 600)
    press(h, LEFT)
    check('not narrower than 300', state(h)['w'] == 300, state(h))
    press(h, LEFT)
    check('stays at 300', state(h)['w'] == 300, state(h))

    place(h, wa.right - 810, 200, 800, 600)
    press(h, RIGHT)
    check('widens only up to the screen edge', state(h)['x'] + state(h)['w'] == wa.right, state(h))
    press(h, RIGHT)
    check('no change at the screen edge', state(h)['x'] + state(h)['w'] == wa.right, state(h))

    place(h, wa.right - 120, 200, 800, 600)
    press(h, RIGHT)
    check('a window past the edge does not shrink on right', state(h)['w'] == 800, state(h))
    press(h, LEFT)
    check('a window past the edge can still narrow', state(h)['w'] == 760, state(h))

    place(h, 300, 200, 800, 600)
    maximize(h)
    full = state(h)
    check('maximized', full['zoom'], full)
    press(h, RIGHT)
    check('maximized + right changes nothing', state(h) == full, state(h))
    press(h, LEFT)
    s = state(h)
    check('maximized + left leaves full screen',
          not s['zoom'] and s['x'] == wa.left and s['y'] == wa.top and s['w'] == ww - 40 and s['h'] == wh, s)
    check('the content follows the size', s['w'] - 40 < s['cw'] < s['w'], s)
    press(h, LEFT)
    check('the next left works normally', not state(h)['zoom'] and state(h)['w'] == ww - 80, state(h))
    press(h, RIGHT)
    press(h, RIGHT)
    check('right returns to full width without damage', not state(h)['zoom'] and state(h)['w'] == ww, state(h))
    press(h, UP)
    check('up afterwards gives real full screen', state(h)['zoom'], state(h))
    press(h, DOWN)
    s = state(h)
    check('down afterwards fits on screen', not s['zoom'] and s['y'] + s['h'] <= wa.bottom, s)
    press(h, LEFT)
    s2 = state(h)
    check('left afterwards changes only the width',
          not s2['zoom'] and s2['w'] == s['w'] - 40 and s2['x'] == s['x'] and s2['h'] == s['h'], s2)


def run_move(dx, dy):
    subprocess.run([APP, f'--move-test={dx},{dy}'], timeout=20)
    time.sleep(0.25)


def test_move(h, wa, mon, other):
    print('window move')
    # keep the test window on top, so it is surely the window under the cursor
    u.SetWindowPos(h, W.HWND(-1), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE)
    place(h, 300, 200, 800, 600)
    activate(h)
    a = state(h)
    run_move(300, 180)
    b = state(h)
    check('normal move +300,+180', not b['zoom'] and b['x'] == a['x'] + 300 and b['y'] == a['y'] + 180
          and b['w'] == a['w'] and b['h'] == a['h'], b)
    cx, cy = cursor()
    check('the cursor follows', cx == a['x'] + a['w'] // 2 + 300 and cy == a['y'] + a['h'] // 2 + 180, (cx, cy))

    run_move(-6000, -6000)
    s = state(h)
    check('cannot get lost at the top left', s['y'] == wa.top and s['x'] == wa.left + 120 - s['w'], s)
    run_move(700, 300)
    run_move(6000, 6000)
    s = state(h)
    check('cannot get lost at the bottom right', s['x'] == wa.right - 120 and s['y'] == wa.bottom - 40, s)

    place(h, 300, 200, 800, 600)
    maximize(h)
    check('maximized', state(h)['zoom'], state(h))
    activate(h)
    run_move(100, 50)
    s = state(h)
    check('moved while maximized: leaves full screen with its normal size',
          not s['zoom'] and s['w'] == 800 and s['h'] == 600, s)
    cx, cy = cursor()
    check('the cursor stays inside the window', s['x'] <= cx <= s['x'] + s['w'] and s['y'] <= cy <= s['y'] + s['h'], (cx, cy))

    # a borderless window covering the monitor must not move (like F11 or a game)
    old = u.GetWindowLongPtrW(h, -16)
    u.SetWindowLongPtrW(h, -16, (old & ~0x00CF0000) | 0x80000000)
    u.SetWindowPos(h, None, mon.left, mon.top, mon.right - mon.left, mon.bottom - mon.top,
                   SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE)
    time.sleep(0.3)
    f0 = state(h)
    run_move(200, 200)
    f1 = state(h)
    check('borderless full screen stays put', f0 == f1, f1)
    u.SetWindowLongPtrW(h, -16, old)
    u.SetWindowPos(h, None, 200, 150, 800, 600, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE)
    time.sleep(0.3)

    # the moved window becomes active even if it was not active before
    if other and other != h and activate(other):
        run_move(50, 20)
        check('the moved window becomes active', u.GetForegroundWindow() == h, (other, u.GetForegroundWindow()))
    u.SetWindowPos(h, W.HWND(-2), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE)


def main():
    for path in (TESTWIN, APP):
        if not os.path.exists(path):
            print(f'{path} not found. Run: build.cmd test')
            sys.exit(2)
    previous = u.GetForegroundWindow()
    start_cursor = cursor()
    window = subprocess.Popen([TESTWIN])
    h = None
    for _ in range(30):
        time.sleep(0.1)
        h = u.FindWindowW(None, 'TOUCHTAMER_TEST_WINDOW')
        if h:
            break
    if not h:
        print('the test window did not appear')
        sys.exit(2)
    time.sleep(0.3)
    mi = MI()
    mi.cb = C.sizeof(MI)
    u.GetMonitorInfoW(u.MonitorFromWindow(h, 2), C.byref(mi))
    wa, mon = mi.work, mi.mon
    print('work area', wa.left, wa.top, wa.right - wa.left, wa.bottom - wa.top)

    app = None
    try:
        app = start_hotkeys()
        test_size_hotkeys(h, wa)
        test_width_hotkeys(h, wa)
        stop_app(app)
        app = None

        test_move(h, wa, mon, previous)

        app = start_hotkeys()
        place(h, 300, 200, 800, 600)
        maximize(h)
        activate(h)
        stop_app(app)
        app = None
        run_move(100, 50)
        app = start_hotkeys()
        press(h, DOWN)
        s = state(h)
        print('move then hotkey')
        check('down after a move out of full screen is not broken',
              not s['zoom'] and s['y'] + s['h'] <= wa.bottom, s)
        stop_app(app)
        app = None
    finally:
        if app:
            subprocess.run([APP, '--exit'], timeout=10)
        u.PostMessageW(h, 0x0010, 0, 0)
        try:
            window.wait(timeout=5)
        except Exception:
            window.kill()
        u.SetCursorPos(*start_cursor)
        if previous:
            activate(previous)
    print('RESULT:', 'all passed' if not FAIL else f'{len(FAIL)} FAILED {FAIL}')
    sys.exit(1 if FAIL else 0)


main()
