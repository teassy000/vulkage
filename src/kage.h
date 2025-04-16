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
    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants = 0, const BindlessHandle _bindless = {});

    BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const Memory* _mem = nullptr, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const Memory* _mem = nullptr, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    BindlessHandle registBindless(const char* _name, const BindlessDesc& _desc);

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
        , PipelineStageFlags _stage
        , AccessFlags _access
        , const BufferHandle _outAlias = { kInvalidHandle }
    );

    void bindImage(
        PassHandle _hPass
        , ImageHandle _hImg
        , PipelineStageFlags _stage
        , AccessFlags _access
        , ImageLayout _layout
        , const ImageHandle _outAlias = { kInvalidHandle }
    );

    SamplerHandle sampleImage(
        PassHandle _hPass
        , ImageHandle _hImg
        , PipelineStageFlags _stage
        , SamplerFilter _filter
        , SamplerMipmapMode _mipmapMode
        , SamplerAddressMode _addrMode
        , SamplerReductionMode _reductionMode
    );

    void setBindlessTextures(BindlessHandle _bindless, const Memory* _mem, uint32_t _texCount, SamplerReductionMode _reductionMode);

    void setAttachmentOutput(
        const PassHandle _hPass
        , const ImageHandle _hImg
        , const ImageHandle _outAlias = { kInvalidHandle });

    void setPresentImage(ImageHandle _rt, uint32_t _mipLv = 0);

    void updateBuffer(
        const BufferHandle _buf
        , const Memory* _mem
        , uint32_t _offset = 0
        , uint32_t _size = 0
    );

    void updateImage(
        const ImageHandle _hImg
        , uint32_t _width
        , uint32_t _height
        , uint32_t _layers = 1
        , const Memory* _mem = nullptr
    );

    // APIs that would used in the render loop
    void startRec(const PassHandle _hPass);

    void setConstants(const Memory* _mem);

    void pushBindings(const Binding* _desc, uint16_t _count);

    void bindBindings(const Binding* _desc, uint16_t _descCount, const uint32_t* _arrayCounts, uint32_t _bindCount);

    void setColorAttachments(const Attachment* _colors, uint16_t _count);

    void setDepthAttachment(Attachment _depth);

    void setBindless(BindlessHandle _bindless);

    void dispatch(
        const uint32_t _groupCountX
        , const uint32_t _groupCountY
        , const uint32_t _groupCountZ
    );

    void dispatchIndirect(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offse
    );

    void fillBuffer(
        BufferHandle _hBuf
        , uint32_t _value
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

    // draw indirect
    void drawIndirect(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const uint32_t _count
        , const uint32_t _stride
    );

    // draw indirect count
    void drawIndirect(
        const uint32_t _offset
        , const BufferHandle _countBuf
        , const uint32_t _countOffset
        , const uint32_t _maxCount
        , const uint32_t _stride
    );

    // draw indexed
    void drawIndexed(
        const uint32_t _indexCount
        , const uint32_t _instanceCount
        , const uint32_t _firstIndex
        , const int32_t _vertexOffset
        , const uint32_t _firstInstance
    );

    // draw indexed indirect 
    void drawIndexed(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const uint32_t _count
        , const uint32_t _stride
    );

    // draw indexed indirect count
    void drawIndexed(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const BufferHandle _countBuf
        , const uint32_t _countOffset
        , const uint32_t _maxCount
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

    // end recording
    void endRec();
    // loop API Ends

    // memory related
    const Memory* alloc(uint32_t _sz);
    const Memory* copy(const void* _data, uint32_t _sz);
    const Memory* copy(const Memory* _mem);
    const Memory* makeRef(const void* _data, uint32_t _sz, ReleaseFn _releaseFn = nullptr, void* _userData = nullptr);

    // basic functions
    bool init(kage::Init _config = {});

    void render();
    void reset(uint32_t _width, uint32_t _height, uint32_t _reset);
    void shutdown();

    // naive profiling data
    double getPassTime(const PassHandle _hPass);
    double getGpuTime();
    uint64_t getPassClipping(const PassHandle _hPass);

    // ffx expose ========================================

    // set brixelizer instances
    void brx_setGeoInstances(const Memory* _desc);
    
    // set index/vertex buffers for brixelizer
    void brx_regGeoBuffers(const Memory* _bufs, BufferHandle _vtx, BufferHandle _idx);
    
    // set brixelizer buffers that required user provided data
    // 0 debug descs
    // 1 scratch buffer
    // 2 sdf-atlas
    // 3 brick-aabb
    // [4, 4 + FFX_BRIXELIZER_MAX_CASCADES - 1]: cascade aabb trees
    // [4 + FFX_BRIXELIZER_MAX_CASCADES, 4 + FFX_BRIXELIZER_MAX_CASCADES * 2 - 1]: cascade brick maps
    // [4 + FFX_BRIXELIZER_MAX_CASCADES * 2]: cascade infos
    // ------------------------------------------------------------------------------------------
    // this will mark these buffers as static to framegraph
    void brx_setUserResources(const Memory* _reses);

    void brx_setDebugInfos(const Memory* _info);
}
