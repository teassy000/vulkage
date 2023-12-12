#ifndef __VKZ_RES_CREATOR_H__
#define __VKZ_RES_CREATOR_H__

#include <stdint.h>
#include "vkz_structs_inner.h"

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

    struct PassCreateInfo
    {
        uint16_t    idx;

        PassExeQueue queue;

        // bind pass with pipeline, thus implicitly bind with program
        uint16_t    programId;
        uint16_t    vtxBindingNum;
        uint16_t    vtxAttrNum;
        PipelineConfig pipelineConfig;

        std::vector<uint16_t>   vtxBindingIdxs;
        std::vector<uint16_t>   vtxAttrIdxs;
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

#endif // __VKZ_RES_CREATOR_H__