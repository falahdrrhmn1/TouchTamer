// An empty window for the end-to-end test, so tests never touch a real application window.
#include <windows.h>

static LRESULT CALLBACK Proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

int main()
{
    HINSTANCE hi = GetModuleHandleW(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = Proc;
    wc.hInstance = hi;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"TouchTamerTestWindowClass";
    RegisterClassW(&wc);
    HWND h = CreateWindowExW(0, L"TouchTamerTestWindowClass", L"TOUCHTAMER_TEST_WINDOW", WS_OVERLAPPEDWINDOW,
                             300, 200, 800, 600, nullptr, nullptr, hi, nullptr);
    ShowWindow(h, SW_SHOW);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
