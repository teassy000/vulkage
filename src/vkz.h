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
    using ImageHandle = Handle<struct TextureHandleTag>;

    using ShaderHandleList = std::initializer_list<const ShaderHandle>;
    using PassHandleList = std::initializer_list<const PassHandle>;
    using BufferHandleList = std::initializer_list<const BufferHandle>;
    using TextureHandleList = std::initializer_list<const ImageHandle>;

    // resource management functions
    ShaderHandle registShader(const char* _name, const char* _path);
    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants = 0);

    BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition, const Memory* _mem = nullptr);
    ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);
    ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime = ResourceLifetime::transition);

    PassHandle registPass(const char* _name, PassDesc _desc);

    BufferHandle alias(const BufferHandle _buf);
    ImageHandle alias(const ImageHandle _img);

    void passReadBuffer(PassHandle _pass, BufferHandle _buf, ResInteractDesc _interact);
    void passWriteBuffer(PassHandle _pass, BufferHandle _buf, ResInteractDesc _interact);

    void passReadImage(PassHandle _pass, ImageHandle _img, ResInteractDesc _interact);
    void passWriteImage(PassHandle _pass, ImageHandle _img, ResInteractDesc _interact);

    void setPresentImage(ImageHandle _rt);

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
