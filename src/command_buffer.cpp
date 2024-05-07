
#include "command_buffer.h"

namespace kage
{
#if 0
    CommandBuffer::CommandBuffer()
        : m_buffer(nullptr)
        , m_pos(0)
        , m_size(0)
        , m_minCapacity(0)
    {
        resize();
        finish();
    }

    CommandBuffer::~CommandBuffer()
    {
        bx::free(g_bxAllocator, m_buffer);
    }

    void CommandBuffer::init(uint32_t _minCapacity)
    {
        m_minCapacity = bx::alignUp(_minCapacity, 1024);
        resize();
    }

    void CommandBuffer::resize(uint32_t _capacity /*= 0*/)
    {
        m_capacity = bx::alignUp(bx::max(_capacity, m_minCapacity), 1024);
        m_buffer = (uint8_t*)bx::realloc(g_bxAllocator, m_buffer, m_capacity);
    }

    void CommandBuffer::write(const void* _data, uint32_t _size)
    {
        BX_ASSERT(m_size == 0, "Called write outside start/finish (m_size: %d)?", m_size);
        if (m_pos + _size > m_capacity)
        {
            resize(m_capacity + (16 << 10));
        }

        bx::memCopy(&m_buffer[m_pos], _data, _size);
        m_pos += _size;
    }

    template<typename Ty>
    void CommandBuffer::write(const Ty& _data)
    {
        align(BX_ALIGNOF(Ty));
        write(reinterpret_cast<const uint8_t*>(&_data), sizeof(Ty));
    }

    void CommandBuffer::tile()
    {
        uint8_t cmd = static_cast<uint8_t>(cmd_tail);
        write(cmd);
    }

    void CommandBuffer::read(void* _data, uint32_t _size)
    {
        BX_ASSERT(m_pos + _size <= m_size
            , "CommandBuffer::read error (pos: %d-%d, size: %d)."
            , m_pos
            , m_pos + _size
            , m_size
        );
        bx::memCopy(_data, &m_buffer[m_pos], _size);
        m_pos += _size;
    }

    template<typename Ty>
    void CommandBuffer::read(Ty& _in)
    {
        align(BX_ALIGNOF(Ty));
        read(reinterpret_cast<uint8_t*>(&_in), sizeof(Ty));
    }

    const uint8_t* CommandBuffer::skip(uint32_t _size)
    {
        BX_ASSERT(m_pos + _size <= m_size
            , "CommandBuffer::skip error (pos: %d-%d, size: %d)."
            , m_pos
            , m_pos + _size
            , m_size
        );
        const uint8_t* result = &m_buffer[m_pos];
        m_pos += _size;
        return result;
    }

    template<typename Ty>
    void CommandBuffer::skip()
    {
        align(BX_ALIGNOF(Ty));
        skip(sizeof(Ty));
    }

    void CommandBuffer::align(uint32_t _alignment)
    {
        const uint32_t mask = _alignment - 1;
        const uint32_t pos = (m_pos + mask) & (~mask);
        m_pos = pos;
    }

    void CommandBuffer::reset()
    {
        m_pos = 0;
    }

    void CommandBuffer::start()
    {
        m_pos = 0;
        m_size = 0;
    }

    void CommandBuffer::finish()
    {
        uint8_t cmd = end;
        write(cmd);
        m_size = m_pos;
        m_pos = 0;

        if (m_size < m_minCapacity
            && m_capacity != m_minCapacity)
        {
            resize();
        }
    }
#endif
}



