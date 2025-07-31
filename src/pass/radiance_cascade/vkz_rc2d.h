#pragma once

#include "core/kage_math.h"
#include "core/common.h"
#include "core/kage.h"
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

    kage::ImageHandle mergedInterval;
    kage::ImageHandle mergedCasOutAlias;
};

struct Rc2DUse
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;

    kage::ImageHandle rt;
    
    kage::ImageHandle rc;
    kage::ImageHandle mergedInterval;
    kage::ImageHandle mergedProbe;
    kage::SamplerHandle nearedSamp;
    kage::SamplerHandle linearSamp;

    kage::ImageHandle rtOutAlias;
};

struct Rc2D
{
    Rc2DBuild build;
    Rc2DMerge mergeInterval;
    Rc2DMerge mergeProbe;
    Rc2DUse use;

    uint32_t rt_width;
    uint32_t rt_height;
    uint32_t nCascades;
};

struct Rc2dInfo
{
    uint32_t rt_width;
    uint32_t rt_height;
    uint32_t c0_dRes;
    uint32_t nCascades;
    float mpx;
    float mpy;
};

void initRc2D(Rc2D& _rc, const Rc2dInfo& _info);
void updateRc2D(Rc2D& _rc, const Rc2dInfo& _info, const Dbg_Rc2d& _dbg);