#include "common.h"
#include "imgui.h"

#include "glm/glm.hpp"

#include "uidata.h"
#include "vkz_ui.h"

#include "bx/readerwriter.h"
#include "ffx_intg/brixel_structs.h"
#include "radiance_cascade/vkz_radiance_cascade.h"


constexpr uint32_t kInitialVertexBufferSize = 1024 * 1024; // 1MB
constexpr uint32_t kInitialIndexBufferSize = 1024 * 1024; // 1MB

using Stage = kage::PipelineStageFlagBits::Enum;

struct PushConstBlock {
    glm::vec2 scale;
    glm::vec2 translate;
};

void recordUI(const UIRendering& _ui)
{
    KG_ZoneScopedC(kage::Color::blue);

    kage::startRec(_ui.pass);

    ImDrawData* imDrawData = ImGui::GetDrawData();

    if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
        kage::endRec();
        return;
    }

    kage::setViewport(
        (int32_t)imDrawData->DisplayPos.x
        , (int32_t)imDrawData->DisplayPos.y
        , (uint32_t)imDrawData->DisplaySize.x
        , (uint32_t)imDrawData->DisplaySize.y
    );

    kage::setVertexBuffer(_ui.vb);
    kage::setIndexBuffer(_ui.ib, 0, sizeof(ImDrawIdx) == 2 ? kage::IndexType::uint16 : kage::IndexType::uint32);

    const ImGuiIO& io = ImGui::GetIO();
    PushConstBlock c{};
    c.scale = { 2.f / io.DisplaySize.x, -2.f / io.DisplaySize.y };
    c.translate = { -1.f, 1.f }; // translate from x: [0,2] to [-1,1], y: [0,2] to [1,-1]
    const kage::Memory* mem = kage::alloc(sizeof(PushConstBlock));
    memcpy(mem->data, &c, mem->size);
    kage::setConstants(mem);

    int32_t vtxOffset = 0;
    int32_t idxOffset = 0;
    for (int32_t i = 0; i < imDrawData->CmdListsCount; ++i)
    {
        const ImDrawList* imCmdList = imDrawData->CmdLists[i];
        for (int32_t j = 0; j < imCmdList->CmdBuffer.Size; ++j)
        {
            const ImDrawCmd& cmd = imCmdList->CmdBuffer[j];

            uint32_t x = bx::max((int32_t)(cmd.ClipRect.x), 0);
            uint32_t y = bx::max((int32_t)(cmd.ClipRect.y), 0);
            uint32_t w = (uint32_t)(cmd.ClipRect.z - cmd.ClipRect.x);
            uint32_t h = (uint32_t)(cmd.ClipRect.w - cmd.ClipRect.y);

            BX_ASSERT((uint64_t)cmd.TextureId < UINT16_MAX, "the texture Id should be a valid kage::ImageHandle");

            kage::ImageHandle img = 
                (cmd.TextureId == 0) ? _ui.fontImage : kage::ImageHandle{ (uint16_t)((uint64_t)cmd.TextureId & 0xffff) };

            kage::Binding binds[] = {
                {img, _ui.sampler, Stage::fragment_shader}, 
            };

            kage::pushBindings(binds, COUNTOF(binds));

            kage::setScissor(x, y, w, h);

            kage::Attachment attachments[] = {
                {_ui.color, kage::AttachmentLoadOp::dont_care, kage::AttachmentStoreOp::store},
            };
            kage::setColorAttachments(attachments, COUNTOF(attachments));


            kage::Attachment depthAttachment = { _ui.depth, kage::AttachmentLoadOp::dont_care, kage::AttachmentStoreOp::dont_care };
            kage::setDepthAttachment(depthAttachment);

            kage::drawIndexed(cmd.ElemCount, 1, idxOffset, vtxOffset, 0);

            idxOffset += cmd.ElemCount;
        }
        vtxOffset += imCmdList->VtxBuffer.Size;
    }

    kage::endRec();
}

void prepareUI(UIRendering& _ui, kage::ImageHandle _color, kage::ImageHandle _depth, float _scale /*= 1.f*/, bool _useChinese /*= false*/)
{
    KG_ZoneScopedC(kage::Color::blue);

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = _scale;

    kage::ShaderHandle vs = kage::registShader("ui.vs", "shaders/ui.vert.spv");
    kage::ShaderHandle fs = kage::registShader("ui.fs", "shaders/ui.frag.spv");

    kage::ProgramHandle program = kage::registProgram("ui.program", { vs, fs }, sizeof(PushConstBlock));

    kage::VertexBindingDesc bindings[] =
    {
        {0, sizeof(ImDrawVert), kage::VertexInputRate::vertex},
    };

    kage::VertexAttributeDesc attributes[] =
    {
        {0, 0, kage::ResourceFormat::r32g32_sfloat, offsetof(ImDrawVert, pos)},
        {1, 0, kage::ResourceFormat::r32g32_sfloat, offsetof(ImDrawVert, uv)},
        {2, 0, kage::ResourceFormat::r8g8b8a8_unorm, offsetof(ImDrawVert, col)},
    };

    const kage::Memory* vtxBindingMem = kage::alloc(uint32_t(sizeof(kage::VertexBindingDesc) * COUNTOF(bindings)));
    memcpy(vtxBindingMem->data, bindings, sizeof(kage::VertexBindingDesc) * COUNTOF(bindings));

    const kage::Memory* vtxAttributeMem = kage::alloc(uint32_t(sizeof(kage::VertexAttributeDesc) * COUNTOF(attributes)));
    memcpy(vtxAttributeMem->data, attributes, sizeof(kage::VertexAttributeDesc) * COUNTOF(attributes));

    kage::PassDesc passDesc;
    passDesc.programId = program.id;
    passDesc.queue = kage::PassExeQueue::graphics;
    passDesc.vertexBindingNum = (uint32_t)COUNTOF(bindings);
    passDesc.vertexBindings = vtxBindingMem->data;
    passDesc.vertexAttributeNum = (uint32_t)COUNTOF(attributes);
    passDesc.vertexAttributes = vtxAttributeMem->data;
    passDesc.pipelineConfig = { true, true, kage::CompareOp::always };

    kage::PassHandle pass = kage::registPass("ui_pass", passDesc);

    // create font texture
    unsigned char* fontData = nullptr;
    int width = 0, height = 0;

    if (_useChinese)
        io.Fonts->AddFontFromFileTTF("./data/fonts/SmileySans-Oblique.ttf", 16.f * io.FontGlobalScale, 0, _useChinese ? io.Fonts->GetGlyphRangesChineseSimplifiedCommon() : 0);
    else
        io.Fonts->AddFontFromFileTTF("./data/fonts/consola.ttf", 16.f * io.FontGlobalScale);

    io.Fonts->GetTexDataAsRGBA32(&fontData, &width, &height);
    assert(fontData);
    size_t uploadSize = width * height * 4 * sizeof(char);

    const kage::Memory* fontImgMem = kage::alloc((uint32_t)uploadSize);
    memcpy(fontImgMem->data, fontData, uploadSize);

    kage::ImageDesc imageDesc{};
    imageDesc.width = width;
    imageDesc.height = height;
    imageDesc.numMips = 1;
    imageDesc.format = kage::ResourceFormat::r8g8b8a8_unorm;
    imageDesc.usage = kage::ImageUsageFlagBits::sampled | kage::ImageUsageFlagBits::transfer_dst;
    kage::ImageHandle fontImage = kage::registTexture("ui.fontImage", imageDesc, fontImgMem, kage::ResourceLifetime::non_transition);

    kage::BufferDesc vbDesc{};
    vbDesc.size = kInitialVertexBufferSize;
    vbDesc.fillVal = 0;
    vbDesc.usage = kage::BufferUsageFlagBits::vertex | kage::BufferUsageFlagBits::transfer_dst;
    vbDesc.memFlags = kage::MemoryPropFlagBits::host_visible | kage::MemoryPropFlagBits::host_coherent;
    kage::BufferHandle vb = kage::registBuffer("ui.vb", vbDesc, nullptr, kage::ResourceLifetime::non_transition);

    kage::BufferDesc ibDesc{};
    ibDesc.size = kInitialIndexBufferSize;
    ibDesc.fillVal = 0;
    ibDesc.usage = kage::BufferUsageFlagBits::index | kage::BufferUsageFlagBits::transfer_dst;
    ibDesc.memFlags = kage::MemoryPropFlagBits::host_visible | kage::MemoryPropFlagBits::host_coherent;
    kage::BufferHandle ib = kage::registBuffer("ui.ib", ibDesc, nullptr, kage::ResourceLifetime::non_transition);

    kage::ImageHandle colorOutAlias = kage::alias(_color);
    kage::ImageHandle depthOutAlias = kage::alias(_depth);

    _ui.vs = vs;
    _ui.fs = fs;
    _ui.program = program;
    _ui.pass = pass;
    _ui.vb = vb;
    _ui.ib = ib;
    _ui.fontImage = fontImage;
    _ui.color = _color;
    _ui.depth = _depth;
    _ui.colorOutAlias = colorOutAlias;
    _ui.depthOutAlias = depthOutAlias;

    kage::bindIndexBuffer(pass, ib); // TODO: bind in the custom func already, this is for frame-graph
    kage::bindVertexBuffer(pass, vb);
    _ui.sampler = kage::sampleImage(pass, fontImage
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::setAttachmentOutput(_ui.pass, _color, colorOutAlias);
    kage::setAttachmentOutput(_ui.pass, _depth, depthOutAlias);
}

void destroyUI(UIRendering& _ui)
{
    KG_ZoneScopedC(kage::Color::blue);;

    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }

    _ui = {};
}

void updateImGuiIO(const UIInput& input)
{
    KG_ZoneScopedC(kage::Color::blue);;

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)input.width, (float)input.height);

    io.MousePos = ImVec2(input.mousePosx, input.mousePosy);
    io.MouseDown[0] = input.mouseButtons.left;
    io.MouseDown[1] = input.mouseButtons.right;
    io.MouseDown[2] = input.mouseButtons.middle;
}

void updataContentRCBuild(Dbg_RadianceCascades& _rc)
{
    KG_ZoneScopedC(kage::Color::blue);
    ImGui::Begin("rc build:");


    static const char* const debug_idx_types[(uint32_t)RCDbgIndexType::count] = { "probe", "ray"};
    static const char* const debug_color_types[(uint32_t)RCDbgColorType::count] = { "albedo", "normal", "world pos", "emmision", "hit distance", "hit normal", "ray dir", "probe center", "probe hash"};
    
    ImGui::SliderFloat("brx_tmin", &_rc.brx_tmin, 0.f, 100.f);
    ImGui::SliderFloat("brx_tmax", &_rc.brx_tmax, 1.f, 1000.f);

    ImGui::SliderInt("brx_offset", (int*)&_rc.brx_offset, 0, 100);
    ImGui::SliderInt("brx_startCas", (int*)&_rc.brx_startCas, 0, kage::k_brixelizerCascadeCount - 1);
    ImGui::SliderInt("brx_endCas", (int*)&_rc.brx_endCas, 0, kage::k_brixelizerCascadeCount - 1);
    ImGui::SliderFloat("brx_sdfEps", &_rc.brx_sdfEps, 0.01f, 10.f);
    
    ImGui::SliderFloat("totalRadius", &_rc.totalRadius, 0.f, 1000.f);

    ImGui::Combo("idx type", (int*)&_rc.idx_type, debug_idx_types, COUNTOF(debug_idx_types));
    ImGui::Combo("color type", (int*)&_rc.color_type, debug_color_types, COUNTOF(debug_color_types));

    ImGui::Text("rc:");
    ImGui::SliderFloat3("probeCenter", &_rc.probePosOffset[0], -30.f, 30.f);
    ImGui::SliderFloat("probeDebugScale", &_rc.probeDebugScale, 0.01f, 1.f);

    ImGui::SliderInt("start cas", (int*)&_rc.startCascade, 0, 8);
    ImGui::SliderInt("cas count", (int*)&_rc.cascadeCount, 1, 8);
    ImGui::Checkbox("follow Cam", &_rc.followCam);
    ImGui::Checkbox("pause update", &_rc.pauseUpdate);  
    
    ImGui::End();
}

void updateRc2d(Dbg_Rc2d& _rc2d)
{
    KG_ZoneScopedC(kage::Color::blue);
    ImGui::Begin("rc2d:");

    static const char* const debug_stage_types[(uint32_t)Rc2dStage::count] = { "use", "build", "merge_r", "merge_p"};

    ImGui::Combo("stage", (int*)&_rc2d.stage, debug_stage_types, COUNTOF(debug_stage_types));
    ImGui::SliderInt("lv", (int*)&_rc2d.lv, 0, 8);
    ImGui::SliderFloat("c0 ray len", &_rc2d.c0_rLen, 4.f, 40.f);
    ImGui::Checkbox("arrow", &_rc2d.showArrow);
    ImGui::Checkbox("show c0 border", &_rc2d.show_c0_Border);

    ImGui::End();
}

void updateContentBrx(Dbg_Brixel& _brx)
{
    KG_ZoneScopedC(kage::Color::blue);

    ImGui::Begin("brx:");

    const char* const items[(uint32_t)BrixelDebugType::count] = { "distance", "uvw", "iterations", "grad", "brick_id", "cascade_id" };

    ImGui::Combo("type", (int*)&_brx.debugBrixelType, items, COUNTOF(items));
    ImGui::SliderInt("start cas", (int*)&_brx.startCas, 0, kage::k_brixelizerCascadeCount - 1);
    ImGui::SliderInt("end cas", (int*)&_brx.endCas, 0, kage::k_brixelizerCascadeCount - 1);
    ImGui::SliderFloat("sdf eps", &_brx.sdfEps, 0.1f, 10.f);
    ImGui::SliderFloat("tmin", &_brx.tmin, .2f, 10.f);
    ImGui::SliderFloat("tmax", &_brx.tmax, 1000.f, 10000.f);
    ImGui::Checkbox("follow Cam", &_brx.followCam);
    ImGui::Image((ImTextureID)(_brx.presentImg.id), { 640, 360 });

    ImGui::End();
}

void updateContentCommon(Dbg_Common& _common, const DebugProfilingData& _pd, const DebugLogicData& _ld)
{
    KG_ZoneScopedC(kage::Color::blue);

    ImGui::NewFrame();
    ImGui::SetNextWindowSize({ 400, 450 }, ImGuiCond_FirstUseEver);
    ImGui::Begin("info:");

    ImGui::Text("fps: [%.2f]", 1000.f / _pd.avgCpuTime);

    ImGui::Checkbox("brx", &_common.dbgBrx);
    ImGui::Checkbox("rc3d", &_common.dbgRc3d);
    ImGui::Checkbox("rc2d", &_common.dbgRc2d);

    ImGui::Checkbox("pause cull transform", &_common.dbgPauseCullTransform);

    ImGui::Text("avg cpu: [%.3f]ms", _pd.avgCpuTime);
    ImGui::Text("avg gpu: [%.3f]ms", _pd.avgGpuTime);
    ImGui::Text("cull early: [%.3f]ms", _pd.cullEarlyTime);
    ImGui::Text("draw early: [%.3f]ms", _pd.drawEarlyTime);

    if (_common.ocEnabled)
    {
        ImGui::Text("pyramid: [%.3f]ms", _pd.pyramidTime);
        ImGui::Text("cull late: [%.3f]ms", _pd.cullLateTime);
        ImGui::Text("draw late: [%.3f]ms", _pd.drawLateTime);
    }


    ImGui::Text("ui: [%.3f]ms", _pd.uiTime);
    ImGui::Text("deferred: [%.3f]ms", _pd.deferredTime);
    ImGui::Text("build cascade: [%.3f]ms", _pd.buildCascadeTime);
    ImGui::Text("merge cascade ray: [%.3f]ms", _pd.mergeCascadeRayTime);
    ImGui::Text("build cascade probe: [%.3f]ms", _pd.mergeCascadeProbeTime);
    ImGui::Text("debug probe gen: [%.3f]ms", _pd.debugProbeGenTime);
    ImGui::Text("debug probe draw: [%.3f]ms", _pd.debugProbeDrawTime);

    if (ImGui::TreeNode("Static Data:"))
    {
        ImGui::Text("primitives : [%d]", _pd.primitiveCount);
        ImGui::Text("meshlets: [%d]", _pd.meshletCount);
        ImGui::Text("tri E: [%.3f]M", _pd.triangleEarlyCount * 1e-6);
        ImGui::Text("tri L: [%.3f]M", _pd.triangleLateCount * 1e-6);
        ImGui::Text("triangles: [%.3f]M", _pd.triangleCount * 1e-6);
        ImGui::Text("tri/sec: [%.2f]B", (1000.f / _pd.avgCpuTime) * _pd.triangleCount * 1e-9);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Options"))
    {
        ImGui::Checkbox("Mesh Shading", &_common.meshShadingEnabled);
        ImGui::Checkbox("Cull", &_common.objCullEnabled);
        ImGui::Checkbox("Lod", &_common.lodEnabled);
        ImGui::Checkbox("Occlusion", &_common.ocEnabled);
        ImGui::Checkbox("Task Submit", &_common.taskSubmitEnabled);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("camera:"))
    {
        ImGui::SliderFloat("speed", &_common.speed, 0.01f, 3.f);
        ImGui::Text("pos: %.2f, %.2f, %.2f", _ld.posX, _ld.posY, _ld.posZ);
        ImGui::Text("dir: %.2f, %.2f, %.2f", _ld.frontX, _ld.frontY, _ld.frontZ);
        ImGui::TreePop();
    }

    ImGui::End();
}

void updateImGuiContent(DebugFeatures& _ft, const DebugProfilingData& _pd, const DebugLogicData& _ld)
{
    KG_ZoneScopedC(kage::Color::blue);

    updateContentCommon(_ft.common, _pd, _ld);

    if(_ft.common.dbgBrx)
        updateContentBrx(_ft.brx);

    if (_ft.common.dbgRc3d)
        updataContentRCBuild(_ft.rc3d);

    if (_ft.common.dbgRc2d)
        updateRc2d(_ft.rc2d);
}

void updateImGui(const UIInput& input, DebugFeatures& ft, const DebugProfilingData& pd, const DebugLogicData& ld)
{
    KG_ZoneScopedC(kage::Color::blue);

    updateImGuiIO(input);

    updateImGuiContent(ft, pd, ld);

    ImGui::Render();
}

void updateUIRenderData(UIRendering& _ui)
{
    KG_ZoneScopedC(kage::Color::blue);;

    ImDrawData* imDrawData = ImGui::GetDrawData();

    if (!imDrawData) {
        return;
    }

    size_t vbSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    size_t ibSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    if (vbSize == 0 || ibSize == 0)
        return;

    // TODO: fix the hard-code to fit the atomic no coherent memory.
    vbSize += (0x40 - (vbSize % 0x40));
    ibSize += (0x40 - (ibSize % 0x40));

    assert((vbSize % 0x40 == 0) && (ibSize % 0x40 == 0));

    
    const kage::Memory* vbMem = kage::alloc((uint32_t)vbSize);
    const kage::Memory* ibMem = kage::alloc((uint32_t)ibSize);

    uint32_t vbOffset = 0;
    uint32_t ibOffset = 0;
    for (int32_t ii = 0; ii < imDrawData->CmdListsCount; ++ii) {
        const ImDrawList* imCmdList = imDrawData->CmdLists[ii];
        memcpy(vbMem->data + vbOffset, imCmdList->VtxBuffer.Data, imCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(ibMem->data + ibOffset, imCmdList->IdxBuffer.Data, imCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vbOffset += imCmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        ibOffset += imCmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
    }

    kage::updateBuffer(_ui.vb, vbMem);
    kage::updateBuffer(_ui.ib, ibMem);
}

void updateUI(
    UIRendering& _ui
    , const UIInput& _input
    , DebugFeatures& _features
    , const DebugProfilingData& _pd
    , const DebugLogicData& _ld
)
{
    updateImGui(_input, _features, _pd, _ld);
    updateUIRenderData(_ui);
    recordUI(_ui);
}
