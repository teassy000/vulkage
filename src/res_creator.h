#ifndef __VKZ_RES_CREATOR_H__
#define __VKZ_RES_CREATOR_H__

#include <stdint.h>
#include "vkz_structs_inner.h"
#include "config.h"

namespace vkz
{
    enum class ResCreatorOpMagic : uint32_t
    {
        InvalidMagic = 0,

        Init,

        CreatePass,
        CreateImage,
        CreateBuffer,
        CreateDescriptorSet,
        CreateProgram,
        CreateShader,

        End,
    };

    enum class PassType : uint16_t
    {
        Invalid = 0,
        Compute,
        Graphics,
    };

    class ResCreatorI
    {
    private:
        virtual void init(MemoryReader& reader) = 0;
        virtual void createShader(MemoryReader& reader) = 0;
        virtual void createProgram(MemoryReader& reader) = 0;
        virtual void createPass(MemoryReader& reader) = 0;
        virtual void createImage(MemoryReader& reader) = 0;
        virtual void createBuffer(MemoryReader& reader) = 0;
    };

    class ResCreator : public ResCreatorI
    {
    public:
        ResCreator(AllocatorI* _allocator)
            : m_pAllocator{ _allocator }
            , m_pMemBlock{ nullptr }
        {
            m_pMemBlock = VKZ_NEW(m_pAllocator, MemoryBlock(m_pAllocator));
            m_pMemBlock->expand(kInitialFrameGraphMemSize);
        }

        virtual ~ResCreator()
        {
            deleteObject(m_pAllocator, m_pMemBlock);
        }

        inline MemoryBlockI* getMemoryBlock() const {
            return m_pMemBlock;
        }

    private:
        void parseOp();

        void init(MemoryReader& reader) override {};
        void createShader(MemoryReader& reader) override {};
        void createProgram(MemoryReader& reader) override {};
        void createPass(MemoryReader& reader) override {};
        void createImage(MemoryReader& reader) override {};
        void createBuffer(MemoryReader& reader) override {};

    private:
        AllocatorI*     m_pAllocator;
        MemoryBlockI*   m_pMemBlock;
    };

} // namespace vkz

#endif // __VKZ_RES_CREATOR_H__