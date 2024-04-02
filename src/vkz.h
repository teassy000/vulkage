#pragma once

#include <stdint.h> // uint32_t
#include <initializer_list>

#include "vkz_structs.h"
#include "cmd_list.h"

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

    ImageViewHandle registImageView(const char* _name, const ImageHandle _hImg, const uint32_t _baseMip, const uint32_t _mipLevel);

    PassHandle registPass(const char* _name, PassDesc _desc);

    BufferHandle alias(const BufferHandle _hBuf);
    ImageHandle alias(const ImageHandle _img);

    void bindVertexBuffer(PassHandle _hPass, BufferHandle _hBuf, const uint32_t _vtxCount = 0);
    void bindIndexBuffer(PassHandle _hPass, BufferHandle _hBuf, const uint32_t _idxCount = 0);

    void setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _maxCount);
    void setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset);

    void bindBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias = {kInvalidHandle});
    void bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
        , const ImageHandle _outAlias = { kInvalidHandle }, const ImageViewHandle _hImgView = {kInvalidHandle});

    SamplerHandle sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, ImageLayout _layout, SamplerReductionMode _reductionMode);

    void setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias = { kInvalidHandle });

    void resizeTexture(ImageHandle _hImg, uint32_t _width, uint32_t _height);

    // following interface would trigger in a render pass, which means the render-graph would affect.
    void fillBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _offset, const uint32_t _size, const uint32_t _value, const BufferHandle _outAlias);
    void copyBuffer(const PassHandle _hPass, const BufferHandle _hSrc, const BufferHandle _hDst, const uint32_t _size);
    void blitImage(const PassHandle _hPass, const ImageHandle _hSrc, const ImageHandle _hDst);

    using RenderFuncPtr = void (*)(CommandListI& _cmdList, const void* _data, uint32_t size);
    void setCustomRenderFunc(const PassHandle _hPass, RenderFuncPtr _func, const Memory* _dataMem);
    void setPresentImage(ImageHandle _rt, uint32_t _mipLv = 0);

    void updateCustomRenderFuncData(const PassHandle _hPass, const Memory* _dataMem);
    void updateBuffer(const BufferHandle _buffer, const Memory* _mem);
    void updatePushConstants(const PassHandle _hPass, const Memory* _mem);

    void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ);

    // memory related
    const Memory* alloc(uint32_t _sz);
    const Memory* copy(const void* _data, uint32_t _sz);
    const Memory* copy(const Memory* _mem);
    const Memory* makeRef(const void* _data, uint32_t _sz, ReleaseFn _releaseFn = nullptr, void* _userData = nullptr);


    // basic functions
    bool init(kage::VKZInitConfig _config = {});
    bool shouldClose();
    void getWindowSize(uint32_t& _width, uint32_t _height);
    void resizeBackBuffer(uint32_t _width, uint32_t _height);
    void bake();
    void run();
    void shutdown();

    // callback func pointers
    using KeyCallbackFunc = void (*)(KeyEnum _key, KeyState _action, KeyModFlags _mods);
    void setKeyCallback(KeyCallbackFunc _func);

    using MouseMoveCallbackFunc = void (*)(double _x, double _y);
    void setMouseMoveCallback(MouseMoveCallbackFunc _func);

    using MouseButtonCallbackFunc = void (*)(int, int, int);
    void setMouseButtonCallback(MouseButtonCallbackFunc _func);

    // naive profiling data
    double getPassTime(const PassHandle _hPass);

}
