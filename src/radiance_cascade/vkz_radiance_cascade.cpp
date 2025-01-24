#include "vkz_radiance_cascade.h"


using Stage = kage::PipelineStageFlagBits::Enum;
using Access = kage::BindingAccess;
using LoadOp = kage::AttachmentLoadOp;
using StoreOp = kage::AttachmentStoreOp;

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

void recFullDrawCmdPass(const VoxelizationCmd& _vc)
{
    kage::startRec(_vc.pass);

    kage::Binding binds[] =
    {
        {_vc.in_meshBuf,        Access::read,       Stage::compute_shader },
        { _vc.in_drawBuf,       Access::read,       Stage::compute_shader },
        { _vc.out_cmdBuf,       Access::write,      Stage::compute_shader },
        { _vc.out_cmdCountBuf,  Access::read_write, Stage::compute_shader },
    };

    kage::pushBindings(binds, COUNTOF(binds));
    kage::dispatch(1, 1, 1);
    kage::endRec();
}

struct VoxInitData
{
    kage::BufferHandle cmdBuf;
    kage::BufferHandle cmdCountBuf;
    kage::BufferHandle drawBuf;
    kage::BufferHandle vtxBuf;
    kage::BufferHandle transBuf;

    float sceneRadius;
};

void prepareVoxelization(Voxelization& _vox, const VoxInitData& _init)
{
    // voxelization
    // in order to use on old hardware, use a geometry shader to generate the voxelization
    // should possible to use a mesh shader to do the same thing 
    kage::ShaderHandle vs = kage::registShader("build_cascade", "shaders/rc_voxelize.vert.spv");
    kage::ShaderHandle gs = kage::registShader("build_cascade", "shaders/rc_voxelize.geom.spv");
    kage::ShaderHandle fs = kage::registShader("build_cascade", "shaders/rc_voxelize.frag.spv");
    kage::ProgramHandle program = kage::registProgram("build_cascade", { vs, gs, fs }, sizeof(RadianceCascadesConfig));

    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::graphics;
    passDesc.pipelineConfig = { false, false, kage::CompareOp::never };

    kage::PassHandle pass = kage::registPass("voxelize", passDesc);

    kage::ImageHandle albedo;
    {
        kage::ImageDesc imgDesc{};
        imgDesc.width = 128;
        imgDesc.height = 128;
        imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
        imgDesc.depth = 128;
        imgDesc.numLayers = 1;
        imgDesc.numMips = 1;
        imgDesc.type = kage::ImageType::type_3d;
        imgDesc.viewType = kage::ImageViewType::type_3d;
        imgDesc.usage = kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled;
        imgDesc.layout = kage::ImageLayout::general;
        
        albedo = kage::registRenderTarget("rt_voxel_color", imgDesc, kage::ResourceLifetime::transition);
    }

    //use a 1k * 1k attachment for voxelization
    kage::ImageHandle rt;
    {
        kage::ImageDesc imgDesc{};
        imgDesc.width = 128;
        imgDesc.height = 128;
        imgDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
        imgDesc.depth = 1;
        imgDesc.numLayers = 1;
        imgDesc.numMips = 1;
        imgDesc.type = kage::ImageType::type_2d;
        imgDesc.viewType = kage::ImageViewType::type_2d;
        imgDesc.usage = kage::ImageUsageFlagBits::storage | kage::ImageUsageFlagBits::sampled;
        imgDesc.layout = kage::ImageLayout::general;

        rt = kage::registRenderTarget("rc_voxelize_rt", imgDesc, kage::ResourceLifetime::transition);
    }

    kage::bindBuffer(pass, _init.cmdBuf
        , 0
        , kage::PipelineStageFlagBits::mesh_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.drawBuf
        , 1
        , kage::PipelineStageFlagBits::mesh_shader
        , kage::AccessFlagBits::shader_read
    );
    
    kage::bindBuffer(pass, _init.vtxBuf
        , 2
        , kage::PipelineStageFlagBits::mesh_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::bindBuffer(pass, _init.transBuf
        , 4
        , kage::PipelineStageFlagBits::mesh_shader
        , kage::AccessFlagBits::shader_read
    );

    kage::ImageHandle rtAlias = kage::alias(rt);
    kage::ImageHandle albedoAlias = kage::alias(albedo);
    kage::setAttachmentOutput(pass, rt, 0, rtAlias);
    kage::setAttachmentOutput(pass, albedo, 1, albedoAlias);


    _vox.pass = pass;
    _vox.program = program;
    _vox.vs = vs;
    _vox.gs = gs;
    _vox.fs = fs;

    _vox.cmdBuf = _init.cmdBuf;
    _vox.cmdCountBuf = _init.cmdCountBuf;
    _vox.drawBuf = _init.drawBuf;
    _vox.vtxBuf = _init.vtxBuf;
    _vox.transBuf = _init.transBuf;
    _vox.voxelAlbedo = albedo;
    _vox.voxelAlbedoOutAlias = albedoAlias;

}

void recVoxelization(const Voxelization& _vox)
{
    kage::startRec(_vox.pass);

    kage::endRec();
}

void prepareRC(RadianceCascadeBuild& _rc, const GBuffer& _gb, const kage::ImageHandle _depth, const kage::ImageHandle _voxAlbedo, const kage::BufferHandle _trans)
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
    imgDesc.format = kage::ResourceFormat::r32g32b32a32_sfloat;
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

    kage::SamplerHandle voxSamp = kage::sampleImage(pass, _voxAlbedo
        , 6
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::min
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
    _rc.voxAlbedoSampler = voxSamp;

    _rc.cascadeImg = img;
    _rc.outAlias = outAlias;
}

void recRC(const RadianceCascadeBuild& _rc)
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
            {_rc.voxAlbedo,         _rc.voxAlbedoSampler,           Stage::compute_shader},
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
    voxelizeInitData.vtxBuf = _init.vtxBuf;
    voxelizeInitData.transBuf = _init.transBuf;
    voxelizeInitData.sceneRadius = _init.sceneRadius;

    prepareVoxelization(_rc.vox, voxelizeInitData);

    // next step should use the voxelization data to build the cascade
    prepareRC(_rc.rcBuild, _init.g_buffer, _init.depth, _rc.vox.voxelAlbedoOutAlias, _init.transBuf);
}

void updateRadianceCascade(const RadianceCascade& _rc)
{
    recFullDrawCmdPass(_rc.voxCmd);
    recVoxelization(_rc.vox);
    recRC(_rc.rcBuild);
}
