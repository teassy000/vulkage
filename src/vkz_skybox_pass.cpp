
#include "vkz_skybox_pass.h"
#include "scene.h"
#include "profiler.h"

void initSkyboxPass(Skybox& _skybox, const kage::BufferHandle _trans, const kage::ImageHandle _color, const kage::ImageHandle _skycube)
{
    // shader
    kage::ShaderHandle vs = kage::registShader("skybox_vert_shader", "shaders/skybox.vert.spv");
    kage::ShaderHandle fs = kage::registShader("skybox_frag_shader", "shaders/skybox.frag.spv");
    kage::ProgramHandle prog = kage::registProgram("mesh_prog", { vs, fs });

    // pass
    kage::PassHandle pass;
    {
        kage::VertexBindingDesc bindings[] =
        {
            {0, sizeof(Vertex), kage::VertexInputRate::vertex},
        };

        kage::VertexAttributeDesc attributes[] =
        {
            {0, 0, kage::ResourceFormat::r32g32b32_sfloat, offsetof(Vertex, vx)},
        };

        const kage::Memory* vtxBindingMem = kage::alloc(uint32_t(sizeof(kage::VertexBindingDesc) * COUNTOF(bindings)));
        memcpy(vtxBindingMem->data, bindings, sizeof(kage::VertexBindingDesc) * COUNTOF(bindings));

        const kage::Memory* vtxAttributeMem = kage::alloc(uint32_t(sizeof(kage::VertexAttributeDesc) * COUNTOF(attributes)));
        memcpy(vtxAttributeMem->data, attributes, sizeof(kage::VertexAttributeDesc) * COUNTOF(attributes));

        kage::PassDesc passDesc;
        passDesc.programId = prog.id;
        passDesc.queue = kage::PassExeQueue::graphics;
        passDesc.vertexBindingNum = (uint32_t)COUNTOF(bindings);
        passDesc.vertexBindings = vtxBindingMem->data;
        passDesc.vertexAttributeNum = (uint32_t)COUNTOF(attributes);
        passDesc.vertexAttributes = vtxAttributeMem->data;

        passDesc.pipelineConfig.enableDepthTest = false;
        passDesc.pipelineConfig.enableDepthWrite = false;
        passDesc.pipelineConfig.depthCompOp = kage::CompareOp::never;
        passDesc.pipelineConfig.cullMode = kage::CullModeFlagBits::front;

        pass = kage::registPass("skybox_pass", passDesc);
    }

    // skybox mesh
    Geometry geom = {};
    loadObj(geom, "./data/cube.obj", false, false);

    const kage::Memory* vtxMem = kage::alloc(uint32_t(geom.vertices.size() * sizeof(Vertex)));
    memcpy(vtxMem->data, geom.vertices.data(), geom.vertices.size() * sizeof(Vertex));

    const kage::Memory* idxMem = kage::alloc(uint32_t(geom.indices.size() * sizeof(uint32_t)));
    memcpy(idxMem->data, geom.indices.data(), geom.indices.size() * sizeof(uint32_t));

    kage::BufferDesc vtxDesc;
    vtxDesc.size = vtxMem->size;
    vtxDesc.usage = kage::BufferUsageFlagBits::vertex;
    kage::BufferHandle vtxBuf = kage::registBuffer("skybox_vtx", vtxDesc, vtxMem, kage::ResourceLifetime::non_transition);

    kage::BufferDesc idxDesc;
    idxDesc.size = idxMem->size;
    idxDesc.usage = kage::BufferUsageFlagBits::index;
    kage::BufferHandle idxBuf = kage::registBuffer("skybox_idx", idxDesc, idxMem, kage::ResourceLifetime::non_transition);


    // load texture 

    kage::ImageHandle outColor = kage::alias(_color);

    kage::bindVertexBuffer(pass, vtxBuf);
    kage::bindIndexBuffer(pass, idxBuf, (uint32_t)geom.indices.size());

    kage::bindBuffer(pass, _trans
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read
    );

    _skybox.cubemapSampler = kage::sampleImage(pass, _skycube
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::setAttachmentOutput(pass, _color, outColor);

    _skybox.pass = pass;
    _skybox.prog = prog;
    _skybox.vs = vs;
    _skybox.fs = fs;

    _skybox.vb = vtxBuf;
    _skybox.ib = idxBuf;

    _skybox.color = _color;
    _skybox.cubemap = _skycube;
    _skybox.trans = _trans;

    _skybox.colorOutAlias = outColor;
}

void skyboxRec(const Skybox& _skybox, uint32_t _w, uint32_t _h)
{
    KG_ZoneScopedC(kage::Color::blue);

    using Stage = kage::PipelineStageFlagBits::Enum;
    using Access = kage::BindingAccess;
    kage::Binding binds[] =
    {
        {_skybox.trans,     Access::read,           Stage::vertex_shader},
        {_skybox.cubemap,   _skybox.cubemapSampler, Stage::fragment_shader},
    };

    kage::startRec(_skybox.pass);

    kage::setViewport(0, 0, _w, _h);
    kage::setScissor(0, 0, _w, _h);

    kage::setIndexBuffer(_skybox.ib, 0, kage::IndexType::uint32);
    kage::setVertexBuffer(_skybox.vb);

    kage::pushBindings(binds, COUNTOF(binds));



    kage::Attachment attachments[] = {
        {_skybox.color, kage::AttachmentLoadOp::clear, kage::AttachmentStoreOp::store},
    };
    kage::setColorAttachments(attachments, COUNTOF(attachments));

    kage::drawIndexed(36, 1, 0, 0, 0);

    kage::endRec();

}

void updateSkybox(const Skybox& _skybox, uint32_t _w, uint32_t _h)
{
    skyboxRec(_skybox, _w, _h);
}

