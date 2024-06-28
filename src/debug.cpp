#include "common.h"
#include "debug.h"
#include "macro.h"

#include <stdio.h>
#include "bx/bx.h"

#ifdef _WIN32
#include <windows.h>
#endif


namespace kage {

    constexpr DebugMessageType c_debugLv = DebugMessageType::warning;

    void message(DebugMessageType type, const char* format, ...)
    {
        if (BX_ENABLED(KAGE_DEBUG))
        {
            if (type < c_debugLv)
            {
                return;
            }

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
            snprintf(out, COUNTOF(out), "[kage::%s]: %s\n", ts, str);

            printf("%s", out);

#ifdef _WIN32
            OutputDebugStringA(out);
#endif // _WIN32

            BX_ASSERT(type < DebugMessageType::error, "%s", out);
        }
    }

} // namespace kage

