#include "vkz_modify_indirect_cmds.h"
#include "vkz_pass.h"
#include "demo_structs.h"
#include "core/kage_math.h"

#include <string>

// get pass name based on culling stage and pass type
void getBaseName(std::string& _out, const char* _baseName, ModifyCommandMode _mode)
{
    _out = _baseName;

    const char* stageStr[static_cast<uint32_t>(ModifyCommandMode::MAX_COUNT)] = {
        "_clear"
        , "_to_meshlet_cull"
        , "_to_triangle_cull" 
        , "_to_soft_raster"
        , "_to_hard_raster"
        , "_to_task"
    };

    _out += stageStr[static_cast<uint32_t>(_mode)];
}

void initModifyIndirectCmds(ModifyIndirectCmds& _cmds, const kage::BufferHandle _indirectCmdBuf, const kage::BufferHandle _cmdBuf, ModifyCommandMode _mode, PassStage _stage)
{
    std::string passName;
    getBaseName(passName, "modify_cmd", _mode);

    passName = getPassName(passName.c_str(), _stage);

    kage::ShaderHandle cs = kage::registShader(passName.c_str(), "shader/modify_indirect_cmds.comp.spv");

    kage::ProgramHandle prog = kage::registProgram(passName.c_str(), { cs }, sizeof(vec2));

    uint32_t pipelineSpecs[] = { uint32_t(_mode) };

    const kage::Memory* pConst = kage::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
    memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

    kage::PassDesc passDesc;
    passDesc.prog = prog;
    passDesc.queue = kage::PassExeQueue::compute;
    passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
    passDesc.pipelineSpecData = (void*)pConst->data;

    kage::PassHandle pass = kage::registPass(passName.c_str(), passDesc);

    kage::BufferHandle indirectCmdBufOutAlias = kage::alias(_indirectCmdBuf);
    kage::BufferHandle cmdBufOutAlias = kage::alias(_cmdBuf);


    kage::bindBuffer(pass
        , _indirectCmdBuf
        , Stage::compute_shader
        , Access::shader_write
        , indirectCmdBufOutAlias
    );

    kage::bindBuffer(pass
        , _cmdBuf
        , Stage::compute_shader
        , Access::shader_write
        , cmdBufOutAlias
    );

    _cmds.pass = pass;
    _cmds.cs = cs;
    _cmds.prog = prog;
    _cmds.mode = _mode;
    _cmds.width = 0;
    _cmds.height = 0;

    _cmds.inIndirectCmdBuf = _indirectCmdBuf;
    _cmds.inCmdBuf = _cmdBuf;
    _cmds.indirectCmdBufOutAlias = indirectCmdBufOutAlias;
    _cmds.cmdBufOutAlias = cmdBufOutAlias;
}

void recModifyIndirectCmds(const ModifyIndirectCmds& _cmds)
{
    kage::startRec(_cmds.pass);

    // only set constants when in soft raster mode
    uvec2 res = vec2(_cmds.width, _cmds.height);
    const kage::Memory* mem = kage::alloc(sizeof(res));
    bx::memCopy(mem->data, &res, mem->size);

    // set constants
    kage::setConstants(mem);
    
    // bind resources
    kage::Binding binds[] =
    {
        { _cmds.inIndirectCmdBuf,   BindingAccess::read_write, Stage::compute_shader },
        { _cmds.inCmdBuf,           BindingAccess::write,      Stage::compute_shader },
    };

    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatch(1, 1, 1);

    kage::endRec();
}

void updateModifyIndirectCmds(ModifyIndirectCmds& _cmd, uint32_t _width, uint32_t _height)
{
    _cmd.width = _width;
    _cmd.height = _height;
    recModifyIndirectCmds(_cmd);
}