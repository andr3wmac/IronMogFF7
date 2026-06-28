#pragma once

#include "AppFrame/ImGui.h"

#define APPFRAME_VERSION_MAJOR 0
#define APPFRAME_VERSION_MINOR 1
#define APPFRAME_VERSION_PATCH 0
#define APPFRAME_VERSION_STRING "0.1.0"

namespace AppFrame
{
    inline constexpr int VersionMajor = APPFRAME_VERSION_MAJOR;
    inline constexpr int VersionMinor = APPFRAME_VERSION_MINOR;
    inline constexpr int VersionPatch = APPFRAME_VERSION_PATCH;
    inline constexpr const char* VersionString = APPFRAME_VERSION_STRING;
}
