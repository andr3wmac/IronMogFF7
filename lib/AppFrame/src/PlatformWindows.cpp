#include "PlatformWindows.h"

#ifdef _WIN32

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

namespace AppFrame
{
static LRESULT CALLBACK HorizontalResizeLockWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData)
{
    switch (message)
    {
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            long lockedOuterWidth = static_cast<long>(refData);
            minMaxInfo->ptMinTrackSize.x = lockedOuterWidth;
            minMaxInfo->ptMaxTrackSize.x = lockedOuterWidth;
            return 0;
        }

        case WM_SETCURSOR:
        {
            WORD hitTest = LOWORD(lParam);
            if (hitTest == HTLEFT || hitTest == HTRIGHT)
            {
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
                return TRUE;
            }
            break;
        }
    }

    return DefSubclassProc(hwnd, message, wParam, lParam);
}

void lockHorizontalResizeOnWindows(GLFWwindow* window)
{
    HWND hwnd = glfwGetWin32Window(window);
    RECT rect;
    GetWindowRect(hwnd, &rect);
    SetWindowSubclass(hwnd, HorizontalResizeLockWindowProc, 1, rect.right - rect.left);
}
}

#endif
