#include "glm/common.hpp"

#ifndef __VKZ_STRING_INLINE__
# error "must include from memory_operation.h"
#endif

namespace vkz
{
    StringView::StringView()
    {
        clear();
    }

    StringView::StringView(const StringView& _other)
    {
        set(_other);
    }

    StringView::StringView(const StringView& _rhs, int32_t _start, int32_t _len)
    {
        set(_rhs, _start, _len);
    }

    StringView::StringView(const char* _ptr)
    {
        set(_ptr);
    }

    StringView::StringView(const char* _ptr, int32_t _len)
    {
        set(_ptr, _len);
    }

    StringView::StringView(const char* _ptr, const char* _term)
    {
        set(_ptr, _term);
    }

    StringView& StringView::operator=(const StringView& _other)
    {
        set(_other);
        return *this;
    }

    StringView& StringView::operator=(const char* _ptr)
    {
        set(_ptr);
        return *this;
    }

    inline void StringView::set(const char* _ptr)
    {
        clear();

        if (nullptr == _ptr)
        {
            return;
        }

        m_ptr = _ptr;
        m_len = (int32_t)strlen(_ptr);
    }

    inline void StringView::set(const char* _ptr, int32_t _len)
    {
        clear();

        if (nullptr == _ptr)
        {
            return;
        }

        m_ptr = _ptr;
        m_len = _len;
    }

    inline void StringView::set(const char* _ptr, const char* _term)
    {
        clear();

        if (nullptr == _ptr || nullptr == _term)
        {
            return;
        }

        m_ptr = _ptr;
        m_len = int32_t(_term - _ptr);
    }

    inline void StringView::set(const StringView& _other)
    {
        set(_other, 0, _other.m_len);
    }

    inline void StringView::set(const StringView& _other, int32_t _start, int32_t _len)
    {
        const int32_t start = glm::min(_start, _other.m_len);
        const int32_t len = glm::clamp(_other.m_len - _start, 0, glm::min(_len, _other.m_len));
        
        set(_other.m_ptr + start, len);
    }

    inline const char* StringView::getPtr() const
    {
        return m_ptr;
    }

    inline int32_t StringView::getLen() const
    {
        return m_len;
    }

    inline bool StringView::empty() const
    {
        return 0 == m_len;
    }

    inline void StringView::clear()
    {
        m_ptr = nullptr;
        m_len = 0;
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------------------------------------------------
    template<AllocatorI** Allocator>
    SimpleString<Allocator>::SimpleString()
        : StringView()
        , m_capacity(0)
    {
        clear();
    }

    template<AllocatorI** Allocator>
    SimpleString<Allocator>::SimpleString(const StringView& _other)
        : StringView()
        , m_capacity(0)
    {
        set(_other);
    }

    template<AllocatorI** Allocator>
    SimpleString<Allocator>::SimpleString(const SimpleString<Allocator>& _other)
        : StringView()
        , m_capacity(0)
    {
        set(_other);
    }

    template<AllocatorI** Allocator>
    SimpleString<Allocator>::~SimpleString()
    {
        clear();
    }

    template<AllocatorI** Allocator>
    SimpleString<Allocator>& SimpleString<Allocator>::operator=(const SimpleString<Allocator>& _other)
    {
        set(_other);
        return *this;
    }

    template<AllocatorI** Allocator>
    void SimpleString<Allocator>::set(const StringView& _str)
    {
        clear();
        append(_str);
    }

    template<AllocatorI** Allocator>
    void SimpleString<Allocator>::append(const StringView& _str)
    {
        if (_str.getLen() == 0)
        {
            return;
        }

        const int32_t oldLen = m_len;
        const int32_t len = m_len + _str.getLen();
        char* ptr = const_cast<char*>(m_ptr);

        if (len + 1 > m_capacity)
        {
            const int32_t newCapacity = alignUp(len + 1, 256);
            ptr = (char*)realloc(*Allocator, 0 == newCapacity ? nullptr : ptr, newCapacity);

            *const_cast<char**>(&m_ptr) = ptr;

            m_capacity = newCapacity;
        }

        m_len = len;
        memcpy(ptr + oldLen, _str.getPtr(), _str.getLen());
        // set the ending
        memcpy(ptr + m_len, "\0", 1);
    }

    template<AllocatorI** Allocator>
    void SimpleString<Allocator>::append(const char* _ptr, const char* _term)
    {
        append(StringView(_ptr, _term));
    }

    template<AllocatorI** Allocator>
    void SimpleString<Allocator>::clear()
    {
        if (m_capacity > 0)
        {
            free(*Allocator, const_cast<char*>(m_ptr));
        }

        m_capacity = 0;
        m_ptr = nullptr;
        m_len = 0;
    }

    template<AllocatorI** Allocator>
    const char* SimpleString<Allocator>::getCStr() const
    {
        return getPtr();
    }
}