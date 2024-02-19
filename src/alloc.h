#pragma once

#include "macro.h"

#include <glm/common.hpp> // for glm::max


#define VKZ_NEW(_allocator, _type)                 VKZ_PLACEMENT_NEW(vkz::alloc(_allocator, sizeof(_type) ), _type)
#define VKZ_ALIGNED_NEW(_allocator, _type, _align) VKZ_PLACEMENT_NEW(vkz::alloc(_allocator, sizeof(_type), _align), _type)
#define VKZ_PLACEMENT_NEW(_ptr, _type)             ::new(vkz::PlacementNew, _ptr) _type

namespace vkz
{
    struct NO_VTABLE AllocatorI
    {
        virtual ~AllocatorI() = 0;

        virtual void* realloc(
            void* _ptr
            , size_t _sz
            , size_t _align
            ) = 0;
    };

    class DefaultAllocator : public AllocatorI
    {
    public:
        DefaultAllocator();

        virtual ~DefaultAllocator() override;

        void* realloc(
              void* _ptr
            , size_t _sz
            , size_t _align
            ) override;
    };

    inline void* alignPtr(
          void* _ptr
        , size_t _extra
        , size_t _align);

    inline void* alloc(
          AllocatorI* _allocator
        , size_t _sz
        , size_t _align = 0
        );

    inline void free(
          AllocatorI* _allocator
        , void* _ptr
        , size_t _align = 0
        );

    inline void* realloc(
          AllocatorI* _allocator
        , void* _ptr
        , size_t _sz
        , size_t _align = 0
        );

    inline void* alignedAlloc(
          AllocatorI* _allocator
        , size_t _sz
        , size_t _align
        );

    inline void alignedFree(
          AllocatorI* _allocator
        , void* _ptr
        , size_t _align
        );

    inline void* alignedRealloc(
          AllocatorI* _allocator
        , void* _ptr
        , size_t _sz
        , size_t _align
        );

    template <typename ObjectT>
    inline void deleteObject(
          AllocatorI* _allocator
        , ObjectT* _object
        , size_t _align = 0
        );

    
    // implementations
    inline AllocatorI::~AllocatorI()
    {
    }

    template<typename Ty>
    inline Ty alignUp(Ty _a, int32_t _align)
    {
        const Ty mask = Ty(_align - 1);
        return Ty((_a + mask) & ~mask);
    }

    inline void* alignPtr(void* _ptr, size_t _extra, size_t _align)
    {
        uintptr_t ptr = (uintptr_t)_ptr;
        uintptr_t unaligned = ptr + _extra; // space for header

        uintptr_t aligned = alignUp(unaligned, (int32_t)_align);

        return (void*)aligned;
    }

    inline void* alloc(AllocatorI* _allocator, size_t _sz, size_t _align)
    {
        return _allocator->realloc(nullptr, _sz, _align);
    }

    inline void free(AllocatorI* _allocator, void* _ptr, size_t _align)
    {
        _allocator->realloc(_ptr, 0, _align);
    }

    inline void* realloc(AllocatorI* _allocator, void* _ptr, size_t _sz, size_t _align)
    {
        return _allocator->realloc(_ptr, _sz, _align);
    }

    inline void* alignedAlloc(AllocatorI* _allocator, size_t _sz, size_t _align)
    {
        const size_t align = glm::max(_align, sizeof(uint32_t));
        const size_t total = _sz + align;
        uint8_t* ptr = (uint8_t*)alloc(_allocator, total, 0);
        uint8_t* aligned = (uint8_t*)alignPtr(ptr, sizeof(uint32_t), align);
        
        // header stores offset to aligned pointer
        uint32_t* header = (uint32_t*)aligned - 1;
        *header = uint32_t(aligned - ptr);

        return _allocator->realloc(nullptr, _sz, _align);
    }

    inline void alignedFree(AllocatorI* _allocator, void* _ptr, size_t _align)
    {
        if (nullptr != _ptr)
        {
            uint8_t* aligned = (uint8_t*)_ptr;
            uint32_t* header = (uint32_t*)aligned - 1;
            uint8_t* ptr = aligned - *header;
            free(_allocator, ptr, 0);
        }
    }

    inline void* alignedRealloc(AllocatorI* _allocator, void* _ptr, size_t _sz, size_t _align)
    {
        if (nullptr != _ptr)
        {
            uint8_t* aligned = (uint8_t*)_ptr;
            uint32_t* header = (uint32_t*)aligned - 1;
            uint8_t* ptr = aligned - *header;
            ptr = (uint8_t*)realloc(_allocator, ptr, _sz, 0);
            uint8_t* newAligned = (uint8_t*)alignPtr(ptr, sizeof(uint32_t), _align);
            
            uint32_t* newHeader = (uint32_t*)newAligned - 1;
            *newHeader = uint32_t(newAligned - ptr);
            
            return newAligned;
        }

        return alignedAlloc(_allocator, _sz, _align);
    }

    template <typename ObjectT>
    inline void deleteObject(AllocatorI* _allocator, ObjectT* _object, size_t _align)
    {
        if (nullptr != _object)
        {
            _object->~ObjectT();
            free(_allocator, _object, _align);
        }
    }
}

namespace vkz
{
    VKZ_DECLARE_TAG(PlacementNew);
}

inline void* operator new(size_t, vkz::PlacementNewTag, void* _ptr)
{
    return _ptr;
}

inline void operator delete(void*, vkz::PlacementNewTag, void*) throw()
{
}
