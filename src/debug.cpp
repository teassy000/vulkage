#include "common.h"
#include "debug.h"

#include <stdio.h>


#ifdef _WIN32
#include <windows.h>
#endif

namespace vkz {
    void message(DebugMessageType type, const char* format, ...)
    {
        // parsing args
        va_list args;
        va_start(args, format);
        char str[4096];
        vsprintf(str, format, args);
        va_end(args);

        const char* ts =
            type == DebugMessageType::info
            ? "info"
            : type == DebugMessageType::warning
            ? "warning"
            : "error";

        char out[4096];
        snprintf(out, COUNTOF(out), "[vkz::%s]: %s", ts, str);

        printf("%s", out);

#ifdef _WIN32
        OutputDebugStringA(out);
#endif

        assert(type < DebugMessageType::error);
    }
}

