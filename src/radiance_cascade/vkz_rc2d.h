#pragma once

#include "kage_math.h"
#include "common.h"
#include "kage.h"
#include "demo_structs.h"
#include "vkz_rc_common.h"

struct Rc2DBuild
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;

    kage::ImageHandle rcImage;
    kage::ImageHandle rcImageOutAlias;
};

struct Rc2DMerge
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;

    kage::ImageHandle in_rc;
    kage::SamplerHandle nearedSamp;
    kage::SamplerHandle linearSamp;

    kage::ImageHandle mergedCas;
    kage::ImageHandle mergedCasOutAlias;
};

struct Rc2DUse
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;

    kage::ImageHandle rt;
    
    kage::ImageHandle mergedCas;
    kage::SamplerHandle nearedSamp;
    kage::SamplerHandle linearSamp;

    kage::ImageHandle rtOutAlias;
};

struct Rc2D
{
    Rc2DBuild build;
    Rc2DMerge merge;
    Rc2DUse use;
};



void initRc2D(Rc2D& _rc, uint32_t _w, uint32_t _h, uint32_t _rayRes, uint32_t _casCount);
void updateRc2D(Rc2D& _rc, uint32_t _w, uint32_t _h, uint32_t _rayRes, uint32_t _casCount);