#pragma once

#include "vkz_structs.h"

namespace vkz
{
    class ICommandList
    {
    public:
        virtual void setViewPort(uint32_t _firstViewport, uint32_t _viewportCount, const Viewport* _pViewports) = 0;
        virtual void setScissorRect(uint32_t _firstScissor, uint32_t _scissorCount, const Rect2D* _pScissors) = 0;
        virtual void pushConstants(const PassHandle _hPass, const void* _data, uint32_t _size) = 0;
        virtual void pushDescriptorSets(const PassHandle _hPass) = 0;
        virtual void pushDescriptorSetWithTemplate(const PassHandle _hPass, const uint16_t* _resIds, uint32_t _count, const ImageViewHandle* _imgViews, const ResourceType* _types, const SamplerHandle* _samplerIds) = 0;
        virtual void sampleImage(ImageHandle _hImg, uint32_t _binding, SamplerReductionMode _reductionMode) = 0;

        // resource binding
        virtual void bindVertexBuffer(uint32_t _firstBinding, uint32_t _bindingCount, const BufferHandle* _pBuffers, const uint64_t* _pOffsets) = 0;
        virtual void bindIndexBuffer(BufferHandle _buffer, int64_t _offset, IndexType _idxType) = 0;

        // copy / moving
        virtual void fillBuffer() = 0;
        virtual void copyBuffer() = 0;
        virtual void blitImage() = 0;
        virtual void copyImage() = 0;

        // compute
        virtual void dispatch(const ShaderHandle _hShader, uint32_t _groupX, uint32_t _groupY, uint32_t _groupZ) = 0;
        
        // rendering
        virtual void beginRendering(PassHandle _hPass) = 0;
        virtual void endRendering() = 0;

        virtual void draw() = 0;
        virtual void drawIndexed(uint32_t _indexCount, uint32_t _instanceCount, uint32_t _firstIndex, int32_t _vertexOffset, uint32_t _firstInstance) = 0;
        virtual void drawIndirect() = 0;
        virtual void drawIndexedIndirect() = 0;
        virtual void drawIndexedIndirectCount() = 0;

        // barrier
        virtual void barrier(BufferHandle _hBuf , AccessFlags _access, PipelineStageFlags _stage) = 0;
        virtual void barrier(ImageHandle _hImg, AccessFlags _access, ImageLayout _layout, PipelineStageFlags _stage) = 0;
        virtual void dispatchBarriers() = 0;
    };
};

