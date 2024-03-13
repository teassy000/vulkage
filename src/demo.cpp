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

static DemoData demo{};

void mouseMoveCallback(double _xpos, double _ypos)
{
    demo.input.mousePosx = (float)_xpos;
    demo.input.mousePosy = (float)_ypos;

    freeCameraProcessMouseMovement(demo.camera, (float)_xpos, (float)_ypos);
}

void keyCallback(vkz::KeyEnum _key, vkz::KeyState _state, vkz::KeyModFlags _flags)
{
    if (vkz::KeyState::press == _state 
        || vkz::KeyState::repeat == _state)
    {
        freeCameraProcessKeyboard(demo.camera, _key, 0.1f);
    }
}

void meshDemo()
{
    vkz::VKZInitConfig config = {};
    config.windowWidth = 2560;
    config.windowHeight = 1440;

    vkz::init(config);

    bool supportMeshShading = vkz::checkSupports(vkz::VulkanSupportExtension::ext_mesh_shader);

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
    vkz::setKeyCallback(keyCallback);
    vkz::setMouseMoveCallback(mouseMoveCallback);

    // buffers
    // mesh data
    const vkz::Memory* memMeshBuf = vkz::alloc((uint32_t)(sizeof(Mesh) * scene.geometry.meshes.size()));
    memcpy(memMeshBuf->data, scene.geometry.meshes.data(), memMeshBuf->size);

    vkz::BufferDesc meshBufDesc;
    meshBufDesc.size = memMeshBuf->size;
    meshBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    meshBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshBuf = vkz::registBuffer("mesh", meshBufDesc, memMeshBuf);

    // mesh draw instance buffer

    const vkz::Memory* memMeshDrawBuf = vkz::alloc((uint32_t)(scene.meshDraws.size() * sizeof(MeshDraw)));
    memcpy(memMeshDrawBuf->data, scene.meshDraws.data(), memMeshDrawBuf->size);

    vkz::BufferDesc meshDrawBufDesc;
    meshDrawBufDesc.size = memMeshDrawBuf->size;
    meshDrawBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    meshDrawBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshDrawBuf = vkz::registBuffer("meshDraw", meshDrawBufDesc, memMeshDrawBuf);

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
    vkz::BufferHandle meshletVisBuf = vkz::registBuffer("meshletVis", meshletVisBufDesc);

    // index buffer
    const vkz::Memory* memIdxBuf = vkz::alloc((uint32_t)(scene.geometry.indices.size() * sizeof(uint32_t)));
    memcpy(memIdxBuf->data, scene.geometry.indices.data(), memIdxBuf->size);

    vkz::BufferDesc idxBufDesc;
    idxBufDesc.size = memIdxBuf->size;
    idxBufDesc.usage = vkz::BufferUsageFlagBits::index | vkz::BufferUsageFlagBits::transfer_dst;
    idxBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle idxBuf = vkz::registBuffer("idx", idxBufDesc, memIdxBuf);

    // vertex buffer
    const vkz::Memory* memVtxBuf = vkz::alloc((uint32_t)(scene.geometry.vertices.size() * sizeof(Vertex)));
    memcpy(memVtxBuf->data, scene.geometry.vertices.data(), memVtxBuf->size);

    vkz::BufferDesc vtxBufDesc;
    vtxBufDesc.size = memVtxBuf->size;
    vtxBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    vtxBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle vtxBuf = vkz::registBuffer("vtx", vtxBufDesc, memVtxBuf);

    // meshlet buffer
    const vkz::Memory* memMeshletBuf = vkz::alloc((uint32_t)(scene.geometry.meshlets.size() * sizeof(Meshlet)));
    memcpy(memMeshletBuf->data, scene.geometry.meshlets.data(), memMeshletBuf->size);

    vkz::BufferDesc meshletBufferDesc;
    meshletBufferDesc.size = memMeshletBuf->size;
    meshletBufferDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    meshletBufferDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshletBuffer = vkz::registBuffer("meshlet_buffer", meshletBufferDesc, memMeshletBuf);
    
    // meshlet data buffer
    const vkz::Memory* memMeshletDataBuf = vkz::alloc((uint32_t)(scene.geometry.meshletdata.size() * sizeof(uint32_t)));
    memcpy(memMeshletDataBuf->data, scene.geometry.meshletdata.data(), memMeshletDataBuf->size);

    vkz::BufferDesc meshletDataBufferDesc;
    meshletDataBufferDesc.size = memMeshletDataBuf->size;
    meshletDataBufferDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    meshletDataBufferDesc.memFlags = vkz::MemoryPropFlagBits::device_local;
    vkz::BufferHandle meshletDataBuffer = vkz::registBuffer("meshlet_data_buffer", meshletDataBufferDesc, memMeshletDataBuf);

    // transform
    vkz::BufferDesc transformBufDesc;
    transformBufDesc.size = (uint32_t)(sizeof(TransformData));
    transformBufDesc.usage = vkz::BufferUsageFlagBits::storage | vkz::BufferUsageFlagBits::transfer_dst;
    transformBufDesc.memFlags = vkz::MemoryPropFlagBits::device_local | vkz::MemoryPropFlagBits::host_visible;
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

        prepareCullingComp(culling, cullingInit, false, supportMeshShading);
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

        vkz_prepareUI(ui, uiColorIn, uiDepthIn, 1.3f);
    }

    vkz::PassHandle pass_py = pyRendering.pass;
    vkz::PassHandle pass_ui = ui.pass;

    vkz::setPresentImage(ui.colorOutAlias);

    vkz::bake();

    // data used in loop
    const vkz::Memory* memTransform = vkz::alloc(sizeof(TransformData));

    double clockDuration = 0.0;
    double avgCpuTime = 0.0;
    while (!vkz::shouldClose())
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

        vkz::updateThreadCount(culling.pass, (uint32_t)scene.meshDraws.size(), 1, 1);
        vkz::updateThreadCount(cullingLate.pass, (uint32_t)scene.meshDraws.size(), 1, 1);

        memcpy_s(memTransform->data, memTransform->size, &demo.trans, sizeof(TransformData));
        vkz::updateBuffer(transformBuf, vkz::copy(memTransform));

        // update profiling data to GPU from last frame
        if ( clockDuration > 1000.0 )
        {
            vkz_updateImGui(demo.input, demo.renderOptions, demo.profiling, demo.logic);
            vkz_updateUIRenderData(ui);

            clockDuration = 0.0;
        }

        // render
        vkz::run();

        clock_t endFrameTime = clock();
        double cpuTimeMS = (double)(endFrameTime - startFrameTime);
        clockDuration += cpuTimeMS;
        
        avgCpuTime = float(avgCpuTime * 0.95 + (cpuTimeMS) * 0.05);
        demo.profiling.avgCpuTime = (float)avgCpuTime;
        demo.profiling.cullEarlyTime = (float)vkz::getPassTime(culling.pass);
        demo.profiling.drawEarlyTime = (float)vkz::getPassTime(meshShading.pass);
        demo.profiling.cullLateTime = (float)vkz::getPassTime(cullingLate.pass);
        demo.profiling.drawLateTime = (float)vkz::getPassTime(meshShadingLate.pass);
        demo.profiling.pyramidTime = (float)vkz::getPassTime(pyRendering.pass);
        demo.profiling.uiTime = (float)vkz::getPassTime(ui.pass);

        VKZ_FrameMark;
    }

    vkz::shutdown();
}

void DemoMain()
{
    meshDemo();
}