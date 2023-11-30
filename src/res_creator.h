#pragma once

namespace vkz
{
    enum class ResCreatorOpMagic : uint32_t
    {
        InvalidMagic = 0,

        CreatePass,
        CreateImage,
        CreateBuffer,
        CreateDescriptorSet,
        CreatePipeline,
        CreateShader,

        End,
    };

    class ResCreator
    {
    private:
        void parseOp();

        virtual void createPass(MemoryReader& reader) = 0;
        virtual void createImage(MemoryReader& reader) = 0;
        virtual void createBuffer(MemoryReader& reader) = 0;

    private:
        MemoryBlockI* m_pMemBlock;
    };

} // namespace vkz