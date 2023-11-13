#ifndef VKZ_MEMORY_OPERATION_H
#define VKZ_MEMORY_OPERATION_H
// from bgfx, with modifications

#include "alloc.h"

namespace vkz
{
    enum class Whence
    {
        Begin,
        Current,
        End,
    };

    struct ReaderI
    {
        virtual ~ReaderI() = 0;

        virtual int32_t read(void* _data, int32_t _size) = 0;
    };

    struct WriterI
    {
        virtual ~WriterI() = 0;

        virtual int32_t write(const void* _data, int32_t _size) = 0;
    };

    struct SeekerI
    {
        virtual ~SeekerI() = 0;

        virtual int64_t seek(int64_t _offset, Whence _whence) = 0;
    };

    struct ReaderSeekerI : public ReaderI, public SeekerI
    {
    };

    struct WriterSeekerI : public WriterI, public SeekerI
    {
    };

    struct ReaderOpenerI
    {
        virtual ~ReaderOpenerI() = 0;

        virtual bool open(const char* _filePath) = 0;
    };

    struct WriterOpenerI
    {
        virtual ~WriterOpenerI() = 0;

        virtual bool open(const char* _filePath, bool _append = false) = 0;
    };

    struct CloserI
    {
        virtual ~CloserI() = 0;

        virtual void close() = 0;
    };

    struct MemoryBlockI
    {
        virtual ~MemoryBlockI() = 0;

        virtual void* expand(uint32_t _size = 0) = 0;
        virtual uint32_t size() const = 0;
    };

    class StaticMemoryBlock : public MemoryBlockI
    {
    public:
        StaticMemoryBlock(void* _data, uint32_t _size);

        virtual ~StaticMemoryBlock() override;

        void* expand(uint32_t _size = 0) override;
        uint32_t size() const override;
    private:
        uint8_t* m_data;
        uint32_t m_size;
    };

    class MemoryBlock : public MemoryBlockI
    {
    public:
        MemoryBlock(AllocatorI* _allocator);

        virtual ~MemoryBlock() override;

        void* expand(uint32_t _size = 0) override;
        uint32_t size() const override;

    private:
        AllocatorI* m_allocator;
        uint8_t* m_data;
        uint32_t m_size;
    };

    class MemoryReader : public ReaderSeekerI
    {
    public:
        MemoryReader(const void* _data, uint32_t _size);

        virtual ~MemoryReader() override;

        int32_t read(void* _data, int32_t _size) override;
        int64_t seek(int64_t _offset, Whence _whence) override;

        const uint8_t* data() const;

        int64_t pos() const;
        int64_t remaining() const;

    private:
        const uint8_t* m_data;
        int64_t m_pos;
        int64_t m_top;
    };

    class MemoryWriter : public WriterSeekerI
    {
    public:
        MemoryWriter(void* _data, uint32_t _size);

        virtual ~MemoryWriter() override;

        int32_t write(const void* _data, int32_t _size) override;
        int64_t seek(int64_t _offset, Whence _whence) override;

    private:
        MemoryBlockI* m_memBlock;
        uint8_t* m_data;
        int64_t m_pos;
        int64_t m_top;
        int64_t m_size;
    };

    // API
    int32_t read(ReaderI* _reader, void* _data, int32_t _size);
    template<typename T>
    int32_t read(ReaderI* _reader, T& _value);

    int32_t write(WriterI* _writer, const void* _data, int32_t _size);
    template<typename T>
    int32_t write(WriterI* _writer, const T& _value);

    int64_t skip(SeekerI* _seeker, int64_t _offset);

    int64_t seek(SeekerI* _seeker, int64_t _offset, Whence _whence = Whence::Current);
    
    int64_t size(SeekerI* _seeker);
    int64_t remaining(SeekerI* _seeker);

    bool open(ReaderOpenerI* _reader, const char* _filePath);
    bool open(WriterOpenerI* _writer, const char* _filePath, bool _append = false);

    void close(CloserI* _closer);


} // namespace vkz

#include "memory_operation.inl"

#endif // VKZ_MEMORY_OPERATION_H