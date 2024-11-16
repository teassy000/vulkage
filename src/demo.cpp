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

            m_width = _width;
            m_height = _height;

            entry::setMouseLock(entry::kDefaultWindowHandle, true);

            kage::init(config);
            
            m_supportMeshShading = kage::checkSupports(kage::VulkanSupportExtension::ext_mesh_shader);

            
            initScene(kage::kUseSeamlessLod);

            // ui data
            m_demoData.input.width = (float)_width;
            m_demoData.input.height = (float)_height;

            // basic data
            freeCameraInit();

            createBuffers();
            createImages();
            createPasses();

            kage::setPresentImage(m_ui.colorOutAlias);
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

            freeCameraUpdate(deltaTimeMS, m_mouseState);

            updatePyramid(m_pyramid, m_width, m_height);

            refreshData();

            updateSkybox(m_skybox, m_width, m_height);

            updateCulling(m_culling, m_demoData.drawCull, m_scene.drawCount);
            updateCulling(m_cullingLate, m_demoData.drawCull, m_scene.drawCount);

            if (m_supportMeshShading)
            {
                updateTaskSubmit(m_taskSubmit);
                updateTaskSubmit(m_taskSubmitLate);

                updateMeshShading(m_meshShading, m_demoData.globals);
                updateMeshShading(m_meshShadingLate, m_demoData.globals);
            }
            else
            {
                updateVtxShadingConstants(m_vtxShading, m_demoData.globals);
                updateVtxShadingConstants(m_vtxShadingLate, m_demoData.globals);
            }

            const kage::Memory* memTransform = kage::alloc(sizeof(TransformData));
            memcpy_s(memTransform->data, memTransform->size, &m_demoData.trans, sizeof(TransformData));
            kage::updateBuffer(m_transformBuf, memTransform);

            m_demoData.input.width = (float)m_width;
            m_demoData.input.height = (float)m_height;
            m_demoData.input.mouseButtons.left = m_mouseState.m_buttons[entry::MouseButton::Left];
            m_demoData.input.mouseButtons.right = m_mouseState.m_buttons[entry::MouseButton::Right];
            m_demoData.input.mousePosx = (float)m_mouseState.m_mx;
            m_demoData.input.mousePosy = (float)m_mouseState.m_my;

            bx::Vec3 front = freeCameraGetFront();
            bx::Vec3 pos = freeCameraGetPos();

            m_demoData.logic.posX = pos.x;
            m_demoData.logic.posY = pos.y;
            m_demoData.logic.posZ = pos.z;
            m_demoData.logic.frontX = front.x;
            m_demoData.logic.frontY = front.y;
            m_demoData.logic.frontZ = front.z;

            updateUI(m_ui, m_demoData.input, m_demoData.renderOptions, m_demoData.profiling, m_demoData.logic);

            // render
            kage::render();

            static float avgCpuTime = 0.0f;
            avgCpuTime = avgCpuTime * 0.95f + (deltaTimeMS) * 0.05f;

            static float avgGpuTime = 0.0f;
            avgGpuTime = avgGpuTime * 0.95f + (float)kage::getGpuTime() * 0.05f;
            m_demoData.profiling.avgCpuTime = avgCpuTime;
            m_demoData.profiling.avgGpuTime = avgGpuTime;
            m_demoData.profiling.cullEarlyTime = (float)kage::getPassTime(m_culling.pass);
            m_demoData.profiling.drawEarlyTime = (float)kage::getPassTime(m_meshShading.pass);
            m_demoData.profiling.cullLateTime = (float)kage::getPassTime(m_cullingLate.pass);
            m_demoData.profiling.drawLateTime = (float)kage::getPassTime(m_meshShadingLate.pass);
            m_demoData.profiling.pyramidTime = (float)kage::getPassTime(m_pyramid.pass);
            m_demoData.profiling.uiTime = (float)kage::getPassTime(m_ui.pass);

            m_demoData.profiling.triangleEarlyCount = (float)(kage::getPassClipping(m_meshShading.pass));
            m_demoData.profiling.triangleLateCount = (float)(kage::getPassClipping(m_meshShadingLate.pass));
            m_demoData.profiling.triangleCount = m_demoData.profiling.triangleEarlyCount + m_demoData.profiling.triangleLateCount;

            m_demoData.profiling.meshletCount = (uint32_t)m_scene.geometry.meshlets.size();
            m_demoData.profiling.primitiveCount = (uint32_t)(m_scene.geometry.indices.size()) / 3; // include all lods

            KG_FrameMark;

            return true;
        }

        int shutdown() override
        {
            freeCameraDestroy();

            kage::shutdown();
            return 0;
        }

        bool initScene(bool _seamlessLod)
        {
            const char* pathes[] = { "./data/kitten.obj" };
            bool lmr = loadScene(m_scene, pathes, COUNTOF(pathes), m_supportMeshShading, _seamlessLod);
            return lmr;
        }


        void createBuffers()
        {
            // mesh data
            {
                const kage::Memory* memMeshBuf = kage::alloc((uint32_t)(sizeof(Mesh) * m_scene.geometry.meshes.size()));
                memcpy(memMeshBuf->data, m_scene.geometry.meshes.data(), memMeshBuf->size);

                kage::BufferDesc meshBufDesc;
                meshBufDesc.size = memMeshBuf->size;
                meshBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_meshBuf = kage::registBuffer("mesh", meshBufDesc, memMeshBuf);
            }

            // mesh draw instance buffer
            {
                const kage::Memory* memMeshDrawBuf = kage::alloc((uint32_t)(m_scene.meshDraws.size() * sizeof(MeshDraw)));
                memcpy(memMeshDrawBuf->data, m_scene.meshDraws.data(), memMeshDrawBuf->size);

                kage::BufferDesc meshDrawBufDesc;
                meshDrawBufDesc.size = memMeshDrawBuf->size;
                meshDrawBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshDrawBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_meshDrawBuf = kage::registBuffer("meshDraw", meshDrawBufDesc, memMeshDrawBuf);
            }

            // mesh draw instance buffer
            {
                kage::BufferDesc meshDrawCmdBufDesc;
                meshDrawCmdBufDesc.size = 128 * 1024 * 1024; // 128M
                meshDrawCmdBufDesc.usage = kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshDrawCmdBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_meshDrawCmdBuf = kage::registBuffer("meshDrawCmd", meshDrawCmdBufDesc);
            }

            // mesh draw instance count buffer
            {
                kage::BufferDesc meshDrawCmdCountBufDesc;
                meshDrawCmdCountBufDesc.size = 16;
                meshDrawCmdCountBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
                meshDrawCmdCountBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_meshDrawCmdCountBuf = kage::registBuffer("meshDrawCmdCount", meshDrawCmdCountBufDesc);
            }

            // mesh draw instance visibility buffer
            {
                kage::BufferDesc meshDrawVisBufDesc;
                meshDrawVisBufDesc.size = uint32_t(m_scene.drawCount * sizeof(uint32_t));
                meshDrawVisBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
                meshDrawVisBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_meshDrawVisBuf = kage::registBuffer("meshDrawVis", meshDrawVisBufDesc);
            }

            // meshlet visibility buffer
            {
                kage::BufferDesc meshletVisBufDesc;
                meshletVisBufDesc.size = (m_scene.meshletVisibilityCount + 31) / 32 * sizeof(uint32_t);
                meshletVisBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
                meshletVisBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_meshletVisBuf = kage::registBuffer("meshletVis", meshletVisBufDesc);
            }

            // index buffer
            {
                const kage::Memory* memIdxBuf = kage::alloc((uint32_t)(m_scene.geometry.indices.size() * sizeof(uint32_t)));
                memcpy(memIdxBuf->data, m_scene.geometry.indices.data(), memIdxBuf->size);

                kage::BufferDesc idxBufDesc;
                idxBufDesc.size = memIdxBuf->size;
                idxBufDesc.usage = kage::BufferUsageFlagBits::index | kage::BufferUsageFlagBits::transfer_dst;
                idxBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_idxBuf = kage::registBuffer("idx", idxBufDesc, memIdxBuf);
            }

            // vertex buffer
            {
                const kage::Memory* memVtxBuf = kage::alloc((uint32_t)(m_scene.geometry.vertices.size() * sizeof(Vertex)));
                memcpy(memVtxBuf->data, m_scene.geometry.vertices.data(), memVtxBuf->size);

                kage::BufferDesc vtxBufDesc;
                vtxBufDesc.size = memVtxBuf->size;
                vtxBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                vtxBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_vtxBuf = kage::registBuffer("vtx", vtxBufDesc, memVtxBuf);
            }

            // meshlet buffer
            {
                size_t sz = (kage::kUseSeamlessLod == 1)
                    ? m_scene.geometry.clusters.size() * sizeof(Cluster)
                    : m_scene.geometry.meshlets.size() * sizeof(Meshlet);

                const kage::Memory* memMeshletBuf = kage::alloc((uint32_t)sz);
                void* srcData = (kage::kUseSeamlessLod == 1)
                    ? (void*)m_scene.geometry.clusters.data()
                    : (void*)m_scene.geometry.meshlets.data();
                memcpy(memMeshletBuf->data, srcData, memMeshletBuf->size);

                kage::BufferDesc meshletBufferDesc;
                meshletBufferDesc.size = memMeshletBuf->size;
                meshletBufferDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshletBufferDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_meshletBuffer = kage::registBuffer("meshlet(cluster)_buffer", meshletBufferDesc, memMeshletBuf);
            }

            // meshlet data buffer
            {
                const kage::Memory* memMeshletDataBuf = kage::alloc((uint32_t)(m_scene.geometry.meshletdata.size() * sizeof(uint32_t)));
                memcpy(memMeshletDataBuf->data, m_scene.geometry.meshletdata.data(), memMeshletDataBuf->size);

                kage::BufferDesc meshletDataBufferDesc;
                meshletDataBufferDesc.size = memMeshletDataBuf->size;
                meshletDataBufferDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::transfer_dst;
                meshletDataBufferDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_meshletDataBuffer = kage::registBuffer("meshlet_data_buffer", meshletDataBufferDesc, memMeshletDataBuf);
            }

            // transform
            {
                kage::BufferDesc transformBufDesc;
                transformBufDesc.size = (uint32_t)(sizeof(TransformData));
                transformBufDesc.usage = kage::BufferUsageFlagBits::uniform | kage::BufferUsageFlagBits::transfer_dst;
                transformBufDesc.memFlags = kage::MemoryPropFlagBits::device_local | kage::MemoryPropFlagBits::host_visible;
                m_transformBuf = kage::registBuffer("transform", transformBufDesc);
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
                m_color = kage::registRenderTarget("color", rtDesc, kage::ResourceLifetime::non_transition);
            }

            {
                kage::ImageDesc dpDesc;
                dpDesc.depth = 1;
                dpDesc.numLayers = 1;
                dpDesc.numMips = 1;
                dpDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled;
                m_depth = kage::registDepthStencil("depth", dpDesc, kage::ResourceLifetime::non_transition);
            }
        }

        void createPasses()
        {
            preparePyramid(m_pyramid, m_width, m_height);

            // culling pass
            {
                CullingCompInitData cullingInit{};
                cullingInit.meshBuf = m_meshBuf;
                cullingInit.meshDrawBuf = m_meshDrawBuf;
                cullingInit.transBuf = m_transformBuf;
                cullingInit.pyramid = m_pyramid.image;
                cullingInit.meshDrawCmdBuf = m_meshDrawCmdBuf;
                cullingInit.meshDrawCmdCountBuf = m_meshDrawCmdCountBuf;
                cullingInit.meshDrawVisBuf = m_meshDrawVisBuf;

                prepareCullingComp(m_culling, cullingInit, false, m_supportMeshShading);
            }

            // skybox pass
            {
                kage::ImageHandle sbColorIn = m_color;
                initSkyboxPass(m_skybox, m_transformBuf, sbColorIn);
            }

            // draw early pass
            if (!m_supportMeshShading)
            {
                VtxShadingInitData vsInit{};

                vsInit.idxBuf = m_idxBuf;
                vsInit.vtxBuf = m_vtxBuf;
                vsInit.meshDrawBuf = m_meshDrawBuf;
                vsInit.meshDrawCmdBuf = m_culling.meshDrawCmdBufOutAlias;
                vsInit.transformBuf = m_transformBuf;
                vsInit.meshDrawCmdCountBuf = m_culling.meshDrawCmdCountBufOutAlias;
                vsInit.color = m_skybox.colorOutAlias;
                vsInit.depth = m_depth;

                prepareVtxShading(m_vtxShading, m_scene, vsInit);
            }
            else
            {
                prepareTaskSubmit(m_taskSubmit, m_culling.meshDrawCmdBufOutAlias, m_culling.meshDrawCmdCountBufOutAlias);

                MeshShadingInitData msInit{};
                msInit.vtxBuffer = m_vtxBuf;
                msInit.meshBuffer = m_meshBuf;
                msInit.meshletBuffer = m_meshletBuffer;
                msInit.meshletDataBuffer = m_meshletDataBuffer;
                msInit.meshDrawBuffer = m_meshDrawBuf;
                msInit.meshDrawCmdBuffer = m_taskSubmit.drawCmdBufferOutAlias;
                msInit.meshDrawCmdCountBuffer = m_culling.meshDrawCmdCountBufOutAlias;
                msInit.meshletVisBuffer = m_meshletVisBuf;
                msInit.transformBuffer = m_transformBuf;

                msInit.pyramid = m_pyramid.image;

                msInit.color = m_skybox.colorOutAlias;
                msInit.depth = m_depth;
                prepareMeshShading(m_meshShading, m_scene, m_width, m_height, msInit);
            }

            // pyramid pass
            setPyramidPassDependency(m_pyramid, m_supportMeshShading ? m_meshShading.depthOutAlias : m_vtxShading.depthOutAlias);

            // culling late pass
            {
                CullingCompInitData cullingInit{};
                cullingInit.meshBuf = m_meshBuf;
                cullingInit.meshDrawBuf = m_meshDrawBuf;
                cullingInit.transBuf = m_transformBuf;
                cullingInit.pyramid = m_pyramid.imgOutAlias;
                cullingInit.meshDrawCmdBuf = m_supportMeshShading ? m_taskSubmit.drawCmdBufferOutAlias : m_culling.meshDrawCmdBufOutAlias;
                cullingInit.meshDrawCmdCountBuf = m_culling.meshDrawCmdCountBufOutAlias;
                cullingInit.meshDrawVisBuf = m_culling.meshDrawVisBufOutAlias;

                prepareCullingComp(m_cullingLate, cullingInit, true, m_supportMeshShading);
            }

            // draw late
            if (!m_supportMeshShading)
            {
                VtxShadingInitData vsInit{};

                vsInit.idxBuf = m_idxBuf;
                vsInit.vtxBuf = m_vtxBuf;
                vsInit.meshDrawBuf = m_meshDrawBuf;
                vsInit.meshDrawCmdBuf = m_cullingLate.meshDrawCmdBufOutAlias;
                vsInit.transformBuf = m_transformBuf;
                vsInit.meshDrawCmdCountBuf = m_cullingLate.meshDrawCmdCountBufOutAlias;
                vsInit.color = m_vtxShading.colorOutAlias;
                vsInit.depth = m_vtxShading.depthOutAlias;

                prepareVtxShading(m_vtxShadingLate, m_scene, vsInit, true);
            }
            else
            {
                prepareTaskSubmit(m_taskSubmitLate, m_cullingLate.meshDrawCmdBufOutAlias, m_cullingLate.meshDrawCmdCountBufOutAlias, true);

                MeshShadingInitData msInit{};
                msInit.vtxBuffer = m_vtxBuf;
                msInit.meshBuffer = m_meshBuf;
                msInit.meshletBuffer = m_meshletBuffer;
                msInit.meshletDataBuffer = m_meshletDataBuffer;
                msInit.meshDrawBuffer = m_meshDrawBuf;
                msInit.meshDrawCmdBuffer = m_taskSubmitLate.drawCmdBufferOutAlias;
                msInit.meshDrawCmdCountBuffer = m_cullingLate.meshDrawCmdCountBufOutAlias;
                msInit.meshletVisBuffer = m_meshShading.meshletVisBufferOutAlias;
                msInit.transformBuffer = m_transformBuf;

                msInit.pyramid = m_pyramid.imgOutAlias;

                msInit.color = m_meshShading.colorOutAlias;
                msInit.depth = m_meshShading.depthOutAlias;
                prepareMeshShading(m_meshShadingLate, m_scene, m_width, m_height, msInit, true);
            }

            {
                kage::ImageHandle uiColorIn = m_supportMeshShading ? m_meshShadingLate.colorOutAlias : m_vtxShadingLate.colorOutAlias;
                kage::ImageHandle uiDepthIn = m_supportMeshShading ? m_meshShadingLate.depthOutAlias : m_vtxShadingLate.depthOutAlias;

                prepareUI(m_ui, uiColorIn, uiDepthIn, 1.3f);
            }
        }

        void refreshData()
        {
            float znear = .1f;
            mat4 projection = perspectiveProjection(glm::radians(70.f), (float)m_width / (float)m_height, znear);
            mat4 projectionT = glm::transpose(projection);
            vec4 frustumX = normalizePlane(projectionT[3] - projectionT[0]);
            vec4 frustumY = normalizePlane(projectionT[3] - projectionT[1]);

            freeCameraGetViewMatrix(m_demoData.trans.view);
            m_demoData.trans.proj = projection;

            bx::Vec3 cameraPos = freeCameraGetPos();
            m_demoData.trans.cameraPos.x = cameraPos.x;
            m_demoData.trans.cameraPos.y = cameraPos.y;
            m_demoData.trans.cameraPos.z = cameraPos.z;

            m_demoData.drawCull.P00 = projection[0][0];
            m_demoData.drawCull.P11 = projection[1][1];
            m_demoData.drawCull.zfar = m_scene.drawDistance;
            m_demoData.drawCull.znear = znear;
            m_demoData.drawCull.pyramidWidth = (float)m_pyramid.width;
            m_demoData.drawCull.pyramidHeight = (float)m_pyramid.height;
            m_demoData.drawCull.frustum[0] = frustumX.x;
            m_demoData.drawCull.frustum[1] = frustumX.z;
            m_demoData.drawCull.frustum[2] = frustumY.y;
            m_demoData.drawCull.frustum[3] = frustumY.z;
            m_demoData.drawCull.enableCull = 1;
            m_demoData.drawCull.enableLod = kage::kEnableLodCull;
            m_demoData.drawCull.enableSeamlessLod = kage::kUseSeamlessLod;
            m_demoData.drawCull.enableOcclusion = 1;
            m_demoData.drawCull.enableMeshletOcclusion = 1;

            m_demoData.globals.zfar = m_scene.drawDistance;
            m_demoData.globals.znear = znear;
            m_demoData.globals.frustum[0] = frustumX.x;
            m_demoData.globals.frustum[1] = frustumX.z;
            m_demoData.globals.frustum[2] = frustumY.y;
            m_demoData.globals.frustum[3] = frustumY.z;
            m_demoData.globals.pyramidWidth = (float)m_pyramid.width;
            m_demoData.globals.pyramidHeight = (float)m_pyramid.height;
            m_demoData.globals.screenWidth = (float)m_width;
            m_demoData.globals.screenHeight = (float)m_height;
            m_demoData.globals.enableMeshletOcclusion = 1;
            m_demoData.globals.lodErrorThreshold = (2 / projection[1][1]) * (1.f / float(m_height)); // 1px
        }

        Scene m_scene{};
        DemoData m_demoData{};
        bool m_supportMeshShading;

        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_debug;
        uint32_t m_reset;
        entry::MouseState m_mouseState;

        kage::BufferHandle m_meshBuf;
        kage::BufferHandle m_meshDrawBuf;
        kage::BufferHandle m_meshDrawCmdBuf;
        kage::BufferHandle m_meshDrawCmdCountBuf;
        kage::BufferHandle m_meshDrawVisBuf;
        kage::BufferHandle m_meshletVisBuf;
        kage::BufferHandle m_idxBuf;
        kage::BufferHandle m_vtxBuf;
        kage::BufferHandle m_meshletBuffer;
        kage::BufferHandle m_meshletDataBuffer;
        kage::BufferHandle m_transformBuf;

        // images
        kage::ImageHandle m_color;
        kage::ImageHandle m_depth;

        // passes
        Pyramid m_pyramid{};
        
        TaskSubmit m_taskSubmit{};
        TaskSubmit m_taskSubmitLate{};
        Culling m_culling{};
        Culling m_cullingLate{};
        MeshShading m_meshShading{};
        MeshShading m_meshShadingLate{};
        VtxShading m_vtxShading{};
        VtxShading m_vtxShadingLate{};
        Skybox m_skybox{};

        UIRendering m_ui{};
    };
}

ENTRY_IMPLEMENT_MAIN(
    Demo
    , "vk_ms"
    , "vk_ms"
    , "none"
);


