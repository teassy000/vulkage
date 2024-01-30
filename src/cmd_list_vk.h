#ifndef __CMD_LIST_H__
#define __CMD_LIST_H__

#include "cmd_list.h"
#include "volk.h"

namespace vkz
{
    class RHIContext_vk;

    class CmdList_vk : public ICommandList
    {
    public:
        CmdList_vk(VkCommandBuffer& _vkCmdBuf, const RHIContext_vk* _pCtx)
            : m_cmdBuf{ _vkCmdBuf }
            , m_pCtx{ _pCtx }
        {
        }

        void setViewPort(uint32_t _firstViewport, uint32_t _viewportCount, const Viewport* _pViewports) override;
        void setScissorRect(uint32_t _firstScissor, uint32_t _scissorCount, const Rect2D* _pScissors) override;
        void pushConstants(const PassHandle _hPass, const Memory* _mem) override;
        void pushDescriptorSets(const PassHandle _pass) override;

        void sampleImage(ImageHandle _hImg, uint32_t _binding, SamplerReductionMode _reductionMode) override;

        // resource binding
        void bindVertexBuffer(uint32_t _firstBinding, uint32_t _bindingCount, const BufferHandle* _pBuffers, const uint64_t* _pOffsets) override;
        void bindIndexBuffer(BufferHandle _buffer, int64_t _offset, IndexType _idxType) override;

        // copy / moving
        void fillBuffer() override;
        void copyBuffer() override;
        void blitImage() override;
        void copyImage() override;

        // compute
        void dispatch(uint32_t _groupX, uint32_t _groupY, uint32_t _groupZ) override;

        // rendering
        void beginRendering(PassHandle _hPass) override;
        void endRendering() override;

        void draw() override;
        void drawIndexed(uint32_t _indexCount, uint32_t _instanceCount, uint32_t _firstIndex, int32_t _vertexOffset, uint32_t _firstInstance) override;
        void drawIndirect() override;
        void drawIndexedIndirect() override;
        void drawIndexedIndirectCount() override;

        // barrier
        void barrier(BufferHandle _hBuf, AccessFlags _access, PipelineStageFlags _stage) override;
        void barrier(ImageHandle _hImg, AccessFlags _access, ImageLayout _layout, PipelineStageFlags _stage) override;
    private:
        VkCommandBuffer m_cmdBuf{ VK_NULL_HANDLE };
        const RHIContext_vk* m_pCtx{ nullptr };
    };
}

#endif