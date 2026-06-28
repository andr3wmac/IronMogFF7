#pragma once

#include "AppFrame/Application.h"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #define APPFRAME_MAIN(AppType)                 \
        int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) \
        {                                          \
            return AppFrame::runApplication<AppType>();       \
        }
#else
    #define APPFRAME_MAIN(AppType)                 \
        int main(int, char**)                      \
        {                                          \
            return AppFrame::runApplication<AppType>();       \
        }
#endif
