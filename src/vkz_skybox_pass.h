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

    kage::BufferHandle ib;
    kage::BufferHandle vb;

    kage::ImageHandle color;
    kage::ImageHandle cubemap;
    kage::BufferHandle trans;

    kage::ImageHandle colorOutAlias;
};

void initSkyboxPass(Skybox& _skybox
    , const kage::BufferHandle _trans
    , const kage::ImageHandle _color, const kage::ImageHandle _skycube
);

void updateSkybox(const Skybox& _skybox, uint32_t _w, uint32_t _h);