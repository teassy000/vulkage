#include "vkz_aa_pip.h"

#include "kage.h"
#include "kage_math.h"
#include "bx/bx.h"


void prepareAA(AntiAliasingPass& _aa, uint32_t _width, uint32_t _height)
{
    kage::ShaderHandle cs = kage::registShader("aa_shader", "shaders/postproc.comp.spv");
    kage::ProgramHandle prog = kage::registProgram("aa_prog", { cs }, sizeof(glm::vec2));

    kage::PassDesc passDesc{};
    passDesc.programId = prog.id;
    passDesc.queue = kage::PassExeQueue::compute;

    kage::PassHandle pass = kage::registPass("aa_pass", passDesc);

    // set anti-aliasing data
    _aa.program = prog;
    _aa.cs = cs;
    _aa.pass = pass;
    _aa.w = _width;
    _aa.h = _height;
}

void setAAPassDependency(AntiAliasingPass& _aa, const kage::ImageHandle _inColor)
{
    
    kage::ImageHandle outAlias = kage::alias(_inColor);

    _aa.color = _inColor;
    _aa.colorOutAlias = outAlias;

    _aa.sampler = kage::sampleImage(_aa.pass, _inColor
        , 0
        , kage::PipelineStageFlagBits::compute_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::nearest
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::bindImage(_aa.pass, _aa.color
        , 1
        , kage::PipelineStageFlagBits::compute_shader
        , kage::AccessFlagBits::shader_write
        , kage::ImageLayout::general
        , _aa.colorOutAlias
    );
}

void recAntiAliasing(AntiAliasingPass& _aa)
{
    KG_ZoneScopedC(kage::Color::blue);

    kage::startRec(_aa.pass);

    uint32_t width = _aa.w;
    uint32_t height = _aa.h;

    vec2 levelSize = vec2(width, height);
    const kage::Memory* mem = kage::alloc(sizeof(vec2));
    bx::memCopy(mem->data, &levelSize, mem->size);

    kage::setConstants(mem);

    using Stage = kage::PipelineStageFlagBits::Enum;
    kage::Binding binds[] =
    {
        {_aa.color, _aa.sampler, Stage::compute_shader},
        {_aa.color, 0, Stage::compute_shader},
    };

    kage::pushBindings(binds, COUNTOF(binds));

    kage::dispatch(width, height, 1);

    kage::endRec();
}

void updateAA(AntiAliasingPass& _aa, uint32_t _rtWidth, uint32_t _rtHeight)
{
    if (_rtWidth != _aa.w || _rtHeight != _aa.h)
    {
        _aa.w = _rtWidth;
        _aa.h = _rtHeight;
        kage::updateImage2D(_aa.color, _rtWidth, _rtHeight);
    }

    recAntiAliasing(_aa);
}
