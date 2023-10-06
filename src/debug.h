#pragma once

namespace vkz
{
    enum DebugMessageType
    {
        info = 0u,
        warning = 1u,
        error = 2u,
    };

    void message(DebugMessageType type, const char* format, ...);
}



