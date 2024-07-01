#pragma once

namespace kage
{
    enum DebugMsgType : uint16_t
    {
        info,
        essential,
        warning,
        error,
        count,
    };

#define VK_CHECK(call) \
	do{ \
		VkResult result = call; \
		assert(result == VK_SUCCESS);\
	} while (0)

    void message(DebugMsgType type, const char* format, ...);

}