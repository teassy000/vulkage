#include "kage.h"
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

#include "entry/entry.h"
#include "bx/timer.h"

namespace
{
    class Demo : public entry::AppI
    {
    public:
        Demo(
            const char* _name
            , const char* _description
            , const char* _url = "https://bkaradzic.github.io/bgfx/index.html"
        )
            : entry::AppI(_name, _description, _url)
        {

        }
        ~Demo()
        {

        }

        void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
        {
            kage::Init config = {};
            config.resolution.width = _width;
            config.resolution.height = _height;
            config.name = "vulkage demo";
            config.windowHandle = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);

            demoData.width = _width;
            demoData.height = _height;

            entry::setMouseLock(entry::kDefaultWindowHandle, true);

            kage::init(config);
            
            supportMeshShading = kage::checkSupports(kage::VulkanSupportExtension::ext_mesh_shader);

            initScene();

            // ui data
            demoData.input.width = (float)_width;
            demoData.input.height = (float)_height;

            // basic data
            freeCameraInit();

            createBuffers();
            createImages();
            createPasses();
            postSetting();
            
        }

        bool update() override
        {
            if (entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState))
            {
                return false;
            }

            int64_t now = bx::getHPCounter();
            static int64_t last = now;
            const int64_t frameTime = now - last;
            last = now;

            const double freq = double(bx::getHPFrequency());
            const float deltaTimeMS = float(frameTime / freq) * 1000.f;

            freeCameraUpdate(deltaTimeMS, m_mouseState, m_reset);


            float znear = .1f;
            mat4 projection = perspectiveProjection2(glm::radians(70.f), (float)demoData.width / (float)demoData.height, znear);
            mat4 projectionT = glm::transpose(projection);
            vec4 frustumX = normalizePlane2(projectionT[3] - projectionT[0]);
            vec4 frustumY = normalizePlane2(projectionT[3] - projectionT[1]);

            uint32_t pyramidLevelWidth = previousPow2_new(demoData.width);
            uint32_t pyramidLevelHeight = previousPow2_new(demoData.height);


            freeCameraGetViewMatrix(demoData.trans.view);
            demoData.trans.proj = projection;

            bx::Vec3 cameraPos = freeCameraGetPos();
            demoData.trans.cameraPos.x = cameraPos.x;
            demoData.trans.cameraPos.y = cameraPos.y;
            demoData.trans.cameraPos.z = cameraPos.z;

            demoData.drawCull.P00 = projection[0][0];
            demoData.drawCull.P11 = projection[1][1];
            demoData.drawCull.zfar = 10000.f; //scene.drawDistance;
            demoData.drawCull.znear = znear;
            demoData.drawCull.pyramidWidth = (float)pyramidLevelWidth;
            demoData.drawCull.pyramidHeight = (float)pyramidLevelHeight;
            demoData.drawCull.frustum[0] = frustumX.x;
            demoData.drawCull.frustum[1] = frustumX.z;
            demoData.drawCull.frustum[2] = frustumY.y;
            demoData.drawCull.frustum[3] = frustumY.z;
            demoData.drawCull.enableCull = 1;
            demoData.drawCull.enableLod = 1;
            demoData.drawCull.enableOcclusion = 1;
            demoData.drawCull.enableMeshletOcclusion = 1;

            updateCullingConstants(culling, demoData.drawCull);
            updateCullingConstants(cullingLate, demoData.drawCull);

            demoData.globals.projection = projection;
            demoData.globals.zfar = 10000.f;//scene.drawDistance;
            demoData.globals.znear = znear;
            demoData.globals.frustum[0] = frustumX.x;
            demoData.globals.frustum[1] = frustumX.z;
            demoData.globals.frustum[2] = frustumY.y;
            demoData.globals.frustum[3] = frustumY.z;
            demoData.globals.pyramidWidth = (float)pyramidLevelWidth;
            demoData.globals.pyramidHeight = (float)pyramidLevelHeight;
            demoData.globals.screenWidth = (float)demoData.width;
            demoData.globals.screenHeight = (float)demoData.height;
            demoData.globals.enableMeshletOcclusion = 1;

            if (supportMeshShading)
            {
                updateMeshShadingConstants(meshShading, demoData.globals);
                updateMeshShadingConstants(meshShadingLate, demoData.globals);
            }
            else
            {
                updateVtxShadingConstants(vtxShading, demoData.globals);
                updateVtxShadingConstants(vtxShadingLate, demoData.globals);
            }

            kage::updateThreadCount(culling.pass, (uint32_t)scene.meshDraws.size(), 1, 1);
            kage::updateThreadCount(cullingLate.pass, (uint32_t)scene.meshDraws.size(), 1, 1);

            const kage::Memory* memTransform = kage::alloc(sizeof(TransformData));
            memcpy_s(memTransform->data, memTransform->size, &demoData.trans, sizeof(TransformData));
            kage::updateBuffer(transformBuf, memTransform);

            // update profiling data to GPU from last frame

            vkz_updateImGui(demoData.input, demoData.renderOptions, demoData.profiling, demoData.logic);
            vkz_updateUIRenderData(ui);

            kage::update(transformBuf, memTransform);

            kage::submit();
            // render
            kage::run();


            static float avgCpuTime = 0.0f;
            avgCpuTime = avgCpuTime * 0.95f + (deltaTimeMS) * 0.05f;
            demoData.profiling.avgCpuTime = avgCpuTime;
            demoData.profiling.cullEarlyTime = (float)kage::getPassTime(culling.pass);
            demoData.profiling.drawEarlyTime = (float)kage::getPassTime(meshShading.pass);
            demoData.profiling.cullLateTime = (float)kage::getPassTime(cullingLate.pass);
            demoData.profiling.drawLateTime = (float)kage::getPassTime(meshShadingLate.pass);
            demoData.profiling.pyramidTime = (float)kage::getPassTime(pyramid.pass);
            demoData.profiling.uiTime = (float)kage::getPassTime(ui.pass);

            VKZ_FrameMark;

            return true;
        }

        int shutdown() override
        {
            freeCameraDestroy();

            kage::shutdown();
            return 0;
        }

        bool initScene()
        {
            const char* pathes[] = { "../data/kitten.obj" };
            bool lmr = loadScene(scene, pathes, COUNTOF(pathes), supportMeshShading);
            return lmr;
        }

        void createBuffers()
        {
            // mesh data
            {
                const kage::Memory* memMeshBuf = kage::alloc((uint32_t)(sizeof(Mesh) * scene.geometry.meshes.size()));
                memcpy(memMeshBuf->data, scene.geometry.meshes.data(), memMeshBuf->size);

                kage::BufferDesc meshBufDesc;
                meshBufDesc.size = memMeshBuf->size;
                meshBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                meshBuf = kage::registBuffer("mesh", meshBufDesc, memMeshBuf);
            }

            // mesh draw instance buffer
            {
                const kage::Memory* memMeshDrawBuf = kage::alloc((uint32_t)(scene.meshDraws.size() * sizeof(MeshDraw)));
                memcpy(memMeshDrawBuf->data, scene.meshDraws.data(), memMeshDrawBuf->size);

                kage::BufferDesc meshDrawBufDesc;
                meshDrawBufDesc.size = memMeshDrawBuf->size;
                meshDrawBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshDrawBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                meshDrawBuf = kage::registBuffer("meshDraw", meshDrawBufDesc, memMeshDrawBuf);
            }

            // mesh draw instance buffer
            {
                kage::BufferDesc meshDrawCmdBufDesc;
                meshDrawCmdBufDesc.size = 128 * 1024 * 1024; // 128M
                meshDrawCmdBufDesc.usage = kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshDrawCmdBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                meshDrawCmdBuf = kage::registBuffer("meshDrawCmd", meshDrawCmdBufDesc);
            }

            // mesh draw instance count buffer
            {
                kage::BufferDesc meshDrawCmdCountBufDesc;
                meshDrawCmdCountBufDesc.size = 16;
                meshDrawCmdCountBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
                meshDrawCmdCountBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                meshDrawCmdCountBuf = kage::registBuffer("meshDrawCmdCount", meshDrawCmdCountBufDesc);
                meshDrawCmdCountBufAfterFill = kage::alias(meshDrawCmdCountBuf);
                meshDrawCmdCountBufAfterFillLate = kage::alias(meshDrawCmdCountBuf);
            }

            // mesh draw instance visibility buffer
            {
                kage::BufferDesc meshDrawVisBufDesc;
                meshDrawVisBufDesc.size = uint32_t(scene.drawCount * sizeof(uint32_t));
                meshDrawVisBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
                meshDrawVisBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                meshDrawVisBuf = kage::registBuffer("meshDrawVis", meshDrawVisBufDesc);
            }

            // meshlet visibility buffer
            {
                kage::BufferDesc meshletVisBufDesc;
                meshletVisBufDesc.size = (scene.meshletVisibilityCount + 31) / 32 * sizeof(uint32_t);
                meshletVisBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
                meshletVisBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                meshletVisBuf = kage::registBuffer("meshletVis", meshletVisBufDesc);
            }

            // index buffer
            {
                const kage::Memory* memIdxBuf = kage::alloc((uint32_t)(scene.geometry.indices.size() * sizeof(uint32_t)));
                memcpy(memIdxBuf->data, scene.geometry.indices.data(), memIdxBuf->size);

                kage::BufferDesc idxBufDesc;
                idxBufDesc.size = memIdxBuf->size;
                idxBufDesc.usage = kage::BufferUsageFlagBits::index | kage::BufferUsageFlagBits::transfer_dst;
                idxBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                idxBuf = kage::registBuffer("idx", idxBufDesc, memIdxBuf);
            }

            // vertex buffer
            {
                const kage::Memory* memVtxBuf = kage::alloc((uint32_t)(scene.geometry.vertices.size() * sizeof(Vertex)));
                memcpy(memVtxBuf->data, scene.geometry.vertices.data(), memVtxBuf->size);

                kage::BufferDesc vtxBufDesc;
                vtxBufDesc.size = memVtxBuf->size;
                vtxBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                vtxBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                vtxBuf = kage::registBuffer("vtx", vtxBufDesc, memVtxBuf);
            }

            // meshlet buffer
            {
                const kage::Memory* memMeshletBuf = kage::alloc((uint32_t)(scene.geometry.meshlets.size() * sizeof(Meshlet)));
                memcpy(memMeshletBuf->data, scene.geometry.meshlets.data(), memMeshletBuf->size);

                kage::BufferDesc meshletBufferDesc;
                meshletBufferDesc.size = memMeshletBuf->size;
                meshletBufferDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshletBufferDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                meshletBuffer = kage::registBuffer("meshlet_buffer", meshletBufferDesc, memMeshletBuf);
            }

            // meshlet data buffer
            {
                const kage::Memory* memMeshletDataBuf = kage::alloc((uint32_t)(scene.geometry.meshletdata.size() * sizeof(uint32_t)));
                memcpy(memMeshletDataBuf->data, scene.geometry.meshletdata.data(), memMeshletDataBuf->size);

                kage::BufferDesc meshletDataBufferDesc;
                meshletDataBufferDesc.size = memMeshletDataBuf->size;
                meshletDataBufferDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshletDataBufferDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                meshletDataBuffer = kage::registBuffer("meshlet_data_buffer", meshletDataBufferDesc, memMeshletDataBuf);
            }

            // transform
            {
                kage::BufferDesc transformBufDesc;
                transformBufDesc.size = (uint32_t)(sizeof(TransformData));
                transformBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                transformBufDesc.memFlags = kage::MemoryPropFlagBits::device_local | kage::MemoryPropFlagBits::host_visible;
                transformBuf = kage::registBuffer("transform", transformBufDesc);
            }
        }

        void createImages()
        {
            {
                kage::ImageDesc rtDesc;
                rtDesc.depth = 1;
                rtDesc.numLayers = 1;
                rtDesc.numMips = 1;
                rtDesc.usage = kage::ImageUsageFlagBits::transfer_src;
                color = kage::registRenderTarget("color", rtDesc, kage::ResourceLifetime::non_transition);
            }

            {
                kage::ImageDesc dpDesc;
                dpDesc.depth = 1;
                dpDesc.numLayers = 1;
                dpDesc.numMips = 1;
                dpDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled;
                depth = kage::registDepthStencil("depth", dpDesc, kage::ResourceLifetime::non_transition);
            }
        }

        void createPasses()
        {
            preparePyramid(pyramid, demoData.width, demoData.height);

            // culling pass
            {
                CullingCompInitData cullingInit{};
                cullingInit.meshBuf = meshBuf;
                cullingInit.meshDrawBuf = meshDrawBuf;
                cullingInit.transBuf = transformBuf;
                cullingInit.pyramid = pyramid.image;
                cullingInit.meshDrawCmdBuf = meshDrawCmdBuf;
                cullingInit.meshDrawCmdCountBuf = meshDrawCmdCountBufAfterFill;
                cullingInit.meshDrawVisBuf = meshDrawVisBuf;

                prepareCullingComp(culling, cullingInit, false, supportMeshShading);
            }

            // skybox pass
            {
                kage::ImageHandle sbColorIn = color;
                initSkyboxPass(skybox, transformBuf, sbColorIn);
            }

            // draw early pass
            if (!supportMeshShading)
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

                msInit.pyramid = pyramid.image;

                msInit.color = skybox.colorOutAlias;
                msInit.depth = depth;
                prepareMeshShading(meshShading, scene, demoData.width, demoData.height, msInit);
            }

            // pyramid pass
            setPyramidPassDependency(pyramid, supportMeshShading ? meshShading.depthOutAlias : vtxShading.depthOutAlias);

            // culling late pass
            {
                CullingCompInitData cullingInit{};
                cullingInit.meshBuf = meshBuf;
                cullingInit.meshDrawBuf = meshDrawBuf;
                cullingInit.transBuf = transformBuf;
                cullingInit.pyramid = pyramid.imgOutAlias;
                cullingInit.meshDrawCmdBuf = supportMeshShading ? taskSubmit.drawCmdBufferOutAlias : culling.meshDrawCmdBufOutAlias;
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

                msInit.pyramid = pyramid.imgOutAlias;

                msInit.color = meshShading.colorOutAlias;
                msInit.depth = meshShading.depthOutAlias;
                prepareMeshShading(meshShadingLate, scene, demoData.width, demoData.height, msInit, true);
            }

            {
                kage::PassDesc passDesc;
                passDesc.queue = kage::PassExeQueue::fill_buffer;

                pass_fill_dccb = kage::registPass("fill_dccb", passDesc);

                kage::fillBuffer(pass_fill_dccb, meshDrawCmdCountBuf, 0, sizeof(uint32_t), 0, meshDrawCmdCountBufAfterFill);
            }

            {
                kage::PassDesc passDesc;
                passDesc.queue = kage::PassExeQueue::fill_buffer;

                pass_fill_dccb_late = kage::registPass("fill_dccb_late", passDesc);

                kage::BufferHandle dccb = culling.meshDrawCmdCountBufOutAlias;
                kage::fillBuffer(pass_fill_dccb_late, dccb, 0, sizeof(uint32_t), 0, meshDrawCmdCountBufAfterFillLate);
            }

            {
                kage::ImageHandle uiColorIn = supportMeshShading ? meshShadingLate.colorOutAlias : vtxShadingLate.colorOutAlias;
                kage::ImageHandle uiDepthIn = supportMeshShading ? meshShadingLate.depthOutAlias : vtxShadingLate.depthOutAlias;

                vkz_prepareUI(ui, uiColorIn, uiDepthIn, 1.3f);
            }
        }

        void postSetting()
        {
            kage::setPresentImage(ui.colorOutAlias);

            kage::bake();
        }

        Scene scene{};
        DemoData demoData{};
        bool supportMeshShading;

        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_debug;
        uint32_t m_reset;
        entry::MouseState m_mouseState;


        kage::BufferHandle meshBuf;
        kage::BufferHandle meshDrawBuf;
        kage::BufferHandle meshDrawCmdBuf;
        kage::BufferHandle meshDrawCmdCountBuf;
        kage::BufferHandle meshDrawVisBuf;
        kage::BufferHandle meshletVisBuf;
        kage::BufferHandle idxBuf;
        kage::BufferHandle vtxBuf;
        kage::BufferHandle meshletBuffer;
        kage::BufferHandle meshletDataBuffer;

        kage::BufferHandle transformBuf;

        // aliases
        kage::BufferHandle meshDrawCmdCountBufAfterFill;
        kage::BufferHandle meshDrawCmdCountBufAfterFillLate;

        // images
        kage::ImageHandle color;
        kage::ImageHandle depth;

        // passes
        kage::PassHandle pass_fill_dccb;
        kage::PassHandle pass_fill_dccb_late;

        PyramidRendering pyramid{};
        
        TaskSubmit taskSubmit{};
        TaskSubmit taskSubmitLate{};
        CullingComp culling{};
        CullingComp cullingLate{};
        MeshShading meshShading{};
        MeshShading meshShadingLate{};
        VtxShading vtxShading{};
        VtxShading vtxShadingLate{};
        SkyboxRendering skybox{};

        UIRendering ui{};
    };
}

ENTRY_IMPLEMENT_MAIN(
    Demo
    , "vk_ms"
    , "vk_ms"
    , "none"
);


