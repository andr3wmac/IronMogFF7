#pragma once

struct GLFWwindow;

namespace AppFrame
{
#ifdef _WIN32
    void lockHorizontalResizeOnWindows(GLFWwindow* window);
#endif
}
