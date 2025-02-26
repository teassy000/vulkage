#include "radiance_cascade/vkz_rc_debug.h"

#include "demo_structs.h"
#include "mesh.h"
#include "kage_math.h"
#include "gltf_loader.h"


using Binding = kage::Binding;
using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;

struct alignas(16) VoxDebugConsts
{
    float voxRadius; // the radius of circumcircle of the voxel
    float voxSideLen;
    float sceneRadius;
    uint32_t sceneSideCount;

    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
};

struct VoxDebugCmdInit
{
    kage::BufferHandle voxWorldPos;
    kage::BufferHandle voxAlbedo;
    kage::ImageHandle pyramid;
    kage::BufferHandle trans;
    kage::BufferHandle threadCountBuf;
};

struct alignas(16) VoxDrawBuf
{
    vec3 pos;
    vec3 col;
};

struct VoxDebugDrawInit
{
    kage::BufferHandle cmd;
    kage::BufferHandle draw;
    kage::BufferHandle trans;
    kage::ImageHandle color;
};

void prepareVoxDebugCmd(VoxDebugCmdGen& _debugCmd, const VoxDebugCmdInit& _init)
{
    kage::ShaderHandle cs = kage::registShader("rc_vox_debug_cmd", "shaders/rc_vox_debug_cmd.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("rc_vox_debug_cmd", { cs }, sizeof(VoxDebugConsts));

    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("rc_vox_debug_cmd", passDesc);

    kage::BufferHandle cmdBuf;
    {
        kage::BufferDesc cmdBufDesc{};
        cmdBufDesc.size = sizeof(MeshDrawCommand);
        cmdBufDesc.usage = kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;

        cmdBuf = kage::registBuffer("rc_vox_debug_cmd", cmdBufDesc);
    }

    kage::BufferDesc voxDrawBufDesc{};
    voxDrawBufDesc.size = 128 * 1024 * 1024; // 128M
    voxDrawBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle voxDrawBuf = kage::registBuffer("rc_vox_debug_draw", voxDrawBufDesc);

    kage::BufferHandle cmdBufAlias = kage::alias(cmdBuf);
    kage::BufferHandle voxDrawBufAlias = kage::alias(voxDrawBuf);


    kage::bindBuffer(pass, _init.trans
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, cmdBuf
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , cmdBufAlias
    );

    kage::bindBuffer(pass, voxDrawBuf
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , voxDrawBufAlias
    );

    kage::SamplerHandle samp = kage::sampleImage(pass, _init.pyramid
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::bindBuffer(pass, _init.voxWorldPos
        , 4
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.voxAlbedo
        , 5
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::setIndirectCountBuffer(pass, _init.threadCountBuf, 0);

    _debugCmd.pass = pass;
    _debugCmd.program = prog;
    _debugCmd.cs = cs;

    _debugCmd.voxWorldPos = _init.voxWorldPos;
    _debugCmd.voxAlbedo = _init.voxAlbedo;
    _debugCmd.trans = _init.trans;
    _debugCmd.pyramid = _init.pyramid;
    _debugCmd.threadCountBuf = _init.threadCountBuf;

    _debugCmd.cmdBuf = cmdBuf;
    _debugCmd.drawBuf = voxDrawBuf;
    _debugCmd.sampler = samp;

    _debugCmd.outCmdAlias = cmdBufAlias;
    _debugCmd.outDrawBufAlias = voxDrawBufAlias;
}


void recVoxDebugGen(const VoxDebugCmdGen& _vcmd, const DrawCull& _camCull, const float _sceneRadius)
{
    kage::startRec(_vcmd.pass);

    float voxSideLen = _sceneRadius * 2.f / (float)kage::kVoxelSideCount;
    VoxDebugConsts consts{};

    consts.voxRadius = sqrt(3.f) * voxSideLen;
    consts.voxSideLen = voxSideLen;
    consts.sceneRadius = _sceneRadius;
    consts.sceneSideCount = kage::kVoxelSideCount;

    consts.P00 = _camCull.P00;
    consts.P11 = _camCull.P11;
    consts.znear = _camCull.znear;
    consts.zfar = _camCull.zfar;
    consts.frustum[0] = _camCull.frustum[0];
    consts.frustum[1] = _camCull.frustum[1];
    consts.frustum[2] = _camCull.frustum[2];
    consts.frustum[3] = _camCull.frustum[3];
    consts.pyramidWidth = _camCull.pyramidWidth;
    consts.pyramidHeight = _camCull.pyramidHeight;

    const kage::Memory* mem = kage::alloc(sizeof(VoxDebugConsts));
    memcpy(mem->data, &consts, mem->size);
    kage::setConstants(mem);

    kage::fillBuffer(_vcmd.cmdBuf, 0);
    kage::fillBuffer(_vcmd.drawBuf, 0);

    kage::Binding binds[] =
    {
        { _vcmd.trans,          Access::read,       Stage::compute_shader },
        { _vcmd.cmdBuf,         Access::read_write, Stage::compute_shader },
        { _vcmd.drawBuf,        Access::read_write, Stage::compute_shader },
        { _vcmd.pyramid,        _vcmd.sampler,      Stage::compute_shader },
        { _vcmd.voxWorldPos,    Access::read,       Stage::compute_shader },
        { _vcmd.voxAlbedo,      Access::read,       Stage::compute_shader },
    };

    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatchIndirect(_vcmd.threadCountBuf, 0);

    kage::endRec();
}


void prepareVoxDebugDraw(VoxDebugDraw& _vd, const VoxDebugDrawInit _init)
{
    kage::ShaderHandle vs = kage::registShader("rc_vox_debug", "shaders/rc_vox_debug.vert.spv");
    kage::ShaderHandle fs = kage::registShader("rc_vox_debug", "shaders/rc_vox_debug.frag.spv");
    kage::ProgramHandle prog = kage::registProgram("rc_vox_debug", { vs, fs }, sizeof(VoxDebugConsts));
    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::graphics;
    passDesc.pipelineConfig = { true, false, kage::CompareOp::greater_or_equal , kage::CullModeFlagBits::back, kage::PolygonMode::fill };
    kage::PassHandle pass = kage::registPass("rc_vox_debug", passDesc);

    // load cube mesh
    Geometry geom = {};
    loadObj(geom, "./data/cube.obj", false, false);

    const kage::Memory* vtxMem = kage::alloc(uint32_t(geom.vertices.size() * sizeof(Vertex)));
    memcpy(vtxMem->data, geom.vertices.data(), geom.vertices.size() * sizeof(Vertex));

    const kage::Memory* idxMem = kage::alloc(uint32_t(geom.indices.size() * sizeof(uint32_t)));
    memcpy(idxMem->data, geom.indices.data(), geom.indices.size() * sizeof(uint32_t));

    kage::BufferDesc vtxDesc;
    vtxDesc.size = vtxMem->size;
    vtxDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle vtxBuf = kage::registBuffer("cube_vtx", vtxDesc, vtxMem, kage::ResourceLifetime::non_transition);

    kage::BufferDesc idxDesc;
    idxDesc.size = idxMem->size;
    idxDesc.usage = kage::BufferUsageFlagBits::index | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle idxBuf = kage::registBuffer("cube_idx", idxDesc, idxMem, kage::ResourceLifetime::non_transition);

    kage::bindBuffer(pass, _init.cmd
        , 0
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.trans
        , 1
        , kage::PipelineStageFlagBits::vertex_shader | kage::PipelineStageFlagBits::fragment_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.draw
        , 2
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindVertexBuffer(pass, vtxBuf);
    kage::bindIndexBuffer(pass, idxBuf, (uint32_t)geom.indices.size());

    kage::setIndirectBuffer(pass, _init.cmd, offsetof(MeshDrawCommand, indexCount), sizeof(MeshDrawCommand), 1u);

    kage::ImageHandle colorAlias = kage::alias(_init.color);
    kage::setAttachmentOutput(pass, _init.color, 0, colorAlias);

    _vd.pass = pass;
    _vd.program = prog;
    _vd.vs = vs;
    _vd.fs = fs;

    _vd.idxBuf = idxBuf;
    _vd.vtxBuf = vtxBuf;

    _vd.trans = _init.trans;
    _vd.drawCmdBuf = _init.cmd;
    _vd.drawBuf = _init.draw;
    _vd.renderTarget = _init.color;
    _vd.rtOutAlias = colorAlias;
}


void recVoxDebugDraw(const VoxDebugDraw& _vd, const DrawCull& _camCull, const uint32_t _width, const uint32_t _height, float _sceneRadius)
{
    kage::startRec(_vd.pass);

    kage::setViewport(0, 0, _width, _height);
    kage::setScissor(0, 0, _width, _height);

    VoxDebugConsts consts{};
    consts.sceneRadius = _sceneRadius;
    consts.sceneSideCount = kage::kVoxelSideCount;
    consts.voxSideLen = _sceneRadius * 2.f / (float)kage::kVoxelSideCount;
    consts.voxRadius = sqrt(3.f) * consts.voxSideLen;

    const kage::Memory* mem = kage::alloc(sizeof(VoxDebugConsts));
    memcpy(mem->data, &consts, mem->size);
    kage::setConstants(mem);

    kage::Binding binds[] = {
        { _vd.trans,        Access::read,   Stage::vertex_shader | Stage::fragment_shader },
        { _vd.vtxBuf,       Access::read,   Stage::vertex_shader},
        { _vd.drawBuf,      Access::read,   Stage::vertex_shader},
    };
    kage::pushBindings(binds, COUNTOF(binds));


    kage::Attachment colorAttchs[] = {
        {_vd.renderTarget, LoadOp::dont_care, StoreOp::store},
    };
    kage::setColorAttachments(colorAttchs, COUNTOF(colorAttchs));

    kage::setIndexBuffer(_vd.idxBuf, 0, kage::IndexType::uint32);

    kage::drawIndexed(
        _vd.drawCmdBuf
        , offsetof(MeshDrawCommand, indexCount)
        , 1
        , sizeof(MeshDrawCommand)
    );

    kage::endRec();
}


void prepareVoxDebug(VoxDebug& _vd, const RCDebugInit& _init)
{
    VoxDebugCmdInit cmdInit{};
    cmdInit.pyramid = _init.pyramid;
    cmdInit.trans = _init.trans;
    cmdInit.voxWorldPos = _init.voxWPos;
    cmdInit.voxAlbedo = _init.voxAlbedo;
    cmdInit.threadCountBuf = _init.threadCount;
    prepareVoxDebugCmd(_vd.cmdGen, cmdInit);

    VoxDebugDrawInit drawInit{};
    drawInit.cmd = _vd.cmdGen.outCmdAlias;
    drawInit.draw = _vd.cmdGen.outDrawBufAlias;
    drawInit.trans = _init.trans;
    drawInit.color = _init.color;

    prepareVoxDebugDraw(_vd.draw, drawInit);
}


void updateVoxDebug(const VoxDebug& _vd, const DrawCull& _camCull, const uint32_t _width, const uint32_t _height, const float _sceneRadius)
{
    recVoxDebugGen(_vd.cmdGen, _camCull, _sceneRadius);
    recVoxDebugDraw(_vd.draw, _camCull, _width, _height, _sceneRadius);
}

struct alignas(16) ProbeDebugCmdConsts
{
    float sceneRadius;
    float probeSideLen;
    float sphereRadius;

    uint32_t probeSideCount;
    uint32_t layerOffset;
    uint32_t idxCnt;

    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
};

struct alignas(16) ProbeDraw
{
    vec3 pos;
    uvec3 id;
};

struct alignas(16) ProbeDebugDrawConsts
{
    float sceneRadius;
    float sphereRadius;

    uint32_t probeSideCount;
};

struct ProbeDebugGenInit
{
    kage::ImageHandle pyramid;
    kage::BufferHandle trans;

    uint32_t idxCount;
};

struct ProbeDebugDrawInit
{
    kage::ImageHandle color;
    kage::ImageHandle cascade;

    kage::BufferHandle trans;
    kage::BufferHandle cmd;
    kage::BufferHandle drawData;

    const kage::Memory* idxMem;
    const kage::Memory* vtxMem;

    uint32_t idxCount;
};

void prepareProbeDbgCmdGen(ProbeDbgCmdGen& _gen, const ProbeDebugGenInit& _init)
{
    kage::ShaderHandle cs = kage::registShader("rc_probe_debug_cmd", "shaders/rc_probe_debug_cmd.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("rc_probe_debug_cmd", { cs }, sizeof(ProbeDebugCmdConsts));

    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("rc_probe_debug_cmd", passDesc);

    kage::BufferHandle cmdBuf;
    {
        kage::BufferDesc cmdBufDesc{};
        cmdBufDesc.size = sizeof(MeshDrawCommand);
        cmdBufDesc.usage = kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;

        cmdBuf = kage::registBuffer("rc_probe_debug_cmd", cmdBufDesc);
    }

    kage::BufferDesc probeDrawBufDesc{};
    probeDrawBufDesc.size = kage::k_rclv0_probeSideCount * kage::k_rclv0_probeSideCount* kage::k_rclv0_probeSideCount* sizeof(ProbeDraw);
    probeDrawBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle probeDrawBuf = kage::registBuffer("rc_probe_debug_draw", probeDrawBufDesc);

    kage::BufferHandle cmdBufAlias = kage::alias(cmdBuf);
    kage::BufferHandle probeDrawBufAlias = kage::alias(probeDrawBuf);


    kage::bindBuffer(pass, _init.trans
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, cmdBuf
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , cmdBufAlias
    );

    kage::bindBuffer(pass, probeDrawBuf
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , probeDrawBufAlias
    );

    kage::SamplerHandle samp = kage::sampleImage(pass, _init.pyramid
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    _gen.pass = pass;
    _gen.program = prog;
    _gen.cs = cs;

    _gen.trans = _init.trans;
    _gen.cmdBuf = cmdBuf;
    _gen.drawDataBuf = probeDrawBuf;

    _gen.pyramid = _init.pyramid;
    _gen.sampler = samp;

    _gen.idxCnt = _init.idxCount;

    _gen.outCmdAlias = cmdBufAlias;
    _gen.outDrawDataBufAlias = probeDrawBufAlias;
}

void recProbeDbgCmdGen(const ProbeDbgCmdGen& _gen, const DrawCull& _camCull, const float _sceneRadius, uint32_t _lv)
{
    uint32_t probeSideCount = kage::k_rclv0_probeSideCount / (1 << glm::min(_lv, kage::k_rclv0_cascadeLv));
    const float probeSideLen = _sceneRadius * 2.f / (float)probeSideCount;
    const float sphereRadius = probeSideLen * 0.2f;

    kage::startRec(_gen.pass);
    ProbeDebugCmdConsts consts{};
    consts.sceneRadius = _sceneRadius;
    consts.probeSideLen = probeSideLen;
    consts.sphereRadius = sphereRadius;
    consts.probeSideCount = probeSideCount;
    consts.layerOffset = (kage::k_rclv0_probeSideCount - probeSideCount) * 2;
    consts.idxCnt = _gen.idxCnt;

    consts.P00 = _camCull.P00;
    consts.P11 = _camCull.P11;
    consts.znear = _camCull.znear;
    consts.zfar = _camCull.zfar;
    consts.frustum[0] = _camCull.frustum[0];
    consts.frustum[1] = _camCull.frustum[1];
    consts.frustum[2] = _camCull.frustum[2];
    consts.frustum[3] = _camCull.frustum[3];
    consts.pyramidWidth = _camCull.pyramidWidth;
    consts.pyramidHeight = _camCull.pyramidHeight;

    const kage::Memory* mem = kage::alloc(sizeof(ProbeDebugCmdConsts));
    memcpy(mem->data, &consts, mem->size);

    kage::setConstants(mem);

    kage::fillBuffer(_gen.cmdBuf, 0);
    kage::fillBuffer(_gen.drawDataBuf, 0);

    kage::Binding binds[] =
    {
        { _gen.trans,       Access::read,       Stage::compute_shader },
        { _gen.cmdBuf,      Access::read_write, Stage::compute_shader },
        { _gen.drawDataBuf, Access::read_write, Stage::compute_shader },
        { _gen.pyramid,     _gen.sampler,       Stage::compute_shader },
    };

    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatch(probeSideCount, probeSideCount, probeSideCount);

    kage::endRec();
}

void prepareProbeDbgDraw(ProbeDbgDraw& _pd, const ProbeDebugDrawInit& _init)
{
    kage::ShaderHandle vs = kage::registShader("rc_probe_debug", "shaders/rc_probe_debug.vert.spv");
    kage::ShaderHandle fs = kage::registShader("rc_probe_debug", "shaders/rc_probe_debug.frag.spv");
    kage::ProgramHandle prog = kage::registProgram("rc_probe_debug", { vs, fs }, sizeof(ProbeDebugDrawConsts));
    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::graphics;
    passDesc.pipelineConfig = { true, false, kage::CompareOp::greater_or_equal , kage::CullModeFlagBits::back, kage::PolygonMode::fill };
    kage::PassHandle pass = kage::registPass("rc_probe_debug", passDesc);

    kage::BufferDesc vtxDesc;
    vtxDesc.size = _init.vtxMem->size;
    vtxDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle vtxBuf = kage::registBuffer("sphere_vtx", vtxDesc, _init.vtxMem, kage::ResourceLifetime::non_transition);

    kage::BufferDesc idxDesc;
    idxDesc.size = _init.idxMem->size;
    idxDesc.usage = kage::BufferUsageFlagBits::index | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle idxBuf = kage::registBuffer("sphere_idx", idxDesc, _init.idxMem, kage::ResourceLifetime::non_transition);

    kage::bindBuffer(pass, _init.trans
        , 0
        , kage::PipelineStageFlagBits::vertex_shader | kage::PipelineStageFlagBits::fragment_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, vtxBuf
        , 1
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.drawData
        , 2
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::SamplerHandle samp = kage::sampleImage(
        pass, _init.cascade
        , 3
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::bindVertexBuffer(pass, vtxBuf);
    kage::bindIndexBuffer(pass, idxBuf, (uint32_t)_init.idxCount);

    kage::setIndirectBuffer(pass, _init.cmd, offsetof(MeshDrawCommand, indexCount), sizeof(MeshDrawCommand), 1u);

    kage::ImageHandle colorAlias = kage::alias(_init.color);
    kage::setAttachmentOutput(pass, _init.color, 0, colorAlias);

    _pd.pass = pass;
    _pd.program = prog;
    _pd.vs = vs;
    _pd.fs = fs;

    _pd.idxBuf = idxBuf;
    _pd.vtxBuf = vtxBuf;

    _pd.trans = _init.trans;
    _pd.drawCmdBuf = _init.cmd;
    _pd.drawDataBuf = _init.drawData;
    _pd.renderTarget = _init.color;
    _pd.radianceCascade = _init.cascade;
    _pd.rcSamp = samp;

    _pd.rtOutAlias = colorAlias;
}

void recProbeDbgDraw(const ProbeDbgDraw& _pd, const DrawCull& _camCull, const uint32_t _width, const uint32_t _height, const float _sceneRadius, const uint32_t _lv, const float _szFactor = .5f)
{
    kage::startRec(_pd.pass);

    kage::setViewport(0, 0, _width, _height);
    kage::setScissor(0, 0, _width, _height);

    uint32_t probeSideCount = kage::k_rclv0_probeSideCount / (1 << glm::min(_lv, kage::k_rclv0_cascadeLv));
    float sphereRadius = _sceneRadius / probeSideCount;

    ProbeDebugDrawConsts consts{};
    consts.sceneRadius = _sceneRadius;
    consts.sphereRadius = _szFactor * sphereRadius;
    consts.probeSideCount = probeSideCount;

    const kage::Memory* mem = kage::alloc(sizeof(ProbeDebugDrawConsts));
    memcpy(mem->data, &consts, mem->size);
    kage::setConstants(mem);

    kage::Binding binds[] = {
        { _pd.trans,            Access::read,   Stage::vertex_shader},
        { _pd.vtxBuf,           Access::read,   Stage::vertex_shader},
        { _pd.drawDataBuf,      Access::read,   Stage::vertex_shader},
        { _pd.radianceCascade,  _pd.rcSamp,  Stage::fragment_shader},
    };
    kage::pushBindings(binds, COUNTOF(binds));


    kage::Attachment colorAttchs[] = {
        {_pd.renderTarget, LoadOp::dont_care, StoreOp::store},
    };
    kage::setColorAttachments(colorAttchs, COUNTOF(colorAttchs));

    kage::setIndexBuffer(_pd.idxBuf, 0, kage::IndexType::uint32);

    kage::drawIndexed(
        _pd.drawCmdBuf
        , offsetof(MeshDrawCommand, indexCount)
        , 1
        , sizeof(MeshDrawCommand)
    );

    kage::endRec();
}

void prepareProbeDebug(ProbeDebug& _pd, const RCDebugInit& _init)
{
    // load cube mesh
    Geometry geom = {};
    bool result = loadObj(geom, "./data/sphere.obj", false, false);
    assert(result);


    const kage::Memory* vtxMem = kage::alloc(uint32_t(geom.vertices.size() * sizeof(Vertex)));
    memcpy(vtxMem->data, geom.vertices.data(), geom.vertices.size() * sizeof(Vertex));

    const kage::Memory* idxMem = kage::alloc(uint32_t(geom.indices.size() * sizeof(uint32_t)));
    memcpy(idxMem->data, geom.indices.data(), geom.indices.size() * sizeof(uint32_t));


    ProbeDebugGenInit cmdInit{};
    cmdInit.pyramid = _init.pyramid;
    cmdInit.trans = _init.trans;
    cmdInit.idxCount = (uint32_t)geom.indices.size();

    prepareProbeDbgCmdGen(_pd.cmdGen, cmdInit);

    ProbeDebugDrawInit drawInit{};
    drawInit.color = _init.color;
    drawInit.cascade = _init.cascade;
    drawInit.trans = _init.trans;
    drawInit.cmd = _pd.cmdGen.outCmdAlias;
    drawInit.drawData = _pd.cmdGen.outDrawDataBufAlias;
    drawInit.idxMem = idxMem;
    drawInit.vtxMem = vtxMem;

    prepareProbeDbgDraw(_pd.draw, drawInit);
}

void updateProbeDebug(const ProbeDebug& _pd, const DrawCull& _camCull, const uint32_t _width, const uint32_t _height, const float _sceneRadius, const uint32_t _lv /* = 0u*/)
{
    recProbeDbgCmdGen(_pd.cmdGen, _camCull, _sceneRadius, _pd.debugLv);
    recProbeDbgDraw(_pd.draw, _camCull, _width, _height, _sceneRadius, _lv);
}
