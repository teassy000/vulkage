#include <malloc.h>
#include "common.h"
#include "alloc.h"

#include "profiler.h"



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
                VKZ_ProfFree(_ptr);
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
        
        if (nullptr == _ptr)
        {
            void* ret = nullptr;
            if (kDefaultAlign >= _align)
            {
                ret = ::malloc(_sz);

            }
            else
            {
                ret = _aligned_malloc(_sz, _align);
            }

            VKZ_ProfAlloc(ret, _sz);
            return ret;
        }

        if (kDefaultAlign >= _align)
        {
            VKZ_ProfFree(_ptr);
            void* ret = ::realloc(_ptr, _sz);
            VKZ_ProfAlloc(ret, _sz);
            return ret;
        }

        VKZ_ProfFree(_ptr);
        void* ret = _aligned_realloc(_ptr, _sz, _align);
        VKZ_ProfAlloc(ret, _sz);
        return ret;
    }

}
