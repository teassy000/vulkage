#pragma once
#include "kage_structs.h"
#include "bimg/bimg.h"


struct textureResolution
{
    uint16_t width;
    uint16_t height;
};

kage::ResourceFormat bimgToKageFromat(bimg::TextureFormat::Enum _btf);

const char* getExtension(const char* _path);
const kage::ImageHandle loadImageFromFile(const char* _name, const char* _path, textureResolution & = textureResolution{});

