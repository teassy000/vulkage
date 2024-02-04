#pragma once

namespace vkz
{
    enum DebugMessageType
    {
        info = 0u,
        warning = 1u,
        error = 2u,
    };

#ifdef _DEBUG
    void message(DebugMessageType type, const char* format, ...);
#else
    void message(DebugMessageType type, const char* format, ...);
#endif // _DEBUG

}