#include "common.h"
#include "handle.h"
#include "vkz.h"

#include "config.h"
#include "name.h"

#include "framegraph_2.h"
#include "resource_manager.h"



namespace vkz
{
    struct Context
    {
        // Context API
        ShaderHandle registShader(const char* _name, const char* code);
        ProgramHandle registProgram(const char* _name, ShaderHandleList shaders);
        PipelineHandle registPipeline(const char* _name, ProgramHandle program);
        PassHandle registPass(const char* _name, PassDesc _desc);

        BufferHandle registBuffer(const char* _name, BufferDesc _desc);
        TextureHandle registTexture(const char* _name, ImageDesc _desc);
        RenderTargetHandle registRenderTarget(const char* _name, ImageDesc _desc);
        DepthStencilHandle registDepthStencil(const char* _name, ImageDesc _desc);


        HandleArrayT<kMaxNumOfShaderHandle> m_shaderHandles;
        HandleArrayT<kMaxNumOfProgramHandle> m_programHandles;
        HandleArrayT<kMaxNumOfPipelineHandle> m_pipelineHandles;
        HandleArrayT<kMaxNumOfPassHandle> m_passHandles;
        HandleArrayT<kMaxNumOfRenderTargetHandle> m_renderTargetHandles;
        HandleArrayT<kMaxNumOfDepthStencilHandle> m_depthStencilHandles;
        HandleArrayT<kMaxNumOfTextureHandle> m_textureHandles;
        HandleArrayT<kMaxNumOfBufferHandle> m_bufferHandles;

        // frame graph
        Framegraph2 m_frameGraph;

        // store actual resource data, after frame graph processed
        // so all resources are late allocated
        ResMgr m_resMgr;
    };

    ShaderHandle Context::registShader(const char* _name, const char* _code)
    {
        uint16_t idx = m_shaderHandles.alloc();

        // TODO: backup resources info
        // maybe in resource manager, setup those data first, and try to allocate resources after frame graph processed

        return ShaderHandle{ idx };
    }

    ProgramHandle Context::registProgram(const char* _name, ShaderHandleList _shaders)
    {
        return ProgramHandle{ kInvalidHandle };
    }

    PipelineHandle Context::registPipeline(const char* _name, ProgramHandle _program)
    {
        return PipelineHandle{ kInvalidHandle };
    }

    PassHandle Context::registPass(const char* _name, PassDesc _desc)
    {
        return PassHandle{ kInvalidHandle };
    }

    BufferHandle Context::registBuffer(const char* _name, BufferDesc _desc)
    {
        return BufferHandle{ kInvalidHandle };
    }

    TextureHandle Context::registTexture(const char* _name, ImageDesc _desc)
    {
        return TextureHandle{ kInvalidHandle };
    }

    RenderTargetHandle Context::registRenderTarget(const char* _name, ImageDesc _desc)
    {
        return RenderTargetHandle{ kInvalidHandle };
    }

    DepthStencilHandle Context::registDepthStencil(const char* _name, ImageDesc _desc)
    {
        return DepthStencilHandle{ kInvalidHandle };
    }


    static Context* s_ctx = nullptr;


    ShaderHandle registShader(const char* _name, const char* _code)
    {
        return s_ctx->registShader(_name, _code);
    }

    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders)
    {
        return s_ctx->registProgram(_name, _shaders);
    }

    PipelineHandle registPipeline(const char* _name, ProgramHandle _program)
    {
        return s_ctx->registPipeline(_name, _program);
    }

    BufferHandle registBuffer(const char* _name, BufferDesc _desc)
    {
        return s_ctx->registBuffer(_name, _desc);
    }

    TextureHandle registTexture(const char* _name, ImageDesc _desc)
    {
        return s_ctx->registTexture(_name, _desc);
    }


    RenderTargetHandle registRenderTarget(const char* _name, ImageDesc _desc)
    {
        return s_ctx->registRenderTarget(_name, _desc);
    }

    DepthStencilHandle registDepthStencil(const char* _name, ImageDesc _desc)
    {
        return s_ctx->registDepthStencil(_name, _desc);
    }

    PassHandle registPass(const char* _name, PassDesc _desc)
    {
        return PassHandle{ kInvalidHandle };
    }

    void aliasWriteBufferByOrder(BufferHandle _buf, BufferHandleList _bufs)
    {

    }

    void aliasWriteTextureByOrder(TextureHandle _img, TextureHandleList _passes)
    {

    }

    void aliasWriteRenderTargetByOrder(RenderTargetHandle _img, RenderTargetHandleList _passes)
    {

    }

    void aliasWriteDepthStencilByOrder(DepthStencilHandle _img, DepthStencilHandleList _passes)
    {

    }

    void writeBuffersByPass(PassHandle _pass, BufferHandleList _bufs)
    {

    }

    void readBuffersByPass(PassHandle _pass, BufferHandleList _bufs)
    {

    }

    void readTextureByPass(PassHandle _pass, TextureHandle _texes)
    {

    }

    void writeRenderTargetByPass(PassHandle _pass, RenderTargetHandle _texes)
    {

    }

    void readRenderTargetByPass(PassHandle _pass, RenderTargetHandle _texes)
    {

    }

    void writeDepthStencilByPass(PassHandle _pass, DepthStencilHandle _texes)
    {

    }

    void readDepthStencilByPass(PassHandle _pass, DepthStencilHandle _texes)
    {

    }

    void writeTextureByPass(PassHandle _pass, TextureHandle _texes)
    {

    }


    // main data here
    bool init()
    {
        s_ctx = new Context();

        return true;
    }

    void shutdown()
    {
        s_ctx = nullptr;
        delete s_ctx;
    }




}