#pragma once

#include <assert.h>
#include <volk.h>

#include <vector>
#include <unordered_map>
#include <set>
#include <utility>

#include <string>

#define VK_CHECK(call) \
	do{ \
		VkResult result = call; \
		assert(result == VK_SUCCESS);\
	} while (0)


template <typename T, size_t Size>
char(*countof_helper(T(&_Array)[Size]))[Size];

#define COUNTOF(array) (sizeof(*countof_helper(array)) + 0)

typedef uint32_t ResourceID;
typedef uint32_t PassID;
const PassID invalidPassID = ~0u;
const ResourceID invalidResourceID = ~0u;

typedef std::initializer_list<ResourceID> ResourceIDs;

#include "debug.h"