#pragma once
#include "kage_structs.h"


struct textureResolution
{
    uint16_t width;
    uint16_t height;
};

const char* getExtension(const char* _path);
const kage::ImageHandle loadImageFromFile(const char* _name, const char* _path, textureResolution & = textureResolution{});
const kage::ImageHandle loadImageFromMemory(const char* _name, const void* _data, uint32_t _size, textureResolution& _outRes = textureResolution{});

