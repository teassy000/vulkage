#include "vkz_radiance_cascade.h"
#include "demo_structs.h"
#include "mesh.h"


using Binding = kage::Binding;
using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;

constexpr uint32_t c_voxelLength = 512;

void prepareFullDrawCmdPass(VoxelizationCmd& _vc, kage::BufferHandle _meshBuf, kage::BufferHandle _meshDrawBuf)
{
    kage::ShaderHandle cs = kage::registShader("rc_draw_cmd", "shaders/rc_drawcmd.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("mesh_draw_cmd", { cs });

    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;

    const char* passName = "rc_draw_cmd";
    kage::PassHandle pass = kage::registPass(passName, passDesc);

    kage::BufferDesc cmdBufDesc{};
    cmdBufDesc.size = 128 * 1024 * 1024; // 128M
    cmdBufDesc.usage = kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    cmdBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle cmdBuf = kage::registBuffer("rc_meshDrawCmd", cmdBufDesc);

    kage::BufferDesc cmdCountBufDesc{};
    cmdCountBufDesc.size = 16;
    cmdCountBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
    cmdCountBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle cmdCntBuf = kage::registBuffer("meshDrawCmdCount", cmdCountBufDesc);

    kage::BufferHandle cmdBufOutAlias = kage::alias(cmdBuf);
    kage::BufferHandle cmdCountBufOutAlias = kage::alias(cmdCntBuf);

    kage::bindBuffer(pass, _meshBuf
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _meshDrawBuf
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, cmdBuf
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , cmdBufOutAlias
    );

    kage::bindBuffer(pass, cmdCntBuf
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , cmdCountBufOutAlias
    );

    _vc.pass = pass;
    _vc.cs = cs;
    _vc.program = prog;
    _vc.in_meshBuf = _meshBuf;
    _vc.in_drawBuf = _meshDrawBuf;
    _vc.out_cmdBuf = cmdBuf;
    _vc.out_cmdCountBuf = cmdCntBuf;
    _vc.out_cmdBufAlias = cmdBufOutAlias;
    _vc.out_cmdCountBufAlias = cmdCountBufOutAlias;
}

void recFullDrawCmdPass(const VoxelizationCmd& _vc, uint32_t _dc)
{
    kage::startRec(_vc.pass);

    kage::fillBuffer(_vc.out_cmdCountBuf, 0);
    kage::fillBuffer(_vc.out_cmdBuf, 0);

    kage::Binding binds[] =
    {
        {_vc.in_meshBuf,        Access::read,       Stage::compute_shader },
        { _vc.in_drawBuf,       Access::read,       Stage::compute_shader },
        { _vc.out_cmdBuf,       Access::read_write,      Stage::compute_shader },
        { _vc.out_cmdCountBuf,  Access::read_write, Stage::compute_shader },
    };

    kage::pushBindings(binds, COUNTOF(binds));
    kage::dispatch(_dc, 1, 1);
    kage::endRec();
}

struct VoxInitData
{
    kage::BufferHandle cmdBuf;
    kage::BufferHandle cmdCountBuf;
    kage::BufferHandle drawBuf;
    kage::BufferHandle idxBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle transBuf;

    float sceneRadius;
    uint32_t maxDrawCmdCount;
};

struct VoxelizationConfig
{
    mat4 proj;
    uint32_t edgeLen;
};


void prepareVoxelization(Voxelization& _vox, const VoxInitData& _init)
{
    // voxelization
    // in order to use on old hardware, use a geometry shader to generate the voxelization
    // should possible to use a mesh shader to do the same thing 
    kage::ShaderHandle vs = kage::registShader("vox", "shaders/rc_voxelize.vert.spv");
    kage::ShaderHandle gs = kage::registShader("vox", "shaders/rc_voxelize.geom.spv");
    kage::ShaderHandle fs = kage::registShader("vox", "shaders/rc_voxelize.frag.spv");
    kage::ProgramHandle program = kage::registProgram("vox", { vs, gs, fs }, sizeof(VoxelizationConfig));

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::graphics;
    passDesc.pipelineConfig = { false, false, kage::CompareOp::never };

    kage::PassHandle pass = kage::registPass("voxelize", passDesc);

    kage::BufferHandle worldPos;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = c_voxelLength * c_voxelLength * c_voxelLength * 4;
        bufDesc.format = kage::ResourceFormat::r16g16b16a16_uint;
        bufDesc.usage = kage::BufferUsageFlagBits::storage_texel | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        
        worldPos = kage::registBuffer("rc_vox_wpos", bufDesc);
    }

    kage::BufferHandle albedo;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = c_voxelLength * c_voxelLength * c_voxelLength * 2;
        bufDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
        bufDesc.usage = kage::BufferUsageFlagBits::storage_texel | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;

        albedo = kage::registBuffer("rc_vox_albedo", bufDesc);
    }

    kage::BufferHandle normal;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = c_voxelLength * c_voxelLength * c_voxelLength * 4;
        bufDesc.format = kage::ResourceFormat::r16g16b16a16_sfloat;
        bufDesc.usage = kage::BufferUsageFlagBits::storage_texel | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;

        normal = kage::registBuffer("rc_vox_normal", bufDesc);
    }

    kage::BufferHandle voxMapBuf;
    {
        // each voxel contains a index
        kage::BufferDesc bufDesc{};
        bufDesc.size = c_voxelLength * c_voxelLength * c_voxelLength * sizeof(uint32_t);
        bufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        voxMapBuf = kage::registBuffer("rc_vox_map", bufDesc);
    }

    // dummy render target, should share the same size with the voxelization
    kage::ImageHandle dummy_rt;
    {
        kage::ImageDesc imgDesc{};
        imgDesc.width = c_voxelLength;
        imgDesc.height = c_voxelLength;
        imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
        imgDesc.depth = 1;
        imgDesc.numLayers = 1;
        imgDesc.numMips = 1;
        imgDesc.type = kage::ImageType::type_2d;
        imgDesc.viewType = kage::ImageViewType::type_2d;
        imgDesc.usage = kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled;
        imgDesc.layout = kage::ImageLayout::general;

        dummy_rt = kage::registRenderTarget("rc_vox_rt", imgDesc, kage::ResourceLifetime::transition);
    }

    // sparse oct tree image buffers
    kage::BufferDesc fragCountBufDesc{};
    fragCountBufDesc.size = 16;
    fragCountBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    fragCountBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle fragCountBuf = kage::registBuffer("rc_vox_frag_count", fragCountBufDesc);

    kage::bindIndexBuffer(pass, _init.idxBuf);

    kage::bindBuffer(pass, _init.cmdBuf
        , 0
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.drawBuf
        , 1
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_read
    );
    
    kage::bindBuffer(pass, _init.vtxBuf
        , 2
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::BufferHandle fragCountBufAlias = kage::alias(fragCountBuf);
    kage::bindBuffer(pass, fragCountBuf
        , 3
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , fragCountBufAlias
    );

    kage::BufferHandle wposAlias = kage::alias(worldPos);

    kage::bindBuffer(pass, worldPos
        , 4
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_write
        , wposAlias
    );

    kage::BufferHandle albedoAlias = kage::alias(albedo);
    kage::bindBuffer(pass, albedo
        , 5
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_write
        , albedoAlias
    );

    kage::BufferHandle normalAlias = kage::alias(normal);
    kage::bindBuffer(pass, normal
        , 6
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_write
        , normalAlias
    );

    kage::BufferHandle voxMapAlias = kage::alias(voxMapBuf);
    kage::bindBuffer(pass, voxMapBuf
        , 7
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , voxMapAlias
    );

    kage::setIndirectBuffer(pass, _init.cmdBuf, offsetof(MeshDrawCommand, indexCount), sizeof(MeshDrawCommand), _init.maxDrawCmdCount);
    kage::setIndirectCountBuffer(pass, _init.cmdCountBuf, 0);

    kage::ImageHandle rtAlias = kage::alias(dummy_rt);

    kage::setAttachmentOutput(pass, dummy_rt, 0, rtAlias);

    _vox.pass = pass;
    _vox.program = program;
    _vox.vs = vs;
    _vox.gs = gs;
    _vox.fs = fs;

    _vox.cmdBuf = _init.cmdBuf;
    _vox.cmdCountBuf = _init.cmdCountBuf;
    _vox.drawBuf = _init.drawBuf;
    _vox.idxBuf = _init.idxBuf;
    _vox.vtxBuf = _init.vtxBuf;

    _vox.voxMap = voxMapBuf;
    _vox.voxelWorldPos = worldPos;
    _vox.voxelAlbedo = albedo;
    _vox.voxelNormal = normal;

    _vox.voxMapOutAlias = voxMapAlias;
    _vox.wposOutAlias = wposAlias;
    _vox.albedoOutAlias = albedoAlias;
    _vox.normalOutAlias = normalAlias;

    _vox.fragCountBuf = fragCountBuf;
    _vox.fragCountBufOutAlias = fragCountBufAlias;

    _vox.rt = dummy_rt;

    _vox.maxDrawCmdCount = _init.maxDrawCmdCount;
}

void recVoxelization(const Voxelization& _vox)
{
    kage::startRec(_vox.pass);

    kage::setViewport(0, 0, c_voxelLength, c_voxelLength);
    kage::setScissor(0, 0, c_voxelLength, c_voxelLength);

    kage::fillBuffer(_vox.fragCountBuf, 0);
    kage::fillBuffer(_vox.voxMap, UINT32_MAX);

    VoxelizationConfig vc{};
    vc.proj = freeCameraGetOrthoProjMatrix(_vox.sceneRadius, _vox.sceneRadius, _vox.sceneRadius);
    vc.edgeLen = c_voxelLength;
    const kage::Memory* mem = kage::alloc(sizeof(VoxelizationConfig));
    memcpy(mem->data, &vc, mem->size);
    kage::setConstants(mem);

    kage::Binding binds[] =
    {
        { _vox.cmdBuf,          Access::read,       Stage::vertex_shader | Stage::geometry_shader },
        { _vox.vtxBuf,          Access::read,       Stage::vertex_shader | Stage::geometry_shader },
        { _vox.drawBuf,         Access::read,       Stage::vertex_shader | Stage::geometry_shader },
        { _vox.fragCountBuf,    Access::read_write, Stage::fragment_shader },
        { _vox.voxelWorldPos,   Access::write,      Stage::fragment_shader },
        { _vox.voxelAlbedo,     Access::write,      Stage::fragment_shader },
        { _vox.voxelNormal,     Access::write,      Stage::fragment_shader },
        { _vox.voxMap,          Access::read_write, Stage::fragment_shader },
    };
    kage::pushBindings(binds, COUNTOF(binds));

    kage::Attachment attachs[] = {
        { _vox.rt,          LoadOp::dont_care,  StoreOp::dont_care  },
    };
    kage::setColorAttachments(attachs, COUNTOF(attachs));

    kage::setIndexBuffer(_vox.idxBuf, 0, kage::IndexType::uint32);
    kage::drawIndexed(
        _vox.cmdBuf
        , offsetof(MeshDrawCommand, indexCount)
        , _vox.cmdCountBuf
        , 0
        , _vox.maxDrawCmdCount
        , sizeof(MeshDrawCommand)
    );

    kage::endRec();
}

struct VoxDebugCmdInit
{
    kage::BufferHandle voxMap;
    kage::ImageHandle pyramid;
    kage::BufferHandle trans;
};

struct alignas(16) VoxDebugCmdConsts
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

void prepareVoxDebugCmd(VoxDebugCmdGen& _debugCmd, const VoxDebugCmdInit& _init)
{
    kage::ShaderHandle cs = kage::registShader("rc_vox_debug_cmd", "shaders/rc_vox_debug_cmd.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("rc_vox_debug_cmd", { cs }, sizeof(VoxDebugCmdConsts));

    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("rc_vox_debug_cmd", passDesc);

    kage::BufferDesc cmdBufDesc{};
    cmdBufDesc.size = 4 * 1024 * 1024; // 4M
    cmdBufDesc.usage = kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle cmdBuf = kage::registBuffer("rc_vox_debug_cmd", cmdBufDesc);

    kage::BufferDesc cmdCountBufDesc{};
    cmdCountBufDesc.size = 16;
    cmdCountBufDesc.usage = kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    kage::BufferHandle cmdCountBuf = kage::registBuffer("rc_vox_debug_cmd_count", cmdCountBufDesc);

    kage::BufferHandle cmdBufAlias = kage::alias(cmdBuf);
    kage::BufferHandle cmdCountBufAlias = kage::alias(cmdCountBuf);

    kage::bindBuffer(pass, _init.voxMap
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.trans
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, cmdBuf
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , cmdBufAlias
    );

    kage::bindBuffer(pass, cmdCountBuf
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , cmdCountBufAlias
    );

    kage::SamplerHandle samp = kage::sampleImage(pass, _init.pyramid
        , 4
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );


    _debugCmd.pass = pass;
    _debugCmd.program = prog;
    _debugCmd.cs = cs;

    _debugCmd.trans = _init.trans;
    _debugCmd.pyramid = _init.pyramid;
    _debugCmd.voxmap = _init.voxMap;
    _debugCmd.pyramid = _init.pyramid;

    _debugCmd.cmdBuf = cmdBuf;
    _debugCmd.cmdCountBuf = cmdCountBuf;
    _debugCmd.sampler = samp;

    _debugCmd.outCmdAlias = cmdBufAlias;
    _debugCmd.outCmdCountAlias = cmdCountBufAlias;
}

void recVoxDebugGen(const VoxDebugCmdGen& _vcmd, const DrawCull& _camCull)
{
    kage::startRec(_vcmd.pass);
    kage::fillBuffer(_vcmd.cmdCountBuf, 0);
    kage::fillBuffer(_vcmd.cmdBuf, 0);
    kage::Binding binds[] =
    {
        { _vcmd.voxmap,         Access::read,       Stage::compute_shader },
        { _vcmd.trans,          Access::read,       Stage::compute_shader},
        { _vcmd.cmdBuf,         Access::read_write, Stage::compute_shader },
        { _vcmd.cmdCountBuf,    Access::read_write, Stage::compute_shader },
        { _vcmd.pyramid,        _vcmd.sampler,        Stage::compute_shader },
    };

    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatch(c_voxelLength * c_voxelLength * c_voxelLength, 1, 1);
    kage::endRec();
}

struct VoxDebugDrawInit
{
    kage::BufferHandle cmd;
    kage::BufferHandle cmdCount;
    kage::BufferHandle trans;
    kage::ImageHandle color;
};

constexpr uint32_t c_maxVoxDrawCmdCnt = 50000;// not a special max value, just the bistro scene could generate this much voxel
void prepareVoxDebug(VoxDebug& _vd, const VoxDebugDrawInit _init)
{
    kage::ShaderHandle vs = kage::registShader("rc_vox_debug", "shaders/rc_vox_debug.vert.spv");
    kage::ShaderHandle fs = kage::registShader("rc_vox_debug", "shaders/rc_vox_debug.frag.spv");
    kage::ProgramHandle prog = kage::registProgram("rc_vox_debug", { vs, fs });
    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::graphics;
    passDesc.pipelineConfig = { true, false, kage::CompareOp::greater };
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

    kage::bindVertexBuffer(pass, vtxBuf);
    kage::bindIndexBuffer(pass, idxBuf, (uint32_t)geom.indices.size());

    kage::setIndirectBuffer(pass, _init.cmd, offsetof(MeshDrawCommand, indexCount), sizeof(MeshDrawCommand), (uint32_t)c_maxVoxDrawCmdCnt); 
    kage::setIndirectCountBuffer(pass, _init.cmdCount, 0);

    kage::ImageHandle colorAlias =  kage::alias(_init.color);
    kage::setAttachmentOutput(pass, _init.color, 0, colorAlias);

    _vd.pass = pass;
    _vd.program = prog;
    _vd.vs = vs;
    _vd.fs = fs;

    _vd.idxBuf = idxBuf;
    _vd.vtxBuf = vtxBuf;

    _vd.trans = _init.trans;
    _vd.drawCmdBuf = _init.cmd;
    _vd.drawCmdCountBuf = _init.cmdCount;
    _vd.renderTarget = _init.color;
    _vd.rtOutAlias = colorAlias;
}

void recVoxDebug(const VoxDebug& _vd, const DrawCull& _camCull, const uint32_t _width, const uint32_t _height)
{
    kage::startRec(_vd.pass);

    kage::setViewport(0, 0, _width, _height);
    kage::setScissor(0, 0, _width, _height);


    kage::Binding binds[] = { 
        { _vd.trans,    Access::read,   Stage::vertex_shader | Stage::fragment_shader },
        { _vd.vtxBuf,   Access::read,   Stage::vertex_shader},
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
        , _vd.drawCmdCountBuf
        , 0
        , c_maxVoxDrawCmdCnt
        , sizeof(MeshDrawCommand)
    );

    kage::endRec();
}

struct alignas(16) OctTreeNode
{
    uint32_t voxIdx;
    uint32_t isFinalLv;
    uint32_t childs[8]; // index of the child node in OctTree
};

struct alignas(16) OctTreeProcessConfig
{
    uint32_t lv;
    uint32_t voxLen;
    uint32_t readOffset;
    uint32_t currOffset;
};

void prepareOctTree(OctTree& _oc, const kage::BufferHandle _voxMap)
{
    kage::ShaderHandle cs = kage::registShader("rc_oct_tree", "shaders/rc_oct_tree.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("rc_oct_tree", { cs }, sizeof(OctTreeProcessConfig));

    kage::PassDesc passDesc;
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;

    const char* passName = "rc_oct_tree";
    kage::PassHandle pass = kage::registPass(passName, passDesc);

    kage::BufferHandle VoxMediumMap;
    {
        uint32_t v0 = c_voxelLength / 2;
        float size = static_cast<float>(v0 * v0 * v0 * sizeof(uint32_t)) * 1.2f;// 1.2 times the size of the voxel map which approximates the size of the oct tree
        uint32_t fmtSz = static_cast<uint32_t>(glm::ceil(size));

        kage::BufferDesc bufDesc{};
        bufDesc.size = (fmtSz + (0x40 - (fmtSz % 0x40)));
        bufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        VoxMediumMap = kage::registBuffer("rc_vox_medium", bufDesc);
    }

    kage::BufferHandle octTreeBuf;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = 128 * 1024 * 1024; // 128M
        bufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        octTreeBuf = kage::registBuffer("rc_oct_tree", bufDesc);
    }

    kage::BufferHandle nodeCountBuf;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = 16;
        bufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        nodeCountBuf = kage::registBuffer("rc_oct_tree_node_count", bufDesc);
    }

    kage::bindBuffer(pass, _voxMap
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::BufferHandle VoxMediemMapAls = kage::alias(VoxMediumMap);
    kage::bindBuffer(pass, VoxMediumMap
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , VoxMediemMapAls
    );

    kage::BufferHandle octTreeBufAlias = kage::alias(octTreeBuf);
    kage::bindBuffer(pass, octTreeBuf
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , octTreeBufAlias
    );

    kage::BufferHandle nodeCountBufAlias = kage::alias(nodeCountBuf);
    kage::bindBuffer(pass, nodeCountBuf
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , octTreeBufAlias
    );

    _oc.cs = cs;
    _oc.program = prog;
    _oc.pass = pass;

    _oc.inVoxMap = _voxMap;
    _oc.voxMediemMap = VoxMediumMap;
    _oc.outOctTree = octTreeBuf;
    _oc.nodeCount = nodeCountBuf;
    _oc.nodeCountOutAlias = nodeCountBufAlias;
    _oc.octTreeOutAlias = octTreeBufAlias;
}

void recOctTree(const OctTree& _oc)
{
    kage::startRec(_oc.pass);

    kage::fillBuffer(_oc.outOctTree, 0);
    kage::fillBuffer(_oc.nodeCount, 0);
    kage::fillBuffer(_oc.voxMediemMap, UINT32_MAX);

    uint32_t maxLv = calcMipLevelCount(c_voxelLength);
    uint32_t curr_vl = c_voxelLength;
    uint32_t writeOffset = 0;
    uint32_t readOffset = 0;
    for (uint32_t ii= 0; ii < maxLv; ++ii)
    {
        curr_vl = glm::max(previousPow2(curr_vl), 1u);

        OctTreeProcessConfig conf{};
        conf.lv = ii;
        conf.readOffset = readOffset;
        conf.currOffset = writeOffset;
        conf.voxLen = curr_vl;

        const kage::Memory* mem = kage::alloc(sizeof(OctTreeProcessConfig));
        memcpy(mem->data, &conf, mem->size);
        kage::setConstants(mem);

        kage::Binding binds[] =
        {
            { _oc.inVoxMap,         Access::read,       Stage::compute_shader },
            { _oc.voxMediemMap,     Access::read_write, Stage::compute_shader },
            { _oc.outOctTree,       Access::read_write, Stage::compute_shader },
            { _oc.nodeCount,        Access::read_write, Stage::compute_shader },
        };
        kage::pushBindings(binds, COUNTOF(binds));

        kage::dispatch(curr_vl, curr_vl, curr_vl);

        readOffset = writeOffset;
        writeOffset += curr_vl * curr_vl * curr_vl;
    }

    kage::endRec();
}

struct RCBuildInit
{
    kage::BufferHandle octTreeCount;
    kage::BufferHandle octTree;
    kage::BufferHandle voxAlbedo;
    kage::BufferHandle voxWPos;
    kage::BufferHandle voxNormal;

    GBuffer g_buffer;
    kage::ImageHandle depth;
    kage::BufferHandle trans;

    float sceneRadius;
};

void prepareRCbuild(RadianceCascadeBuild& _rc, const RCBuildInit& _init)
{
    _rc.lv0Config.probe_sideCount = 32;
    _rc.lv0Config.ray_gridSideCount = 16;
    _rc.lv0Config.level = 6;
    _rc.lv0Config.rayLength = 1.0f;

    // build the cascade image
    kage::ShaderHandle cs = kage::registShader("build_cascade", "shaders/rc_build_cascade.comp.spv");
    kage::ProgramHandle program = kage::registProgram("build_cascade", { cs }, sizeof(RadianceCascadesConfig));

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("build_cascade", passDesc);

    uint32_t texelSideCount = _rc.lv0Config.probe_sideCount * _rc.lv0Config.ray_gridSideCount;
    uint32_t imgLayers = _rc.lv0Config.probe_sideCount * 2 - 1;

    kage::ImageDesc imgDesc{};
    imgDesc.width = texelSideCount;
    imgDesc.height = texelSideCount;
    imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imgDesc.depth = 1;
    imgDesc.numLayers = imgLayers;
    imgDesc.numMips = 1;
    imgDesc.type = kage::ImageType::type_2d;
    imgDesc.viewType = kage::ImageViewType::type_2d_array;
    imgDesc.usage = kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled;
    imgDesc.layout = kage::ImageLayout::general;

    kage::ImageHandle img = kage::registTexture("cascade", imgDesc, nullptr, kage::ResourceLifetime::non_transition);
    kage::ImageHandle outAlias = kage::alias(img);

    kage::bindBuffer(pass, _init.trans
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::SamplerHandle albedoSamp = kage::sampleImage(pass, _init.g_buffer.albedo
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle normSamp = kage::sampleImage(pass, _init.g_buffer.normal
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle wpSamp = kage::sampleImage(pass, _init.g_buffer.worldPos
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle emmiSamp = kage::sampleImage(pass, _init.g_buffer.emissive
        , 4
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle depthSamp = kage::sampleImage(pass, _init.depth
        , 5
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::bindBuffer(pass, _init.octTreeCount
        , 6
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.octTree
        , 7
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.voxAlbedo
        , 8
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindImage(pass, img
        , 9
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    _rc.pass = pass;
    _rc.program = program;
    _rc.cs = cs;

    _rc.g_buffer = _init.g_buffer;
    _rc.g_bufferSamplers.albedo = albedoSamp;
    _rc.g_bufferSamplers.normal = normSamp;
    _rc.g_bufferSamplers.worldPos = wpSamp;
    _rc.g_bufferSamplers.emissive = emmiSamp;

    _rc.trans = _init.trans;

    _rc.inOctTreeNodeCount = _init.octTreeCount;
    _rc.inOctTree = _init.octTree;
    _rc.inDepth = _init.depth;
    _rc.depthSampler = depthSamp;

    _rc.voxAlbedo = _init.voxAlbedo;

    _rc.cascadeImg = img;
    _rc.outAlias = outAlias;
    _rc.sceneRadius = _init.sceneRadius;
}

void recRCBuild(const RadianceCascadeBuild& _rc)
{
    kage::startRec(_rc.pass);

    uint32_t layerOffset = 0;
    for (uint32_t ii = 0; ii < _rc.lv0Config.level; ++ii)
    {
        uint32_t    level_factor    = uint32_t(1 << ii);
        uint32_t    prob_sideCount  = _rc.lv0Config.probe_sideCount / level_factor;
        uint32_t    prob_count      = uint32_t(pow(prob_sideCount, 3));
        uint32_t    ray_sideCount   = _rc.lv0Config.ray_gridSideCount * level_factor;
        uint32_t    ray_count       = ray_sideCount * ray_sideCount; // each probe has a 2d grid of rays
        float prob_sideLen = _rc.sceneRadius * 2.f / float(prob_sideCount);
        float rayLen = length(vec3(prob_sideLen * .5f));

        RadianceCascadesConfig config;
        config.probe_sideCount = prob_sideCount;
        config.ray_gridSideCount = ray_sideCount;
        config.level = ii;
        config.layerOffset = layerOffset;
        config.rayLength = rayLen;
        config.probeSideLen = prob_sideLen;

        config.ot_voxSideCount = c_voxelLength;
        config.ot_sceneSideLen = _rc.sceneRadius * 2.f;

        layerOffset += prob_sideCount;

        const kage::Memory* mem = kage::alloc(sizeof(RadianceCascadesConfig));
        memcpy(mem->data, &config, mem->size);

        kage::setConstants(mem);

        kage::Binding binds[] =
        {
            {_rc.trans,                 Access::read,                   Stage::compute_shader},
            {_rc.g_buffer.albedo,       _rc.g_bufferSamplers.albedo,    Stage::compute_shader},
            {_rc.g_buffer.normal,       _rc.g_bufferSamplers.normal,    Stage::compute_shader},
            {_rc.g_buffer.worldPos,     _rc.g_bufferSamplers.worldPos,  Stage::compute_shader},
            {_rc.g_buffer.emissive,     _rc.g_bufferSamplers.emissive,  Stage::compute_shader},
            {_rc.inDepth,               _rc.depthSampler,               Stage::compute_shader},
            {_rc.inOctTreeNodeCount,    Access::read,                   Stage::compute_shader},
            {_rc.inOctTree,             Access::read,                   Stage::compute_shader},
            {_rc.voxAlbedo,             Access::read,                   Stage::compute_shader},
            {_rc.cascadeImg,            0,                              Stage::compute_shader},
        };
        kage::pushBindings(binds, COUNTOF(binds));
        kage::dispatch(prob_count * ray_count, 1, 1);
    }

    kage::endRec();
}

void prepareRadianceCascade(RadianceCascade& _rc, const RadianceCascadeInitData _init)
{
    prepareFullDrawCmdPass(_rc.voxCmd, _init.meshBuf, _init.meshDrawBuf);

    VoxInitData voxelizeInitData{};
    voxelizeInitData.cmdBuf = _rc.voxCmd.out_cmdBufAlias;
    voxelizeInitData.cmdCountBuf = _rc.voxCmd.out_cmdCountBufAlias;
    voxelizeInitData.drawBuf = _init.meshDrawBuf;
    voxelizeInitData.idxBuf = _init.idxBuf;
    voxelizeInitData.vtxBuf = _init.vtxBuf;
    voxelizeInitData.transBuf = _init.transBuf;
    voxelizeInitData.sceneRadius = _init.sceneRadius;
    voxelizeInitData.maxDrawCmdCount = _init.maxDrawCmdCount;
    prepareVoxelization(_rc.vox, voxelizeInitData);

    prepareOctTree(_rc.octTree, _rc.vox.voxMapOutAlias);

    // next step should use the voxelization data to build the cascade
    RCBuildInit rcInit{};
    rcInit.voxAlbedo = _rc.vox.albedoOutAlias;
    rcInit.voxWPos = _rc.vox.wposOutAlias;
    rcInit.voxNormal = _rc.vox.normalOutAlias;
    rcInit.octTreeCount = _rc.octTree.nodeCountOutAlias;
    rcInit.octTree = _rc.octTree.octTreeOutAlias;

    rcInit.g_buffer = _init.g_buffer;
    rcInit.depth = _init.depth;
    rcInit.trans = _init.transBuf;
    rcInit.sceneRadius = _init.sceneRadius;
    prepareRCbuild(_rc.rcBuild, rcInit);


    VoxDebugCmdInit cmdInit{};
    cmdInit.pyramid = _init.pyramid;
    cmdInit.trans = _init.transBuf;
    cmdInit.voxMap = _rc.vox.voxMapOutAlias;
    prepareVoxDebugCmd(_rc.voxDebugCmdGen, cmdInit);

    VoxDebugDrawInit drawInit{};
    drawInit.cmd = _rc.voxDebugCmdGen.outCmdAlias;
    drawInit.cmdCount = _rc.voxDebugCmdGen.outCmdCountAlias;
    drawInit.trans = _init.transBuf;
    drawInit.color = _init.color;

    prepareVoxDebug(_rc.voxDebug, drawInit);
}

void updateRadianceCascade(
    const RadianceCascade& _rc
    , uint32_t _drawCount
    , const DrawCull& _camCull
    , const uint32_t _width
    , const uint32_t _height
)
{
    recFullDrawCmdPass(_rc.voxCmd, _drawCount);
    recVoxelization(_rc.vox);
    recOctTree(_rc.octTree);
    recRCBuild(_rc.rcBuild);

    recVoxDebugGen(_rc.voxDebugCmdGen, _camCull);
    recVoxDebug(_rc.voxDebug, _camCull, _width, _height);
}
