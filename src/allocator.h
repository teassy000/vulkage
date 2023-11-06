#pragma once

namespace vkz
{
    struct AllocatorI
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
        DefaultAllocator();

        virtual ~DefaultAllocator() override;

        virtual void* realloc(
              void* _ptr
            , size_t _sz
            , size_t _align
            ) override;
    };

    inline void* alloc(
        AllocatorI* _allocator
        , size_t _size
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
        , size_t _size
        , size_t _align = 0
        );

    template <typename ObjectT>
    inline void deleteObject(
        AllocatorI* _allocator
        , ObjectT* _object
        , size_t _align = 0
        );


    // implementations
    inline void* alloc(AllocatorI* _allocator, size_t _size, size_t _align)
    {
        return _allocator->realloc(nullptr, _size, _align);
    }

    inline void free(AllocatorI* _allocator, void* _ptr, size_t _align)
    {
        _allocator->realloc(_ptr, 0, _align);
    }

    inline void* realloc(AllocatorI* _allocator, void* _ptr, size_t _size, size_t _align)
    {
        return _allocator->realloc(_ptr, _size, _align);
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
