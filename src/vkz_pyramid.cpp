#include "vkz_pyramid.h"
#include "kage_math.h"
#include "profiler.h"

#include "bx/readerwriter.h"

void renderPyr(const PyramidRendering& _pyramid)
{
    VKZ_ZoneScopedC(kage::Color::blue);

    kage::startRec(_pyramid.pass);

    for (uint16_t ii = 0; ii < _pyramid.levels; ++ii)
    {
        uint32_t levelWidth = glm::max(1u, _pyramid.width >> ii);
        uint32_t levelHeight = glm::max(1u, _pyramid.height >> ii);

        vec2 levelSize = vec2(levelWidth, levelHeight);
        const kage::Memory* mem = kage::alloc(sizeof(vec2));
        bx::memCopy(mem->data, &levelSize, mem->size);

        kage::setConstants(mem);

        kage::ImageHandle srcImg = (ii == 0) ? _pyramid.inDepth : _pyramid.image;

        uint16_t srcMip =
            ii == 0
            ? kage::kAllMips
            : (ii - 1);

        using Stage = kage::PipelineStageFlagBits::Enum;

        kage::Binding binds[] =
        {
            {srcImg, srcMip, _pyramid.sampler, Stage::compute_shader},
            {_pyramid.image, ii, Stage::compute_shader}
        };

        kage::setBindings(binds, COUNTOF(binds));

        kage::dispatch(levelWidth, levelHeight, 1);
    }

    kage::endRec();
}

uint32_t previousPow2_py(uint32_t v)
{
    uint32_t r = 1;

    while (r < v)
        r <<= 1;
    r >>= 1;
    return r;
}

uint32_t calculateMipLevelCount_py(uint32_t width, uint32_t height)
{
    uint32_t result = 0;
    while (width > 1 || height > 1)
    {
        result++;
        width >>= 1;
        height >>= 1;
    }

    return result;
}

void preparePyramid(PyramidRendering& _pyramid, uint32_t _width, uint32_t _height)
{
    uint32_t level_width = previousPow2_py(_width);
    uint32_t level_height = previousPow2_py(_height);
    uint32_t levels = calculateMipLevelCount_py(level_width, level_height);

    // create image
    kage::ImageDesc desc{};
    desc.width = level_width;
    desc.height = level_height;
    desc.format = kage::ResourceFormat::r32_sfloat;
    desc.depth = 1;
    desc.numLayers = 1;
    desc.numMips = levels;
    desc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled | kage::ImageUsageFlagBits::storage;
    desc.layout = kage::ImageLayout::general;

    kage::ImageHandle img = kage::registTexture("pyramid", desc, nullptr, kage::ResourceLifetime::non_transition);
    kage::ImageHandle outAlias = kage::alias(img);

    // create shader
    kage::ShaderHandle cs = kage::registShader("pyramid_shader", "shaders/depthpyramid.comp.spv");
    kage::ProgramHandle program = kage::registProgram("pyramid_prog", { cs }, sizeof(glm::vec2));

    // create pass
    kage::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("pyramid_pass", passDesc);

    // set the pyramid data
    _pyramid.image = img;
    _pyramid.program = program;
    _pyramid.cs = cs;
    _pyramid.pass = pass;

    _pyramid.width = level_width;
    _pyramid.height = level_height;
    _pyramid.levels = levels;

    _pyramid.imgOutAlias = outAlias;
}

void setPyramidPassDependency(PyramidRendering& _pyramid, const kage::ImageHandle _inDepth)
{
    _pyramid.inDepth = _inDepth;

    _pyramid.sampler = kage::sampleImage(_pyramid.pass, _inDepth
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerReductionMode::min
    );

    kage::bindImage(_pyramid.pass, _pyramid.image
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , _pyramid.imgOutAlias
    );
}

void updatePyramid(PyramidRendering& _pyramid, uint32_t _width, uint32_t _height)
{

    const uint32_t level_width = previousPow2_py(_width);
    const uint32_t level_height = previousPow2_py(_height);
    const uint32_t levels = calculateMipLevelCount_py(level_width, level_height);
    if (level_width != _pyramid.width || level_height != _pyramid.height)
    {
        _pyramid.width = level_width;
        _pyramid.height = level_height;
        _pyramid.levels = levels;

        kage::updateImage2D(_pyramid.image, level_width, level_height);
    }

    renderPyr(_pyramid);
}

