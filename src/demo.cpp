#include "vkz.h"
#include "mesh.h"
#include "scene.h"
#include <cstddef>
#include "camera.h"
#include "debug.h"
#include "vkz_ui.h"
#include "vkz_pyramid.h"
#include "demo_structs.h"
#include "vkz_ms_pip.h"
#include "vkz_vtx_pip.h"
#include "vkz_culling_pass.h"


void meshDemo()
{
    vkz::VKZInitConfig config = {};
    config.windowWidth = 2560;
    config.windowHeight = 1440;

    vkz::init(config);

    bool supportMeshShading = vkz::checkSupports(vkz::VulkanSupportExtension::ext_mesh_shader);

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
    bool lmr = loadScene(scene, pathes, COUNTOF(pathes), supportMeshShading);
    assert(lmr);

    // basic data
    FreeCamera freeCamera = {};
    freeCameraInit(freeCamera, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f);

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

    // mesh draw instance count buffer
    vkz::BufferDesc meshDrawCmdCountBufDesc;
    meshDrawCmdCountBufDesc.size = 16;
    meshDrawCmdCountBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::indirect | vkz::BufferUsageFlagBits::transfer_dst;
    meshDrawCmdCountBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshDrawCmdCountBuf = vkz::registBuffer("meshDrawCmdCount", meshDrawCmdCountBufDesc);
    vkz::BufferHandle meshDrawCmdCountBufAfterFill = vkz::alias(meshDrawCmdCountBuf);
    vkz::BufferHandle meshDrawCmdCountBufAfterFillLate = vkz::alias(meshDrawCmdCountBuf);

    // mesh draw instance visibility buffer
    vkz::BufferDesc meshDrawVisBufDesc;
    meshDrawVisBufDesc.size = uint32_t(scene.drawCount * sizeof(uint32_t));
    meshDrawVisBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::indirect | vkz::BufferUsageFlagBits::transfer_dst;
    meshDrawVisBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshDrawVisBuf = vkz::registBuffer("meshDrawVis", meshDrawVisBufDesc);

    // mesh draw instance visibility buffer
    vkz::BufferDesc meshletVisBufDesc;
    meshletVisBufDesc.size = (scene.meshletVisibilityCount + 31) / 32 * sizeof(uint32_t);
    meshletVisBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::indirect | vkz::BufferUsageFlagBits::transfer_dst;
    meshletVisBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshletVisBuf = vkz::registBuffer("meshletVis", meshDrawVisBufDesc);

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

    // meshlet buffer
    vkz::BufferDesc meshletBufferDesc;
    meshletBufferDesc.size = (uint32_t)(scene.geometry.meshlets.size() * sizeof(Meshlet));
    meshletBufferDesc.data = scene.geometry.meshlets.data();
    meshletBufferDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    meshletBufferDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshletBuffer = vkz::registBuffer("meshlet_buffer", meshletBufferDesc);
    
    // meshlet data buffer
    vkz::BufferDesc meshletDataBufferDesc;
    meshletDataBufferDesc.size = (uint32_t)(scene.geometry.meshletdata.size() * sizeof(uint32_t));
    meshletDataBufferDesc.data = scene.geometry.meshletdata.data();
    meshletDataBufferDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    meshletDataBufferDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshletDataBuffer = vkz::registBuffer("meshlet_data_buffer", meshletDataBufferDesc);

    // transform
    TransformData trans{};

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

    vkz::ImageDesc dpDesc;
    dpDesc.depth = 1;
    dpDesc.arrayLayers = 1;
    dpDesc.mipLevels = 1;
    dpDesc.usage = vkz::ImageUsageFlagBits::transfer_src | vkz::ImageUsageFlagBits::sampled;
    vkz::ImageHandle depth = vkz::registDepthStencil("depth", dpDesc, vkz::ResourceLifetime::non_transition);

    // pyramid passes
    uint32_t pyramidLevelWidth = previousPow2_new(config.windowWidth);
    uint32_t pyramidLevelHeight = previousPow2_new(config.windowHeight);
    uint32_t pyramidLevel = calculateMipLevelCount2(pyramidLevelWidth, pyramidLevelHeight);


    PyramidRendering pyRendering{};
    preparePyramid(pyRendering, config.windowWidth, config.windowHeight);

    TaskSubmit taskSubmit{};
    TaskSubmit taskSubmitLate{};
    
    MeshShading meshShading{};
    MeshShading meshShadingLate{};

    VtxShading vtxShading{};
    VtxShading vtxShadingLate{};

    CullingComp culling;
    CullingComp cullingLate;
    {

        CullingCompInitData cullingInit{};
        cullingInit.meshBuf = meshBuf;
        cullingInit.meshDrawBuf = meshDrawBuf;
        cullingInit.transBuf = transformBuf;
        cullingInit.pyramid = pyRendering.image;
        cullingInit.meshDrawCmdBuf = meshDrawCmdBuf;
        cullingInit.meshDrawCmdCountBuf = meshDrawCmdCountBufAfterFill;
        cullingInit.meshDrawVisBuf = meshDrawVisBuf;

        prepareCullingComp(culling, cullingInit);
    }

    if(!supportMeshShading)
    {
        VtxShadingInitData vsInit{};

        vsInit.idxBuf = idxBuf;
        vsInit.vtxBuf = vtxBuf;
        vsInit.meshDrawBuf = meshDrawBuf;
        vsInit.meshDrawCmdBuf = culling.meshDrawCmdBufOutAlias;
        vsInit.transformBuf = transformBuf;
        vsInit.meshDrawCmdCountBuf = culling.meshDrawCmdCountBufOutAlias;
        vsInit.color = color;
        vsInit.depth = depth;

        prepareVtxShading(vtxShading, scene, vsInit);
    }
    else
    {
        prepareTaskSubmit(taskSubmit, culling.meshDrawCmdBufOutAlias, culling.meshDrawCmdCountBufOutAlias);

        MeshShadingInitData msInit{};
        msInit.vtxBuffer = vtxBuf;
        msInit.meshBuffer = meshBuf;
        msInit.meshletBuffer = meshletBuffer;
        msInit.meshletDataBuffer = meshletDataBuffer;
        msInit.meshDrawBuffer = meshDrawBuf;
        msInit.meshDrawCmdBuffer = taskSubmit.drawCmdBufferOutAlias;
        msInit.meshDrawCmdCountBuffer = culling.meshDrawCmdCountBufOutAlias;
        msInit.meshletVisBuffer = meshletVisBuf;
        msInit.transformBuffer = transformBuf;

        msInit.pyramid = pyRendering.image;

        msInit.color = color;
        msInit.depth = depth;
        prepareMeshShading(meshShading, scene, config.windowWidth, config.windowHeight, msInit);
    }

    setPyramidPassDependency(pyRendering, supportMeshShading ? meshShading.depthOutAlias : vtxShading.depthOutAlias);

    // culling late
    {
        CullingCompInitData cullingInit{};
        cullingInit.meshBuf = meshBuf;
        cullingInit.meshDrawBuf = meshDrawBuf;
        cullingInit.transBuf = transformBuf;
        cullingInit.pyramid = pyRendering.imgOutAlias;
        cullingInit.meshDrawCmdBuf = supportMeshShading ?  taskSubmit.drawCmdBufferOutAlias : culling.meshDrawCmdBufOutAlias;
        cullingInit.meshDrawCmdCountBuf = meshDrawCmdCountBufAfterFillLate;
        cullingInit.meshDrawVisBuf = culling.meshDrawVisBufOutAlias;

        prepareCullingComp(cullingLate, cullingInit, true);
    }

    // draw late
    if (!supportMeshShading) 
    {
        VtxShadingInitData vsInit{};

        vsInit.idxBuf = idxBuf;
        vsInit.vtxBuf = vtxBuf;
        vsInit.meshDrawBuf = meshDrawBuf;
        vsInit.meshDrawCmdBuf = cullingLate.meshDrawCmdBufOutAlias;
        vsInit.transformBuf = transformBuf;
        vsInit.meshDrawCmdCountBuf = cullingLate.meshDrawCmdCountBufOutAlias;
        vsInit.color = vtxShading.colorOutAlias;
        vsInit.depth = vtxShading.depthOutAlias;

        prepareVtxShading(vtxShadingLate, scene, vsInit, true);
    }
    else
    {
        prepareTaskSubmit(taskSubmitLate, cullingLate.meshDrawCmdBufOutAlias, cullingLate.meshDrawCmdCountBufOutAlias, true);

        MeshShadingInitData msInit{};
        msInit.vtxBuffer = vtxBuf;
        msInit.meshBuffer = meshBuf;
        msInit.meshletBuffer = meshletBuffer;
        msInit.meshletDataBuffer = meshletDataBuffer;
        msInit.meshDrawBuffer = meshDrawBuf;
        msInit.meshDrawCmdBuffer = taskSubmitLate.drawCmdBufferOutAlias;
        msInit.meshDrawCmdCountBuffer = cullingLate.meshDrawCmdCountBufOutAlias;
        msInit.meshletVisBuffer = meshShading.meshletVisBufferOutAlias;
        msInit.transformBuffer = transformBuf;

        msInit.pyramid = pyRendering.imgOutAlias;

        msInit.color = meshShading.colorOutAlias;
        msInit.depth = meshShading.depthOutAlias;
        prepareMeshShading(meshShadingLate, scene, config.windowWidth, config.windowHeight, msInit, true);
    }

    vkz::PassHandle pass_fill_dccb;
    {
        vkz::PassDesc passDesc;
        passDesc.queue = vkz::PassExeQueue::fill_buffer;

        pass_fill_dccb = vkz::registPass("fill_dccb", passDesc);

        vkz::fillBuffer(pass_fill_dccb, meshDrawCmdCountBuf, 0, sizeof(uint32_t), 0, meshDrawCmdCountBufAfterFill);
    } 

    vkz::PassHandle pass_fill_dccb_late;
    {
        vkz::PassDesc passDesc;
        passDesc.queue = vkz::PassExeQueue::fill_buffer;

        pass_fill_dccb_late = vkz::registPass("fill_dccb_late", passDesc);

        vkz::BufferHandle dccb = culling.meshDrawCmdCountBufOutAlias;
        vkz::fillBuffer(pass_fill_dccb_late, dccb, 0, sizeof(uint32_t), 0, meshDrawCmdCountBufAfterFillLate);
    }

    UIRendering ui{};
    {
        vkz::ImageHandle uiColorIn = supportMeshShading ? meshShadingLate.colorOutAlias : vtxShadingLate.colorOutAlias;
        vkz::ImageHandle uiDepthIn = supportMeshShading ? meshShadingLate.depthOutAlias : vtxShadingLate.depthOutAlias;

        vkz_prepareUI(ui, uiColorIn, uiDepthIn);
    }

    vkz::PassHandle pass_py = pyRendering.pass;
    vkz::PassHandle pass_ui = ui.pass;

    vkz::setPresentImage(ui.colorOutAlias);

    vkz::bake();

    // data used in loop
    const vkz::Memory* memTransform = vkz::alloc(sizeof(TransformData));

    MeshDrawCullVKZ drawCull = {};
    GlobalsVKZ globals = {};

    while (!vkz::shouldClose())
    {
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

        drawCull.P00 = projection[0][0];
        drawCull.P11 = projection[1][1];
        drawCull.zfar = 10000.f; //scene.drawDistance;
        drawCull.znear = znear;
        drawCull.pyramidWidth = (float)pyramidLevelWidth;
        drawCull.pyramidHeight = (float)pyramidLevelHeight;
        drawCull.frustum[0] = frustumX.x;
        drawCull.frustum[1] = frustumX.z;
        drawCull.frustum[2] = frustumY.y;
        drawCull.frustum[3] = frustumY.z;
        drawCull.enableCull = 1;
        drawCull.enableLod = 1;
        drawCull.enableOcclusion = 1;
        drawCull.enableMeshletOcclusion = 1;

        updateCullingConstants(culling, drawCull);
        updateCullingConstants(cullingLate, drawCull);

        globals.projection = projection;
        globals.zfar = 10000.f;//scene.drawDistance;
        globals.znear = znear;
        globals.frustum[0] = frustumX.x;
        globals.frustum[1] = frustumX.z;
        globals.frustum[2] = frustumY.y;
        globals.frustum[3] = frustumY.z;
        globals.pyramidWidth = (float)pyramidLevelWidth;
        globals.pyramidHeight = (float)pyramidLevelHeight;
        globals.screenWidth = (float)config.windowWidth;
        globals.screenHeight = (float)config.windowHeight;
        globals.enableMeshletOcclusion = 1;  

        if (supportMeshShading)
        {
            updateMeshShadingConstants(meshShading, globals);
            updateMeshShadingConstants(meshShadingLate, globals);
        }
        else
        {
            updateVtxShadingConstants(vtxShading, globals);
            updateVtxShadingConstants(vtxShadingLate, globals);
        }

        vkz::updateThreadCount(culling.pass, (uint32_t)scene.meshDraws.size(), 1, 1);
        vkz::updateThreadCount(cullingLate.pass, (uint32_t)scene.meshDraws.size(), 1, 1);

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