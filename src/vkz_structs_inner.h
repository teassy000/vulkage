#ifndef __VKZ_STRUCTS_INNER_H__
#define __VKZ_STRUCTS_INNER_H__

#include "vkz_structs.h"
#include "config.h"

namespace vkz
{
    struct ImageMetaData : public ImageDesc
    {
        ImageMetaData(const ImageDesc& desc) : ImageDesc(desc) {}

        uint16_t    imgId{ kInvalidHandle };

        uint16_t    bpp{ 4u };

        ResourceLifetime    lifetime{ ResourceLifetime::transition };
        ImageAspectFlags    aspectFlags{ 0 };
    };

    struct BufferMetaData : public BufferDesc
    {
        BufferMetaData(const BufferDesc& desc) : BufferDesc(desc) {}

        uint16_t    bufId{ kInvalidHandle };

        ResourceLifetime    lifetime{ ResourceLifetime::transition };
    };

    struct PassMetaData : public PassDesc
    {
        PassMetaData(const PassDesc& desc) : PassDesc(desc) {}

        uint16_t    passId{ kInvalidHandle };

        uint16_t    vertexBufferId{ kInvalidHandle };
        uint16_t    indexBufferId{ kInvalidHandle };
        uint16_t    writeDepthId{ kInvalidHandle };

        uint16_t    writeImageNum{ 0 };
        uint16_t    readImageNum{ 0 };

        uint16_t    readBufferNum{ 0 };
        uint32_t    writeBufferNum{ 0 };
    };

    struct BufferCreateInfo : public BufferDesc
    {
        uint16_t    aliasNum{ 0 };

        ResInteractDesc    barrierState;
    };

    struct BufferAliasInfo
    {
        uint16_t    bufId{ kInvalidHandle };
        uint32_t    size{ 0 };
    };

    struct ImageCreateInfo : public ImageDesc
    {
        uint16_t    aliasNum{ 0 };

        ImageAspectFlags    aspectFlags;
        ResInteractDesc    barrierState;
    };

    struct ImageAliasInfo
    {
        uint16_t    imgId{ kInvalidHandle };
    };

    struct ShaderCreateInfo 
    {
        uint16_t shaderId{ kInvalidHandle };
        uint16_t pathLen{ 0 };
    };
    
    struct ProgramDesc
    {
        uint16_t progId{ kInvalidHandle };
        uint16_t shaderNum{ 0 };
        uint32_t sizePushConstants{ 0 };

        uint16_t shaderIds[kMaxNumOfShaderInProgram]{ kInvalidHandle };
    };

    struct ProgramCreateInfo : public ProgramDesc
    {

    };
    

    struct PassCreateInfo : public PassDesc
    {
        uint16_t    passId{ kInvalidHandle };

        uint16_t    vertexBufferId{ kInvalidHandle };
        uint16_t    indexBufferId{ kInvalidHandle };
        uint16_t    writeDepthId{ kInvalidHandle };
        uint16_t    writeColorNum{ 0 };
        
        uint16_t    readImageNum{ 0 };

        uint16_t    readBufferNum{ 0 };
        uint32_t    writeBufferNum{ 0 };
    };


    struct RHIBrief
    {
        uint16_t finalPassId{ kInvalidHandle };
        uint16_t presentImageId{ kInvalidHandle };
    };

    struct RHI_Config
    {
        uint32_t windowWidth{0};
        uint32_t windowHeight{0};
    };
} // namespace vkz

#endif // __VKZ_STRUCTS_INNER_H__