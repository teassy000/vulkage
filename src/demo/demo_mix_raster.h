#pragma once

#include "core/kage.h"
#include "assets/mesh.h"
#include "scene/scene.h"

#include "gfx/camera.h"
#include "core/debug.h"
#include "demo_structs.h"
#include "core/file_helper.h"

#include "pass/vkz_ui.h"
#include "pass/vkz_pyramid.h"
#include "pass/vkz_ms_pip.h"
#include "pass/vkz_vtx_pip.h"
#include "pass/vkz_culling_pass.h"
#include "pass/vkz_skybox_pass.h"
#include "pass/vkz_smaa_pip.h"
#include "pass/mix_rasterzation/vkz_mr_soft_raster.h"
#include "pass/vkz_modify_indirect_cmds.h"

#include "entry/entry.h"
#include "bx/timer.h"

#include "radiance_cascade/vkz_radiance_cascade.h"
#include "deferred/vkz_deferred.h"
#include "radiance_cascade/vkz_rc_debug.h"
#include "ffx_intg/brixel_intg_kage.h"
#include "radiance_cascade/vkz_rc2d.h"


// windows sdk
#include <time.h>
#include <cstddef>

namespace
{
    class Demo_MixRaster : public entry::AppI
    {
    public:
        Demo_MixRaster(
            const char* _name
            , const char* _description
            , const char* _url = "https://bkaradzic.github.io/bgfx/index.html"
        )
            : entry::AppI(_name, _description, _url)
        {

        }
        ~Demo_MixRaster()
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

            updatePyramid(m_pyramid, m_width, m_height, m_demoData.dbg_features.common.dbgPauseCullTransform);

            refreshData();

            updateMeshCulling(m_meshCullingEarly, m_demoData.constants, m_scene.drawCount);
            updateMeshCulling(m_meshCullingLate, m_demoData.constants, m_scene.drawCount);

            updateMeshletCulling(m_meshletCullingEarly, m_demoData.constants);
            updateMeshletCulling(m_meshletCullingLate, m_demoData.constants);

            updateTriangleCulling(m_triangleCullingEarly, m_demoData.constants);
            updateTriangleCulling(m_triangleCullingLate, m_demoData.constants);

            updateModifyIndirectCmds(m_modifyCmd4MeshletCullingEarly);
            updateModifyIndirectCmds(m_modifyCmd4MeshletCullingLate);
            updateModifyIndirectCmds(m_modifyCmd4TriangleCullingEarly);
            updateModifyIndirectCmds(m_modifyCmd4TriangleCullingLate);


            updateModifyIndirectCmds(m_modifyCmd4SoftRasterEarly, m_width, m_height);
            updateModifyIndirectCmds(m_modifyCmd4SoftRasterLate, m_width, m_height);

            updateSoftRaster(m_softRasterEarly);
            updateSoftRaster(m_softRasterLate);

            const kage::Memory* memTransform = kage::alloc(sizeof(TransformData));
            memcpy_s(memTransform->data, memTransform->size, &m_demoData.trans, sizeof(TransformData));
            kage::updateBuffer(m_transformBuf, memTransform);

            m_smaa.update(m_width, m_height);
            {
                m_demoData.dbg_features.brx.presentImg = kage::ImageHandle{};
                updateUI(m_ui, m_demoData.input, m_demoData.dbg_features, m_demoData.profiling, m_demoData.logic);
            }

            // render
            kage::render();

            // update stastic data
            static float avgCpuTime = 0.0f;
            avgCpuTime = avgCpuTime * 0.95f + (deltaTimeMS) * 0.05f;

            static float avgGpuTime = 0.0f;
            avgGpuTime = avgGpuTime * 0.95f + (float)kage::getGpuTime() * 0.05f;
            m_demoData.profiling.avgCpuTime = avgCpuTime;
            m_demoData.profiling.avgGpuTime = avgGpuTime;

            m_demoData.profiling.cullEarlyTime = (float)kage::getPassTime(m_meshCullingEarly.pass);
            m_demoData.profiling.cullLateTime = (float)kage::getPassTime(m_meshCullingLate.pass);

            m_demoData.profiling.mltCullEarlyTime = (float)kage::getPassTime(m_meshletCullingEarly.pass);
            m_demoData.profiling.mltCullLateTime = (float)kage::getPassTime(m_meshletCullingLate.pass);
            m_demoData.profiling.triCullEarlyTime = (float)kage::getPassTime(m_triangleCullingEarly.pass);
            m_demoData.profiling.triCullLateTime = (float)kage::getPassTime(m_triangleCullingLate.pass);
            m_demoData.profiling.softRasterEarlyTime = (float)kage::getPassTime(m_softRasterEarly.pass);
            m_demoData.profiling.softRasterLateTime = (float)kage::getPassTime(m_softRasterLate.pass);

            m_demoData.profiling.modify2MltCullEarly = (float)kage::getPassTime(m_modifyCmd4MeshletCullingEarly.pass);
            m_demoData.profiling.modify2MltCullLate = (float)kage::getPassTime(m_modifyCmd4MeshletCullingLate.pass);
            m_demoData.profiling.modify2TriCullEarly = (float)kage::getPassTime(m_modifyCmd4TriangleCullingEarly.pass);
            m_demoData.profiling.modify2TriCullLate = (float)kage::getPassTime(m_modifyCmd4TriangleCullingLate.pass);
            m_demoData.profiling.modify2SoftRasterEarly = (float)kage::getPassTime(m_modifyCmd4SoftRasterEarly.pass);
            m_demoData.profiling.modify2SoftRasterLate = (float)kage::getPassTime(m_modifyCmd4SoftRasterLate.pass);

            m_demoData.profiling.uiTime = (float)kage::getPassTime(m_ui.pass);
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
                kage::BufferDesc indirectCountBufDesc;
                indirectCountBufDesc.size = 16;
                indirectCountBufDesc.usage = kage::BufferUsageFlagBits::storage | kage::BufferUsageFlagBits::indirect | kage::BufferUsageFlagBits::transfer_dst;
                indirectCountBufDesc.memFlags = kage::MemoryPropFlagBits::device_local;
                m_indirectCountBuf = kage::registBuffer("indirectCount", indirectCountBufDesc);
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
            if (!m_scene.geometry.indices.empty())
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
            for (const ImageInfo& img : m_scene.images)
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
                rtDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage ;
                m_color = kage::registRenderTarget("color", rtDesc, kage::ResourceLifetime::non_transition);
            }

            {
                kage::ImageDesc dpDesc;
                dpDesc.depth = 1;
                dpDesc.numLayers = 1;
                dpDesc.numMips = 1;
                dpDesc.usage = kage::ImageUsageFlagBits::transfer_src | kage::ImageUsageFlagBits::transfer_dst | kage::ImageUsageFlagBits::sampled | kage::BufferUsageFlagBits::storage;
                m_depth = kage::registDepthStencil("depth", dpDesc, kage::ResourceLifetime::non_transition);
            }
        }

        void createPasses()
        {
            preparePyramid(m_pyramid, m_width, m_height);
            // == EARLY passes ==
            // 
            
            // mesh culling pass
            {
                MeshCullingInitData cullingInit{};
                cullingInit.meshBuf = m_meshBuf;
                cullingInit.meshDrawBuf = m_meshDrawBuf;
                cullingInit.transBuf = m_transformBuf;
                cullingInit.pyramid = m_pyramid.image;
                cullingInit.meshDrawCmdBuf = m_meshDrawCmdBuf;
                cullingInit.meshDrawCmdCountBuf = m_indirectCountBuf;
                cullingInit.meshDrawVisBuf = m_meshDrawVisBuf;

                initMeshCulling(m_meshCullingEarly, cullingInit, RenderStage::early, RenderPipeline::compute);
            }

            // modify mesh draw indirect command 
            {
                initModifyIndirectCmds(
                    m_modifyCmd4MeshletCullingEarly
                    , m_meshCullingEarly.cmdCountBufOutAlias
                    , m_meshCullingEarly.cmdBufOutAlias
                    , ModifyCommandMode::to_meshlet_cull
                );
            }

            // meshlet culling pass
            {
                MeshletCullingInitData meshletCullingInit{};
                meshletCullingInit.meshletCmdBuf = m_modifyCmd4MeshletCullingEarly.cmdBufOutAlias;
                meshletCullingInit.meshletCmdCntBuf = m_modifyCmd4MeshletCullingEarly.indirectCmdBufOutAlias;
                meshletCullingInit.meshBuf = m_meshBuf;
                meshletCullingInit.meshDrawBuf = m_meshDrawBuf;
                meshletCullingInit.transformBuf = m_transformBuf;
                meshletCullingInit.meshletBuf = m_meshletBuffer;
                meshletCullingInit.meshletVisBuf = m_meshletVisBuf;
                meshletCullingInit.pyramid = m_pyramid.image;
                initMeshletCulling(m_meshletCullingEarly, meshletCullingInit, RenderStage::early, kage::kSeamlessLod);
            }

            // modify meshlet culling indirect command 
            {
                initModifyIndirectCmds(
                    m_modifyCmd4TriangleCullingEarly
                    , m_meshletCullingEarly.cmdCountBufOutAlias
                    , m_meshletCullingEarly.cmdBufOutAlias
                    , ModifyCommandMode::to_triangle_cull
                );
            }

            // triangle culling pass
            {
                TriangleCullingInitData triangleCullingInit{};
                triangleCullingInit.meshletPayloadBuf = m_modifyCmd4TriangleCullingEarly.cmdBufOutAlias;
                triangleCullingInit.meshletPayloadCntBuf = m_modifyCmd4TriangleCullingEarly.indirectCmdBufOutAlias;
                triangleCullingInit.meshDrawBuf = m_meshDrawBuf;
                triangleCullingInit.transformBuf = m_transformBuf;
                triangleCullingInit.vtxBuf = m_vtxBuf;
                triangleCullingInit.meshletBuf = m_meshletBuffer;
                triangleCullingInit.meshletDataBuf = m_meshletDataBuffer;
                triangleCullingInit.pyramid = m_pyramid.image;
                initTriangleCulling(m_triangleCullingEarly, triangleCullingInit, RenderStage::early, kage::kSeamlessLod);
            }

            // triangle command modify
            {
                initModifyIndirectCmds(
                    m_modifyCmd4SoftRasterEarly
                    , m_triangleCullingEarly.triCountBufOutAlias
                    , m_triangleCullingEarly.hwTriBufOutAlias
                    , ModifyCommandMode::to_soft_rasterize
                    , m_width
                    , m_height
                );
            }

            // soft rasterization
            {
                SoftRasterDataInit initData{};

                initData.payloadBuf = m_modifyCmd4SoftRasterEarly.cmdBufOutAlias;
                initData.payloadCntBuf = m_modifyCmd4SoftRasterEarly.indirectCmdBufOutAlias;
                initData.pyramid = m_pyramid.image;

                initData.meshDrawBuf = m_meshDrawBuf;
                initData.transformBuf = m_transformBuf;
                initData.vtxBuf = m_vtxBuf;
                initData.meshletBuf = m_meshletBuffer;
                initData.meshletDataBuf = m_meshletDataBuffer;
                
                initData.width = m_width;
                initData.height = m_height;

                initData.color = m_color;
                initData.depth = m_depth;

                initData.renderStage = RenderStage::early;
                
                initSoftRaster(m_softRasterEarly, initData);
            }

            // pyramid pass
            setPyramidPassDependency(m_pyramid, m_softRasterEarly.depthOutAlias);

            // == LATE passes ==
            // 

            // mesh culling pass
            {
                MeshCullingInitData cullingInit{};
                cullingInit.meshDrawCmdBuf = m_modifyCmd4MeshletCullingEarly.cmdBufOutAlias;
                cullingInit.meshDrawCmdCountBuf = m_modifyCmd4MeshletCullingEarly.indirectCmdBufOutAlias;
                
                cullingInit.meshDrawVisBuf = m_meshCullingEarly.meshDrawVisBufOutAlias;
                cullingInit.meshBuf = m_meshBuf;
                cullingInit.meshDrawBuf = m_meshDrawBuf;
                cullingInit.transBuf = m_transformBuf;
                cullingInit.pyramid = m_pyramid.imgOutAlias;
                initMeshCulling(m_meshCullingLate, cullingInit, RenderStage::late, RenderPipeline::compute);
            }

            // mesh draw command indirect modify
            {
                initModifyIndirectCmds(
                    m_modifyCmd4MeshletCullingLate
                    , m_meshCullingLate.cmdCountBufOutAlias
                    , m_meshCullingLate.cmdBufOutAlias
                    , ModifyCommandMode::to_meshlet_cull
                );
            }

            {
                // meshlet culling pass
                MeshletCullingInitData meshletCullingInit{};
                meshletCullingInit.meshletVisBuf = m_meshletCullingEarly.meshletVisBufOutAlias;
                meshletCullingInit.meshletCmdBuf = m_modifyCmd4MeshletCullingLate.cmdBufOutAlias;
                meshletCullingInit.meshletCmdCntBuf = m_modifyCmd4MeshletCullingLate.indirectCmdBufOutAlias;

                meshletCullingInit.meshBuf = m_meshBuf;
                meshletCullingInit.meshDrawBuf = m_meshDrawBuf;
                meshletCullingInit.transformBuf = m_transformBuf;
                meshletCullingInit.meshletBuf = m_meshletBuffer;
                meshletCullingInit.pyramid = m_pyramid.imgOutAlias;

                initMeshletCulling(m_meshletCullingLate, meshletCullingInit, RenderStage::late, kage::kSeamlessLod);
            }

            // modify meshlet draw command indirect
            {
                initModifyIndirectCmds(
                    m_modifyCmd4TriangleCullingLate
                    , m_meshletCullingLate.cmdCountBufOutAlias
                    , m_meshletCullingLate.cmdBufOutAlias
                    , ModifyCommandMode::to_triangle_cull
                );
            }

            {
                // triangle culling pass
                TriangleCullingInitData triangleCullingInit{};
                triangleCullingInit.meshletPayloadBuf = m_modifyCmd4TriangleCullingLate.cmdBufOutAlias;
                triangleCullingInit.meshletPayloadCntBuf = m_modifyCmd4TriangleCullingLate.indirectCmdBufOutAlias;

                triangleCullingInit.meshDrawBuf = m_meshDrawBuf;
                triangleCullingInit.transformBuf = m_transformBuf;
                triangleCullingInit.vtxBuf = m_vtxBuf;
                triangleCullingInit.meshletBuf = m_meshletBuffer;
                triangleCullingInit.meshletDataBuf = m_meshletDataBuffer;
                triangleCullingInit.pyramid = m_pyramid.imgOutAlias;
                initTriangleCulling(m_triangleCullingLate, triangleCullingInit, RenderStage::late, kage::kSeamlessLod);
            }

            // modify soft raster command
            {
                initModifyIndirectCmds(
                    m_modifyCmd4SoftRasterLate
                    , m_triangleCullingLate.triCountBufOutAlias
                    , m_triangleCullingLate.hwTriBufOutAlias
                    , ModifyCommandMode::to_soft_rasterize
                    , m_width
                    , m_height
                );
            }

            // soft rasterization
            {
                SoftRasterDataInit initData{};

                initData.pyramid = m_pyramid.imgOutAlias;
                initData.payloadBuf = m_modifyCmd4SoftRasterLate.cmdBufOutAlias;
                initData.payloadCntBuf = m_modifyCmd4SoftRasterLate.indirectCmdBufOutAlias;

                initData.meshDrawBuf = m_meshDrawBuf;
                initData.transformBuf = m_transformBuf;
                initData.vtxBuf = m_vtxBuf;
                initData.meshletBuf = m_meshletBuffer;
                initData.meshletDataBuf = m_meshletDataBuffer;

                initData.vtxBuf = m_vtxBuf;
                initData.width = m_width;
                initData.height = m_height;

                initData.color = m_softRasterEarly.colorOutAlias;
                initData.depth = m_softRasterEarly.depthOutAlias;

                initData.renderStage = RenderStage::late;

                initSoftRaster(m_softRasterLate, initData);
            }

            // smaa
            {
                kage::ImageHandle aaDepthIn = m_softRasterLate.depthOutAlias;
                kage::ImageHandle aaColorIn = m_softRasterLate.colorOutAlias;

                m_smaa.prepare(m_width, m_height, aaColorIn, aaDepthIn);
            }

            // ui
            {
                kage::ImageHandle uiColorIn = m_smaa.m_outAliasImg;
                kage::ImageHandle uiDepthIn = m_softRasterLate.depthOutAlias;
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

            m_demoData.constants.P00 = s_proj[0][0];
            m_demoData.constants.P11 = s_proj[1][1];
            m_demoData.constants.znear = znear;
            m_demoData.constants.zfar = m_scene.drawDistance;
            m_demoData.constants.frustum[0] = s_fx.x;
            m_demoData.constants.frustum[1] = s_fx.z;
            m_demoData.constants.frustum[2] = s_fy.y;
            m_demoData.constants.frustum[3] = s_fy.z;
            m_demoData.constants.screenWidth = (float)m_width;
            m_demoData.constants.screenHeight = (float)m_height;
            m_demoData.constants.pyramidWidth = (float)m_pyramid.width;
            m_demoData.constants.pyramidHeight = (float)m_pyramid.height;
            m_demoData.constants.lodErrorThreshold = lodErrThreshold;
            m_demoData.constants.probeRangeRadius = m_demoData.dbg_features.rc3d.totalRadius;
            m_demoData.constants.enableCull = 1;
            m_demoData.constants.enableLod = kage::kRegularLod;
            m_demoData.constants.enableSeamlessLod = kage::kSeamlessLod;
            m_demoData.constants.enableOcclusion = 1;
            m_demoData.constants.enableMeshletOcclusion = 1;
        }


        Scene m_scene{};
        DemoData m_demoData{};
        bool m_supportMeshShading;
        bool m_debugProb;

        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_debug;
        uint32_t m_reset;
        entry::MouseState m_mouseState;

        kage::BufferHandle m_meshBuf;
        kage::BufferHandle m_meshDrawBuf;
        kage::BufferHandle m_meshDrawCmdBuf;
        kage::BufferHandle m_indirectCountBuf;
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
        std::vector<kage::ImageHandle> m_sceneImages;

        // passes
        MeshCulling m_meshCullingEarly{};
        MeshCulling m_meshCullingLate{};
        MeshletCulling m_meshletCullingEarly{};
        MeshletCulling m_meshletCullingLate{};
        TriangleCulling m_triangleCullingEarly{};
        TriangleCulling m_triangleCullingLate{};

        ModifyIndirectCmds m_modifyCmd4MeshletCullingEarly{};
        ModifyIndirectCmds m_modifyCmd4MeshletCullingLate{};

        ModifyIndirectCmds m_modifyCmd4TriangleCullingEarly{};
        ModifyIndirectCmds m_modifyCmd4TriangleCullingLate{};

        ModifyIndirectCmds m_modifyCmd4SoftRasterEarly{};
        ModifyIndirectCmds m_modifyCmd4SoftRasterLate{};

        SoftRaster m_softRasterEarly{};
        SoftRaster m_softRasterLate{};

        SMAA m_smaa{};
        Pyramid m_pyramid{};
        UIRendering m_ui{};
    };
}