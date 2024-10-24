#pragma once
#include "kage_structs.h"


struct ktxTextureResoluton
{
    uint16_t width;
    uint16_t height;
    uint16_t depth;

    uint32_t dataSize;
};




const kage::ImageHandle loadKtxFromFile(const char* _name, const char* _path, ktxTextureResoluton & = ktxTextureResoluton{});