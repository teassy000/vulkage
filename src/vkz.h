#pragma once

#include <stdint.h> // uint32_t
#include <initializer_list>

#include "vkz_structs.h"

namespace vkz
{
    using ReleaseFn = void (*)(void* _ptr, void* _userData);

    struct Memory
    {
        Memory() = delete;

        uint8_t* data;
        uint32_t size;
    };


    using ShaderHandle = Handle<struct ShaderHandleTag>;
    using ProgramHandle = Handle<struct ProgramHandleTag>;
    using PipelineHandle = Handle<struct PipelineHandleTag>;
    using PassHandle = Handle<struct PassHandleTag>;

    using BufferHandle = Handle<struct BufferHandleTag>;
    using TextureHandle = Handle<struct TextureHandleTag>;
    using RenderTargetHandle = Handle<struct RenderTargetHandleTag>;
    using DepthStencilHandle = Handle<struct DepthStencilHandleTag>;

    using ShaderHandleList = std::initializer_list<const ShaderHandle>;
    using PassHandleList = std::initializer_list<const PassHandle>;
    using BufferHandleList = std::initializer_list<const BufferHandle>;
    using TextureHandleList = std::initializer_list<const TextureHandle>;
    using RenderTargetHandleList = std::initializer_list<const RenderTargetHandle>;
    using DepthStencilHandleList = std::initializer_list<const DepthStencilHandle>;

    // resource management functions
    ShaderHandle registShader(const char* _name, const char* _path);
    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants = 0);

    
    BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    TextureHandle registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition, const Memory* _mem = nullptr);
    RenderTargetHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    DepthStencilHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);

    PassHandle registPass(const char* _name, PassDesc _desc);

    void aliasBuffer(BufferHandle** _aliases, const uint16_t _aliasCount, const BufferHandle _buf);
    void aliasTexture(TextureHandle** _aliases, const uint16_t _aliasCount, const TextureHandle _tex);
    void aliasRenderTarget(RenderTargetHandle** _aliases, const uint16_t _aliasCount, const RenderTargetHandle _rt);
    void aliasDepthStencil(DepthStencilHandle** _aliases, const uint16_t _aliasCount, const DepthStencilHandle _ds);

    BufferHandle aliasBuffer(const BufferHandle _buf);
    TextureHandle aliasTexture(const TextureHandle _tex);
    RenderTargetHandle aliasRenderTarget(const RenderTargetHandle _rt);
    DepthStencilHandle aliasDepthStencil(const DepthStencilHandle _ds);

    void passReadBuffer(PassHandle _pass, BufferHandle _buf, ResInteractDesc _interact);
    void passWriteBuffer(PassHandle _pass, BufferHandle _buf, ResInteractDesc _interact);

    void passReadTexture(PassHandle _pass, TextureHandle _img, ResInteractDesc _interact);
    void passWriteTexture(PassHandle _pass, TextureHandle _img, ResInteractDesc _interact);

    void passReadRT(PassHandle _pass, RenderTargetHandle _rt, ResInteractDesc _interact);
    void passWriteRT(PassHandle _pass, RenderTargetHandle _rt, ResInteractDesc _interact);

    void passReadDS(PassHandle _pass, DepthStencilHandle _ds, ResInteractDesc _interact);
    void passWriteDS(PassHandle _pass, DepthStencilHandle _ds, ResInteractDesc _interact);

    void passWriteBuffers(PassHandle _pass, BufferHandleList _bufList);
    void passReadBuffers(PassHandle _pass, BufferHandleList _bufList);

    void passWriteTextures(PassHandle _pass, TextureHandleList _texList);
    void passReadTextures(PassHandle _pass, TextureHandleList _texesList);

    void passWriteRTs(PassHandle _pass, RenderTargetHandleList _rtList);
    void passReadRTs(PassHandle _pass, RenderTargetHandleList _rtList);

    void passWriteDSs(PassHandle _pass, DepthStencilHandleList _dsList);
    void passReadDSs(PassHandle _pass, DepthStencilHandleList _dsList);

    void setMultiFrameBuffer(BufferHandleList _bufList);
    void setMultiFrameTexture(TextureHandleList _texList);
    void setMultiFrameRenderTarget(RenderTargetHandleList _rtList);
    void setMultiFrameDepthStencil(DepthStencilHandleList _dsList);

    void setPresentImage(RenderTargetHandle _rt);

    // meomory related
    const Memory* alloc(uint32_t _sz);
    const Memory* copy(const void* _data, uint32_t _sz);
    const Memory* copy(const Memory* _mem);
    const Memory* makeRef(const void* _data, uint32_t _sz, ReleaseFn _releaseFn = nullptr, void* _userData = nullptr);


    // engine basic functions
    bool init();
    void loop();
    void shutdown();
}
