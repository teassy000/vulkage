#ifndef __VKZ_STRUCTS_INNER_H__
#define __VKZ_STRUCTS_INNER_H__

#include "vkz_structs.h"

namespace vkz
{
    struct BufferCreateInfo : public BufferDesc
    {
        uint16_t    aliasNum{ 0 };

        ResInteractDesc    barrierState;
    };

    struct BufferAliasInfo
    {
        uint16_t   bufId{ kInvalidHandle };
        uint16_t   size{ 0 };
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
    };

    struct ProgramCreateInfo : public ProgramDesc
    {

    };
    

    struct PassCreateInfo : public PassDesc
    {
        uint16_t    passId{ kInvalidHandle };

        uint16_t    writeDepthId{ kInvalidHandle };
        uint16_t    writeColorNum{ 0 };
        
        uint16_t    readImageNum{ 0 };
        uint16_t    rwBufferNum{ 0 };
    };
} // namespace vkz

#endif // __VKZ_STRUCTS_INNER_H__