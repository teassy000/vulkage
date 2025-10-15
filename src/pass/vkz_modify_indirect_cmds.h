#pragma once

#include "core/kage.h"
#include "core/kage_math.h"
#include "demo_structs.h"

enum ModifyCommandMode : uint32_t
{
    clear = 0,
    to_meshlet_cull = 1,
    to_triangle_cull = 2,
    to_soft_rasterize = 3,
    MAX_COUNT = 4
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
    kage::BufferHandle cmdBufOutAlias;
    kage::BufferHandle indirectCmdBufOutAlias;
};

void initModifyIndirectCmds(ModifyIndirectCmds& _cmds, const kage::BufferHandle _indirectCmdBuf, const kage::BufferHandle _cmdBuf, ModifyCommandMode _mode, uint32_t _width = 0, uint32_t _height = 0);
void updateModifyIndirectCmds(ModifyIndirectCmds& _cmds, uint32_t _width = 0, uint32_t _height = 0);