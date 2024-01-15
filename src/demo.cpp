#include "vkz.h"
#include "mesh.h"
#include "scene.h"
#include <cstddef>
#include "camera.h"

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

void meshDemo()
{
    vkz::VKZInitConfig config = {};
    config.windowWidth = 2560;
    config.windowHeight = 1440;

    vkz::init(config);

    // load scene
    Scene scene;
    const char* pathes[] = { "../data/kitten.obj" };
    bool lmr = loadScene(scene, pathes, COUNTOF(pathes), false);
    assert(lmr);

    // basic data
    FreeCamera freeCamera = {};
    freeCameraInit(freeCamera, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f);

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

    // mesh draw instance count buffer
    vkz::BufferDesc meshDrawCmdCountBufDesc;
    meshDrawCmdCountBufDesc.size = 16;
    meshDrawCmdCountBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::indirect | vkz::BufferUsageFlagBits::transfer_dst;
    meshDrawCmdCountBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshDrawCmdCountBuf = vkz::registBuffer("meshDrawCmdCount", meshDrawCmdCountBufDesc);
    vkz::BufferHandle meshDrawCmdCountBuf2 = vkz::alias(meshDrawCmdCountBuf);

    // mesh draw instance visibility buffer
    vkz::BufferDesc meshDrawVisBufDesc;
    meshDrawVisBufDesc.size = uint32_t(scene.drawCount * sizeof(uint32_t));
    meshDrawVisBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::indirect | vkz::BufferUsageFlagBits::transfer_dst;
    meshDrawVisBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshDrawVisBuf = vkz::registBuffer("meshDrawVis", meshDrawVisBufDesc);
    vkz::BufferHandle meshDrawVisBuf2 = vkz::alias(meshDrawVisBuf);

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
    rtDesc.mips = 1;
    rtDesc.usage = vkz::ImageUsageFlagBits::transfer_src;
    vkz::ImageHandle color = vkz::registRenderTarget("color", rtDesc);
    vkz::ImageHandle color2 = vkz::alias(color);

    vkz::ImageDesc dpDesc;
    dpDesc.depth = 1;
    dpDesc.arrayLayers = 1;
    dpDesc.mips = 1;
    dpDesc.usage = vkz::ImageUsageFlagBits::transfer_src;
    vkz::ImageHandle depth = vkz::registDepthStencil("depth", dpDesc);
    vkz::ImageHandle depth2 = vkz::alias(depth);

    vkz::ImageDesc pyDesc;
    pyDesc.width = previousPow2_new(config.windowWidth);
    pyDesc.height = previousPow2_new(config.windowHeight);
    pyDesc.format = vkz::ResourceFormat::r32_sfloat;
    pyDesc.depth = 1;
    pyDesc.arrayLayers = 1;
    pyDesc.mips = calculateMipLevelCount_new(config.windowWidth, config.windowHeight);;
    pyDesc.usage = vkz::ImageUsageFlagBits::transfer_src | vkz::ImageUsageFlagBits::sampled | vkz::ImageUsageFlagBits::storage;
    vkz::ImageHandle pyramid = vkz::registTexture("pyramid", pyDesc);

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
        vkz::PassHandle renderPass = vkz::registPass("mesh_pass", passDesc);

        // index buffer
        {
            vkz::bindIndexBuffer(renderPass, idxBuf);
        }

        // bindings
        {
            vkz::bindBuffer(renderPass, meshDrawCmdBuf2
                , 0
                , vkz::PipelineStageFlagBits::vertex_shader
                , vkz::AccessFlagBits::shader_read);
        }

        {
            vkz::bindBuffer(renderPass, meshDrawBuf
                , 1
                , vkz::PipelineStageFlagBits::vertex_shader
                , vkz::AccessFlagBits::shader_read);
        }

        {
            vkz::bindBuffer(renderPass, vtxBuf
                , 2
                , vkz::PipelineStageFlagBits::vertex_shader
                , vkz::AccessFlagBits::shader_read);
        }

        {
            vkz::bindBuffer(renderPass, transformBuf
                , 3
                , vkz::PipelineStageFlagBits::vertex_shader
                , vkz::AccessFlagBits::shader_read);
        }

        vkz::setIndirectBuffer(renderPass, meshDrawCmdBuf2, offsetof(MeshDrawCommandVKZ, indexCount), sizeof(MeshDrawCommandVKZ), (uint32_t)scene.meshDraws.size());

        vkz::setAttachmentOutput(renderPass, color, 0, color2);
        vkz::setAttachmentOutput(renderPass, depth, 0, depth2);
    }

    {
        int pushConstants[] = { false };

        const vkz::Memory* pConst = vkz::alloc(sizeof(int) * COUNTOF(pushConstants));
        memcpy_s(pConst->data, pConst->size, pushConstants, sizeof(int)* COUNTOF(pushConstants));

        // cull pass
        vkz::ShaderHandle cs = vkz::registShader("mesh_draw_cmd_shader", "shaders/drawcmd.comp.spv");
        vkz::ProgramHandle csProgram = vkz::registProgram("mesh_draw_cmd_prog", { cs }, sizeof(MeshDrawCullVKZ));

        vkz::PassDesc passDesc;
        passDesc.programId = csProgram.id;
        passDesc.queue = vkz::PassExeQueue::compute;
        passDesc.pipelineSpecNum = COUNTOF(pushConstants);
        passDesc.pipelineSpecData = (void*)pConst->data;
        
        vkz::PassHandle cull_pass = vkz::registPass("cull_pass", passDesc);

        // bindings
        {
            vkz::bindBuffer(cull_pass, meshBuf
                , 0
                , vkz::PipelineStageFlagBits::compute_shader
                , vkz::AccessFlagBits::shader_read);
        }

        {
            vkz::bindBuffer(cull_pass, meshDrawBuf
                , 1
                , vkz::PipelineStageFlagBits::compute_shader
                , vkz::AccessFlagBits::shader_read);
        }

        {
            vkz::bindBuffer(cull_pass, transformBuf
                , 2
                , vkz::PipelineStageFlagBits::compute_shader
                , vkz::AccessFlagBits::shader_read);
        }

        {
            vkz::bindBuffer(cull_pass, meshDrawCmdBuf
                , 3
                , vkz::PipelineStageFlagBits::compute_shader
                , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
                , meshDrawCmdBuf2);
        }

        {
            vkz::bindBuffer(cull_pass, meshDrawCmdCountBuf
                , 4
                , vkz::PipelineStageFlagBits::compute_shader
                , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
                , meshDrawCmdCountBuf2);
        }

        {
            vkz::bindBuffer(cull_pass, meshDrawVisBuf
                , 5
                , vkz::PipelineStageFlagBits::compute_shader
                , vkz::AccessFlagBits::shader_read | vkz::AccessFlagBits::shader_write
                , meshDrawVisBuf2);
        }

        {
            vkz::sampleImage(cull_pass, pyramid
                , 6
                , vkz::PipelineStageFlagBits::compute_shader
                , vkz::ImageLayout::general
                , vkz::SamplerReductionMode::weighted_average );
        }
    }


    vkz::setPresentImage(color2);

    while (vkz::run())
    {
        ;
    }

    vkz::shutdown();
}

void triangle()
{
    vkz::VKZInitConfig config = {};
    config.windowWidth = 2560;
    config.windowHeight = 1440;

    vkz::init(config);

    vkz::ImageDesc rtDesc;
    rtDesc.depth = 1;
    rtDesc.arrayLayers = 1;
    rtDesc.mips = 1;
    rtDesc.usage = vkz::ImageUsageFlagBits::color_attachment
        | vkz::ImageUsageFlagBits::transfer_src;

    vkz::ImageHandle color = vkz::registRenderTarget("color", rtDesc);
    vkz::ImageHandle color2 = vkz::alias(color);


    vkz::ShaderHandle vs = vkz::registShader("triangle_vert_shader", "shaders/triangle.vert.spv");
    vkz::ShaderHandle fs = vkz::registShader("triangle_frag_shader", "shaders/triangle.frag.spv");

    vkz::ProgramHandle program = vkz::registProgram("triangle_prog", {vs, fs});

    vkz::PassDesc result;
    result.programId = program.id;
    result.queue = vkz::PassExeQueue::graphics;
    result.pipelineConfig.depthCompOp = vkz::CompareOp::greater;
    result.pipelineConfig.enableDepthTest = true;
    result.pipelineConfig.enableDepthWrite = true;

    vkz::BufferDesc dummyBufDesc;
    dummyBufDesc.size = 32;
    dummyBufDesc.usage = vkz::BufferUsageFlagBits::storage;
    vkz::BufferHandle dummyBuf = vkz::registBuffer("dummy", dummyBufDesc);

    vkz::PassHandle pass = vkz::registPass("result", result);

    {
        vkz::bindBuffer(pass, dummyBuf
            , 0
            , vkz::PipelineStageFlagBits::vertex_shader
            , vkz::AccessFlagBits::shader_read);
    }

    {
        setAttachmentOutput(pass, color, 0, color2);
    }

    vkz::setPresentImage(color2);

    while (vkz::run())
    {
        ;
    }

    vkz::shutdown();
}

void DemoMain()
{
    triangle();
    //meshDemo();
}