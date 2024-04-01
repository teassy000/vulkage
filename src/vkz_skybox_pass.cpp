
#include "vkz_skybox_pass.h"
#include "mesh.h"
#include "file_helper.h"

void initSkyboxPass(SkyboxRendering& _skybox, const vkz::BufferHandle _trans, const vkz::ImageHandle _color)
{
    // shader
    vkz::ShaderHandle vs = vkz::registShader("skybox_vert_shader", "shaders/skybox.vert.spv");
    vkz::ShaderHandle fs = vkz::registShader("skybox_frag_shader", "shaders/skybox.frag.spv");
    vkz::ProgramHandle prog = vkz::registProgram("mesh_prog", { vs, fs });

    // pass
    vkz::PassHandle pass;
    {
        vkz::VertexBindingDesc bindings[] =
        {
            {0, sizeof(Vertex), vkz::VertexInputRate::vertex},
        };

        vkz::VertexAttributeDesc attributes[] =
        {
            {0, 0, vkz::ResourceFormat::r32g32b32_sfloat, offsetof(Vertex, vx)},
        };

        const vkz::Memory* vtxBindingMem = vkz::alloc(uint32_t(sizeof(vkz::VertexBindingDesc) * COUNTOF(bindings)));
        memcpy(vtxBindingMem->data, bindings, sizeof(vkz::VertexBindingDesc) * COUNTOF(bindings));

        const vkz::Memory* vtxAttributeMem = vkz::alloc(uint32_t(sizeof(vkz::VertexAttributeDesc) * COUNTOF(attributes)));
        memcpy(vtxAttributeMem->data, attributes, sizeof(vkz::VertexAttributeDesc) * COUNTOF(attributes));

        vkz::PassDesc passDesc;
        passDesc.programId = prog.id;
        passDesc.queue = vkz::PassExeQueue::graphics;
        passDesc.vertexBindingNum = (uint32_t)COUNTOF(bindings);
        passDesc.vertexBindings = vtxBindingMem->data;
        passDesc.vertexAttributeNum = (uint32_t)COUNTOF(attributes);
        passDesc.vertexAttributes = vtxAttributeMem->data;

        passDesc.pipelineConfig.enableDepthTest = false;
        passDesc.pipelineConfig.enableDepthWrite = false;
        passDesc.pipelineConfig.depthCompOp = vkz::CompareOp::never;
        passDesc.pipelineConfig.cullMode = vkz::CullModeFlagBits::front;

        passDesc.passConfig.colorLoadOp = vkz::AttachmentLoadOp::clear;
        passDesc.passConfig.colorStoreOp = vkz::AttachmentStoreOp::store;
        passDesc.passConfig.depthLoadOp = vkz::AttachmentLoadOp::dont_care;
        passDesc.passConfig.depthStoreOp = vkz::AttachmentStoreOp::none;
        pass = vkz::registPass("skybox_pass", passDesc);
    }

    // skybox mesh
    Geometry geom = {};
    loadMesh(geom, "../data/cube_skybox.obj", false);

    const vkz::Memory* vtxMem = vkz::alloc(uint32_t(geom.vertices.size() * sizeof(Vertex)));
    memcpy(vtxMem->data, geom.vertices.data(), geom.vertices.size() * sizeof(Vertex));

    const vkz::Memory* idxMem = vkz::alloc(uint32_t(geom.indices.size() * sizeof(uint32_t)));
    memcpy(idxMem->data, geom.indices.data(), geom.indices.size() * sizeof(uint32_t));

    vkz::BufferDesc vtxDesc;
    vtxDesc.size = vtxMem->size;
    vtxDesc.usage = vkz::BufferUsageFlagBits::vertex;
    vkz::BufferHandle vtxBuf = vkz::registBuffer("skybox_vtx", vtxDesc, vtxMem, vkz::ResourceLifetime::non_transition);

    vkz::BufferDesc idxDesc;
    idxDesc.size = idxMem->size;
    idxDesc.usage = vkz::BufferUsageFlagBits::index;
    vkz::BufferHandle idxBuf = vkz::registBuffer("skybox_idx", idxDesc, idxMem, vkz::ResourceLifetime::non_transition);


    // load texture 
    vkz::ImageHandle cubemap = loadTextureFromFile("skybox_tex", "../data/textures/cubemap_vulkan.ktx");
    vkz::ImageHandle outColor = vkz::alias(_color);

    vkz::bindVertexBuffer(pass, vtxBuf);
    vkz::bindIndexBuffer(pass, idxBuf, (uint32_t)geom.indices.size());

    vkz::bindBuffer(pass, _trans
        , 0
        , vkz::PipelineStageFlagBits::task_shader
        , vkz::AccessFlagBits::shader_read
    );

    _skybox.cubemapSampler = vkz::sampleImage(pass, cubemap
        , 1
        , vkz::PipelineStageFlagBits::fragment_shader
        , vkz::ImageLayout::shader_read_only_optimal
        , vkz::SamplerReductionMode::weighted_average
    );

    vkz::setAttachmentOutput(pass, _color, 0, outColor);

    _skybox.pass = pass;
    _skybox.prog = prog;
    _skybox.vs = vs;
    _skybox.fs = fs;

    _skybox.color = _color;
    _skybox.cubemap = cubemap;
    _skybox.trans = _trans;

    _skybox.colorOutAlias = outColor;
}

