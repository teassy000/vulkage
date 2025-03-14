#include "vkz_radiance_cascade.h"
#include "config.h"
#include "mesh.h"



using Binding = kage::Binding;
using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;

uint32_t kVoxelSideCount = kage::kVoxelSideCount;

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
    cmdBufDesc.size = 4 * 1024 * 1024; // 4M
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
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _meshDrawBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, cmdBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , cmdBufOutAlias
    );

    kage::bindBuffer(pass, cmdCntBuf
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
        { _vc.out_cmdBuf,       Access::read_write, Stage::compute_shader },
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

    kage::BindlessHandle bindless;

    uint32_t maxDrawCmdCount;
};

struct VoxelizationConsts
{
    mat4 proj;
    uint32_t voxGridCount;
    float voxCellLen;
    float sceneRadius;
};

void prepareVoxelization(Voxelization& _vox, const VoxInitData& _init)
{
    // voxelization
    // in order to use on old hardware, use a geometry shader to generate the voxelization
    // should possible to use a mesh shader to do the same thing 
    kage::ShaderHandle vs = kage::registShader("vox", "shaders/rc_voxelize.vert.spv");
    kage::ShaderHandle gs = kage::registShader("vox", "shaders/rc_voxelize.geom.spv");
    kage::ShaderHandle fs = kage::registShader("vox", "shaders/rc_voxelize.frag.spv");
    kage::ProgramHandle program = kage::registProgram("vox", { vs, gs, fs }, sizeof(VoxelizationConsts), _init.bindless);

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::graphics;
    passDesc.pipelineConfig = { false, false, kage::CompareOp::never };

    kage::PassHandle pass = kage::registPass("voxelize", passDesc);

    kage::BufferHandle worldPos;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = kVoxelSideCount * kVoxelSideCount * kVoxelSideCount * 4;
        bufDesc.format = kage::ResourceFormat::r32_uint;
        bufDesc.usage = kage::BufferUsageFlagBits::storage_texel | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        
        worldPos = kage::registBuffer("rc_vox_wpos", bufDesc);
    }

    kage::BufferHandle albedo;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = kVoxelSideCount * kVoxelSideCount * kVoxelSideCount * 2;
        bufDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
        bufDesc.usage = kage::BufferUsageFlagBits::storage_texel | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;

        albedo = kage::registBuffer("rc_vox_albedo", bufDesc);
    }

    kage::BufferHandle normal;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = kVoxelSideCount * kVoxelSideCount * kVoxelSideCount * 4;
        bufDesc.format = kage::ResourceFormat::r16g16b16a16_sfloat;
        bufDesc.usage = kage::BufferUsageFlagBits::storage_texel | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;

        normal = kage::registBuffer("rc_vox_normal", bufDesc);
    }

    kage::BufferHandle voxMapBuf;
    {
        // each voxel contains a index
        kage::BufferDesc bufDesc{};
        bufDesc.size = kVoxelSideCount * kVoxelSideCount * kVoxelSideCount * sizeof(uint32_t);
        bufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        voxMapBuf = kage::registBuffer("rc_vox_map", bufDesc);
    }

    // dummy render target, should share the same size with the voxelization
    kage::ImageHandle dummy_rt;
    {
        kage::ImageDesc imgDesc{};
        imgDesc.width = kVoxelSideCount;
        imgDesc.height = kVoxelSideCount;
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
    kage::BufferDesc threadCountBufDesc{};
    threadCountBufDesc.size = 16;
    threadCountBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::indirect;
    threadCountBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle threadCount = kage::registBuffer("rc_vox_frag_count", threadCountBufDesc);

    kage::bindIndexBuffer(pass, _init.idxBuf);

    kage::bindBuffer(pass, _init.cmdBuf
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.drawBuf
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_read
    );
    
    kage::bindBuffer(pass, _init.vtxBuf
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::BufferHandle threadCountBufAlias = kage::alias(threadCount);
    kage::bindBuffer(pass, threadCount
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , threadCountBufAlias
    );

    kage::BufferHandle wposAlias = kage::alias(worldPos);
    kage::bindBuffer(pass, worldPos
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_write
        , wposAlias
    );

    kage::BufferHandle albedoAlias = kage::alias(albedo);
    kage::bindBuffer(pass, albedo
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_write
        , albedoAlias
    );

    kage::BufferHandle normalAlias = kage::alias(normal);
    kage::bindBuffer(pass, normal
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_write
        , normalAlias
    );

    kage::BufferHandle voxMapAlias = kage::alias(voxMapBuf);
    kage::bindBuffer(pass, voxMapBuf
        , kage::PipelineStageFlagBits::geometry_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , voxMapAlias
    );

    kage::setIndirectBuffer(pass, _init.cmdBuf, offsetof(MeshDrawCommand, indexCount), sizeof(MeshDrawCommand), _init.maxDrawCmdCount);
    kage::setIndirectCountBuffer(pass, _init.cmdCountBuf, 0);

    kage::ImageHandle rtAlias = kage::alias(dummy_rt);

    kage::setAttachmentOutput(pass, dummy_rt, rtAlias);

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

    _vox.bindless = _init.bindless;

    _vox.voxMapOutAlias = voxMapAlias;
    _vox.wposOutAlias = wposAlias;
    _vox.albedoOutAlias = albedoAlias;
    _vox.normalOutAlias = normalAlias;

    _vox.threadCountBuf = threadCount;
    _vox.threadCountBufOutAlias = threadCountBufAlias;

    _vox.rt = dummy_rt;

    _vox.maxDrawCmdCount = _init.maxDrawCmdCount;
}

void recVoxelization(const Voxelization& _vox, const float _sceneRadius)
{
    kage::startRec(_vox.pass);

    kage::setViewport(0, 0, kVoxelSideCount, kVoxelSideCount);
    kage::setScissor(0, 0, kVoxelSideCount, kVoxelSideCount);

    kage::fillBuffer(_vox.threadCountBuf, 1);
    kage::fillBuffer(_vox.voxMap, UINT32_MAX);

    VoxelizationConsts vc{};
    vc.proj = freeCameraGetOrthoProjMatrix(_sceneRadius, _sceneRadius, _sceneRadius);
    vc.voxGridCount = kVoxelSideCount;
    vc.voxCellLen = _sceneRadius * 2.f / (float)kVoxelSideCount;
    vc.sceneRadius = _sceneRadius;
    const kage::Memory* mem = kage::alloc(sizeof(VoxelizationConsts));
    memcpy(mem->data, &vc, mem->size);
    kage::setConstants(mem);

    kage::setBindless(_vox.bindless);

    kage::Binding binds[] =
    {
        { _vox.cmdBuf,          Access::read,       Stage::vertex_shader | Stage::geometry_shader },
        { _vox.vtxBuf,          Access::read,       Stage::vertex_shader | Stage::geometry_shader },
        { _vox.drawBuf,         Access::read,       Stage::vertex_shader | Stage::geometry_shader },
        { _vox.threadCountBuf,  Access::read_write, Stage::fragment_shader },
        { _vox.voxelWorldPos,   Access::write,      Stage::fragment_shader },
        { _vox.voxelAlbedo,     Access::write,      Stage::fragment_shader },
        { _vox.voxelNormal,     Access::write,      Stage::fragment_shader },
        { _vox.voxMap,          Access::read_write, Stage::fragment_shader },
    };
    kage::pushBindings(binds, COUNTOF(binds));

    kage::Attachment attachs[] = {
        { _vox.rt, LoadOp::dont_care,  StoreOp::dont_care  },
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



struct alignas(16) OctTreeNode
{
    uint32_t voxIdx;
    uint32_t isFinalLv;
    uint32_t childs[8]; // index of the child node in OctTree
};

struct alignas(16) OctTreeProcessConfig
{
    uint32_t lv;
    uint32_t voxGridSideCount;
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
        uint32_t v0 = kVoxelSideCount / 2;
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
        bufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::uniform;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        nodeCountBuf = kage::registBuffer("rc_oct_tree_node_count", bufDesc);
    }

    kage::BufferHandle voxVisBuf;
    {
        uint32_t v0 = kVoxelSideCount / 2;
        float size = static_cast<float>(v0 * v0 * v0 * sizeof(uint32_t)) * 1.2f;// 1.2 times the size of the voxel map which approximates the size of the oct tree
        uint32_t fmtSz = static_cast<uint32_t>(glm::ceil(size));

        kage::BufferDesc bufDesc{};
        bufDesc.size = (fmtSz + (0x40 - (fmtSz % 0x40)));
        bufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
        bufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
        voxVisBuf = kage::registBuffer("rc_vox_vis", bufDesc);
    }

    kage::bindBuffer(pass, _voxMap
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::BufferHandle VoxMediemMapAls = kage::alias(VoxMediumMap);
    kage::bindBuffer(pass, VoxMediumMap
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , VoxMediemMapAls
    );

    kage::BufferHandle octTreeBufAlias = kage::alias(octTreeBuf);
    kage::bindBuffer(pass, octTreeBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , octTreeBufAlias
    );

    kage::BufferHandle nodeCountBufAlias = kage::alias(nodeCountBuf);
    kage::bindBuffer(pass, nodeCountBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , nodeCountBufAlias
    );

    kage::BufferHandle voxVisBufAlias = kage::alias(voxVisBuf);
    kage::bindBuffer(pass, voxVisBuf
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read | kage::AccessFlagBits::shader_write
        , voxVisBufAlias
    );

    _oc.cs = cs;
    _oc.program = prog;
    _oc.pass = pass;

    _oc.inVoxMap = _voxMap;
    _oc.voxMediemMap = VoxMediumMap;
    _oc.outOctTree = octTreeBuf;
    _oc.nodeCount = nodeCountBuf;
    _oc.voxVis = voxVisBuf;
    _oc.nodeCountOutAlias = nodeCountBufAlias;
    _oc.octTreeOutAlias = octTreeBufAlias;
}

void recOctTree(const OctTree& _oc)
{
    kage::startRec(_oc.pass);

    kage::fillBuffer(_oc.outOctTree, 0);
    kage::fillBuffer(_oc.nodeCount, 0);
    kage::fillBuffer(_oc.voxMediemMap, UINT32_MAX);
    kage::fillBuffer(_oc.voxVis, 0);

    uint32_t maxLv = calcMipLevelCount(kVoxelSideCount);
    uint32_t curr_voxGridCount = kVoxelSideCount;
    uint32_t writeOffset = 0;
    uint32_t readOffset = 0;
    for (uint32_t ii= 0; ii < maxLv; ++ii)
    {
        curr_voxGridCount = kVoxelSideCount >> (ii + 1);

        OctTreeProcessConfig conf{};
        conf.lv = ii;
        conf.readOffset = readOffset;
        conf.currOffset = writeOffset;
        conf.voxGridSideCount = curr_voxGridCount;

        const kage::Memory* mem = kage::alloc(sizeof(OctTreeProcessConfig));
        memcpy(mem->data, &conf, mem->size);
        kage::setConstants(mem);

        kage::Binding binds[] =
        {
            { _oc.inVoxMap,         Access::read,       Stage::compute_shader },
            { _oc.voxMediemMap,     Access::read_write, Stage::compute_shader },
            { _oc.outOctTree,       Access::read_write, Stage::compute_shader },
            { _oc.nodeCount,        Access::read_write, Stage::compute_shader },
            { _oc.voxVis,           Access::read_write, Stage::compute_shader },
        };
        kage::pushBindings(binds, COUNTOF(binds));

        kage::dispatch(curr_voxGridCount, curr_voxGridCount, curr_voxGridCount);

        readOffset = writeOffset;
        writeOffset += curr_voxGridCount * curr_voxGridCount * curr_voxGridCount;
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
    kage::ImageHandle sdfAtlas;
    kage::ImageHandle depth;
    kage::ImageHandle skybox;
    kage::BufferHandle trans;
};

void prepareRCbuild(RadianceCascadeBuild& _rc, const RCBuildInit& _init)
{
    // build the cascade image
    kage::ShaderHandle cs = kage::registShader("build_cascade", "shaders/rc_build_cascade.comp.spv");
    kage::ProgramHandle program = kage::registProgram("build_cascade", { cs }, sizeof(RadianceCascadesConfig));

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("build_cascade", passDesc);

    uint32_t texelSideCount = kage::k_rclv0_probeSideCount * kage::k_rclv0_rayGridSideCount;
    uint32_t imgLayers = kage::k_rclv0_probeSideCount * 2 - 1;

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
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::SamplerHandle albedoSamp = kage::sampleImage(pass, _init.g_buffer.albedo
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle normSamp = kage::sampleImage(pass, _init.g_buffer.normal
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle wpSamp = kage::sampleImage(pass, _init.g_buffer.worldPos
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle emmiSamp = kage::sampleImage(pass, _init.g_buffer.emissive
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle depthSamp = kage::sampleImage(pass, _init.depth
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle skySamp = kage::sampleImage(pass, _init.skybox
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::bindBuffer(pass, _init.octTreeCount
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.octTree
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.voxAlbedo
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindImage(pass, img
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    kage::bindImage(pass, _init.sdfAtlas
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
        , kage::ImageLayout::general
    );

    _rc.pass = pass;
    _rc.program = program;
    _rc.cs = cs;

    _rc.g_buffer = _init.g_buffer;
    _rc.g_bufferSamplers.albedo = albedoSamp;
    _rc.g_bufferSamplers.normal = normSamp;
    _rc.g_bufferSamplers.worldPos = wpSamp;
    _rc.g_bufferSamplers.emissive = emmiSamp;
    _rc.inSkybox = _init.skybox;
    _rc.skySampler = skySamp;

    _rc.trans = _init.trans;

    _rc.inOctTreeNodeCount = _init.octTreeCount;
    _rc.inOctTree = _init.octTree;
    _rc.inSdfAtlas = _init.sdfAtlas;
    _rc.inDepth = _init.depth;
    _rc.depthSampler = depthSamp;

    _rc.voxAlbedo = _init.voxAlbedo;

    _rc.cascadeImg = img;
    _rc.radCascdOutAlias = outAlias;
}

void recRCBuild(const RadianceCascadeBuild& _rc, const float _sceneRadius)
{
    kage::startRec(_rc.pass);

    for (uint32_t ii = 0; ii < kage::k_rclv0_cascadeLv; ++ii)
    {
        uint32_t    level_factor    = uint32_t(1 << ii);
        uint32_t    prob_sideCount  = kage::k_rclv0_probeSideCount / level_factor;
        uint32_t    prob_count      = uint32_t(pow(prob_sideCount, 3));
        uint32_t    ray_sideCount   = kage::k_rclv0_rayGridSideCount * level_factor;
        uint32_t    ray_count       = ray_sideCount * ray_sideCount; // each probe has a 2d grid of rays
        float prob_sideLen = _sceneRadius * 2.f / float(prob_sideCount);
        float rayLen = length(vec3(prob_sideLen)) * .5f;

        RadianceCascadesConfig config;
        config.probe_sideCount = prob_sideCount;
        config.ray_gridSideCount = ray_sideCount;
        config.level = ii;
        config.layerOffset = (kage::k_rclv0_probeSideCount - prob_sideCount) * 2;
        config.rayLength = rayLen;
        config.probeSideLen = prob_sideLen;

        config.ot_voxSideCount = kVoxelSideCount;
        config.ot_sceneRadius = _sceneRadius;

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
            {_rc.inSkybox,              _rc.skySampler,                 Stage::compute_shader},
            {_rc.inOctTreeNodeCount,    Access::read,                   Stage::compute_shader},
            {_rc.inOctTree,             Access::read,                   Stage::compute_shader},
            {_rc.voxAlbedo,             Access::read,                   Stage::compute_shader},
            {_rc.cascadeImg,            0,                              Stage::compute_shader},
        };
        kage::pushBindings(binds, COUNTOF(binds));

        uint32_t groupCnt = prob_sideCount * ray_sideCount;
        kage::dispatch(groupCnt, groupCnt, prob_sideCount);
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
    voxelizeInitData.maxDrawCmdCount = _init.maxDrawCmdCount;
    voxelizeInitData.bindless = _init.bindless;
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
    rcInit.sdfAtlas = _init.sdfAtlas;
    rcInit.skybox = _init.skybox;
    rcInit.trans = _init.transBuf;
    prepareRCbuild(_rc.rcBuild, rcInit);
}

void updateRadianceCascade(
    const RadianceCascade& _rc
    , uint32_t _drawCount
    , const DrawCull& _camCull
    , const uint32_t _width
    , const uint32_t _height
    , const float _sceneRadius
)
{
    recFullDrawCmdPass(_rc.voxCmd, _drawCount);
    recVoxelization(_rc.vox, _sceneRadius);
    recOctTree(_rc.octTree);
    recRCBuild(_rc.rcBuild, _sceneRadius);
}
