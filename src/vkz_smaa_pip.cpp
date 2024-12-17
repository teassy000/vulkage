#include "vkz_smaa_pip.h"

#include "kage.h"
#include "kage_math.h"
#include "bx/bx.h"
#include "file_helper.h"

using ImgUsage = kage::ImageUsageFlagBits::Enum;

using Stage = kage::PipelineStageFlagBits::Enum;

void prepare(SMAAEdgeDepth& _edge, uint32_t _width, uint32_t _height, kage::ImageHandle _inDepth)
{
    kage::ShaderHandle cs = kage::registShader("smaa_edge_depth", "shaders/smaa_edge_depth.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("smaa_edge_depth_prog", { cs }, sizeof(glm::vec2));
    kage::PassDesc passDesc{};
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("smaa_edge_depth_pass", passDesc);

    // edge
    kage::ImageDesc desc{};
    desc.width = _width;
    desc.height = _height;
    desc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    desc.depth = 1;
    desc.numLayers = 1;
    desc.numMips = 1;
    desc.usage = ImgUsage::sampled | ImgUsage::storage | ImgUsage::transfer_src | ImgUsage::color_attachment;
    kage::ImageHandle edge = kage::registTexture("smaa_edge_depth", desc, nullptr, kage::ResourceLifetime::transition);

    // set data
    _edge.prog = prog;
    _edge.pass = pass;
    _edge.cs = cs;

    _edge.inDepth = _inDepth;
    _edge.sampler = kage::sampleImage(_edge.pass, _inDepth
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::ImageHandle edgeOutAlias = kage::alias(edge);
    kage::bindImage(_edge.pass, edge
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , edgeOutAlias
    );

    _edge.depth = edge;
    _edge.depthOutAlias = edgeOutAlias;
}

void prepare(SMAAEdgeColor& _edge, uint32_t _width, uint32_t _height, kage::ImageHandle _inColor)
{
    kage::ShaderHandle cs = kage::registShader("smaa_edge_depth", "shaders/smaa_edge_color.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("smaa_edge_depth_prog", { cs }, sizeof(glm::vec2));
    kage::PassDesc passDesc{};
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("smaa_edge_depth_pass", passDesc);

    // edge
    kage::ImageDesc desc{};
    desc.width = _width;
    desc.height = _height;
    desc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    desc.depth = 1;
    desc.numLayers = 1;
    desc.numMips = 1;
    desc.usage = ImgUsage::sampled | ImgUsage::storage | ImgUsage::transfer_src | ImgUsage::color_attachment;
    kage::ImageHandle edge = kage::registTexture("smaa_edge_depth", desc, nullptr, kage::ResourceLifetime::transition);

    // set data
    _edge.prog = prog;
    _edge.pass = pass;
    _edge.cs = cs;

    _edge.inColor = _inColor;
    _edge.sampler = kage::sampleImage(_edge.pass, _inColor
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::ImageHandle edgeOutAlias = kage::alias(edge);
    kage::bindImage(_edge.pass, edge
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , edgeOutAlias
    );

    _edge.color= edge;
    _edge.colorOutAlias = edgeOutAlias;
}

void prepare(SMAAEdgeLuma& _edge, uint32_t _width, uint32_t _height, kage::ImageHandle _inColor)
{
    kage::ShaderHandle cs = kage::registShader("smaa_edge_color", "shaders/smaa_edge_luma.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("smaa_edge_color", { cs }, sizeof(glm::vec2));
    kage::PassDesc passDesc{};
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("smaa_edge_color", passDesc);
    
    // edge
    kage::ImageDesc desc{};
    desc.width = _width;
    desc.height = _height;
    desc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    desc.depth = 1;
    desc.numLayers = 1;
    desc.numMips = 1;
    desc.usage = ImgUsage::sampled | ImgUsage::storage | ImgUsage::transfer_src | ImgUsage::color_attachment;
    kage::ImageHandle edge = kage::registTexture("smaa_edge_color", desc, nullptr, kage::ResourceLifetime::transition);

    // set data
    _edge.prog = prog;
    _edge.pass = pass;
    _edge.cs = cs;

    _edge.inColor = _inColor;
    _edge.sampler = kage::sampleImage(_edge.pass, _inColor
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::ImageHandle edgeOutAlias = kage::alias(edge);
    kage::bindImage(_edge.pass, edge
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , edgeOutAlias
    );

    _edge.luma = edge;
    _edge.lumaOutAlias = edgeOutAlias;
}

void prepare(SMAAWeight& _weight, uint32_t _width, uint32_t _height, kage::ImageHandle _inEdge)
{
    kage::ShaderHandle cs = kage::registShader("smaa_weight", "shaders/smaa_weight.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("smaa_weight_prog", { cs }, sizeof(glm::vec2));
    kage::PassDesc passDesc{};
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("smaa_weight_pass", passDesc);

    // search image
    kage::ImageHandle search = loadImageFromFile("smaa_search", "data/textures/smaa/smaa_search.png");
    // area image
    kage::ImageHandle area = loadImageFromFile("smaa_area", "data/textures/smaa/smaa_area.png");

    // weight
    kage::ImageDesc desc{};
    desc.width = _width;
    desc.height = _height;
    desc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    desc.depth = 1;
    desc.numLayers = 1;
    desc.numMips = 1;
    desc.usage = ImgUsage::sampled | ImgUsage::storage | ImgUsage::transfer_src;
    kage::ImageHandle weight = kage::registTexture("smaa_weight", desc, nullptr, kage::ResourceLifetime::transition);
    
    
    _weight.prog = prog;
    _weight.pass = pass;
    _weight.cs = cs;
    _weight.inEdge = _inEdge;
    _weight.edgeSampler = kage::sampleImage(_weight.pass, _inEdge
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );
    _weight.areaImg = area;
    _weight.areaSampler = kage::sampleImage(_weight.pass, area
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );
    _weight.searchImg = search;
    _weight.searchSampler = kage::sampleImage(_weight.pass, search
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::nearest
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::ImageHandle weightOutAlias = kage::alias(weight);
    kage::bindImage(_weight.pass, weight
        , 3
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , weightOutAlias
    );

    _weight.weight = weight;
    _weight.weightOutAlias = weightOutAlias;
}

void prepare(SMAABlend& _blend, uint32_t _width, uint32_t _height, kage::ImageHandle _inColor, kage::ImageHandle _inWeight)
{
    kage::ShaderHandle cs = kage::registShader("smaa_blend", "shaders/smaa_blend.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("smaa_blend_prog", { cs }, sizeof(glm::vec2));
    kage::PassDesc passDesc{};
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;
    kage::PassHandle pass = kage::registPass("smaa_blend_pass", passDesc);

    // blend
    kage::ImageDesc desc{};
    desc.width = _width;
    desc.height = _height;
    desc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    desc.depth = 1;
    desc.numLayers = 1;
    desc.numMips = 1;
    desc.usage = ImgUsage::sampled | ImgUsage::storage | ImgUsage::transfer_src | ImgUsage::color_attachment;
    kage::ImageHandle blend = kage::registTexture("smaa_blend", desc, nullptr, kage::ResourceLifetime::transition);

    _blend.prog = prog;
    _blend.pass = pass;
    _blend.cs = cs;

    _blend.inWeight = _inWeight;
    _blend.weightSamper = kage::sampleImage(_blend.pass, _inWeight
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    _blend.inColor = _inColor;
    _blend.colorSampler = kage::sampleImage(_blend.pass, _inColor
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::ImageHandle blendOutAlias = kage::alias(blend);
    kage::bindImage(_blend.pass, blend
        , 2
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , blendOutAlias
    );

    _blend.blend = blend;
    _blend.blendOutAlias = blendOutAlias;
}

void recordCmd(const SMAAEdgeColor& _edge, uint32_t _width, uint32_t _height)
{
    KG_ZoneScopedC(kage::Color::blue);
    kage::startRec(_edge.pass);

    vec2 resolution = vec2(_width, _height);
    const kage::Memory* mem = kage::alloc(sizeof(vec2));
    bx::memCopy(mem->data, &resolution, mem->size);
    kage::setConstants(mem);
    using Stage = kage::PipelineStageFlagBits::Enum;
    kage::Binding binds[] =
    {
        {_edge.inColor, _edge.sampler, Stage::compute_shader},
        {_edge.color, 0, Stage::compute_shader},
    };
    kage::pushBindings(binds, COUNTOF(binds));
    kage::dispatch(_width, _height, 1);
    kage::endRec();
}

void recordCmd(const SMAAEdgeDepth& _edge, uint32_t _width, uint32_t _height)
{
    KG_ZoneScopedC(kage::Color::blue);
    kage::startRec(_edge.pass);

    vec2 resolution = vec2(_width, _height);
    const kage::Memory* mem = kage::alloc(sizeof(vec2));
    bx::memCopy(mem->data, &resolution, mem->size);
    kage::setConstants(mem);
    using Stage = kage::PipelineStageFlagBits::Enum;
    kage::Binding binds[] =
    {
        {_edge.inDepth, _edge.sampler, Stage::compute_shader},
        {_edge.depth, 0, Stage::compute_shader},
    };
    kage::pushBindings(binds, COUNTOF(binds));
    kage::dispatch(_width, _height, 1);
    kage::endRec();
}

void recordCmd(const SMAAEdgeLuma& _edge, uint32_t _width, uint32_t _height)
{
    KG_ZoneScopedC(kage::Color::blue);
    kage::startRec(_edge.pass);

    vec2 resolution = vec2(_width, _height);
    const kage::Memory* mem = kage::alloc(sizeof(vec2));
    bx::memCopy(mem->data, &resolution, mem->size);
    kage::setConstants(mem);
    using Stage = kage::PipelineStageFlagBits::Enum;
    kage::Binding binds[] =
    {
        {_edge.inColor, _edge.sampler, Stage::compute_shader},
        {_edge.luma, 0, Stage::compute_shader},
    };
    kage::pushBindings(binds, COUNTOF(binds));
    kage::dispatch(_width, _height, 1);
    kage::endRec();
}

void recordCmd(const SMAAWeight& _weight, uint32_t _width, uint32_t _height)
{
    KG_ZoneScopedC(kage::Color::blue);
    kage::startRec(_weight.pass);

    vec2 resolution = vec2(_width, _height);
    const kage::Memory* mem = kage::alloc(sizeof(vec2));
    bx::memCopy(mem->data, &resolution, mem->size);
    kage::setConstants(mem);
    using Stage = kage::PipelineStageFlagBits::Enum;
    kage::Binding binds[] =
    {
        {_weight.inEdge, _weight.edgeSampler, Stage::compute_shader},
        {_weight.areaImg, _weight.areaSampler, Stage::compute_shader},
        {_weight.searchImg, _weight.searchSampler, Stage::compute_shader},
        {_weight.weight, 0, Stage::compute_shader},
    };
    kage::pushBindings(binds, COUNTOF(binds));
    kage::dispatch(_width, _height, 1);
    kage::endRec();
}

void recordCmd(const SMAABlend& _blend, uint32_t _width, uint32_t _height)
{
    KG_ZoneScopedC(kage::Color::blue);
    kage::startRec(_blend.pass);
    vec2 resolution = vec2(_width, _height);
    const kage::Memory* mem = kage::alloc(sizeof(vec2));
    bx::memCopy(mem->data, &resolution, mem->size);
    kage::setConstants(mem);
    
    kage::Binding binds[] =
    {
        {_blend.inWeight, _blend.weightSamper, Stage::compute_shader},
        {_blend.inColor, _blend.colorSampler, Stage::compute_shader},
        {_blend.blend, 0, Stage::compute_shader},
    };
    kage::pushBindings(binds, COUNTOF(binds));
    kage::dispatch(_width, _height, 1);
    kage::endRec();
}


void SMAA::prepare(uint32_t _width, uint32_t _height, kage::ImageHandle _inColor, kage::ImageHandle _inDepth)
{
    ::prepare(m_edgeDepth, _width, _height, _inDepth);
    ::prepare(m_edgeColor, _width, _height, _inColor);
    ::prepare(m_edgeLuma, _width, _height, _inColor);

    ::prepare(m_weight, _width, _height, m_edgeLuma.lumaOutAlias);
    ::prepare(m_blend, _width, _height, _inColor, m_weight.weightOutAlias);

    m_w = _width;
    m_h = _height;
    m_outAliasImg =  m_blend.blendOutAlias;
}

void SMAA::update(uint32_t _rtWidth, uint32_t _rtHeight)
{
    if (_rtWidth != m_w || _rtHeight != m_h)
    {
        m_w = _rtWidth;
        m_h = _rtHeight;

        kage::updateImage2D(m_edgeDepth.depth, _rtWidth, _rtHeight);
        kage::updateImage2D(m_edgeColor.color, _rtWidth, _rtHeight);
        kage::updateImage2D(m_edgeLuma.luma, _rtWidth, _rtHeight);


        kage::updateImage2D(m_weight.weight, _rtWidth, _rtHeight);
        kage::updateImage2D(m_blend.blend, _rtWidth, _rtHeight);
    }

    recordCmd(m_edgeDepth, m_w, m_h);
    recordCmd(m_edgeColor, m_w, m_h);
    recordCmd(m_edgeLuma, m_w, m_h);

    recordCmd(m_weight, m_w, m_h);
    recordCmd(m_blend, m_w, m_h);
}
