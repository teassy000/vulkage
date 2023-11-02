#include "common.h"
#include "handle.h"

namespace vkz
{

    vkz::ShaderHandle registShader(const char* _name, const char* code)
    {
        return ShaderHandle{kInvalidHandle};
    }

    vkz::ProgramHandle registProgram(const char* _name, ShaderHandleList shaders)
    {
        return ProgramHandle{ kInvalidHandle };
    }

    vkz::PipelineHandle registPipeline(const char* _name, ProgramHandle program)
    {
        return PipelineHandle{ kInvalidHandle };
    }

    vkz::BufferHandle registBuffer(const char* _name, BufferDesc _desc)
    {
        return BufferHandle{ kInvalidHandle };
    }

    vkz::TextureHandle registTexture(const char* _name, ImageDesc desc)
    {
        return TextureHandle{ kInvalidHandle };
    }

    vkz::PassHandle registPass(const char* _name, PassDesc desc)
    {
        return PassHandle{ kInvalidHandle };
    }

    void aliasWriteBufferByOrder(BufferHandle _buf, BufferHandleList _bufs)
    {

    }

    void aliasWriteTextureByOrder(TextureHandle _img, TextureHandleList _passes)
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

    void writeTextureByPass(PassHandle _pass, TextureHandle _texes)
    {

    }

}