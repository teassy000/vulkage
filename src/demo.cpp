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
#include "file_helper.h"

#include "entry/entry.h"
#include "bx/timer.h"
#include "vkz_smaa_pip.h"
#include "radiance_cascade/vkz_radiance_cascade.h"
#include "deferred/vkz_deferred.h"
#include "radiance_cascade/vkz_rc_debug.h"
#include "ffx_intg/brixel_intg_kage.h"
#include "radiance_cascade/vkz_rc2d.h"

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

            entry::setMouseLock(entry::kDefaultWindowHandle, false);

            kage::init(config);
            
            m_supportMeshShading = kage::checkSupports(kage::VulkanSupportExtension::ext_mesh_shader);
            m_debugProb = false;

            m_useRc2d = false;
            m_useRc3d = false;
            m_useBrixelizer = false;

            bool forceParse = false;
            bool seamlessLod = false;

            size_t pathCount = 0;
            std::vector<std::string> pathes(_argc);
            // check if argv contains -p
            for (int32_t ii = 0; ii < _argc; ++ii)
            {
                const char* arg = _argv[ii];
                if (strcmp(arg, "-p") == 0)
                {
                    forceParse = true;
                    continue;
                }

                if (strcmp(arg, "-l") == 0)
                {
                    seamlessLod = true;
                    continue;
                }

                if (ii > 0)
                {
                    pathes[pathCount] = arg;
                    pathCount++;
                }
            }
            pathes.resize(pathCount);

            initScene(pathes, forceParse, kage::kSeamlessLod);

            // ui data
            m_demoData.input.width = (float)_width;
            m_demoData.input.height = (float)_height;

            // basic data
            Camera camera{
                { 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 1.0f },
                60.f
            };
            if (!m_scene.cameras.empty())
            {
                camera = m_scene.cameras[0];
            }

            freeCameraInit(camera.pos, camera.orit, camera.fov);

            createBuffers();
            createImages();
            createBindlessArray();
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

            freeCameraSetSpeed(m_demoData.dbg_features.common.speed);
            bool camRet = freeCameraUpdate(deltaTimeMS, m_mouseState);
            static bool lockToggle = camRet;
            if (camRet != lockToggle)
            {
                lockToggle = camRet;
                entry::setMouseLock(entry::kDefaultWindowHandle, lockToggle);
            }

            // update brixelizer
            if(m_useBrixelizer)
            {
                BrixelData brxData;
                brxData.camPos = m_demoData.trans.cameraPos;
                brxData.projMat = m_demoData.trans.proj;
                brxData.viewMat = m_demoData.trans.view;

                const Dbg_Brixel& brx = m_demoData.dbg_features.brx;
                brxData.debugType = brx.debugBrixelType;
                brxData.startCas = brx.startCas;
                brxData.endCas = brx.endCas;
                brxData.sdfEps = brx.sdfEps;
                brxData.tmin = brx.tmin;
                brxData.tmax = brx.tmax;
                brxData.followCam = brx.followCam;

                brxUpdate(m_brixel, brxData);
            }

            updatePyramid(m_pyramid, m_width, m_height, m_demoData.dbg_features.common.dbgPauseCullTransform);

            refreshData();

            updateSkybox(m_skybox, m_width, m_height);

            updateCulling(m_culling, m_demoData.drawCull, m_scene.drawCount);
            updateCulling(m_cullingLate, m_demoData.drawCull, m_scene.drawCount);
            updateCulling(m_cullingAlpha, m_demoData.drawCull, m_scene.drawCount);


            updateDeferredShading(m_deferred, m_width, m_height, m_demoData.trans.cameraPos, m_demoData.dbg_features.rc3d.totalRadius, m_demoData.dbg_features.rc3d.idx_type, m_demoData.dbg_features.rc3d);

            if (m_supportMeshShading)
            {
                updateTaskSubmit(m_taskSubmit);
                updateTaskSubmit(m_taskSubmitLate);
                updateTaskSubmit(m_taskSubmitAlpha);

                updateMeshShading(m_meshShading, m_demoData.globals);
                updateMeshShading(m_meshShadingLate, m_demoData.globals);
                updateMeshShading(m_meshShadingAlpha, m_demoData.globals);
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

            vec3 front = freeCameraGetFront();
            vec3 pos = freeCameraGetPos();

            m_demoData.logic.posX = pos.x;
            m_demoData.logic.posY = pos.y;
            m_demoData.logic.posZ = pos.z;
            m_demoData.logic.frontX = front.x;
            m_demoData.logic.frontY = front.y;
            m_demoData.logic.frontZ = front.z;

            m_smaa.update(m_width, m_height);


            if (m_useRc3d) 
            {
                updateRadianceCascade(m_radianceCascade, m_demoData.dbg_features.rc3d, m_demoData.trans);
            }
            
            if (m_debugProb)
            {
                updateProbeDebug(m_probDebug, m_demoData.drawCull, m_width, m_height, m_demoData.dbg_features.rc3d);
            }

            if(m_useRc2d)
            {
                Rc2dInfo info{ m_width, m_height, 2, 6, (float)m_mouseState.m_mx, (float)m_mouseState.m_my };
                updateRc2D(m_rc2d, info, m_demoData.dbg_features.rc2d);
            }

            {
                m_demoData.dbg_features.brx.presentImg = m_useBrixelizer ? m_brixel.debugDestImg : kage::ImageHandle{};
                updateUI(m_ui, m_demoData.input, m_demoData.dbg_features, m_demoData.profiling, m_demoData.logic);
            }

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
            m_demoData.profiling.cullAlphaTime = (float)kage::getPassTime(m_cullingAlpha.pass);
            m_demoData.profiling.drawAlphaTime = (float)kage::getPassTime(m_meshShadingAlpha.pass);
            m_demoData.profiling.pyramidTime = (float)kage::getPassTime(m_pyramid.pass);
            m_demoData.profiling.uiTime = (float)kage::getPassTime(m_ui.pass);
            m_demoData.profiling.deferredTime = (float)kage::getPassTime(m_deferred.pass);

            if (m_useRc3d) {
                m_demoData.profiling.buildCascadeTime = (float)kage::getPassTime(m_radianceCascade.build.pass);
                m_demoData.profiling.mergeCascadeRayTime = (float)kage::getPassTime(m_radianceCascade.mergeRay.pass);
                m_demoData.profiling.mergeCascadeProbeTime = (float)kage::getPassTime(m_radianceCascade.mergeProbe.pass);

                m_demoData.profiling.debugProbeGenTime = (float)kage::getPassTime(m_probDebug.cmdGen.pass);
                m_demoData.profiling.debugProbeDrawTime = (float)kage::getPassTime(m_probDebug.draw.pass);
            }

            m_demoData.profiling.triangleEarlyCount = (float)(kage::getPassClipping(m_meshShading.pass));
            m_demoData.profiling.triangleLateCount = (float)(kage::getPassClipping(m_meshShadingLate.pass));
            m_demoData.profiling.triangleCount = m_demoData.profiling.triangleEarlyCount + m_demoData.profiling.triangleLateCount;

            m_demoData.profiling.meshletCount = kage::kSeamlessLod ?
                                                    (uint32_t)m_scene.geometry.clusters.size() :
                                                    (uint32_t)m_scene.geometry.meshlets.size();
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

        bool initScene(const std::vector<std::string>& _pathes, bool _forceParse, bool _seamlessLod)
        {
            bool lmr = loadScene(m_scene, _pathes, m_supportMeshShading, _seamlessLod, _forceParse);
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

            // index buffer
            if(!m_scene.geometry.indices.empty())
            {
                const kage::Memory* memIdxBuf = kage::alloc((uint32_t)(m_scene.geometry.indices.size() * sizeof(uint32_t)));
                memcpy(memIdxBuf->data, m_scene.geometry.indices.data(), memIdxBuf->size);

                kage::BufferDesc idxBufDesc;
                idxBufDesc.size = memIdxBuf->size;
                idxBufDesc.usage = kage::BufferUsageFlagBits::index | kage::BufferUsageFlagBits::transfer_dst | kage::BufferUsageFlagBits::storage;
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

            // transform
            {
                kage::BufferDesc transformBufDesc;
                transformBufDesc.size = (uint32_t)(sizeof(TransformData));
                transformBufDesc.usage = kage::BufferUsageFlagBits::uniform | kage::BufferUsageFlagBits::transfer_dst;
                transformBufDesc.memFlags = kage::MemoryPropFlagBits::device_local | kage::MemoryPropFlagBits::host_visible;
                m_transformBuf = kage::registBuffer("transform", transformBufDesc);
            }

            if (m_supportMeshShading)
            {
                // meshlet visibility buffer
                {
                    kage::BufferDesc meshletVisBufDesc;
                    meshletVisBufDesc.size = (m_scene.meshletVisibilityCount + 31) / 32 * sizeof(uint32_t);
                    meshletVisBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
                    meshletVisBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                    m_meshletVisBuf = kage::registBuffer("meshletVis", meshletVisBufDesc);
                }

                // meshlet buffer
                {
                    size_t sz = (kage::kSeamlessLod == 1)
                        ? m_scene.geometry.clusters.size() * sizeof(Cluster)
                        : m_scene.geometry.meshlets.size() * sizeof(Meshlet);

                    const kage::Memory* memMeshletBuf = kage::alloc((uint32_t)sz);
                    void* srcData = (kage::kSeamlessLod == 1)
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
            }
        }

        void createImages()
        {
            // create scene images
            for (const ImageInfo & img : m_scene.images)
            {
                const kage::Memory* mem = kage::copy(m_scene.imageDatas.data() + img.dataOffset, img.dataSize);
                kage::ImageDesc imgDesc;
                imgDesc.width = img.w;
                imgDesc.height = img.h;
                imgDesc.depth = 1;
                imgDesc.numLayers = img.layerCount;
                imgDesc.numMips = img.mipCount;
                imgDesc.format = img.format;
                imgDesc.type = (img.layerCount > 1) ? kage::ImageType::type_3d : kage::ImageType::type_2d;
                imgDesc.viewType = img.isCubeMap ? kage::ImageViewType::type_cube : kage::ImageViewType::type_2d;
                imgDesc.usage = kage::ImageUsageFlagBits::sampled | kage::ImageUsageFlagBits::transfer_dst;

                kage::ImageHandle imgHandle = kage::registTexture(img.name, imgDesc, mem);
                m_sceneImages.emplace_back(imgHandle);
            }

            {
                kage::ImageDesc rtDesc;
                rtDesc.depth = 1;
                rtDesc.numLayers = 1;
                rtDesc.numMips = 1;
                rtDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
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

            {
                m_skybox_cube = loadImageFromFile("skybox_cubemap", "./data/textures/cubemap_vulkan.ktx");
            }

            {
                m_gBuffer = createGBuffer();
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
                initSkyboxPass(m_skybox, m_transformBuf, sbColorIn, m_skybox_cube);
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
                vsInit.bindless = m_bindlessArray;

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

                msInit.depth = m_depth;
                msInit.bindless = m_bindlessArray;

                msInit.g_buffer = m_gBuffer;
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
                vsInit.bindless = m_bindlessArray;

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

                msInit.depth = m_meshShading.depthOutAlias;
                msInit.bindless = m_bindlessArray;

                msInit.g_buffer = m_meshShading.g_bufferOutAlias;
                prepareMeshShading(m_meshShadingLate, m_scene, m_width, m_height, msInit, true);
            }

            // cull alpha
            {
                CullingCompInitData cullingInit{};
                cullingInit.meshBuf = m_meshBuf;
                cullingInit.meshDrawBuf = m_meshDrawBuf;
                cullingInit.transBuf = m_transformBuf;
                cullingInit.pyramid = m_pyramid.imgOutAlias;
                cullingInit.meshDrawCmdBuf = m_supportMeshShading ? m_taskSubmitLate.drawCmdBufferOutAlias : m_cullingLate.meshDrawCmdBufOutAlias;
                cullingInit.meshDrawCmdCountBuf = m_cullingLate.meshDrawCmdCountBufOutAlias;
                cullingInit.meshDrawVisBuf = m_cullingLate.meshDrawVisBufOutAlias;
                prepareCullingComp(m_cullingAlpha, cullingInit, true, m_supportMeshShading, true);
            }

            // alpha 
            if (!m_supportMeshShading)
            {
                // vtx pass
            }
            else
            {
                prepareTaskSubmit(m_taskSubmitAlpha, m_cullingAlpha.meshDrawCmdBufOutAlias, m_cullingAlpha.meshDrawCmdCountBufOutAlias, true, true);

                MeshShadingInitData msInit{};
                msInit.vtxBuffer = m_vtxBuf;
                msInit.meshBuffer = m_meshBuf;
                msInit.meshletBuffer = m_meshletBuffer;
                msInit.meshletDataBuffer = m_meshletDataBuffer;
                msInit.meshDrawBuffer = m_meshDrawBuf;
                msInit.meshDrawCmdBuffer = m_taskSubmitAlpha.drawCmdBufferOutAlias;
                msInit.meshDrawCmdCountBuffer = m_cullingAlpha.meshDrawCmdCountBufOutAlias;
                msInit.meshletVisBuffer = m_meshShadingLate.meshletVisBufferOutAlias;
                msInit.transformBuffer = m_transformBuf;
                
                msInit.pyramid = m_pyramid.imgOutAlias;
                msInit.depth = m_meshShadingLate.depthOutAlias;
                
                msInit.bindless = m_bindlessArray;
                msInit.g_buffer = m_meshShadingLate.g_bufferOutAlias;
                prepareMeshShading(m_meshShadingAlpha, m_scene, m_width, m_height, msInit, true, true);
            }

            // brixelizer 
            if (m_useBrixelizer)
            {
                BrixelInitDesc bxlInitDesc{};
                bxlInitDesc.vtxBuf = m_vtxBuf;
                bxlInitDesc.vtxSz = (uint32_t)(m_scene.geometry.vertices.size() * sizeof(Vertex));
                bxlInitDesc.vtxStride = sizeof(Vertex);
                bxlInitDesc.idxBuf = m_idxBuf;
                bxlInitDesc.idxSz = (uint32_t)(m_scene.geometry.indices.size() * sizeof(uint32_t));
                bxlInitDesc.idxStride = sizeof(uint32_t);
                bxlInitDesc.seamless = kage::kSeamlessLod == 1;

                brxInit(m_brixel, bxlInitDesc, m_scene);
            }

            // radiance cascade
            if(m_useRc3d && m_useBrixelizer)
            {
                kage::ImageHandle cascadeDepthIn = m_supportMeshShading ? m_meshShadingAlpha.depthOutAlias : m_vtxShadingLate.depthOutAlias;
                kage::ImageHandle cascadeColorIn = m_deferred.outColorAlias;
                GBuffer gb = m_supportMeshShading ? m_meshShadingAlpha.g_bufferOutAlias : GBuffer();

                RadianceCascadeInitData rcInit{};
                rcInit.g_buffer = gb;
                rcInit.depth = cascadeDepthIn;
                rcInit.meshBuf = m_meshBuf;
                rcInit.meshDrawBuf = m_meshDrawBuf;
                rcInit.idxBuf = m_idxBuf;
                rcInit.vtxBuf = m_vtxBuf;
                rcInit.maxDrawCmdCount = (uint32_t)m_scene.meshDraws.size();
                rcInit.bindless = m_bindlessArray;
                rcInit.skybox = m_skybox_cube;
                rcInit.currCas = m_demoData.dbg_features.rc3d.startCascade;

                memcpy(&rcInit.brx, &m_brixel.userReses, sizeof(BRX_UserResources));

                prepareRadianceCascade(m_radianceCascade, rcInit);
            }

            // rc2d
            {
                Rc2dInfo info{ m_width, m_height, 2, 6, 0.f, 0.f };
                initRc2D(m_rc2d, info);
            }

            // deferred
            {
                RadianceCascadesData rcData{};
                if (m_useRc3d) {
                    rcData.cascades = m_radianceCascade.build.radCascdOutAlias;
                    rcData.mergedCascade = m_radianceCascade.mergeProbe.mergedCascadesAlias;
                }

                initDeferredShading(m_deferred, m_meshShadingAlpha.g_bufferOutAlias, m_skybox.colorOutAlias, rcData);
            }

            // probe debug
            if (m_useRc3d && m_debugProb)
            {
                RCDebugInit vdinit;
                vdinit.pyramid = m_pyramid.imgOutAlias;
                vdinit.color = m_deferred.outColorAlias;
                vdinit.depth = m_supportMeshShading ? m_meshShadingAlpha.depthOutAlias : m_vtxShadingLate.depthOutAlias;

                vdinit.trans = m_radianceCascade.build.trans;

                vdinit.cascade = m_radianceCascade.build.radCascdOutAlias;

                prepareProbeDebug(m_probDebug, vdinit);
            }

            // smaa
            {
                kage::ImageHandle aaDepthIn = m_supportMeshShading ? 
                    m_debugProb ?
                        m_probDebug.draw.depthOutAlias
                        : m_meshShadingAlpha.depthOutAlias
                    : m_vtxShadingLate.depthOutAlias;

                kage::ImageHandle aaColorIn = m_supportMeshShading ?
                    m_debugProb ?
                        m_probDebug.draw.colorOutAlias 
                        : m_deferred.outColorAlias
                    : m_vtxShadingLate.colorOutAlias;

                m_smaa.prepare(m_width, m_height, aaColorIn, aaDepthIn);
            }

            // ui
            {
                kage::ImageHandle uiColorIn = m_smaa.m_outAliasImg;
                //kage::ImageHandle uiColorIn = m_rc2d.use.rtOutAlias;
                kage::ImageHandle uiDepthIn = m_supportMeshShading ? m_meshShadingAlpha.depthOutAlias : m_vtxShadingLate.depthOutAlias;
                prepareUI(m_ui, uiColorIn, uiDepthIn, 1.3f);
            }
        }

        void refreshData()
        {
            float znear = .1f;
            mat4 proj = freeCameraGetPerpProjMatrix((float)m_width / (float)m_height, znear);
            mat4 projT = glm::transpose(proj);
            vec4 frustumX = normalizePlane(projT[3] - projT[0]);
            vec4 frustumY = normalizePlane(projT[3] - projT[1]);

            mat4 view = freeCameraGetViewMatrix();
            vec3 cameraPos = freeCameraGetPos();
            static vec3 s_campos = cameraPos;
            static vec4 s_fx = frustumX;
            static vec4 s_fy = frustumY;
            static mat4 s_proj = proj;
            static mat4 s_view = view;

            // update camera position only if culling is not paused
            if (!m_demoData.dbg_features.common.dbgPauseCullTransform) {
                s_fx = frustumX;
                s_fy = frustumY;
                s_proj = proj;
                s_view = view;
                s_campos = cameraPos;
            }

            float lodErrThreshold = (2 / s_proj[1][1]) * (1.f / float(m_height)); // 1px

            m_demoData.trans.cull_view = s_view;
            m_demoData.trans.cull_proj = s_proj;
            m_demoData.trans.cull_cameraPos = vec4(s_campos, 1.f);

            m_demoData.trans.view = view;
            m_demoData.trans.proj = proj;
            m_demoData.trans.cameraPos = vec4(cameraPos, 1.f);


            m_demoData.drawCull.P00 = s_proj[0][0];
            m_demoData.drawCull.P11 = s_proj[1][1];
            m_demoData.drawCull.zfar = m_scene.drawDistance;
            m_demoData.drawCull.znear = znear;
            m_demoData.drawCull.pyramidWidth = (float)m_pyramid.width;
            m_demoData.drawCull.pyramidHeight = (float)m_pyramid.height;
            m_demoData.drawCull.frustum[0] = s_fx.x;
            m_demoData.drawCull.frustum[1] = s_fx.z;
            m_demoData.drawCull.frustum[2] = s_fy.y;
            m_demoData.drawCull.frustum[3] = s_fy.z;
            m_demoData.drawCull.enableCull = 1;
            m_demoData.drawCull.enableLod = kage::kRegularLod;
            m_demoData.drawCull.enableSeamlessLod = kage::kSeamlessLod;
            m_demoData.drawCull.enableOcclusion = 1;
            m_demoData.drawCull.enableMeshletOcclusion = 1;
            m_demoData.drawCull.lodErrorThreshold = lodErrThreshold;

            m_demoData.globals.zfar = m_scene.drawDistance;
            m_demoData.globals.znear = znear;
            m_demoData.globals.frustum[0] = s_fx.x;
            m_demoData.globals.frustum[1] = s_fx.z;
            m_demoData.globals.frustum[2] = s_fy.y;
            m_demoData.globals.frustum[3] = s_fy.z;
            m_demoData.globals.pyramidWidth = (float)m_pyramid.width;
            m_demoData.globals.pyramidHeight = (float)m_pyramid.height;
            m_demoData.globals.screenWidth = (float)m_width;
            m_demoData.globals.screenHeight = (float)m_height;
            m_demoData.globals.enableMeshletOcclusion = 1;
            m_demoData.globals.lodErrorThreshold = lodErrThreshold;
            m_demoData.globals.probeRangeRadius = m_demoData.dbg_features.rc3d.totalRadius;
        }

        void createBindlessArray()
        {
            kage::BindlessDesc desc;
            desc.binding = 0;
            desc.resType = kage::ResourceType::image;
            desc.setIdx = 1;

            m_bindlessArray = kage::registBindless("bindless_sampler", desc);

            // set textures into bindless array
            const kage::Memory* mem = kage::alloc(uint32_t(sizeof(kage::ImageHandle) * m_sceneImages.size()));
            memcpy(mem->data, m_sceneImages.data(), mem->size);

            kage::setBindlessTextures(m_bindlessArray, mem, uint32_t(m_sceneImages.size()), kage::SamplerReductionMode::weighted_average);
        }

        Scene m_scene{};
        DemoData m_demoData{};
        bool m_supportMeshShading;
        bool m_debugProb;

        bool m_useRc3d;
        bool m_useBrixelizer;
        bool m_useRc2d;

        std::vector<kage::ImageHandle> m_sceneImages;

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

        kage::ImageHandle m_skybox_cube;

        kage::BindlessHandle m_bindlessArray;

        GBuffer m_gBuffer{};
        DeferredShading m_deferred{};

        // images
        kage::ImageHandle m_color;
        kage::ImageHandle m_depth;

        // passes
        Pyramid m_pyramid{};
        
        TaskSubmit m_taskSubmit{};
        TaskSubmit m_taskSubmitLate{};
        TaskSubmit m_taskSubmitAlpha{};
        Culling m_culling{};
        Culling m_cullingLate{};
        Culling m_cullingAlpha{};
        MeshShading m_meshShading{};
        MeshShading m_meshShadingLate{};
        MeshShading m_meshShadingAlpha{};
        VtxShading m_vtxShading{};
        VtxShading m_vtxShadingLate{};
        Skybox m_skybox{};

        BrixelResources m_brixel{};
        RadianceCascade m_radianceCascade{};
        ProbeDebug m_probDebug{};

        Rc2D m_rc2d;

        UIRendering m_ui{};

        SMAA m_smaa{};
    };
}

ENTRY_IMPLEMENT_MAIN(
    Demo
    , "vk_ms"
    , "vk_ms"
    , "none"
);


