#ifndef __VKZ_DEBUG_H__
#define __VKZ_DEBUG_H__
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
#endif // DEBUG

}

#endif // __VKZ_DEBUG_H__



