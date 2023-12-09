#pragma once

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
        CreatePipeline,
        CreateShader,

        End,
    };

    enum class PassType : uint16_t
    {
        Invalid = 0,
        Compute,
        Graphics,
    };


    class ResCreator
    {
    public:
        inline void setMemoryBlock(MemoryBlockI* pMemBlock) {
            m_pMemBlock = pMemBlock;
        }
    private:
        void parseOp();

        virtual void init(MemoryReader& reader) = 0;
        virtual void createPass(MemoryReader& reader) = 0;
        virtual void createImage(MemoryReader& reader) = 0;
        virtual void createBuffer(MemoryReader& reader) = 0;

    private:
        MemoryBlockI* m_pMemBlock;
    };

} // namespace vkz