#pragma once

#include <stdint.h> // uint32_t
#include <initializer_list>

#include "vkz_structs.h"
#include "cmd_list.h"

namespace vkz
{
    using ShaderHandleList = std::initializer_list<const ShaderHandle>;
    // resource management functions
    ShaderHandle registShader(const char* _name, const char* _path);
    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants = 0);

    BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);

    PassHandle registPass(const char* _name, PassDesc _desc);

    BufferHandle alias(const BufferHandle _hBuf);
    ImageHandle alias(const ImageHandle _img);

    void bindVertexBuffer(PassHandle _hPass, BufferHandle _hBuf);
    void bindIndexBuffer(PassHandle _hPass, BufferHandle _hBuf);

    void setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _maxCount);
    void setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset);

    void bindBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias = {kInvalidHandle});
    void sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, ImageLayout _layout, SamplerReductionMode _reductionMode);
    void bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
        , const ImageHandle _outAlias = { kInvalidHandle }, const uint32_t _baseMip = 0, const uint32_t _mipLevel = 1); // _baseMip and _mipLevel used means independent image view would use

    void setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias = { kInvalidHandle });

    // following interface would trigger in a render pass, which means the render-graph would affect.
    void fillBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _offset, const uint32_t _size, const uint32_t _value, const BufferHandle _outAlias);
    void copyBuffer(const PassHandle _hPass, const BufferHandle _hSrc, const BufferHandle _hDst, const uint32_t _size);
    void blitImage(const PassHandle _hPass, const ImageHandle _hSrc, const ImageHandle _hDst);

    using RenderFuncPtr = void (*)(ICommandList& _cmdList, const Memory* _dataMem);
    void setCustomRenderFunc(const PassHandle _hPass, RenderFuncPtr _func, const Memory* _dataMem);

    void setPresentImage(ImageHandle _rt);

    void updateCustomRenderFuncData(const PassHandle _hPass, const Memory* _dataMem);

    void updateBuffer(const BufferHandle _buffer, const Memory* _mem);
    void updatePushConstants(const PassHandle _hPass, const Memory* _mem);

    void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ);

    // meomory related
    const Memory* alloc(uint32_t _sz);
    const Memory* copy(const void* _data, uint32_t _sz);
    const Memory* copy(const Memory* _mem);
    const Memory* makeRef(const void* _data, uint32_t _sz, ReleaseFn _releaseFn = nullptr, void* _userData = nullptr);


    // engine basic functions
    bool init(vkz::VKZInitConfig _config = {});
    bool shouldClose();
    void bake();
    void run();
    void shutdown();
}
