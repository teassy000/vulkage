#pragma once

#include <stdint.h> // uint32_t
#include <initializer_list>

#include "kage_structs.h"

namespace kage
{
    using ShaderHandleList = std::initializer_list<const ShaderHandle>;
    // rendering info
    bool checkSupports(VulkanSupportExtension _ext);

    // resource management functions
    ShaderHandle registShader(const char* _name, const char* _path);
    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants = 0);

    BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const Memory* _mem = nullptr, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const Memory* _mem = nullptr, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);

    PassHandle registPass(const char* _name, PassDesc _desc);

    BufferHandle alias(const BufferHandle _hBuf);
    ImageHandle alias(const ImageHandle _img);

    void bindVertexBuffer(PassHandle _hPass, BufferHandle _hBuf, const uint32_t _vtxCount = 0);
    void bindIndexBuffer(PassHandle _hPass, BufferHandle _hBuf, const uint32_t _idxCount = 0);

    void setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _maxCount);
    void setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset);

    void bindBuffer(
        PassHandle _hPass
        , BufferHandle _hBuf
        , uint32_t _binding
        , PipelineStageFlags _stage
        , AccessFlags _access
        , const BufferHandle _outAlias = { kInvalidHandle }
    );

    void bindImage(
        PassHandle _hPass
        , ImageHandle _hImg
        , uint32_t _binding
        , PipelineStageFlags _stage
        , AccessFlags _access
        , ImageLayout _layout
        , const ImageHandle _outAlias = { kInvalidHandle }
    );

    SamplerHandle sampleImage(
        PassHandle _hPass
        , ImageHandle _hImg
        , uint32_t _binding
        , PipelineStageFlags _stage
        , SamplerReductionMode _reductionMode
    );

    void setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias = { kInvalidHandle });

    void resizeImage(ImageHandle _hImg, uint32_t _width, uint32_t _height);

    // following interface would trigger in a render pass, which means the render-graph would affect.
    void fillBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _offset, const uint32_t _size, const uint32_t _value, const BufferHandle _outAlias);
    void copyBuffer(const PassHandle _hPass, const BufferHandle _hSrc, const BufferHandle _hDst, const uint32_t _size);
    void blitImage(const PassHandle _hPass, const ImageHandle _hSrc, const ImageHandle _hDst);

    void setPresentImage(ImageHandle _rt, uint32_t _mipLv = 0);

    void updatePushConstants(const PassHandle _hPass, const Memory* _mem);

    void updateThreadCount(
        const PassHandle _hPass
        , const uint32_t _threadCountX
        , const uint32_t _threadCountY
        , const uint32_t _threadCountZ
    );

    void updateBuffer(
        const BufferHandle _buf
        , const Memory* _mem
        , uint32_t _offset = 0
        , uint32_t _size = 0
    );

    void updateImage2D(
        const ImageHandle _hImg
        , uint16_t _width
        , uint16_t _height
        , uint16_t _mips
        , const Memory* _mem = nullptr
    );

    // APIs that would used in the render loop
    void startRec(const PassHandle _hPass);

    void setConstants(const Memory* _mem);

    void setBindings(Binding* _desc, uint16_t _count);

    void dispatch(
        const uint32_t _groupCountX
        , const uint32_t _groupCountY
        , const uint32_t _groupCountZ
    );

    void setVertexBuffer(BufferHandle _hBuf);
    
    void setIndexBuffer(
        BufferHandle _hBuf
        , uint32_t _offset
        , IndexType _type
    );
    
    void setViewport(
        int32_t _x
        , int32_t _y
        , uint32_t _width
        , uint32_t _height
    );

    void setScissor(
        int32_t _x
        , int32_t _y
        , uint32_t _width
        , uint32_t _height
    );

    // draw
    void draw(
        const uint32_t _vertexCount
        , const uint32_t _instanceCount
        , const uint32_t _firstVertex
        , const uint32_t _firstInstance
    );

    // draw indexed
    void draw(
        const uint32_t _indexCount
        , const uint32_t _instanceCount
        , const uint32_t _firstIndex
        , const int32_t _vertexOffset
        , const uint32_t _firstInstance
    );

    // draw indirect
    void draw(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const uint32_t _count
        , const uint32_t _stride
    );

    // draw mesh task
    void drawMeshTask(
        const uint32_t _groupCountX
        , const uint32_t _groupCountY
        , const uint32_t _groupCountZ
    );

    // draw mesh task indirect
    void drawMeshTask(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const uint32_t _count
        , const uint32_t _stride
    );

    // draw mesh task indirect count
    void drawMeshTask(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const BufferHandle _countBuf
        , const uint32_t _countOffset
        , const uint32_t _maxCount
        , const uint32_t _stride
    );

    void endRec();
    // loop API Ends

    // memory related
    const Memory* alloc(uint32_t _sz);
    const Memory* copy(const void* _data, uint32_t _sz);
    const Memory* copy(const Memory* _mem);
    const Memory* makeRef(const void* _data, uint32_t _sz, ReleaseFn _releaseFn = nullptr, void* _userData = nullptr);

    // basic functions
    bool init(kage::Init _config = {});

    void bake();
    void render();
    void reset(uint32_t _width, uint32_t _height, uint32_t _reset);
    void shutdown();

    // naive profiling data
    double getPassTime(const PassHandle _hPass);

}
