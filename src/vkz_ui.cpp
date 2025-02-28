#include "common.h"
#include "imgui.h"

#include "glm/glm.hpp"

#include "uidata.h"
#include "vkz_ui.h"

#include "bx/readerwriter.h"


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

    kage::Binding binds[] = {
        {_ui.fontImage, _ui.fontSampler, Stage::fragment_shader}
        , {_ui.dummyColor, _ui.fontSampler, Stage::fragment_shader}
    };
    
    kage::pushBindings(binds, COUNTOF(binds));

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
    _ui.fontSampler = kage::sampleImage(pass, fontImage
        , 0
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    _ui.dummySampler = kage::sampleImage(pass, _ui.dummyColor
        , 1
        , kage::PipelineStageFlagBits::fragment_shader
        , kage::SamplerFilter::linear
        , kage::SamplerMipmapMode::linear
        , kage::SamplerAddressMode::clamp_to_edge
        , kage::SamplerReductionMode::weighted_average
    );

    kage::setAttachmentOutput(_ui.pass, _color, 0, colorOutAlias);
    kage::setAttachmentOutput(_ui.pass, _depth, 0, depthOutAlias);
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

void updateImGuiContent(DebugRenderOptionsData& _rod, const DebugProfilingData& _pd, const DebugLogicData& _ld)
{
    KG_ZoneScopedC(kage::Color::blue);;

    ImGui::NewFrame();
    ImGui::SetNextWindowSize({ 400, 450 }, ImGuiCond_FirstUseEver);
    ImGui::Begin("info:");

    ImGui::Text("fps: [%.2f]", 1000.f / _pd.avgCpuTime);
    ImGui::Text("avg cpu: [%.3f]ms", _pd.avgCpuTime);
    ImGui::Text("avg gpu: [%.3f]ms", _pd.avgGpuTime);
    ImGui::Text("cull early: [%.3f]ms", _pd.cullEarlyTime);
    ImGui::Text("draw early: [%.3f]ms", _pd.drawEarlyTime);
    
    if (_rod.ocEnabled)
    {
        ImGui::Text("pyramid: [%.3f]ms", _pd.pyramidTime);
        ImGui::Text("cull late: [%.3f]ms", _pd.cullLateTime);
        ImGui::Text("draw late: [%.3f]ms", _pd.drawLateTime);
    }

    ImGui::Text("ui: [%.3f]ms", _pd.uiTime);
    ImGui::Text("deferred: [%.3f]ms", _pd.deferredTime);
    ImGui::Text("voxelization: [%.3f]ms", _pd.voxelizationTime);
    ImGui::Text("build cascade: [%.3f]ms", _pd.buildCascadeTime);

    ImGui::SliderInt("cas lv", &_rod.debugCascadeLevel, 0, kage::k_rclv0_cascadeLv - 1);

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
        ImGui::Checkbox("Mesh Shading", &_rod.meshShadingEnabled);
        ImGui::Checkbox("Cull", &_rod.objCullEnabled);
        ImGui::Checkbox("Lod", &_rod.lodEnabled);
        ImGui::Checkbox("Occlusion", &_rod.ocEnabled);
        ImGui::Checkbox("Task Submit", &_rod.taskSubmitEnabled);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("camera:"))
    {
        ImGui::Text("pos: %.2f, %.2f, %.2f", _ld.posX, _ld.posY, _ld.posZ);
        ImGui::Text("dir: %.2f, %.2f, %.2f", _ld.frontX, _ld.frontY, _ld.frontZ);
        ImGui::TreePop();
    }

    ImGui::Text("pos: %.2f, %.2f, %.2f", _ld.posX, _ld.posY, _ld.posZ);
    ImGui::Text("dir: %.2f, %.2f, %.2f", _ld.frontX, _ld.frontY, _ld.frontZ);

    ImGui::End();
}

void updateImGui(const UIInput& input, DebugRenderOptionsData& rd, const DebugProfilingData& pd, const DebugLogicData& ld)
{
    KG_ZoneScopedC(kage::Color::blue);

    updateImGuiIO(input);

    updateImGuiContent(rd, pd, ld);

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
    , DebugRenderOptionsData& _rd
    , const DebugProfilingData& _pd
    , const DebugLogicData& _ld
)
{
    updateImGui(_input, _rd, _pd, _ld);
    updateUIRenderData(_ui);
    recordUI(_ui);
}
