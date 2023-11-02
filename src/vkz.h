#pragma once

namespace vkz
{
    static const uint16_t kInvalidHandle = UINT16_MAX;

    template <class HandleType>
    struct Handle {
        uint16_t idx;
    };

    template <class HandleType>
    bool inline isValid(const Handle<HandleType>& handle) {
        return kInvalidHandle != handle.idx;
    }

    typedef enum DeviceQueueType
    {
        DeviceQueue_Graphics,
        DeviceQueue_AsyncCompute0,
        DeviceQueue_AsyncCompute1,
        DeviceQueue_AsyncCompute2,

        DeviceQueue_Count,
    } DeviceQueueType;

    typedef enum AccessType
    {
        Access_Read,
        Access_Write,
        Access_ReadWrite,

        Access_Count,
    } AccessType;

    typedef enum TextureFormatType
    {
        TextureFormat_Unknown, // plain color formats below

        TextureFormat_A8,
        TextureFormat_R8,
        TextureFormat_R8I,
        TextureFormat_R8S,
        TextureFormat_R8U,
        TextureFormat_R16I,
        TextureFormat_R16U,
        TextureFormat_R16F,
        TextureFormat_R16S,
        TextureFormat_BGRA8,
        TextureFormat_BGRA8I,
        TextureFormat_BGRA8U,
        TextureFormat_BGRA8F,
        TextureFormat_BGRA8S,
        TextureFormat_RGBA8,
        TextureFormat_RGBA8I,
        TextureFormat_RGBA8U,
        TextureFormat_RGBA8F,
        TextureFormat_RGBA8S,

        TextureFormat_UnknownDepth, // Depth formats below.

        TextureFormat_D16,
        TextureFormat_D32,
        TextureFormat_D16F,
        TextureFormat_D32F,

        TextureFormat_Count,
    } TextureFormatType;


    using ShaderHandle = Handle<struct ShaderHandleTag>;
    using ProgramHandle = Handle<struct ProgramHandleTag>;
    using PipelineHandle = Handle<struct PipelineHandleTag>;
    using PassHandle = Handle<struct PassHandleTag>;
    
    using BufferHandle = Handle<struct BufferHandleTag>;
    using TextureHandle = Handle<struct TextureHandleTag>;

    struct BufferDesc {
        uint32_t size;
        uint32_t memPropFlags;
    };

    struct ImageDesc {
        uint32_t x, y, z;
        uint16_t mips;

        uint16_t layout;
        uint8_t type;
        uint8_t viewType;
    };

    struct PassDesc
    {
        PipelineHandle pipeline;
        ProgramHandle program;
        DeviceQueueType queue;
    };

    using ShaderHandleList = std::initializer_list<const ShaderHandle>;
    using BufferHandleList = std::initializer_list<const BufferHandle>;
    using TextureHandleList = std::initializer_list<const TextureHandle>;
    using PassHandleList = std::initializer_list<const PassHandle>;
    
    // resource management functions
    ShaderHandle registShader(const char* _name, const char* code);
    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders);
    PipelineHandle registPipeline(const char* _name, ProgramHandle _program);

    BufferHandle registBuffer(const char* _name, BufferDesc _desc);
    TextureHandle registTexture(const char* _name, ImageDesc _desc);

    PassHandle registPass(const char* _name, PassDesc _desc);

    // must set alias write by order manually when multiple passes write to the same buffer
    void aliasWriteBufferByOrder(BufferHandle _buf, BufferHandleList _passes);
    void aliasWriteTextureByOrder(TextureHandle _img, TextureHandleList _passes);
    
    void writeBuffersByPass(PassHandle _pass, BufferHandleList _bufs);
    void readBuffersByPass(PassHandle _pass, BufferHandleList _bufs);

    void readTextureByPass(PassHandle _pass, TextureHandle _texes);
    void writeTextureByPass(PassHandle _pass, TextureHandle _texes);

    // engine basic functions
    bool init();
    void shutdown();
}
