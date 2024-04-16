#pragma once

#include "cmd_list.h"
#include "kage_rhi_vk.h"

namespace kage { namespace vk
{
    struct RHIContext_vk;

    class CmdList_vk : public CommandListI
    {
    public:
        CmdList_vk(VkCommandBuffer& _vkCmdBuf)
            : m_cmdBuf{ _vkCmdBuf }
        {
        }

        void setViewPort(uint32_t _firstViewport, uint32_t _viewportCount, const Viewport* _pViewports) override;
        void setScissorRect(uint32_t _firstScissor, uint32_t _scissorCount, const Rect2D* _pScissors) override;
        void pushConstants(const PassHandle _hPass, const void* _data, uint32_t _size) override;
        void pushDescriptorSets(const PassHandle _pass) override;

        void pushDescriptorSetWithTemplate(const PassHandle _hPass, const DescriptorSet* _descSets, uint32_t _count) override;

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
        void dispatch(const ShaderHandle _hPass, uint32_t _groupX, uint32_t _groupY, uint32_t _groupZ) override;

        // rendering
        void beginRendering(PassHandle _hPass) override;
        void endRendering() override;

        void draw() override;
        void drawIndexed(uint32_t _indexCount, uint32_t _instanceCount, uint32_t _firstIndex, int32_t _vertexOffset, uint32_t _firstInstance) override;
        void drawIndirect() override;
        void drawIndexedIndirect() override;
        void drawIndexedIndirectCount() override;
        void drawMeshTaskIndirect(BufferHandle _hBuf, uint32_t _offset, uint32_t _drawCount, uint32_t _stride) override;

        // barrier
        void barrier(BufferHandle _hBuf, AccessFlags _access, PipelineStageFlags _stage) override;
        void barrier(ImageHandle _hImg, AccessFlags _access, ImageLayout _layout, PipelineStageFlags _stage) override;
        void dispatchBarriers() override;

        void update(VkCommandBuffer& _vkCmdBuf);
    private:
        VkCommandBuffer m_cmdBuf{ VK_NULL_HANDLE };
    };
} // namespace vk
} // namespace kage