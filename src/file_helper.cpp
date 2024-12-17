#include "file_helper.h"

#include "kage.h"
#include "common.h"
#include <ktx.h>

#include "scene.h"
#include "entry/entry.h" // for allocator
#include "bimg/decode.h"
#include "bx/file.h"

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

kage::ResourceFormat bimgToKageFromat(bimg::TextureFormat::Enum _btf)
{
    using KageFormat = kage::ResourceFormat;

    KageFormat result = KageFormat::undefined;
    // translate bimg format to kage format
    switch (_btf)
    {
    case bimg::TextureFormat::R1:       /*unsupport*/                               break;
    case bimg::TextureFormat::A8:       /*unsupport*/                               break;
    case bimg::TextureFormat::R8:       result = KageFormat::r8_unorm;              break;
    case bimg::TextureFormat::R8I:      result = KageFormat::r8_sint;               break;
    case bimg::TextureFormat::R8U:      result = KageFormat::r8_uint;               break;
    case bimg::TextureFormat::R8S:      result = KageFormat::r8_snorm;              break;
    case bimg::TextureFormat::R16:      result = KageFormat::r16_unorm;             break;
    case bimg::TextureFormat::R16I:     result = KageFormat::r16_sint;              break;
    case bimg::TextureFormat::R16U:     result = KageFormat::r16_uint;              break;
    case bimg::TextureFormat::R16F:     result = KageFormat::r16_sfloat;            break;
    case bimg::TextureFormat::R16S:     result = KageFormat::r16_snorm;             break;
    case bimg::TextureFormat::R32I:     result = KageFormat::r32_sint;              break;
    case bimg::TextureFormat::R32U:     result = KageFormat::r32_uint;              break;
    case bimg::TextureFormat::R32F:     result = KageFormat::r32_sfloat;            break;
    case bimg::TextureFormat::RG8:      result = KageFormat::r8g8_unorm;            break;
    case bimg::TextureFormat::RG8I:     result = KageFormat::r8g8_sint;             break;
    case bimg::TextureFormat::RG8U:     result = KageFormat::r8g8_uint;             break;
    case bimg::TextureFormat::RG8S:     result = KageFormat::r8g8_snorm;            break;
    case bimg::TextureFormat::RG16:     result = KageFormat::r16g16_unorm;          break;
    case bimg::TextureFormat::RG16I:    result = KageFormat::r16g16_sint;           break;
    case bimg::TextureFormat::RG16U:    result = KageFormat::r16g16_uint;           break;
    case bimg::TextureFormat::RG16F:    result = KageFormat::r16g16_sfloat;         break;
    case bimg::TextureFormat::RG16S:    result = KageFormat::r16g16_snorm;          break;
    case bimg::TextureFormat::RG32I:    result = KageFormat::r32g32_sint;           break;
    case bimg::TextureFormat::RG32U:    result = KageFormat::r32g32_uint;           break;
    case bimg::TextureFormat::RG32F:    result = KageFormat::r32g32_sfloat;         break;
    case bimg::TextureFormat::RGB8:     result = KageFormat::r8g8b8_unorm;          break;
    case bimg::TextureFormat::RGB8I:    result = KageFormat::r8g8b8_sint;           break;
    case bimg::TextureFormat::RGB8U:    result = KageFormat::r8g8b8_uint;           break;
    case bimg::TextureFormat::RGB8S:    result = KageFormat::r8g8b8_snorm;          break;
    case bimg::TextureFormat::RGB9E5F:  /*unsupport*/                               break;
    case bimg::TextureFormat::BGRA8:    result = KageFormat::b8g8r8a8_unorm;        break;
    case bimg::TextureFormat::RGBA8:    result = KageFormat::r8g8b8a8_unorm;        break;
    case bimg::TextureFormat::RGBA8I:   result = KageFormat::r8g8b8a8_sint;         break;
    case bimg::TextureFormat::RGBA8U:   result = KageFormat::r8g8b8a8_uint;         break;
    case bimg::TextureFormat::RGBA8S:   result = KageFormat::r8g8b8a8_snorm;        break;
    case bimg::TextureFormat::RGBA16:   result = KageFormat::r16g16b16a16_unorm;    break;
    case bimg::TextureFormat::RGBA16I:  result = KageFormat::r16g16b16a16_sint;     break;
    case bimg::TextureFormat::RGBA16U:  result = KageFormat::r16g16b16a16_uint;     break;
    case bimg::TextureFormat::RGBA16F:  result = KageFormat::r16g16b16a16_sfloat;   break;
    case bimg::TextureFormat::RGBA16S:  result = KageFormat::r16g16b16a16_snorm;    break;
    case bimg::TextureFormat::RGBA32I:  result = KageFormat::r32g32b32a32_sint;     break;
    case bimg::TextureFormat::RGBA32U:  result = KageFormat::r32g32b32a32_uint;     break;
    case bimg::TextureFormat::RGBA32F:  result = KageFormat::r32g32b32a32_sfloat;   break;
    case bimg::TextureFormat::B5G6R5:   result = KageFormat::b5g6r5_unorm;          break;
    case bimg::TextureFormat::R5G6B5:   result = KageFormat::r5g6b5_unorm;          break;
    case bimg::TextureFormat::BGRA4:    result = KageFormat::b4g4r4a4_unorm;        break;
    case bimg::TextureFormat::RGBA4:    result = KageFormat::r4g4b4a4_unorm;        break;
    case bimg::TextureFormat::BGR5A1:   result = KageFormat::b5g5r5a1_unorm;        break;
    case bimg::TextureFormat::RGB5A1:   result = KageFormat::r5g5b5a1_unorm;        break;
    case bimg::TextureFormat::RGB10A2:  /*unsupport*/                               break;
    case bimg::TextureFormat::RG11B10F: /*unsupport*/                               break;
    case bimg::TextureFormat::D16:      result = KageFormat::d16;                   break;
    case bimg::TextureFormat::D24:      /*unsupport*/                               break;
    case bimg::TextureFormat::D24S8:    result = KageFormat::d24_sfloat_s8_uint;    break;
    case bimg::TextureFormat::D32:      /*unsupport*/                               break;
    case bimg::TextureFormat::D16F:     /*unsupport*/                               break;
    case bimg::TextureFormat::D24F:     /*unsupport*/                               break;
    case bimg::TextureFormat::D32F:     result = KageFormat::d32_sfloat;            break;
    case bimg::TextureFormat::D0S8:     /*unsupport*/                               break;
    default:
        kage::message(kage::error, "Unsupported bimg format: %d", _btf);
        result = KageFormat::undefined;
    }

    return result;
}

// use the bx version
static void* load(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _path, uint32_t* _size)
{
    if (bx::open(_reader, _path))
    {
        uint32_t size = (uint32_t)bx::getSize(_reader);
        void* data = bx::alloc(_allocator, size);
        bx::read(_reader, data, size, bx::ErrorAssert{});
        bx::close(_reader);

        if (nullptr != _size)
        {
            *_size = size;
        }
        return data;
    }

    kage::message(kage::error, "failed to load: %s", _path);
    return nullptr;
}

kage::ImageHandle loadWithBimg(const char* _name, const char* _path, textureResolution& _outRes)
{
    uint32_t sz = 0;
    void* data = load(entry::getFileReader(), entry::getAllocator(), _path, &sz);

    bimg::ImageContainer* imageContainer = bimg::imageParse(entry::getAllocator(), data, sz);
    if (imageContainer == nullptr)
    {
        kage::message(kage::error, "Failed to parse image from memory");
        return {kage::kInvalidHandle};
    }

    // convert rgb8 to rgba8
    if (imageContainer->m_format == bimg::TextureFormat::RGB8)
    {
        bimg::ImageContainer* rgba8Container = bimg::imageConvert(entry::getAllocator(), bimg::TextureFormat::RGBA8, *imageContainer);
        bimg::imageFree(imageContainer);
        imageContainer = rgba8Container;
    }

    const kage::Memory* mem = kage::alloc((uint32_t)imageContainer->m_size);
    memcpy(mem->data, imageContainer->m_data, imageContainer->m_size);

    kage::ImageDesc desc = {};
    desc.width = imageContainer->m_width;
    desc.height = imageContainer->m_height;
    desc.numMips = imageContainer->m_numMips;
    desc.numLayers = imageContainer->m_numLayers;
    desc.format = bimgToKageFromat(imageContainer->m_format);
    desc.type = (imageContainer->m_numLayers > 1) ? kage::ImageType::type_3d : kage::ImageType::type_2d;
    desc.viewType = imageContainer->m_cubeMap ? kage::ImageViewType::type_cube : kage::ImageViewType::type_2d;
    desc.usage = kage::ImageUsageFlagBits::sampled | kage::ImageUsageFlagBits::transfer_dst;

    kage::ImageHandle hImg = kage::registTexture(_name, desc, mem);

    _outRes.width = imageContainer->m_width;
    _outRes.height = imageContainer->m_height;

    return hImg;
}


const kage::ImageHandle loadImageFromFile(const char* _name, const char* _path, textureResolution& _outRes /*= {}*/)
{
    const char* ext = getExtension(_path);
    if (strcmp(ext, "ktx") == 0)
    {
        return loadKtxFromFile(_name, _path, _outRes);
    }
    
    return loadWithBimg(_name, _path, _outRes);
}