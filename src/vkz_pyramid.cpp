#include "vkz_pyramid.h"
#include "vkz_math.h"
#include "profiler.h"

#include "bx/readerwriter.h"


void pyramid_renderFunc(vkz::CommandListI& _cmdList, const void* _data, uint32_t _size)
{
    VKZ_ZoneScopedC(vkz::Color::cyan);

    bx::MemoryReader reader(_data, _size);

    PyramidRendering pyramid{};
    bx::read(&reader, pyramid, nullptr);

    for (uint32_t ii = 0; ii < pyramid.levels; ++ii)
    {
        _cmdList.barrier(pyramid.image, vkz::AccessFlagBits::shader_write, vkz::ImageLayout::general, vkz::PipelineStageFlagBits::compute_shader);
        _cmdList.dispatchBarriers();

        uint32_t levelWidth = glm::max(1u, pyramid.width >> ii);
        uint32_t levelHeight = glm::max(1u, pyramid.height>> ii);

        vec2 levelSize = vec2(levelWidth, levelHeight);
        _cmdList.pushConstants(pyramid.pass, &levelSize, sizeof(levelSize));

        vkz::ImageHandle srcImg = (ii == 0) ? pyramid.inDepth : pyramid.image;

        vkz::ImageViewHandle srcImgView = ii == 0 ? vkz::ImageViewHandle{vkz::kInvalidHandle} : pyramid.imgMips[ii - 1];


        vkz::CommandListI::DescriptorSet desc[] =
        {
            {srcImg, srcImgView, pyramid.sampler},
            {pyramid.image, pyramid.imgMips[ii]}
        };

        _cmdList.pushDescriptorSetWithTemplate(pyramid.pass, desc, COUNTOF(desc));
        _cmdList.dispatch(pyramid.cs, levelWidth, levelHeight, 1);

        _cmdList.barrier(pyramid.image, vkz::AccessFlagBits::shader_read, vkz::ImageLayout::general, vkz::PipelineStageFlagBits::compute_shader);
        _cmdList.dispatchBarriers();
    }
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
    vkz::ImageDesc desc{};
    desc.width = level_width;
    desc.height = level_height;
    desc.format = vkz::ResourceFormat::r32_sfloat;
    desc.depth = 1;
    desc.numLayers = 1;
    desc.numMips = levels;
    desc.usage = vkz::ImageUsageFlagBits::transfer_src | vkz::ImageUsageFlagBits::sampled | vkz::ImageUsageFlagBits::storage;

    vkz::ImageHandle img = vkz::registTexture("pyramid", desc, nullptr, vkz::ResourceLifetime::non_transition);
    vkz::ImageHandle outAlias = vkz::alias(img);

    // create shader
    vkz::ShaderHandle cs = vkz::registShader("pyramid_shader", "shaders/depthpyramid.comp.spv");
    vkz::ProgramHandle program = vkz::registProgram("pyramid_prog", { cs }, sizeof(glm::vec2));

    // create pass
    vkz::PassDesc passDesc{};
    passDesc.programId = program.id;
    passDesc.queue = vkz::PassExeQueue::compute;

    vkz::PassHandle pass = vkz::registPass("pyramid_pass", passDesc);

    // set the pyramid data
    _pyramid.image = img;
    _pyramid.program = program;
    _pyramid.cs = cs;
    _pyramid.pass = pass;

    _pyramid.width = level_width;
    _pyramid.height = level_height;
    _pyramid.levels = levels;

    _pyramid.imgOutAlias = outAlias;

    for (uint32_t ii = 0; ii < levels; ++ii)
    {
        _pyramid.imgMips[ii] = vkz::registImageView("pyramidMips", img, ii, 1);
    }
}

void setPyramidPassDependency(PyramidRendering& _pyramid, const vkz::ImageHandle _inDepth)
{
    _pyramid.inDepth = _inDepth;

    _pyramid.sampler = vkz::sampleImage(_pyramid.pass, _inDepth
        , 0
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::ImageLayout::shader_read_only_optimal
        , vkz::SamplerReductionMode::min
    );

    vkz::bindImage(_pyramid.pass, _pyramid.image
        , 1
        , vkz::PipelineStageFlagBits::compute_shader
        , vkz::AccessFlagBits::shader_write
        , vkz::ImageLayout::general
        , _pyramid.imgOutAlias
    );

    const vkz::Memory* mem = vkz::alloc(sizeof(PyramidRendering));
    memcpy(mem->data, &_pyramid, mem->size);

    vkz::setCustomRenderFunc(_pyramid.pass, pyramid_renderFunc, mem);
}
