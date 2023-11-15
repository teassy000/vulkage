#pragma once

#include <stdint.h> // uint32_t
#include <initializer_list>

namespace vkz
{
    static const uint16_t kInvalidHandle = UINT16_MAX;

    template <class HandleType>
    struct __declspec(novtable) Handle {
        uint16_t idx;

        bool operator == (const Handle<HandleType>& rhs) const {
            return idx == rhs.idx;
        }
    };

    template <class HandleType>
    bool inline isValid(const Handle<HandleType>& handle) {
        return kInvalidHandle != handle.idx;
    }

    enum class PassExeQueue : uint8_t
    {
        Graphics,
        AsyncCompute0,
        AsyncCompute1,
        AsyncCompute2,
        
        Count,
    };

    enum class TextureFormat : uint16_t
    {
        Unknown, // plain color formats below

        A8,
        R8,
        R8I,
        R8S,
        R8U,
        R16,
        R16I,
        R16U,
        R16F,
        R16S,
        R32,
        R32I,
        R32U,
        R32F,
        R32S,
        BGRA8,
        BGRA8I,
        BGRA8U,
        BGRA8F,
        BGRA8S,
        RGBA8,
        RGBA8I,
        RGBA8U,
        RGBA8F,
        RGBA8S,

        UnknownDepth, // Depth formats below.

        D16,
        D32,
        D16F,
        D32F,

        Count,
    };

    enum class ResourceUsage : uint8_t
    {
        SingleFrame,
        MultiFrame,
        Count,
    };


    using ShaderHandle = Handle<struct ShaderHandleTag>;
    using ProgramHandle = Handle<struct ProgramHandleTag>;
    using PipelineHandle = Handle<struct PipelineHandleTag>;
    using PassHandle = Handle<struct PassHandleTag>;
    
    using BufferHandle = Handle<struct BufferHandleTag>;
    using TextureHandle = Handle<struct TextureHandleTag>;
    using RenderTargetHandle = Handle<struct RenderTargetHandleTag>;
    using DepthStencilHandle = Handle<struct DepthStencilHandleTag>;

    struct BufferDesc {
        uint32_t size;
        uint32_t memPropFlags;
        ResourceUsage usage{ ResourceUsage::SingleFrame };
    };

    struct ImageDesc {
        TextureFormat format;
        
        uint16_t x, y, z;
        uint16_t layers;
        uint16_t mips;
        ResourceUsage usage{ ResourceUsage::SingleFrame };
    };

    struct PassDesc
    {
        PipelineHandle pipeline;
        ProgramHandle program;
        PassExeQueue queue;
    };

    typedef void (*ReleaseFn)(void* _ptr, void* _userData);

    struct Memory
    {
        Memory() = delete;

        uint8_t* data;
        uint32_t size;
    };

    using ShaderHandleList = std::initializer_list<const ShaderHandle>;
    using PassHandleList = std::initializer_list<const PassHandle>;
    using BufferHandleList = std::initializer_list<const BufferHandle>;
    using TextureHandleList = std::initializer_list<const TextureHandle>;
    using RenderTargetHandleList = std::initializer_list<const RenderTargetHandle>;
    using DepthStencilHandleList = std::initializer_list<const DepthStencilHandle>;

    // resource management functions
    ShaderHandle registShader(const char* _name, const Memory* code);
    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders);
    PipelineHandle registPipeline(const char* _name, ProgramHandle _program);

    BufferHandle registBuffer(const char* _name, const BufferDesc& _desc);
    TextureHandle registTexture(const char* _name, ImageDesc& _desc, const Memory* _mem = nullptr);
    RenderTargetHandle registRenderTarget(const char* _name, ImageDesc& _desc);
    DepthStencilHandle registDepthStencil(const char* _name, ImageDesc& _desc);

    PassHandle registPass(const char* _name, PassDesc _desc);

    void aliasBuffer(BufferHandle** _aliases, const uint16_t _aliasCount, const BufferHandle _buf);
    void aliasTexture(TextureHandle** _aliases, const uint16_t _aliasCount, const TextureHandle _tex);
    void aliasRenderTarget(RenderTargetHandle** _aliases, const uint16_t _aliasCount, const RenderTargetHandle _rt);
    void aliasDepthStencil(DepthStencilHandle** _aliases, const uint16_t _aliasCount, const DepthStencilHandle _ds);

    BufferHandle aliasBuffer(const BufferHandle _buf);
    TextureHandle aliasTexture(const TextureHandle _tex);
    RenderTargetHandle aliasRenderTarget(const RenderTargetHandle _rt);
    DepthStencilHandle aliasDepthStencil(const DepthStencilHandle _ds);

    void writeBuffers(PassHandle _pass, BufferHandleList _bufList);
    void readBuffers(PassHandle _pass, BufferHandleList _bufList);

    void writeTextures(PassHandle _pass, TextureHandleList _texList);
    void readTextures(PassHandle _pass, TextureHandleList _texesList);

    void writeRenderTargets(PassHandle _pass, RenderTargetHandleList _rtList);
    void readRenderTargets(PassHandle _pass, RenderTargetHandleList _rtList);

    void writeDepthStencils(PassHandle _pass, DepthStencilHandleList _dsList);
    void readDepthStencils(PassHandle _pass, DepthStencilHandleList _dsList);

    void setResultRenderTarget(RenderTargetHandle _rt);

    // meomory related
    const Memory* alloc(uint32_t _sz);
    const Memory* copy(const void* _data, uint32_t _sz);
    const Memory* copy(const Memory* _mem);
    const Memory* makeRef(const void* _data, uint32_t _sz, ReleaseFn _releaseFn = nullptr, void* _userData = nullptr);


    // engine basic functions
    bool init();
    void update();
    void shutdown();
}
