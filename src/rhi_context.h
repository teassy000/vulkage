#ifndef __VKZ_RES_CREATOR_H__
#define __VKZ_RES_CREATOR_H__

#include <stdint.h>
#include "vkz_structs_inner.h"
#include "config.h"

namespace vkz
{
    enum class RHIContextOpMagic : uint32_t
    {
        invalid_magic = 0,

        magic_body_end,

        create_pass,
        create_image,
        create_buffer,
        create_descriptor_set,
        create_program,
        create_shader,

        set_brief,

        end,
    };



    class RHIContextI
    {
    public:
        virtual void init(RHI_Config _config) = 0;
        virtual void update() = 0;
        virtual void render() = 0;

    private:
        virtual void createShader(MemoryReader& reader) = 0;
        virtual void createProgram(MemoryReader& reader) = 0;
        virtual void createPass(MemoryReader& reader) = 0;
        virtual void createImage(MemoryReader& reader) = 0;
        virtual void createBuffer(MemoryReader& reader) = 0;
        virtual void setBrief(MemoryReader& reader) = 0;
    };

    class RHIContext : public RHIContextI
    {
    public:
        RHIContext(AllocatorI* _allocator)
            : m_pAllocator{ _allocator }
            , m_pMemBlock{ nullptr }
        {
            m_pMemBlock = VKZ_NEW(m_pAllocator, MemoryBlock(m_pAllocator));
            m_pMemBlock->expand(kInitialFrameGraphMemSize);
        }

        virtual ~RHIContext()
        {
            deleteObject(m_pAllocator, m_pMemBlock);
        }

        inline MemoryBlockI* getMemoryBlock() const {return m_pMemBlock;}
        inline AllocatorI* getAllocator() const { return m_pAllocator; }

        void init(RHI_Config _config) override {};

        void update() override;
        void render() override {};
    protected:

    private:
        void parseOp();

        void createShader(MemoryReader& reader) override {};
        void createProgram(MemoryReader& reader) override {};
        void createPass(MemoryReader& reader) override {};
        void createImage(MemoryReader& reader) override {};
        void createBuffer(MemoryReader& reader) override {};
        void setBrief(MemoryReader& reader) override {};

    private:
        AllocatorI*     m_pAllocator;
        MemoryBlockI*   m_pMemBlock;
    };

} // namespace vkz

#endif // __VKZ_RES_CREATOR_H__