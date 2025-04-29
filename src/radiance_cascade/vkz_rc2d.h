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
    
    kage::ImageHandle rc;
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

struct Rc2dInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t c0_dRes;
    uint32_t nCascades;
    float mpx;
    float mpy;
};

void initRc2D(Rc2D& _rc, const Rc2dInfo& _info);
void updateRc2D(Rc2D& _rc, const Rc2dInfo& _info, const Dbg_Rc2d& _dbg);