#pragma once

#include <assert.h>

#include <utility>
#include <string>
#include <array>

#include <stdlib.h>
#include <stdint.h>
#include "profiler.h"

#include "debug.h"

namespace kage
{
    struct TinyStlAllocator
    {
        static void* static_allocate(size_t _bytes);
        static void static_deallocate(void* _ptr, size_t /*_bytes*/);
    };
} // namespace kage
#define TINYSTL_ALLOCATOR kage::TinyStlAllocator
#include <tinystl/string.h>
#include <tinystl/unordered_map.h>
#include <tinystl/unordered_set.h>
#include <tinystl/vector.h>

namespace stl = tinystl;
