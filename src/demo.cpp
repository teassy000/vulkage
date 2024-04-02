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
#include "time.h"
#include "vkz_skybox_pass.h"

static DemoData demo{};

void mouseMoveCallback(double _xpos, double _ypos)
{
    demo.input.mousePosx = (float)_xpos;
    demo.input.mousePosy = (float)_ypos;

    freeCameraProcessMouseMovement(demo.camera, (float)_xpos, (float)_ypos);
}

void keyCallback(kage::KeyEnum _key, kage::KeyState _state, kage::KeyModFlags _flags)
{
    if (kage::KeyState::press == _state 
        || kage::KeyState::repeat == _state)
    {
        freeCameraProcessKeyboard(demo.camera, _key, 0.1f);
    }
}

void meshDemo()
{
    kage::VKZInitConfig config = {};
    config.windowWidth = 2560;
    config.windowHeight = 1440;

    kage::init(config);

    bool supportMeshShading = kage::checkSupports(kage::VulkanSupportExtension::ext_mesh_shader);

    // ui data
    demo.input.width = (float)config.windowWidth;
    demo.input.height = (float)config.windowHeight;

    // load scene
    Scene scene;
    const char* pathes[] = { "../data/kitten.obj" };
    bool lmr = loadScene(scene, pathes, COUNTOF(pathes), supportMeshShading);
    assert(lmr);

    // basic data
    freeCameraInit(demo.camera, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f);

    // set input callback
    kage::setKeyCallback(keyCallback);
    kage::setMouseMoveCallback(mouseMoveCallback);

    // buffers
    // mesh data
    const kage::Memory* memMeshBuf = kage::alloc((uint32_t)(sizeof(Mesh) * scene.geometry.meshes.size()));
    memcpy(memMeshBuf->data, scene.geometry.meshes.data(), memMeshBuf->size);

    kage::BufferDesc meshBufDesc;
    meshBufDesc.size = memMeshBuf->size;
    meshBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    meshBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshBuf = kage::registBuffer("mesh", meshBufDesc, memMeshBuf);

    // mesh draw instance buffer

    const kage::Memory* memMeshDrawBuf = kage::alloc((uint32_t)(scene.meshDraws.size() * sizeof(MeshDraw)));
    memcpy(memMeshDrawBuf->data, scene.meshDraws.data(), memMeshDrawBuf->size);

    kage::BufferDesc meshDrawBufDesc;
    meshDrawBufDesc.size = memMeshDrawBuf->size;
    meshDrawBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    meshDrawBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshDrawBuf = kage::registBuffer("meshDraw", meshDrawBufDesc, memMeshDrawBuf);

    // mesh draw instance buffer
    kage::BufferDesc meshDrawCmdBufDesc;
    meshDrawCmdBufDesc.size = 128 * 1024 * 1024; // 128M
    meshDrawCmdBufDesc.usage = kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    meshDrawCmdBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshDrawCmdBuf = kage::registBuffer("meshDrawCmd", meshDrawCmdBufDesc);

    // mesh draw instance count buffer
    kage::BufferDesc meshDrawCmdCountBufDesc;
    meshDrawCmdCountBufDesc.size = 16;
    meshDrawCmdCountBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
    meshDrawCmdCountBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshDrawCmdCountBuf = kage::registBuffer("meshDrawCmdCount", meshDrawCmdCountBufDesc);
    kage::BufferHandle meshDrawCmdCountBufAfterFill = kage::alias(meshDrawCmdCountBuf);
    kage::BufferHandle meshDrawCmdCountBufAfterFillLate = kage::alias(meshDrawCmdCountBuf);

    // mesh draw instance visibility buffer
    kage::BufferDesc meshDrawVisBufDesc;
    meshDrawVisBufDesc.size = uint32_t(scene.drawCount * sizeof(uint32_t));
    meshDrawVisBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
    meshDrawVisBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshDrawVisBuf = kage::registBuffer("meshDrawVis", meshDrawVisBufDesc);

    // mesh draw instance visibility buffer
    kage::BufferDesc meshletVisBufDesc;
    meshletVisBufDesc.size = (scene.meshletVisibilityCount + 31) / 32 * sizeof(uint32_t);
    meshletVisBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
    meshletVisBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshletVisBuf = kage::registBuffer("meshletVis", meshletVisBufDesc);

    // index buffer
    const kage::Memory* memIdxBuf = kage::alloc((uint32_t)(scene.geometry.indices.size() * sizeof(uint32_t)));
    memcpy(memIdxBuf->data, scene.geometry.indices.data(), memIdxBuf->size);

    kage::BufferDesc idxBufDesc;
    idxBufDesc.size = memIdxBuf->size;
    idxBufDesc.usage = kage::BufferUsageFlagBits::index | kage::BufferUsageFlagBits::transfer_dst;
    idxBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle idxBuf = kage::registBuffer("idx", idxBufDesc, memIdxBuf);

    // vertex buffer
    const kage::Memory* memVtxBuf = kage::alloc((uint32_t)(scene.geometry.vertices.size() * sizeof(Vertex)));
    memcpy(memVtxBuf->data, scene.geometry.vertices.data(), memVtxBuf->size);

    kage::BufferDesc vtxBufDesc;
    vtxBufDesc.size = memVtxBuf->size;
    vtxBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    vtxBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle vtxBuf = kage::registBuffer("vtx", vtxBufDesc, memVtxBuf);

    // meshlet buffer
    const kage::Memory* memMeshletBuf = kage::alloc((uint32_t)(scene.geometry.meshlets.size() * sizeof(Meshlet)));
    memcpy(memMeshletBuf->data, scene.geometry.meshlets.data(), memMeshletBuf->size);

    kage::BufferDesc meshletBufferDesc;
    meshletBufferDesc.size = memMeshletBuf->size;
    meshletBufferDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    meshletBufferDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshletBuffer = kage::registBuffer("meshlet_buffer", meshletBufferDesc, memMeshletBuf);
    
    // meshlet data buffer
    const kage::Memory* memMeshletDataBuf = kage::alloc((uint32_t)(scene.geometry.meshletdata.size() * sizeof(uint32_t)));
    memcpy(memMeshletDataBuf->data, scene.geometry.meshletdata.data(), memMeshletDataBuf->size);

    kage::BufferDesc meshletDataBufferDesc;
    meshletDataBufferDesc.size = memMeshletDataBuf->size;
    meshletDataBufferDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    meshletDataBufferDesc.memFlags = kage::MemoryPropFlagBits::device_local;
    kage::BufferHandle meshletDataBuffer = kage::registBuffer("meshlet_data_buffer", meshletDataBufferDesc, memMeshletDataBuf);

    // transform
    kage::BufferDesc transformBufDesc;
    transformBufDesc.size = (uint32_t)(sizeof(TransformData));
    transformBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
    transformBufDesc.memFlags = kage::MemoryPropFlagBits::device_local | kage::MemoryPropFlagBits::host_visible;
    kage::BufferHandle transformBuf = kage::registBuffer("transform", transformBufDesc);
    
    // color
    kage::ImageDesc rtDesc;
    rtDesc.depth = 1;
    rtDesc.numLayers = 1;
    rtDesc.numMips = 1;
    rtDesc.usage = kage::ImageUsageFlagBits::transfer_src;
    kage::ImageHandle color = kage::registRenderTarget("color", rtDesc, kage::ResourceLifetime::non_transition);

    kage::ImageDesc dpDesc;
    dpDesc.depth = 1;
    dpDesc.numLayers = 1;
    dpDesc.numMips = 1;
    dpDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled;
    kage::ImageHandle depth = kage::registDepthStencil("depth", dpDesc, kage::ResourceLifetime::non_transition);

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

        prepareCullingComp(culling, cullingInit, false, supportMeshShading);
    }


    SkyboxRendering skybox{};
    {
        kage::ImageHandle sbColorIn = color;
        initSkyboxPass(skybox, transformBuf, sbColorIn);
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
        vsInit.color = skybox.colorOutAlias;
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

        msInit.color = skybox.colorOutAlias;
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

        prepareCullingComp(cullingLate, cullingInit, true, supportMeshShading);
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

    kage::PassHandle pass_fill_dccb;
    {
        kage::PassDesc passDesc;
        passDesc.queue = kage::PassExeQueue::fill_buffer;

        pass_fill_dccb = kage::registPass("fill_dccb", passDesc);

        kage::fillBuffer(pass_fill_dccb, meshDrawCmdCountBuf, 0, sizeof(uint32_t), 0, meshDrawCmdCountBufAfterFill);
    } 

    kage::PassHandle pass_fill_dccb_late;
    {
        kage::PassDesc passDesc;
        passDesc.queue = kage::PassExeQueue::fill_buffer;

        pass_fill_dccb_late = kage::registPass("fill_dccb_late", passDesc);

        kage::BufferHandle dccb = culling.meshDrawCmdCountBufOutAlias;
        kage::fillBuffer(pass_fill_dccb_late, dccb, 0, sizeof(uint32_t), 0, meshDrawCmdCountBufAfterFillLate);
    }

    UIRendering ui{};
    {
        kage::ImageHandle uiColorIn = supportMeshShading ? meshShadingLate.colorOutAlias : vtxShadingLate.colorOutAlias;
        kage::ImageHandle uiDepthIn = supportMeshShading ? meshShadingLate.depthOutAlias : vtxShadingLate.depthOutAlias;

        vkz_prepareUI(ui, uiColorIn, uiDepthIn, 1.3f);
    }

    kage::PassHandle pass_py = pyRendering.pass;
    kage::PassHandle pass_ui = ui.pass;

    kage::setPresentImage(ui.colorOutAlias);

    kage::bake();

    // data used in loop
    const kage::Memory* memTransform = kage::alloc(sizeof(TransformData));

    double clockDuration = 0.0;
    double avgCpuTime = 0.0;

    while (!kage::shouldClose())
    {
        clock_t startFrameTime = clock();

        float znear = .1f;
        mat4 projection = perspectiveProjection2(glm::radians(70.f), (float)config.windowWidth / (float)config.windowHeight, znear);
        mat4 projectionT = glm::transpose(projection);
        vec4 frustumX = normalizePlane2(projectionT[3] - projectionT[0]);
        vec4 frustumY = normalizePlane2(projectionT[3] - projectionT[1]);

        mat4 view = freeCameraGetViewMatrix(demo.camera);

        demo.trans.view = view;
        demo.trans.proj = projection;
        demo.trans.cameraPos = demo.camera.pos;

        demo.drawCull.P00 = projection[0][0];
        demo.drawCull.P11 = projection[1][1];
        demo.drawCull.zfar = 10000.f; //scene.drawDistance;
        demo.drawCull.znear = znear;
        demo.drawCull.pyramidWidth = (float)pyramidLevelWidth;
        demo.drawCull.pyramidHeight = (float)pyramidLevelHeight;
        demo.drawCull.frustum[0] = frustumX.x;
        demo.drawCull.frustum[1] = frustumX.z;
        demo.drawCull.frustum[2] = frustumY.y;
        demo.drawCull.frustum[3] = frustumY.z;
        demo.drawCull.enableCull = 1;
        demo.drawCull.enableLod = 1;
        demo.drawCull.enableOcclusion = 1;
        demo.drawCull.enableMeshletOcclusion = 1;

        updateCullingConstants(culling, demo.drawCull);
        updateCullingConstants(cullingLate, demo.drawCull);

        demo.globals.projection = projection;
        demo.globals.zfar = 10000.f;//scene.drawDistance;
        demo.globals.znear = znear;
        demo.globals.frustum[0] = frustumX.x;
        demo.globals.frustum[1] = frustumX.z;
        demo.globals.frustum[2] = frustumY.y;
        demo.globals.frustum[3] = frustumY.z;
        demo.globals.pyramidWidth = (float)pyramidLevelWidth;
        demo.globals.pyramidHeight = (float)pyramidLevelHeight;
        demo.globals.screenWidth = (float)config.windowWidth;
        demo.globals.screenHeight = (float)config.windowHeight;
        demo.globals.enableMeshletOcclusion = 1;

        if (supportMeshShading)
        {
            updateMeshShadingConstants(meshShading, demo.globals);
            updateMeshShadingConstants(meshShadingLate, demo.globals);
        }
        else
        {
            updateVtxShadingConstants(vtxShading, demo.globals);
            updateVtxShadingConstants(vtxShadingLate, demo.globals);
        }

        kage::updateThreadCount(culling.pass, (uint32_t)scene.meshDraws.size(), 1, 1);
        kage::updateThreadCount(cullingLate.pass, (uint32_t)scene.meshDraws.size(), 1, 1);

        memcpy_s(memTransform->data, memTransform->size, &demo.trans, sizeof(TransformData));
        kage::updateBuffer(transformBuf, kage::copy(memTransform));

        // update profiling data to GPU from last frame
        if ( clockDuration > 300.0)
        {
            vkz_updateImGui(demo.input, demo.renderOptions, demo.profiling, demo.logic);
            vkz_updateUIRenderData(ui);

            clockDuration = 0.0;
        }

        // render
        kage::run();

        clock_t endFrameTime = clock();
        double cpuTimeMS = (double)(endFrameTime - startFrameTime);
        clockDuration += cpuTimeMS;
        
        avgCpuTime = float(avgCpuTime * 0.95 + (cpuTimeMS) * 0.05);
        demo.profiling.avgCpuTime = (float)avgCpuTime;
        demo.profiling.cullEarlyTime = (float)kage::getPassTime(culling.pass);
        demo.profiling.drawEarlyTime = (float)kage::getPassTime(meshShading.pass);
        demo.profiling.cullLateTime = (float)kage::getPassTime(cullingLate.pass);
        demo.profiling.drawLateTime = (float)kage::getPassTime(meshShadingLate.pass);
        demo.profiling.pyramidTime = (float)kage::getPassTime(pyRendering.pass);
        demo.profiling.uiTime = (float)kage::getPassTime(ui.pass);

        VKZ_FrameMark;
    }

    kage::shutdown();
}

void DemoMain()
{
    meshDemo();
}