#include "file_helper.h"

#include "vkz.h"
#include "common.h"
#include <ktx.h>

const vkz::ImageHandle loadTextureFromFile(const char* _name, const char* _path)
{
    assert(_path);

    ktxTexture* ktxTex = nullptr;
    {
        ktxResult succeed = KTX_SUCCESS;
        succeed = ktxTexture_CreateFromNamedFile(_path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);
        assert(succeed == KTX_SUCCESS);
    }

    vkz::ImageDesc desc = {};
    desc.width = ktxTex->baseWidth;
    desc.height = ktxTex->baseHeight;
    desc.numMips = ktxTex->numLevels;
    desc.numLayers = ktxTex->isCubemap ? 6 : 1;
    desc.type = ktxTex->isArray ? vkz::ImageType::type_3d : vkz::ImageType::type_2d;
    desc.viewType = ktxTex->isCubemap ? vkz::ImageViewType::type_cube : vkz::ImageViewType::type_2d;
    desc.usage = vkz::ImageUsageFlagBits::sampled | vkz::ImageUsageFlagBits::transfer_dst;

    ktx_uint8_t* ktxTexData = ktxTexture_GetData(ktxTex);
    ktx_size_t ktxTexDataSize = ktxTexture_GetDataSize(ktxTex);
    const vkz::Memory* mem = vkz::alloc((uint32_t)ktxTexDataSize);
    memcpy(mem->data, ktxTex->pData, ktxTex->dataSize);

    vkz::ImageHandle hImg = vkz::registTexture(_name, desc, mem);

    ktxTexture_Destroy(ktxTex);

    return hImg;
}
