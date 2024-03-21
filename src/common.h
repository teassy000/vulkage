#pragma once

#include <assert.h>
#include <volk.h>


#include <unordered_map>
#include <utility>
#include <string>
#include <array>

#include "tinystl/allocator.h"
#include "tinystl/vector.h"
#include "tinystl/unordered_set.h"
#include "tinystl/unordered_map.h"

#include <stdlib.h>
#include <stdint.h>
#include "profiler.h"

#define VK_CHECK(call) \
	do{ \
		VkResult result = call; \
		assert(result == VK_SUCCESS);\
	} while (0)



#include "debug.h"

namespace stl = tinystl;
