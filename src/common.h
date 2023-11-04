#pragma once

#include <assert.h>
#include <volk.h>

#include <vector>
#include <unordered_map>
#include <set>
#include <utility>
#include <string>
#include <array>

#include <stdint.h>

#define VK_CHECK(call) \
	do{ \
		VkResult result = call; \
		assert(result == VK_SUCCESS);\
	} while (0)


template <typename T, size_t Size>
char(*countof_helper(T(&_Array)[Size]))[Size];

#define COUNTOF(array) (sizeof(*countof_helper(array)) + 0)


#include "debug.h"

const uint32_t invalidID = UINT32_MAX;

typedef std::initializer_list<uint32_t> ResourceIDs;
