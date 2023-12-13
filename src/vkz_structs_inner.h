#ifndef __VKZ_STRUCTS_INNER_H__
#define __VKZ_STRUCTS_INNER_H__

#include "vkz_structs.h"

namespace vkz
{
    struct BufferCreateInfo : public BufferDesc
    {
        uint16_t    resNum;
    };

    struct BufferAliasInfo
    {
        uint16_t   bufId;
        uint16_t   size;
    };

    struct ImageCreateInfo : public ImageDesc
    {
        uint16_t    resNum;
    };

    struct ImageAliasInfo
    {
        uint16_t    imgId;
    };

    struct ShaderCreateInfo 
    {
        uint16_t shaderId;
        uint16_t pathLen;
    };
    
    struct ProgramDesc
    {
        uint16_t progId;
        uint16_t shaderNum;
        uint32_t sizePushConstants;
    };

    struct ProgramCreateInfo : public ProgramDesc
    {
        std::vector<uint16_t>   shaderIds;
    };

    struct PassCreateInfo : public PassDesc
    {
        uint16_t    passId;

        std::vector<uint16_t>   vtxBindingIdxs;
        std::vector<uint16_t>   vtxAttrIdxs;
    };
} // namespace vkz

#endif // __VKZ_STRUCTS_INNER_H__