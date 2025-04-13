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

struct alignas(16) ProbeDebugCmdConsts
{
    float rcRadius;
    float probeSideLen;
    float sphereRadius;

    uint32_t probeSideCount;
    uint32_t layerOffset;
    uint32_t idxCnt;

    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;

    float posOffsets[3];
};

struct alignas(16) ProbeDraw
{
    vec3 pos;
    uvec3 id;
};

struct alignas(16) ProbeDebugDrawConsts
{
    float probeDebugRadius;
    float sphereRadius;

    uint32_t probeSideCount;
    uint32_t raySideCount;

    uint32_t debugIdxType;
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
    kage::ImageHandle depth;
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
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, cmdBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , cmdBufAlias
    );

    kage::bindBuffer(pass, probeDrawBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , probeDrawBufAlias
    );

    kage::SamplerHandle samp = kage::sampleImage(pass, _init.pyramid
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

void recProbeDbgCmdGen(const ProbeDbgCmdGen& _gen, const DrawCull& _camCull, const Dbg_RCBuild _rcDbg)
{
    uint32_t probeSideCount = kage::k_rclv0_probeSideCount / (1 << glm::min(_rcDbg.startCascade, kage::k_rclv0_cascadeLv));
    const float probeSideLen = _rcDbg.totalRadius * 2.f / (float)probeSideCount;
    const float sphereRadius = probeSideLen * 0.2f * _rcDbg.probeDebugScale;

    kage::startRec(_gen.pass);
    ProbeDebugCmdConsts consts{};
    consts.rcRadius = _rcDbg.totalRadius;
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

    consts.posOffsets[0] = _rcDbg.probePosOffset[0];
    consts.posOffsets[1] = _rcDbg.probePosOffset[1];
    consts.posOffsets[2] = _rcDbg.probePosOffset[2];

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
    passDesc.pipelineConfig = { true, true, kage::CompareOp::greater, kage::CullModeFlagBits::back, kage::PolygonMode::fill };
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
        , kage::PipelineStageFlagBits::vertex_shader | kage::PipelineStageFlagBits::fragment_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, vtxBuf
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.drawData
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::SamplerHandle samp = kage::sampleImage(
        pass, _init.cascade
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::mirrored_repeat
        , kage::SamplerReductionMode::weighted_average
    );

    kage::bindVertexBuffer(pass, vtxBuf);
    kage::bindIndexBuffer(pass, idxBuf, (uint32_t)_init.idxCount);

    kage::setIndirectBuffer(pass, _init.cmd, offsetof(MeshDrawCommand, indexCount), sizeof(MeshDrawCommand), 1u);

    kage::ImageHandle colorAlias = kage::alias(_init.color);
    kage::setAttachmentOutput(pass, _init.color, colorAlias);

    kage::ImageHandle depthAlias = kage::alias(_init.depth);
    kage::setAttachmentOutput(pass, _init.depth, depthAlias);

    _pd.pass = pass;
    _pd.program = prog;
    _pd.vs = vs;
    _pd.fs = fs;

    _pd.idxBuf = idxBuf;
    _pd.vtxBuf = vtxBuf;

    _pd.trans = _init.trans;
    _pd.drawCmdBuf = _init.cmd;
    _pd.drawDataBuf = _init.drawData;
    _pd.radianceCascade = _init.cascade;
    _pd.rcSamp = samp;

    _pd.color = _init.color;
    _pd.depth = _init.depth;

    _pd.colorOutAlias = colorAlias;
    _pd.depthOutAlias = depthAlias;
}

void recProbeDbgDraw(const ProbeDbgDraw& _pd, const DrawCull& _camCull, const uint32_t _width, const uint32_t _height, const Dbg_RCBuild _rcDbg)
{
    kage::startRec(_pd.pass);

    kage::setViewport(0, 0, _width, _height);
    kage::setScissor(0, 0, _width, _height);

    uint32_t shift = (1 << glm::min(_rcDbg.startCascade, kage::k_rclv0_cascadeLv));
    uint32_t probeSideCount = kage::k_rclv0_probeSideCount / shift;
    uint32_t raySideCount = kage::k_rclv0_rayGridSideCount * shift;
    float sphereRadius = (_rcDbg.totalRadius / probeSideCount) * _rcDbg.probeDebugScale;

    ProbeDebugDrawConsts consts{};
    consts.probeDebugRadius = _rcDbg.totalRadius;
    consts.sphereRadius = sphereRadius;
    consts.probeSideCount = probeSideCount;
    consts.raySideCount = raySideCount;
    consts.debugIdxType = _rcDbg.idx_type;

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
        {_pd.color, LoadOp::dont_care, StoreOp::store},
    };
    kage::setColorAttachments(colorAttchs, COUNTOF(colorAttchs));

    kage::setDepthAttachment({ _pd.depth, LoadOp::dont_care, StoreOp::store });

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
    bool result = loadGltfMesh(geom, "./data/gltf/sphere.glb", false, false);
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
    drawInit.depth = _init.depth;
    drawInit.cascade = _init.cascade;
    drawInit.trans = _init.trans;
    drawInit.cmd = _pd.cmdGen.outCmdAlias;
    drawInit.drawData = _pd.cmdGen.outDrawDataBufAlias;
    drawInit.idxMem = idxMem;
    drawInit.vtxMem = vtxMem;

    prepareProbeDbgDraw(_pd.draw, drawInit);
}

void updateProbeDebug(const ProbeDebug& _pd, const DrawCull& _camCull, const uint32_t _width, const uint32_t _height, const Dbg_RCBuild _rcDbg)
{
    recProbeDbgCmdGen(_pd.cmdGen, _camCull, _rcDbg);
    recProbeDbgDraw(_pd.draw, _camCull, _width, _height, _rcDbg);
}
