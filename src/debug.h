#pragma once

namespace vkz
{
    enum DebugMessageType
    {
        info,
        warning,
        error,
    };

    void message(DebugMessageType type, const char* format, ...);
}



