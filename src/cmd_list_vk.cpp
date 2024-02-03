#include "cmd_list_vk.h"
#include "util.h"
#include "rhi_context_vk.h"

namespace vkz
{
    VkIndexType getIndexType(IndexType _type)
    {
        switch (_type)
        {
        case vkz::IndexType::uint16:
            return VK_INDEX_TYPE_UINT16;
        case vkz::IndexType::uint32:
            return VK_INDEX_TYPE_UINT32;
        default:
            return VK_INDEX_TYPE_UINT16;
        }
    }

    void CmdList_vk::setViewPort(uint32_t _firstViewport, uint32_t _viewportCount, const Viewport* _pViewports)
    {
        stl::vector<VkViewport> viewports(_viewportCount);
        for (uint32_t ii = 0; ii < _viewportCount; ++ii)
        {
            viewports[ii] = { _pViewports[ii].x, _pViewports[ii].y, _pViewports[ii].width, _pViewports[ii].height, _pViewports[ii].minDepth, _pViewports[ii].maxDepth };
        }

        vkCmdSetViewport(m_cmdBuf, _firstViewport, _viewportCount, viewports.data());
    }

    void CmdList_vk::setScissorRect(uint32_t _firstScissor, uint32_t _scissorCount, const Rect2D* _pScissors)
    {
        stl::vector<VkRect2D> scissors(_scissorCount);
        for (uint32_t ii = 0; ii < _scissorCount; ++ii)
        {
            scissors[ii] = { { _pScissors[ii].offset.x, _pScissors[ii].offset.y }, { _pScissors[ii].extent.width, _pScissors[ii].extent.height } };
        }

        vkCmdSetScissor(m_cmdBuf, _firstScissor, _scissorCount, scissors.data());
    }

    void CmdList_vk::pushConstants(const PassHandle _hPass, const void* _data, uint32_t _size)
    {
        const Program_vk& prog = m_pCtx->getProgram(_hPass);

        vkCmdPushConstants(m_cmdBuf, prog.layout, prog.pushConstantStages, 0, _size, _data);
    }

    void CmdList_vk::pushDescriptorSets(const PassHandle _pass)
    {
        m_pCtx->pushDescriptorSetWithTemplates(m_cmdBuf, _pass.id);
    }

    void CmdList_vk::sampleImage(ImageHandle _hImg, uint32_t _binding, SamplerReductionMode _reductionMode)
    {

    }

    void CmdList_vk::bindVertexBuffer(uint32_t _firstBinding, uint32_t _bindingCount, const BufferHandle* _pBuffers, const uint64_t* _pOffsets)
    {
        // get buffers
        stl::vector<VkBuffer> buffers(_bindingCount);
        for (uint32_t ii = 0; ii < _bindingCount; ++ii)
        {
            buffers[ii] = m_pCtx->getVkBuffer(_pBuffers[ii]);
        }

        // get offsets
        stl::vector<VkDeviceSize> offsets(_bindingCount);
        for (uint32_t ii = 0; ii < _bindingCount; ++ii)
        {
            offsets[ii] = _pOffsets[ii];
        }

        vkCmdBindVertexBuffers(m_cmdBuf, _firstBinding, _bindingCount, buffers.data(), offsets.data());
    }

    void CmdList_vk::bindIndexBuffer(BufferHandle _buffer, int64_t _offset, IndexType _idxType)
    {
        // get buffer
        VkBuffer buf = m_pCtx->getVkBuffer(_buffer);
        // get idx type
        vkCmdBindIndexBuffer(m_cmdBuf, buf, _offset, getIndexType(_idxType));
    }

    void CmdList_vk::fillBuffer()
    {

    }

    void CmdList_vk::copyBuffer()
    {

    }

    void CmdList_vk::blitImage()
    {

    }

    void CmdList_vk::copyImage()
    {

    }

    void CmdList_vk::dispatch(uint32_t _groupX, uint32_t _groupY, uint32_t _groupZ)
    {
        vkCmdDispatch(m_cmdBuf, _groupX, _groupY, _groupZ);
    }

    void CmdList_vk::beginRendering(PassHandle _hPass)
    {
        m_pCtx->beginRendering(m_cmdBuf, _hPass.id);
    }

    void CmdList_vk::endRendering()
    {
        m_pCtx->endRendering(m_cmdBuf);
    }

    void CmdList_vk::draw()
    {

    }

    void CmdList_vk::drawIndexed(uint32_t _indexCount, uint32_t _instanceCount, uint32_t _firstIndex, int32_t _vertexOffset, uint32_t _firstInstance)
    {
        vkCmdDrawIndexed(m_cmdBuf, _indexCount, _instanceCount, _firstIndex, _vertexOffset, _firstInstance);
    }

    void CmdList_vk::drawIndirect()
    {

    }

    void CmdList_vk::drawIndexedIndirect()
    {

    }

    void CmdList_vk::drawIndexedIndirectCount()
    {

    }

    void CmdList_vk::barrier(BufferHandle _hBuf, AccessFlags _access, PipelineStageFlags _stage)
    {

    }

    void CmdList_vk::barrier(ImageHandle _hImg, AccessFlags _access, ImageLayout _layout, PipelineStageFlags _stage)
    {

    }

}

