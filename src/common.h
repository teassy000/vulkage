#pragma once

#include <assert.h>
#include <volk.h>

#include <vector>

#define VK_CHECK(call) \
	do{ \
		VkResult result = call; \
		assert(result == VK_SUCCESS);\
	} while (0)


template <typename T, size_t Size>
char(*countof_helper(T(&_Array)[Size]))[Size];

#define COUNTOF(array) (sizeof(*countof_helper(array)) + 0)

