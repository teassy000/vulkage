
#ifndef VKZ_MEMORY_OPERATION_H
# error "must include from memory_operation.h"
#endif

namespace vkz
{
    // impletation
    inline ReaderI::~ReaderI()
    {
    }

    inline WriterI::~WriterI()
    {
    }

    inline SeekerI::~SeekerI()
    {
    }

    inline ReaderOpenerI::~ReaderOpenerI()
    {
    }

    inline WriterOpenerI::~WriterOpenerI()
    {
    }

    inline CloserI::~CloserI()
    {
    }

    inline MemoryBlockI::~MemoryBlockI()
    {
    }

    inline StaticMemoryBlock::StaticMemoryBlock(void* _data, uint32_t _size)
        : m_data(reinterpret_cast<uint8_t*>(_data))
        , m_size(_size)
    {
    }

    inline StaticMemoryBlock::~StaticMemoryBlock()
    {
    }

    inline void* StaticMemoryBlock::expand(uint32_t _size)
    {
        // do nothing since this is a static memory block
        return m_data;
    }

    inline uint32_t StaticMemoryBlock::size() const
    {
        return m_size;
    }

    inline MemoryBlock::MemoryBlock(AllocatorI* _allocator)
        : m_allocator(_allocator)
        , m_data(nullptr)
        , m_size(0)
    {
    }

    inline MemoryBlock::~MemoryBlock()
    {
        free(m_allocator, m_data);
    }

    inline void* MemoryBlock::expand(uint32_t _size)
    {
        if (0 == _size)
        {
            return m_data;
        }

        m_data = (uint8_t*)realloc(m_allocator, m_data, m_size + _size, 0);
        m_size += _size;

        return m_data;
    }

    inline uint32_t MemoryBlock::size() const
    {
        return m_size;
    }

    inline MemoryReader::MemoryReader(const void* _data, uint32_t _size)
        : m_data(reinterpret_cast<const uint8_t*>(_data))
        , m_pos(0)
        , m_top(_size)
    {
    }

    inline MemoryReader::~MemoryReader()
    {
    }

    inline int32_t MemoryReader::read(void* _data, int32_t _size)
    {
        int32_t size = std::min(_size, int32_t(m_top - m_pos));
        if (0 < size)
        {
            ::memcpy(_data, &m_data[m_pos], size);
            m_pos += size;
        }

        return size;
    }

    inline int64_t MemoryReader::seek(int64_t _offset, Whence _whence)
    {
        switch (_whence)
        {
        case Whence::Begin:
            m_pos = _offset;
            break;

        case Whence::Current:
            m_pos += _offset;
            break;

        case Whence::End:
            m_pos = m_top - _offset;
            break;
        }

        return m_pos;
    }

    inline const uint8_t* MemoryReader::data() const
    {
        return m_data;
    }

    inline int64_t MemoryReader::pos() const
    {
        return m_pos;
    }

    inline int64_t MemoryReader::remaining() const
    {
        return m_top - m_pos;
    }

    inline MemoryWriter::MemoryWriter(MemoryBlockI* _mb)
        : m_memBlock(_mb)
        , m_data()
        , m_pos(0)
        , m_top(0)
        , m_size(0)
    {
    }

    inline MemoryWriter::~MemoryWriter()
    {
    }

    inline int32_t MemoryWriter::write(const void* _data, int32_t _size)
    {
        int32_t morecore = int32_t(m_pos - m_size) + _size;
        
        if (0 < morecore)
        {
            morecore = alignUp(morecore, 0x1000);

            m_data = (uint8_t*)m_memBlock->expand(morecore);
            m_size = m_memBlock->size();
        }

        int64_t remainder = m_size - m_pos;
        int32_t size = std::min<uint32_t>(_size, uint32_t(std::min<int64_t>(remainder, INT32_MAX)));
        ::memcpy(&m_data[m_pos], _data, size);
        m_pos += size;
        m_top = std::max(m_top, m_pos);

        return size;
    }

    inline int64_t MemoryWriter::seek(int64_t _offset, Whence _whence)
    {
        switch (_whence)
        {
        case Whence::Begin:
            m_pos = _offset;
            break;

        case Whence::Current:
            m_pos += _offset;
            break;

        case Whence::End:
            m_pos = m_top - _offset;
            break;
        }

        return m_pos;
    }

    inline StaticMemoryBlockWriter::StaticMemoryBlockWriter(void* _data, uint32_t _size)
        : MemoryWriter(&m_smb)
        , m_smb(_data, _size)
    {
    }

    inline StaticMemoryBlockWriter::~StaticMemoryBlockWriter()
    {
    }

    inline int32_t read(ReaderI* _reader, void* _data, int32_t _size)
    {
        return _reader->read(_data, _size);
    }

    template<typename T>
    inline int32_t read(ReaderI* _reader, T& _value)
    {
        return _reader->read(&_value, sizeof(T));
    }

    inline int32_t write(WriterI* _writer, const void* _data, int32_t _size)
    {
        return _writer->write(_data, _size);
    }

    template<typename T>
    inline int32_t write(WriterI* _writer, const T& _value)
    {
        return _writer->write(&_value, sizeof(T));
    }


    inline int64_t skip(SeekerI* _seeker, int64_t _offset)
    {
        return _seeker->seek(_offset, Whence::Current);
    }

    inline int64_t seek(SeekerI* _seeker, int64_t _offset, Whence _whence /* = Whence::Current*/)
    {
        return _seeker->seek(_offset, _whence);
    }

    inline int64_t size(SeekerI* _seeker)
    {
        int64_t offset = _seeker->seek(0, Whence::Current);
        int64_t size = _seeker->seek(0, Whence::End);
        _seeker->seek(offset, Whence::Begin);
        return size;
    }

    inline int64_t remaining(SeekerI* _seeker)
    {
        int64_t offset = _seeker->seek(0, Whence::Current);
        int64_t size = _seeker->seek(0, Whence::End);
        _seeker->seek(offset, Whence::Begin);
        return size-offset;
    }

    inline bool open(ReaderOpenerI* _reader, const char* _filePath)
    {
        return _reader->open(_filePath);
    }

    inline bool open(WriterOpenerI* _writer, const char* _filePath, bool _append /*= false*/)
    {
        return _writer->open(_filePath, _append);
    }

    inline void close(CloserI* _closer)
    {
        _closer->close();
    }

} // namespace vkz