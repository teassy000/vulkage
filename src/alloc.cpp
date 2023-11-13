#include <malloc.h>
#include "common.h"
#include "alloc.h"


namespace vkz
{
    constexpr unsigned int kDefaultAlign = 8;

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
                if (kDefaultAlign >= _align)
                {
                    ::free(_ptr);
                }
                else
                {
                    ::_aligned_free(_ptr);
                }
            }

            return nullptr;
        }
        else if (nullptr == _ptr)
        {
            if (kDefaultAlign >= _align)
            {
                return ::malloc(_sz);
            }
            return _aligned_malloc(_sz, _align);
        }

        if (kDefaultAlign >= _align)
        {
            return ::realloc(_ptr, _sz);
        }

        return _aligned_realloc(_ptr, _sz, _align);
    }

}
