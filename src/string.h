#pragma once

/*
 * Copyright 2010-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

// with modifications to fit current porject

#include "common.h"
#include "alloc.h"

namespace vkz
{
    class StringView
    {
    public:
        inline StringView();

        inline StringView(const StringView& _other);

        inline StringView(const StringView& _other, int32_t _start, int32_t _len);

        inline StringView(const char* _ptr);

        inline StringView(const char* _ptr, int32_t _len);

        inline StringView(const char* _ptr, const char* _term);

        inline StringView& operator=(const StringView& _other);

        inline StringView& operator=(const char* _ptr);

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