#include "file_helper.h"

#include "kage.h"
#include "common.h"
#include <ktx.h>

#include "entry/entry.h" // for allocator
#include "bimg/decode.h"

const kage::ImageHandle loadKtxFromFile(const char* _name, const char* _path, textureResolution& _outRes /*= {}*/)
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

    _outRes.width = ktxTex->baseWidth;
    _outRes.height = ktxTex->baseHeight;

    ktxTexture_Destroy(ktxTex);

    return hImg;
}


const char* getExtension(const char* _path)
{
    const char* extension = strrchr(_path, '.');
    if (!extension || extension == _path) {
        kage::message(kage::error, "Invalid file format: %s", _path);
        return nullptr;
    }
    extension++; // Skip the dot
    return extension;
}

const kage::ImageHandle loadImageFromFile(const char* _name, const char* _path, textureResolution& _outRes /*= {}*/)
{
    const char* ext = getExtension(_path);
    if (strcmp(ext, "ktx") == 0)
    {
        return loadKtxFromFile(_name, _path, _outRes);
    }

    kage::message(kage::error, "Unsupported file format: %s", _path);
    return { kage::kInvalidHandle };
}

const kage::ImageHandle loadImageFromMemory(const char* _name, const void* _data, uint32_t _size, textureResolution& _outRes /*= {}*/)
{
    bimg::ImageContainer* imageContainer = bimg::imageParse(entry::getAllocator(), _data, _size);
    if (imageContainer == nullptr)
    {
        kage::message(kage::error, "Failed to parse image from memory");
        return { kage::kInvalidHandle };
    }

    kage::ImageDesc desc = {};
    desc.width = imageContainer->m_width;
    desc.height = imageContainer->m_height;
    desc.numMips = imageContainer->m_numMips;
    desc.numLayers = imageContainer->m_numLayers;
    desc.type = imageContainer->m_cubeMap ? kage::ImageType::type_2d : kage::ImageType::type_3d;
    desc.viewType = imageContainer->m_cubeMap ? kage::ImageViewType::type_cube : kage::ImageViewType::type_2d;
    desc.usage = kage::ImageUsageFlagBits::sampled | kage::ImageUsageFlagBits::transfer_dst;

    const kage::Memory* mem = kage::copy(imageContainer->m_data, imageContainer->m_size);

    kage::ImageHandle img = kage::registTexture(_name, desc, mem);
    
    _outRes.width = imageContainer->m_width;
    _outRes.height = imageContainer->m_height;

    return img;
}
