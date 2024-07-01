#pragma once

#include "kage.h"
#include "mesh.h"

struct Skybox
{
    kage::PassHandle pass;
    kage::ProgramHandle prog;
    kage::ShaderHandle vs;
    kage::ShaderHandle fs;

    kage::SamplerHandle cubemapSampler;

    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;

    kage::ImageHandle color;
    kage::ImageHandle cubemap;
    kage::BufferHandle trans;

    kage::ImageHandle colorOutAlias;
};

void initSkyboxPass(
    Skybox& _skybox
    , const kage::BufferHandle _trans
    , const kage::ImageHandle _color
);