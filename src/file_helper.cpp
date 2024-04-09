#include "file_helper.h"

#include "kage.h"
#include "common.h"
#include <ktx.h>

const kage::ImageHandle loadTextureFromFile(const char* _name, const char* _path)
{
    assert(_path);

    ktxTexture* ktxTex = nullptr;
    {
        ktxResult succeed = KTX_SUCCESS;
        succeed = ktxTexture_CreateFromNamedFile(_path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);
        assert(succeed == KTX_SUCCESS);
    }

    kage::ImageDesc desc = {};
    desc.width = ktxTex->baseWidth;
    desc.height = ktxTex->baseHeight;
    desc.numMips = ktxTex->numLevels;
    desc.numLayers = ktxTex->isCubemap ? 6 : 1;
    desc.type = ktxTex->isArray ? kage::ImageType::type_3d : kage::ImageType::type_2d;
    desc.viewType = ktxTex->isCubemap ? kage::ImageViewType::type_cube : kage::ImageViewType::type_2d;
    desc.usage = kage::ImageUsageFlagBits::sampled | kage::ImageUsageFlagBits::transfer_dst;

    ktx_uint8_t* ktxTexData = ktxTexture_GetData(ktxTex);
    ktx_size_t ktxTexDataSize = ktxTexture_GetDataSize(ktxTex);
    const kage::Memory* mem = kage::alloc((uint32_t)ktxTexDataSize);
    memcpy(mem->data, ktxTex->pData, ktxTex->dataSize);

    kage::ImageHandle hImg = kage::registTexture(_name, desc, mem);

    ktxTexture_Destroy(ktxTex);

    return hImg;
}
