#include "common.h"
#include "vkz.h"
#include "mesh.h"
#include "scene.h"
#include <cstddef>
#include "camera.h"
#include "debug.h"
#include "vkz_ui.h"
#include "vkz_pyramid.h"

struct alignas(16) TransformData
{
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
};

uint32_t previousPow2_new(uint32_t v)
{
    uint32_t r = 1;

    while (r < v)
        r <<= 1;
    r >>= 1;
    return r;
}

uint32_t calculateMipLevelCount_new(uint32_t width, uint32_t height)
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

struct alignas(16) MeshDrawCullVKZ
{
    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float lodBase, lodStep;
    float pyramidWidth, pyramidHeight;

    int32_t enableCull;
    int32_t enableLod;
    int32_t enableOcclusion;
    int32_t enableMeshletOcclusion;
};

struct alignas(16) GlobalsVKZ
{
    mat4 projection;

    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
    float screenWidth, screenHeight;
    int enableMeshletOcclusion;
};

struct MeshDrawCommandVKZ
{
    uint32_t    drawId;
    uint32_t    taskOffset;
    uint32_t    taskCount;
    uint32_t    lateDrawVisibility;

    // struct VkDrawIndexedIndirectCommand
    uint32_t    indexCount;
    uint32_t    instanceCount;
    uint32_t    firstIndex;
    uint32_t    vertexOffset;
    uint32_t    firstInstance;

    // struct VkDrawMeshTasksIndirectCommandEXT
    uint32_t    local_x;
    uint32_t    local_y;
    uint32_t    local_z;
};


// left handed?
mat4 perspectiveProjection2(float fovY, float aspectWbyH, float zNear)
{
    float f = 1.0f / tanf(fovY / 2.0f);

    return mat4(
        f / aspectWbyH, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, zNear, 0.0f);
}

vec4 normalizePlane2(vec4 p)
{
    return p / glm::length(p);
}

uint32_t calculateMipLevelCount2(uint32_t width, uint32_t height)
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

void meshDemo()
{
    vkz::VKZInitConfig config = {};
    config.windowWidth = 2560;
    config.windowHeight = 1440;

    vkz::init(config);

    // ui data
    Input input = {};
    input.width = (float)config.windowWidth;
    input.height = (float)config.windowHeight;

    RenderOptionsData renderOptions = {};
    ProfilingData profiling = {};
    LogicData logics = {};

    // load scene
    Scene scene;
    const char* pathes[] = { "../data/kitten.obj" };
    bool lmr = loadScene(scene, pathes, COUNTOF(pathes), false);
    assert(lmr);

    // basic data
    FreeCamera freeCamera = {};
    freeCameraInit(freeCamera, vec3(0.0f, 2.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f);

    float znear = .1f;
    mat4 projection = perspectiveProjection2(glm::radians(70.f), (float)config.windowWidth / (float)config.windowHeight, znear);
    mat4 projectionT = glm::transpose(projection);
    vec4 frustumX = normalizePlane2(projectionT[3] - projectionT[0]);
    vec4 frustumY = normalizePlane2(projectionT[3] - projectionT[1]);

    mat4 view = freeCameraGetViewMatrix(freeCamera);
    TransformData trans = {};
    trans.view = view;
    trans.proj = projection;
    trans.cameraPos = freeCamera.pos;

    // buffers
    // mesh data
    vkz::BufferDesc meshBufDesc;
    meshBufDesc.size = (uint32_t)(scene.geometry.meshes.size() * sizeof(Mesh));
    meshBufDesc.data = scene.geometry.meshes.data();
    meshBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    meshBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshBuf = vkz::registBuffer("mesh", meshBufDesc);

    // mesh draw instance buffer
    vkz::BufferDesc meshDrawBufDesc;
    meshDrawBufDesc.size = (uint32_t)(scene.meshDraws.size() * sizeof(MeshDraw));
    meshDrawBufDesc.data = scene.meshDraws.data();
    meshDrawBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    meshDrawBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshDrawBuf = vkz::registBuffer("meshDraw", meshDrawBufDesc);

    // mesh draw instance buffer
    vkz::BufferDesc meshDrawCmdBufDesc;
    meshDrawCmdBufDesc.size = 128 * 1024 * 1024; // 128M
    meshDrawCmdBufDesc.usage = vkz::BufferUsageFlagBits::indirect | vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    meshDrawCmdBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshDrawCmdBuf = vkz::registBuffer("meshDrawCmd", meshDrawCmdBufDesc);
    vkz::BufferHandle meshDrawCmdBuf2 = vkz::alias(meshDrawCmdBuf);
    vkz::BufferHandle meshDrawCmdBuf3 = vkz::alias(meshDrawCmdBuf);

    // mesh draw instance count buffer
    vkz::BufferDesc meshDrawCmdCountBufDesc;
    meshDrawCmdCountBufDesc.size = 16;
    meshDrawCmdCountBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::indirect | vkz::BufferUsageFlagBits::transfer_dst;
    meshDrawCmdCountBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshDrawCmdCountBuf = vkz::registBuffer("meshDrawCmdCount", meshDrawCmdCountBufDesc);
    vkz::BufferHandle meshDrawCmdCountBuf2 = vkz::alias(meshDrawCmdCountBuf);
    vkz::BufferHandle meshDrawCmdCountBuf3 = vkz::alias(meshDrawCmdCountBuf);
    vkz::BufferHandle meshDrawCmdCountBuf4 = vkz::alias(meshDrawCmdCountBuf);
    vkz::BufferHandle meshDrawCmdCountBuf5 = vkz::alias(meshDrawCmdCountBuf);

    // mesh draw instance visibility buffer
    vkz::BufferDesc meshDrawVisBufDesc;
    meshDrawVisBufDesc.size = uint32_t(scene.drawCount * sizeof(uint32_t));
    meshDrawVisBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::indirect | vkz::BufferUsageFlagBits::transfer_dst;
    meshDrawVisBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshDrawVisBuf = vkz::registBuffer("meshDrawVis", meshDrawVisBufDesc);
    vkz::BufferHandle meshDrawVisBuf2 = vkz::alias(meshDrawVisBuf);
    vkz::BufferHandle meshDrawVisBuf3 = vkz::alias(meshDrawVisBuf);

    // index buffer
    vkz::BufferDesc idxBufDesc;
    idxBufDesc.size = (uint32_t)(scene.geometry.indices.size() * sizeof(uint32_t));
    idxBufDesc.data = scene.geometry.indices.data();
    idxBufDesc.usage = vkz::BufferUsageFlagBits::index | vkz::BufferUsageFlagBits::transfer_dst;
    idxBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle idxBuf = vkz::registBuffer("idx", idxBufDesc);

    // vertex buffer
    vkz::BufferDesc vtxBufDesc;
    vtxBufDesc.size = (uint32_t)(scene.geometry.vertices.size() * sizeof(Vertex));
    vtxBufDesc.data = scene.geometry.vertices.data();
    vtxBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    vtxBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle vtxBuf = vkz::registBuffer("vtx", vtxBufDesc);

    // transform
    // TODO: requires to upload every frame
    vkz::BufferDesc transformBufDesc;
    transformBufDesc.size = (uint32_t)(sizeof(TransformData));
    transformBufDesc.data = &trans;
    transformBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    transformBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle transformBuf = vkz::registBuffer("transform", transformBufDesc);
    
    // color
    vkz::ImageDesc rtDesc;
    rtDesc.depth = 1;
    rtDesc.arrayLayers = 1;
    rtDesc.mipLevels = 1;
    rtDesc.usage = vkz::ImageUsageFlagBits::transfer_src;
    vkz::ImageHandle color = vkz::registRenderTarget("color", rtDesc, vkz::ResourceLifetime::non_transition);
    vkz::ImageHandle color2 = vkz::alias(color);
    vkz::ImageHandle color3 = vkz::alias(color);
    vkz::ImageHandle color4 = vkz::alias(color);

    vkz::ImageDesc dpDesc;
    dpDesc.depth = 1;
    dpDesc.arrayLayers = 1;
    dpDesc.mipLevels = 1;
    dpDesc.usage = vkz::ImageUsageFlagBits::transfer_src | vkz::ImageUsageFlagBits::sampled;
    vkz::ImageHandle depth = vkz::registDepthStencil("depth", dpDesc, vkz::ResourceLifetime::non_transition);
    vkz::ImageHandle depth2 = vkz::alias(depth);
    vkz::ImageHandle depth3 = vkz::alias(depth);
    vkz::ImageHandle depth4 = vkz::alias(depth);


    // pyramid passes
    uint32_t pyramidLevelWidth = previousPow2_new(config.windowWidth);
    uint32_t pyramidLevelHeight = previousPow2_new(config.windowHeight);
    uint32_t pyramidLevel = calculateMipLevelCount2(pyramidLevelWidth, pyramidLevelHeight);


    PyramidRendering pyRendering{};
    vkz::ImageHandle pyOut = preparePyramid(pyRendering, config.windowWidth, config.windowHeight, depth2);

    vkz::PassHandle pass_draw_0;
    {
        // render shader
        vkz::ShaderHandle vs = vkz::registShader("mesh_vert_shader", "shaders/mesh.vert.spv");
        vkz::ShaderHandle fs = vkz::registShader("mesh_frag_shader", "shaders/mesh.frag.spv");
        vkz::ProgramHandle program = vkz::registProgram("mesh_prog", { vs, fs }, sizeof(GlobalsVKZ));
        // pass
        vkz::PassDesc passDesc;
        passDesc.programId = program.id;
        passDesc.queue = vkz::PassExeQueue::graphics;
        passDesc.pipelineConfig.depthCompOp = vkz::CompareOp::greater;
        passDesc.pipelineConfig.enableDepthTest = true;
        passDesc.pipelineConfig.enableDepthWrite = true;
        pass_draw_0 = vkz::registPass("mesh_pass", passDesc);

        // index buffer
        vkz::bindIndexBuffer(pass_draw_0, idxBuf);

        // bindings
        vkz::bindBuffer(pass_draw_0, meshDrawCmdBuf2
            , 0
            , vkz::PipelineStageFlagBits::vertex_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_draw_0, meshDrawBuf
            , 1
            , vkz::PipelineStageFlagBits::vertex_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_draw_0, vtxBuf
            , 2
            , vkz::PipelineStageFlagBits::vertex_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_draw_0, transformBuf
            , 3
            , vkz::PipelineStageFlagBits::vertex_shader
            , vkz::AccessFlagBits::shader_read);
        

        vkz::setIndirectBuffer(pass_draw_0, meshDrawCmdBuf2, offsetof(MeshDrawCommandVKZ, indexCount), sizeof(MeshDrawCommandVKZ), (uint32_t)scene.meshDraws.size());
        vkz::setIndirectCountBuffer(pass_draw_0, meshDrawCmdCountBuf3, 0);

        vkz::setAttachmentOutput(pass_draw_0, color, 0, color2);
        vkz::setAttachmentOutput(pass_draw_0, depth, 0, depth2);
    }

    vkz::PassHandle pass_cull_0;
    {
        // cull pass
        vkz::ShaderHandle cs = vkz::registShader("mesh_draw_cmd_shader", "shaders/drawcmd.comp.spv");
        vkz::ProgramHandle csProgram = vkz::registProgram("mesh_draw_cmd_prog", { cs }, sizeof(MeshDrawCullVKZ));

        int pipelineSpecs[] = { false, false };

        const vkz::Memory* pConst = vkz::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
        memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int)* COUNTOF(pipelineSpecs));

        vkz::PassDesc passDesc;
        passDesc.programId = csProgram.id;
        passDesc.queue = vkz::PassExeQueue::compute;
        passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
        passDesc.pipelineSpecData = (void*)pConst->data;
        
        pass_cull_0 = vkz::registPass("cull_pass", passDesc);

        // bindings
        vkz::bindBuffer(pass_cull_0, meshBuf
            , 0
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_cull_0, meshDrawBuf
            , 1
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_cull_0, transformBuf
            , 2
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_cull_0, meshDrawCmdBuf
            , 3
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
            , meshDrawCmdBuf2);

        vkz::bindBuffer(pass_cull_0, meshDrawCmdCountBuf2
            , 4
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
            , meshDrawCmdCountBuf3);

        vkz::bindBuffer(pass_cull_0, meshDrawVisBuf
            , 5
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
            , meshDrawVisBuf2);

        vkz::sampleImage(pass_cull_0, pyRendering.image
            , 6
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::ImageLayout::general
            , vkz::SamplerReductionMode::weighted_average );
    }


    vkz::PassHandle pass_cull_1;
    {
        // cull pass
        vkz::ShaderHandle cs = vkz::registShader("mesh_draw_cmd_shader", "shaders/drawcmd.comp.spv");
        vkz::ProgramHandle csProgram = vkz::registProgram("mesh_draw_cmd_prog", { cs }, sizeof(MeshDrawCullVKZ));

        int pipelineSpecs[] = { true, false };

        const vkz::Memory* pConst = vkz::alloc(sizeof(int) * COUNTOF(pipelineSpecs));
        memcpy_s(pConst->data, pConst->size, pipelineSpecs, sizeof(int) * COUNTOF(pipelineSpecs));

        vkz::PassDesc passDesc;
        passDesc.programId = csProgram.id;
        passDesc.queue = vkz::PassExeQueue::compute;
        passDesc.pipelineSpecNum = COUNTOF(pipelineSpecs);
        passDesc.pipelineSpecData = (void*)pConst->data;

        pass_cull_1 = vkz::registPass("cull_pass", passDesc);

        // bindings
        vkz::bindBuffer(pass_cull_1, meshBuf
            , 0
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_cull_1, meshDrawBuf
            , 1
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_cull_1, transformBuf
            , 2
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_cull_1, meshDrawCmdBuf2
            , 3
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
            , meshDrawCmdBuf3);

        vkz::bindBuffer(pass_cull_1, meshDrawCmdCountBuf4
            , 4
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
            , meshDrawCmdCountBuf5);

        vkz::bindBuffer(pass_cull_1, meshDrawVisBuf2
            , 5
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
            , meshDrawVisBuf3);

        vkz::sampleImage(pass_cull_1, pyOut
            , 6
            , vkz::PipelineStageFlagBits::compute_shader
            , vkz::ImageLayout::general
            , vkz::SamplerReductionMode::weighted_average);
    }

    vkz::PassHandle pass_draw_1;
    {
        // render shader
        vkz::ShaderHandle vs = vkz::registShader("mesh_vert_shader", "shaders/mesh.vert.spv");
        vkz::ShaderHandle fs = vkz::registShader("mesh_frag_shader", "shaders/mesh.frag.spv");
        vkz::ProgramHandle program = vkz::registProgram("mesh_prog", { vs, fs }, sizeof(GlobalsVKZ));
        // pass
        vkz::PassDesc passDesc;
        passDesc.programId = program.id;
        passDesc.queue = vkz::PassExeQueue::graphics;
        passDesc.pipelineConfig.depthCompOp = vkz::CompareOp::greater;
        passDesc.pipelineConfig.enableDepthTest = true;
        passDesc.pipelineConfig.enableDepthWrite = true;
        
        passDesc.passConfig.colorLoadOp = vkz::AttachmentLoadOp::dont_care;
        passDesc.passConfig.colorStoreOp = vkz::AttachmentStoreOp::store;
        passDesc.passConfig.depthLoadOp = vkz::AttachmentLoadOp::dont_care;
        passDesc.passConfig.depthStoreOp = vkz::AttachmentStoreOp::store;


        pass_draw_1 = vkz::registPass("mesh_pass", passDesc);

        // index buffer
        vkz::bindIndexBuffer(pass_draw_1, idxBuf);

        // bindings
        vkz::bindBuffer(pass_draw_1, meshDrawCmdBuf3
            , 0
            , vkz::PipelineStageFlagBits::vertex_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_draw_1, meshDrawBuf
            , 1
            , vkz::PipelineStageFlagBits::vertex_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_draw_1, vtxBuf
            , 2
            , vkz::PipelineStageFlagBits::vertex_shader
            , vkz::AccessFlagBits::shader_read);

        vkz::bindBuffer(pass_draw_1, transformBuf
            , 3
            , vkz::PipelineStageFlagBits::vertex_shader
            , vkz::AccessFlagBits::shader_read);


        vkz::setIndirectBuffer(pass_draw_1, meshDrawCmdBuf3, offsetof(MeshDrawCommandVKZ, indexCount), sizeof(MeshDrawCommandVKZ), (uint32_t)scene.meshDraws.size());
        vkz::setIndirectCountBuffer(pass_draw_1, meshDrawCmdCountBuf5, 0);

        vkz::setAttachmentOutput(pass_draw_1, color2, 0, color3);
        vkz::setAttachmentOutput(pass_draw_1, depth2, 0, depth3);
    }

    vkz::PassHandle pass_fill_dccb;
    {
        vkz::PassDesc passDesc;
        passDesc.queue = vkz::PassExeQueue::fill_buffer;

        pass_fill_dccb = vkz::registPass("fill_dccb", passDesc);

        vkz::fillBuffer(pass_fill_dccb, meshDrawCmdCountBuf, 0, sizeof(uint32_t), 0, meshDrawCmdCountBuf2);
    } 

    vkz::PassHandle pass_fill_dccb_late;
    {
        vkz::PassDesc passDesc;
        passDesc.queue = vkz::PassExeQueue::fill_buffer;

        pass_fill_dccb_late = vkz::registPass("fill_dccb", passDesc);

        vkz::fillBuffer(pass_fill_dccb_late, meshDrawCmdCountBuf3, 0, sizeof(uint32_t), 0, meshDrawCmdCountBuf4);
    }

    UIRendering ui{};
    {
        vkz_prepareUI(ui);
        vkz::setAttachmentOutput(ui.pass, color3, 0, color4);
        vkz::setAttachmentOutput(ui.pass, depth3, 0, depth4);
    }

    vkz::PassHandle pass_py = pyRendering.pass;
    vkz::PassHandle pass_ui = ui.pass;

    vkz::setPresentImage(color4);

    vkz::bake();

    // data used in loop
    const vkz::Memory* memDrawCull = vkz::alloc(sizeof(MeshDrawCullVKZ));
    const vkz::Memory* memGlobal = vkz::alloc(sizeof(GlobalsVKZ));
    const vkz::Memory* memTransform = vkz::alloc(sizeof(TransformData));
    const vkz::Memory* memPyramidWndSz = vkz::alloc(sizeof(vec2));

    MeshDrawCullVKZ drawCull = {};
    GlobalsVKZ globals = {};

    while (!vkz::shouldClose())
    {
        znear = .1f;
        projection = perspectiveProjection2(glm::radians(70.f), (float)config.windowWidth / (float)config.windowHeight, znear);
        projectionT = glm::transpose(projection);
        frustumX = normalizePlane2(projectionT[3] - projectionT[0]);
        frustumY = normalizePlane2(projectionT[3] - projectionT[1]);

        drawCull.P00 = projection[0][0];
        drawCull.P11 = projection[1][1];
        drawCull.zfar = scene.drawDistance;
        drawCull.znear = znear;
        drawCull.pyramidWidth = (float)pyramidLevelWidth;
        drawCull.pyramidHeight = (float)pyramidLevelHeight;
        drawCull.frustum[0] = frustumX.x;
        drawCull.frustum[1] = frustumX.z;
        drawCull.frustum[2] = frustumY.y;
        drawCull.frustum[3] = frustumY.z;
        drawCull.enableCull = 1;
        drawCull.enableLod = 0;
        drawCull.enableOcclusion = 0;
        drawCull.enableMeshletOcclusion = 0;
        memcpy_s(memDrawCull->data, memDrawCull->size, &drawCull, sizeof(MeshDrawCullVKZ));
        vkz::updatePushConstants(pass_cull_0, vkz::copy(memDrawCull));
        vkz::updatePushConstants(pass_cull_1, vkz::copy(memDrawCull));

        globals.projection = projection;
        globals.zfar = scene.drawDistance;
        globals.znear = znear;
        globals.frustum[0] = frustumX.x;
        globals.frustum[1] = frustumX.z;
        globals.frustum[2] = frustumY.y;
        globals.frustum[3] = frustumY.z;
        globals.pyramidWidth = (float)pyramidLevelWidth;
        globals.pyramidHeight = (float)pyramidLevelHeight;
        globals.screenWidth = (float)config.windowWidth;
        globals.screenHeight = (float)config.windowHeight;
        globals.enableMeshletOcclusion = 0;  
        memcpy_s(memGlobal->data, memGlobal->size, &globals, sizeof(GlobalsVKZ));
        vkz::updatePushConstants(pass_draw_0, vkz::copy(memGlobal));
        vkz::updatePushConstants(pass_draw_1, vkz::copy(memGlobal));
        
        vkz::updateThreadCount(pass_cull_0, (uint32_t)scene.meshDraws.size(), 1, 1);
        vkz::updateThreadCount(pass_cull_1, (uint32_t)scene.meshDraws.size(), 1, 1);

        freeCamera.pos = vec3{ 0.f, 2.f, 20.f };

        trans.view = freeCameraGetViewMatrix(freeCamera);
        trans.proj = projection;
        trans.cameraPos = freeCamera.pos;
        memcpy_s(memTransform->data, memTransform->size, &trans, sizeof(TransformData));
        vkz::updateBuffer(transformBuf, vkz::copy(memTransform));

        // ui
        vkz_updateImGui(input, renderOptions, profiling, logics);
        vkz_updateUIRenderData(ui);

        vkz::run();

        VKZ_FrameMark;
    }

    vkz::shutdown();
}

void DemoMain()
{
    meshDemo();
}