#pragma once

#include "core/kage.h"
#include "core/kage_math.h"
#include "demo_structs.h"

enum ModifyCommandMode : uint32_t
{
    clear = 0,
    to_meshlet_cull = 1,
    to_triangle_cull = 2,
    to_soft_raster = 3,
    to_hard_raster = 4,
    to_task = 5,
    MAX_COUNT = 6
};

struct ModifyIndirectCmds
{
    kage::PassHandle pass;
    kage::ShaderHandle cs;
    kage::ProgramHandle prog;

    uint32_t width;
    uint32_t height;

    ModifyCommandMode mode;

    kage::BufferHandle inCmdBuf;
    kage::BufferHandle inIndirectCmdBuf;
    
    // out alias
    kage::BufferHandle cmdBufOutAlias;
    kage::BufferHandle indirectCmdBufOutAlias;
};

void initModifyIndirectCmds(ModifyIndirectCmds& _cmds, const kage::BufferHandle _indirectCmdBuf, const kage::BufferHandle _cmdBuf, ModifyCommandMode _mode, PassStage _stage);
void updateModifyIndirectCmds(ModifyIndirectCmds& _cmds, uint32_t _width, uint32_t _height);