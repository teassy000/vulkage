#pragma once

namespace kage
{
    enum DebugMessageType
    {
        info = 0u,
        warning = 1u,
        error = 2u,
    };

#define VK_CHECK(call) \
	do{ \
		VkResult result = call; \
		assert(result == VK_SUCCESS);\
	} while (0)

    void message(DebugMessageType type, const char* format, ...);

}