#include "vkz_radiance_cascade.h"
#include "demo_structs.h"


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
        
        worldPos = kage::registBuffer("rc_vox_wpos", bufDesc);
    }

    kage::BufferHandle albedo;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = c_voxelLength * c_voxelLength * c_voxelLength * 4;
        bufDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
        bufDesc.usage = kage::BufferUsageFlagBits::storage_texel | kage::BufferUsageFlagBits::transfer_dst;

        albedo = kage::registBuffer("rc_vox_albedo", bufDesc);
    }

    kage::BufferHandle normal;
    {
        kage::BufferDesc bufDesc{};
        bufDesc.size = c_voxelLength * c_voxelLength * c_voxelLength * 4;
        bufDesc.format = kage::ResourceFormat::r16g16b16a16_sfloat;
        bufDesc.usage = kage::BufferUsageFlagBits::storage_texel | kage::BufferUsageFlagBits::transfer_dst;


        normal = kage::registBuffer("rc_vox_normal", bufDesc);
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

    _vox.voxelWorldPos = worldPos;
    _vox.voxelAlbedo = albedo;
    _vox.voxelNormal = normal;

    _vox.wposOutAlias = wposAlias;
    _vox.albedoOutAlias = albedoAlias;
    _vox.normalOutAlias = normalAlias;

    _vox.fragCountBuf = fragCountBuf;
    _vox.fragCountBufOutAlias = fragCountBufAlias;

    _vox.rt = dummy_rt;

    _vox.maxDrawCmdCount = _init.maxDrawCmdCount;
}

void recVoxelization(const Voxelization& _vox, const mat4& _proj)
{
    kage::startRec(_vox.pass);

    kage::setViewport(0, 0, c_voxelLength, c_voxelLength);
    kage::setScissor(0, 0, c_voxelLength, c_voxelLength);

    kage::fillBuffer(_vox.fragCountBuf, 0);

    VoxelizationConfig vc{};
    vc.proj = _proj;
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

void prepareRCbuild(RadianceCascadeBuild& _rc, const GBuffer& _gb, const kage::ImageHandle _depth, const kage::BufferHandle _voxAlbedo, const kage::BufferHandle _trans)
{
    _rc.lv0Config.probeDiameter = 32;
    _rc.lv0Config.rayGridDiameter = 16;
    _rc.lv0Config.level = 5;
    _rc.lv0Config.rayLength = 1.0f;
    _rc.lv0Config.rayMarchingSteps = 4;

    // build the cascade image
    kage::ShaderHandle cs = kage::registShader("build_cascade", "shaders/rc_build_cascade.comp.spv");
    kage::ProgramHandle program = kage::registProgram("build_cascade", { cs }, sizeof(RadianceCascadesConfig));

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("build_cascade", passDesc);

    uint32_t imgDiameter = _rc.lv0Config.probeDiameter * _rc.lv0Config.rayGridDiameter;
    uint32_t imgLayers = _rc.lv0Config.probeDiameter * 2 - 1;

    kage::ImageDesc imgDesc{};
    imgDesc.width = imgDiameter;
    imgDesc.height = imgDiameter;
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

    kage::bindBuffer(pass, _trans
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::SamplerHandle albedoSamp = kage::sampleImage(pass, _gb.albedo
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle normSamp = kage::sampleImage(pass, _gb.normal
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle wpSamp = kage::sampleImage(pass, _gb.worldPos
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle emmiSamp = kage::sampleImage(pass, _gb.emissive
        , 4
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::SamplerHandle depthSamp = kage::sampleImage(pass, _depth
        , 5
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
    );

    kage::bindBuffer(pass, _voxAlbedo
        , 6
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindImage(pass, img
        , 7
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    _rc.pass = pass;
    _rc.program = program;
    _rc.cs = cs;

    _rc.g_buffer = _gb;
    _rc.g_bufferSamplers.albedo = albedoSamp;
    _rc.g_bufferSamplers.normal = normSamp;
    _rc.g_bufferSamplers.worldPos = wpSamp;
    _rc.g_bufferSamplers.emissive = emmiSamp;

    _rc.trans = _trans;

    _rc.inDepth = _depth;
    _rc.depthSampler = depthSamp;

    _rc.voxAlbedo = _voxAlbedo;

    _rc.cascadeImg = img;
    _rc.outAlias = outAlias;
}

void recRCBuild(const RadianceCascadeBuild& _rc)
{
    kage::startRec(_rc.pass);

    uint32_t imgLayers = _rc.lv0Config.level * _rc.lv0Config.probeDiameter * 2 - 1;

    uint32_t layerOffset = 0;
    for (uint32_t ii = 0; ii < _rc.lv0Config.level; ++ii)
    {
        uint32_t level_factor = uint32_t(1 << ii);
        uint32_t prob_diameter = _rc.lv0Config.probeDiameter / level_factor;
        uint32_t prob_count = uint32_t(pow(prob_diameter, 3));
        uint32_t ray_diameter = _rc.lv0Config.rayGridDiameter * level_factor;
        uint32_t ray_count = ray_diameter * ray_diameter;

        RadianceCascadesConfig config;
        config.probeDiameter = prob_diameter;
        config.rayGridDiameter = ray_diameter;
        config.level = ii;
        config.layerOffset = layerOffset;

        layerOffset += prob_diameter;

        const kage::Memory* mem = kage::alloc(sizeof(RadianceCascadesConfig));
        memcpy(mem->data, &config, mem->size);

        kage::setConstants(mem);

        kage::Binding binds[] =
        {
            {_rc.trans,             kage::BindingAccess::read,      Stage::compute_shader},
            {_rc.g_buffer.albedo,   _rc.g_bufferSamplers.albedo,    Stage::compute_shader},
            {_rc.g_buffer.normal,   _rc.g_bufferSamplers.normal,    Stage::compute_shader},
            {_rc.g_buffer.worldPos, _rc.g_bufferSamplers.worldPos,  Stage::compute_shader},
            {_rc.g_buffer.emissive, _rc.g_bufferSamplers.emissive,  Stage::compute_shader},
            {_rc.inDepth,           _rc.depthSampler,               Stage::compute_shader},
            {_rc.voxAlbedo,         kage::BindingAccess::read,      Stage::compute_shader},
            {_rc.cascadeImg,        0,                              Stage::compute_shader},
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

    // next step should use the voxelization data to build the cascade
    prepareRCbuild(_rc.rcBuild, _init.g_buffer, _init.depth, _rc.vox.albedoOutAlias, _init.transBuf);
}

void updateRadianceCascade(const RadianceCascade& _rc, uint32_t _drawCount, const mat4& _proj)
{
    recFullDrawCmdPass(_rc.voxCmd, _drawCount);
    recVoxelization(_rc.vox, _proj);
    recRCBuild(_rc.rcBuild);
}
