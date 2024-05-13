#include "cmd_list_vk.h"
#include "util.h"
#include "rhi_context_vk.h"

namespace kage { namespace vk
{
    extern RHIContext_vk* s_renderVK;

    VkIndexType getIndexType(IndexType _type)
    {
        switch (_type)
        {
        case kage::IndexType::uint16:
            return VK_INDEX_TYPE_UINT16;
        case kage::IndexType::uint32:
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
        const Program_vk& prog = s_renderVK->getProgram(_hPass);

        vkCmdPushConstants(m_cmdBuf, prog.layout, prog.pushConstantStages, 0, _size, _data);
    }

    void CmdList_vk::pushDescriptorSets(const PassHandle _hPass)
    {
        s_renderVK->pushDescriptorSetWithTemplates(m_cmdBuf, _hPass.id);
    }

    void CmdList_vk::pushDescriptorSetWithTemplate(const PassHandle _hPass, const DescSet* _descSets, uint32_t _count)
    {
        const Program_vk& prog = s_renderVK->getProgram(_hPass);
        stl::vector<DescriptorInfo> descInfos(_count);

        for (uint32_t ii = 0; ii < _count; ++ii)
        {
            if (_descSets[ii].type == ResourceType::image)
            {
                descInfos[ii] = s_renderVK->getImageDescInfo2(_descSets[ii].img, _descSets[ii].mip, _descSets[ii].sampler);
            }
            else if (_descSets[ii].type == ResourceType::buffer)
            {
                descInfos[ii] = s_renderVK->getBufferDescInfo(_descSets[ii].buf);
            }
            else
            {
                message(DebugMessageType::error, "not a valid resource type!");
            }
        }

        vkCmdPushDescriptorSetWithTemplateKHR(m_cmdBuf, prog.updateTemplate, prog.layout, 0, descInfos.data());
    }

    void CmdList_vk::sampleImage(ImageHandle _hImg, uint32_t _binding, SamplerReductionMode _reductionMode)
    {
        const VkImage& img = s_renderVK->getVkImage(_hImg);
    }

    void CmdList_vk::bindVertexBuffer(uint32_t _firstBinding, uint32_t _bindingCount, const BufferHandle* _pBuffers, const uint64_t* _pOffsets)
    {
        // get buffers
        stl::vector<VkBuffer> bufs(_bindingCount);
        for (uint32_t ii = 0; ii < _bindingCount; ++ii)
        {
            bufs[ii] = s_renderVK->getVkBuffer(_pBuffers[ii]);
        }

        // get offsets
        stl::vector<VkDeviceSize> offsets(_bindingCount);
        for (uint32_t ii = 0; ii < _bindingCount; ++ii)
        {
            offsets[ii] = _pOffsets[ii];
        }

        vkCmdBindVertexBuffers(m_cmdBuf, _firstBinding, _bindingCount, &bufs[0], offsets.data());
    }

    void CmdList_vk::bindIndexBuffer(BufferHandle _buffer, int64_t _offset, IndexType _idxType)
    {
        // get buffer
        VkBuffer buf = s_renderVK->getVkBuffer(_buffer);
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

    void CmdList_vk::dispatch(const ShaderHandle _hShader, uint32_t _groupX, uint32_t _groupY, uint32_t _groupZ)
    {
        const Shader_vk& shader = s_renderVK->getShader(_hShader);

        vkCmdDispatch(m_cmdBuf
            , calcGroupCount(_groupX, shader.localSizeX)
            , calcGroupCount(_groupY, shader.localSizeY)
            , calcGroupCount(_groupZ, shader.localSizeZ)
        );
    }

    void CmdList_vk::beginRendering(PassHandle _hPass)
    {
        s_renderVK->beginRendering(m_cmdBuf, _hPass.id);
    }

    void CmdList_vk::endRendering()
    {
        s_renderVK->endRendering(m_cmdBuf);
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

    void CmdList_vk::drawMeshTaskIndirect(BufferHandle _hBuf, uint32_t _offset, uint32_t _drawCount, uint32_t _stride)
    {
        VkBuffer buf = s_renderVK->getVkBuffer(_hBuf);
        vkCmdDrawMeshTasksIndirectEXT(m_cmdBuf, buf, _offset, _drawCount, _stride);
    }

    void CmdList_vk::barrier(BufferHandle _hBuf, AccessFlags _access, PipelineStageFlags _stage)
    {
        s_renderVK->barrier(_hBuf, _access, _stage);
    }

    void CmdList_vk::barrier(ImageHandle _hImg, AccessFlags _access, ImageLayout _layout, PipelineStageFlags _stage)
    {
        s_renderVK->barrier(_hImg, _access, _layout, _stage);
    }

    void CmdList_vk::dispatchBarriers()
    {
        s_renderVK->dispatchBarriers();
    }

    void CmdList_vk::update(VkCommandBuffer& _vkCmdBuf)
    {
        m_cmdBuf = _vkCmdBuf;
    }

} // namespace vk
} // namespace kage
