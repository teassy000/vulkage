#include "common.h"
#include "debug.h"
#include "macro.h"

#include <stdio.h>
#include "bx/bx.h"

#ifdef _WIN32
#include <windows.h>
#endif


namespace kage {

    constexpr DebugMsgType c_debugLv = DebugMsgType::essential;

    struct DebugMsgName
    {
        DebugMsgType    type;
        const char*     name;
    };

    static const DebugMsgName s_debugMsgName[] =
    {
        { DebugMsgType::info,       "info" },
        { DebugMsgType::essential,  "essential" },
        { DebugMsgType::warning,    "warning" },
        { DebugMsgType::error,      "error" },
        { DebugMsgType::count,      "unknown" },
    };

    static const char* getDebugMsgName(DebugMsgType _type)
    {
        BX_ASSERT(_type < DebugMsgType::count, "invalid debug message type %d", _type);
        const DebugMsgName& n = s_debugMsgName[bx::min(_type, DebugMsgType::count)];
        return n.name;
    }

    void message(DebugMsgType type, const char* format, ...)
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

            const char* ts = getDebugMsgName(type);

            char out[4096];
            snprintf(out, COUNTOF(out), "[kage::%s]: %s\n", ts, str);

            printf("%s", out);

#ifdef _WIN32
            OutputDebugStringA(out);
#endif // _WIN32

            BX_ASSERT(type < DebugMsgType::error, "%s", out);
        }
    }

} // namespace kage

