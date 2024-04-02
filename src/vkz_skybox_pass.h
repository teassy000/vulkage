#pragma once

#include "vkz.h"
#include "mesh.h"

struct SkyboxRendering
{
    kage::PassHandle pass;
    kage::ProgramHandle prog;
    kage::ShaderHandle vs;
    kage::ShaderHandle fs;

    kage::SamplerHandle cubemapSampler;

    kage::ImageHandle color;
    kage::ImageHandle cubemap;
    kage::BufferHandle trans;

    kage::ImageHandle colorOutAlias;
};

void initSkyboxPass(
    SkyboxRendering& _skybox
    , const kage::BufferHandle _trans
    , const kage::ImageHandle _color
);