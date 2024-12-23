#pragma once
#include "kage_structs.h"
#include "bimg/bimg.h"


struct textureResolution
{
    uint16_t width;
    uint16_t height;
};


const char* getExtension(const char* _path);
void getCurrFolder(char* _out, uint32_t _maxSize, const char* _path);

kage::ResourceFormat bimgToKageFromat(bimg::TextureFormat::Enum _btf);

void* load(const char* _path, uint32_t* _size);

const kage::ImageHandle loadImageFromFile(const char* _name, const char* _path, textureResolution & = textureResolution{});

