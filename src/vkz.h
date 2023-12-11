#pragma once

#include <stdint.h> // uint32_t
#include <initializer_list>

#include "macro.h"

namespace vkz
{
    static const uint16_t kInvalidHandle = UINT16_MAX;

    template <class HandleType>
    struct NO_VTABLE Handle {
        uint16_t idx;

        bool operator == (const Handle<HandleType>& rhs) const {
            return idx == rhs.idx;
        }
    };

    template <class HandleType>
    bool inline isValid(const Handle<HandleType>& handle) {
        return kInvalidHandle != handle.idx;
    }

    enum class PassExeQueue : uint16_t
    {
        Graphics = 0,
        AsyncCompute0 = 1,
        AsyncCompute1 = 2,
        AsyncCompute2 = 3,
        
        Count = 4,
    };


    enum class CompareOp : uint16_t
    {
        Op_Never = 0,
        Op_Less = 1,
        Op_Equal = 2,
        Op_Less_or_Equal = 3,
        Op_Greater = 4,
        Op_not_Equal = 5,
        Op_Greater_or_Equal = 6,
        Op_Alwas = 7,
        Op_MAX_ENUM = 0x7FFF
    };

    enum class ResourceFormat : uint16_t
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
        usage_single_frame,
        usage_multi_frame,
        usage_max_count,
    };

    enum class VertexInputRate {
        input_rate_vertex = 0,
        input_rate_instance = 1,
        input_rate_max = 0x7FFFFFFF
    };


    struct VertexBinding
    {
        uint16_t    binding;
        uint16_t    stride;
        uint16_t    inputRate;
    };

    struct VertexAttribute
    {
        uint32_t        location;
        uint32_t        binding;
        ResourceFormat   format;
        uint32_t        offset;
    };

    struct PipelineConfig
    {
        uint16_t enableDepthTest;
        uint16_t enableDepthWrite;
        CompareOp depthCompOp{ CompareOp::Op_Greater };
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
        ResourceUsage usage{ ResourceUsage::usage_single_frame };
    };

    struct ImageDesc {
        ResourceFormat format;
        
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint16_t layers;
        uint16_t mips;
        ResourceUsage usage{ ResourceUsage::usage_single_frame };
    }; 

    struct VertexBindingDesc
    {
        uint32_t            binding;
        uint32_t            stride;
        VertexInputRate     inputRate;
    };

    struct VertexAttributeDesc
    {
        uint32_t        location;
        uint32_t        binding;
        uint32_t        offset;
        ResourceFormat  format;
    };

    struct PassDesc
    {
        ProgramHandle   program;
        PassExeQueue    queue;

        uint16_t        vertexBindingNum;
        uint16_t        vertexAttributeNum;
        void*           vertexBindingInfos;
        void*           vertexAttributeInfos;

        PipelineConfig  pipelineConfig;
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
    ShaderHandle registShader(const char* _name, const char* _path);
    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants = 0);

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
