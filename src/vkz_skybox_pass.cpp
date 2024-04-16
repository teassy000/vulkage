
#include "vkz_skybox_pass.h"
#include "mesh.h"
#include "file_helper.h"

void initSkyboxPass(SkyboxRendering& _skybox, const kage::BufferHandle _trans, const kage::ImageHandle _color)
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

        passDesc.passConfig.colorLoadOp = kage::AttachmentLoadOp::clear;
        passDesc.passConfig.colorStoreOp = kage::AttachmentStoreOp::store;
        passDesc.passConfig.depthLoadOp = kage::AttachmentLoadOp::dont_care;
        passDesc.passConfig.depthStoreOp = kage::AttachmentStoreOp::none;
        pass = kage::registPass("skybox_pass", passDesc);
    }

    // skybox mesh
    Geometry geom = {};
    loadMesh(geom, "../data/cube_skybox.obj", false);

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
    kage::ImageHandle cubemap = loadTextureFromFile("skybox_tex", "../data/textures/cubemap_vulkan.ktx");
    kage::ImageHandle outColor = kage::alias(_color);

    kage::bindVertexBuffer(pass, vtxBuf);
    kage::bindIndexBuffer(pass, idxBuf, (uint32_t)geom.indices.size());

    kage::bindBuffer(pass, _trans
        , 0
        , kage::PipelineStageFlagBits::vertex_shader
        , kage::AccessFlagBits::shader_read
    );

    _skybox.cubemapSampler = kage::sampleImage(pass, cubemap
        , 1
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::ImageLayout::shader_read_only_optimal
        , kage::SamplerReductionMode::weighted_average
    );

    kage::setAttachmentOutput(pass, _color, 0, outColor);

    _skybox.pass = pass;
    _skybox.prog = prog;
    _skybox.vs = vs;
    _skybox.fs = fs;

    _skybox.vtxBuf = vtxBuf;
    _skybox.idxBuf = idxBuf;

    _skybox.color = _color;
    _skybox.cubemap = cubemap;
    _skybox.trans = _trans;

    _skybox.colorOutAlias = outColor;
}

