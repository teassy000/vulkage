#pragma once

#include "vkz.h"
#include "mesh.h"

struct SkyboxRendering
{
    vkz::PassHandle pass;
    vkz::ProgramHandle prog;
    vkz::ShaderHandle vs;
    vkz::ShaderHandle fs;

    vkz::SamplerHandle cubemapSampler;

    vkz::ImageHandle color;
    vkz::ImageHandle cubemap;
    vkz::BufferHandle trans;

    vkz::ImageHandle colorOutAlias;
};

void initSkyboxPass(
    SkyboxRendering& _skybox
    , const vkz::BufferHandle _trans
    , const vkz::ImageHandle _color
);