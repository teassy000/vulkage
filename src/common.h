#pragma once

#include <assert.h>
#include <volk.h>


#include <unordered_map>
#include <utility>
#include <string>
#include <array>

#include <TINYSTL/vector.h>
#include <TINYSTL/unordered_set.h>
#include <TINYSTL/unordered_map.h>

#include <stdint.h>
#include "profiler.h"

#define VK_CHECK(call) \
	do{ \
		VkResult result = call; \
		assert(result == VK_SUCCESS);\
	} while (0)



#include "debug.h"

const uint32_t invalidID = UINT32_MAX;

namespace stl = tinystl;
