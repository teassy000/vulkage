#pragma once

#include "common.h"
#include "alloc.h"

namespace vkz
{
    class StringView
    {
    public:
        StringView();

        StringView(const StringView& _other);

        StringView(const StringView& _other, int32_t _start, int32_t _len);

        StringView(const char* _ptr);

        StringView(const char* _ptr, int32_t _len);

        StringView(const char* _ptr, const char* _term);

        StringView& operator=(const StringView& _other);

        StringView& operator=(const char* _ptr);

        inline void set(const char* _ptr);

        inline void set(const char* _ptr, int32_t _len);

        inline void set(const char* _ptr, const char* _term);

        inline void set(const StringView& _other);

        inline void set(const StringView& _other, int32_t _start, int32_t _len);

        inline const char* getPtr() const;

        inline int32_t getLen() const;

        inline bool empty() const;

        inline void clear();

    protected:
        const char* m_ptr;
        int32_t m_len;
    };

    template<AllocatorI** Allocator>
    class SimpleString : public StringView
    {
    public:
        SimpleString();

        SimpleString(const StringView& _other);

        SimpleString(const SimpleString<Allocator>& _other);

        ~SimpleString();

        SimpleString<Allocator>& operator=(const SimpleString<Allocator>& _other);

        void set(const StringView& _str);

        void append(const StringView& _str);

        void append(const char* _ptr, const char* _term);

        void clear();

        const char* getCStr() const;
    
    protected:
        int32_t m_capacity;
    };
}; // namespace vkz


#define __VKZ_STRING_INLINE__
#include "string.inl"