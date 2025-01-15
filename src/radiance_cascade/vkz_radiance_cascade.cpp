#include "vkz_radiance_cascade.h"


uint32_t previousPow2(uint32_t v)
{
    uint32_t r = 1;

    while (r < v)
        r <<= 1;
    r >>= 1;
    return r;
}

void prepareRadianceCascade(RadianceCascade& _rc, uint32_t _w, uint32_t _h)
{
    _rc.lv0Config.probeDiameter = 32;
    _rc.lv0Config.rayGridDiameter = 16;
    _rc.lv0Config.level = 5;

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

    kage::bindImage(pass, img
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , outAlias
    );

    _rc.pass = pass;
    _rc.program = program;
    _rc.cs = cs;

    _rc.cascadeImg = img;
    _rc.outAlias = outAlias;
}

void recRC(const RadianceCascade& _rc)
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

        using Stage = kage::PipelineStageFlagBits::Enum;
        kage::Binding binds[] =
        {
            {_rc.cascadeImg, 0,  kage::PipelineStageFlagBits::compute_shader},
        };
        kage::pushBindings(binds, COUNTOF(binds));
        kage::dispatch(prob_count * ray_count, 1, 1);
    }

    kage::endRec();
}



void updateRadianceCascade(const RadianceCascade& _rc)
{
    recRC(_rc);
}
