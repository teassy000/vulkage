#include <malloc.h>
#include "common.h"
#include "allocator.h"


namespace vkz
{


    DefaultAllocator::DefaultAllocator()
    {

    }

    DefaultAllocator::~DefaultAllocator()
    {

    }

    void* DefaultAllocator::realloc(void* _ptr, size_t _sz, size_t _align)
    {
        if (0 == _sz)
        {
            if (nullptr != _ptr)
            {
                _aligned_free(_ptr);
            }

            return nullptr;
        }

        else if (nullptr == _ptr)
        {
            return _aligned_malloc(_sz, _align);
        }

        return _aligned_realloc(_ptr, _sz, _align);
    }

}
