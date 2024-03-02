#include "common.h"
#include "imgui.h"

#include "glm/glm.hpp"

#include "uidata.h"
#include "vkz_ui.h"

#include "memory_operation.h"

constexpr uint32_t kInitialVertexBufferSize = 1024 * 1024; // 1MB
constexpr uint32_t kInitialIndexBufferSize = 1024 * 1024; // 1MB

void ui_renderFunc(vkz::CommandListI& _cmdList, const void* _data, uint32_t _size)
{
    VKZ_ZoneScopedC(vkz::Color::cyan);

    vkz::MemoryReader reader(_data, _size);

    UIRendering ui{};
    vkz::read(&reader, ui);
    ImDrawData* imDrawData = ImGui::GetDrawData();

    if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
        return;
    }

    _cmdList.beginRendering(ui.pass);

    vkz::Viewport viewport = { 0.f, 0.f, imDrawData->DisplaySize.x, imDrawData->DisplaySize.y, 0.f, 1.f };
    vkz::Rect2D scissor = { {0, 0}, {uint32_t(imDrawData->DisplaySize.x), uint32_t(imDrawData->DisplaySize.y)} };

    _cmdList.setViewPort(0, 1, &viewport);
    _cmdList.setScissorRect(0, 1, &scissor);

    vkz::BufferHandle vertex_buffers[1] = { ui.vb };
    uint64_t vertex_offset[1] = { 0 };
    _cmdList.bindVertexBuffer(0, 1, vertex_buffers, vertex_offset);
    _cmdList.bindIndexBuffer(ui.ib, 0, sizeof(ImDrawIdx) == 2 ? vkz::IndexType::uint16 : vkz::IndexType::uint32);

    const ImGuiIO& io = ImGui::GetIO();
    PushConstBlock pushConstBlock{};
    pushConstBlock.scale = { 2.f / io.DisplaySize.x, 2.f / io.DisplaySize.y };
    pushConstBlock.translate = { -1.f, -1.f };

    _cmdList.pushConstants(ui.pass, &pushConstBlock, sizeof(PushConstBlock));

    _cmdList.pushDescriptorSets(ui.pass);

    int32_t vtxOffset = 0;
    int32_t idxOffset = 0;

    for (int32_t i = 0; i < imDrawData->CmdListsCount; ++i)
    {
        const ImDrawList* imCmdList = imDrawData->CmdLists[i];
        for (int32_t j = 0; j < imCmdList->CmdBuffer.Size; ++j)
        {
            const ImDrawCmd& cmd = imCmdList->CmdBuffer[j];

            vkz::Rect2D scissor;
            scissor.offset.x = glm::max((int32_t)(cmd.ClipRect.x), 0);
            scissor.offset.y = glm::max((int32_t)(cmd.ClipRect.y), 0);
            scissor.extent.width = (uint32_t)(cmd.ClipRect.z - cmd.ClipRect.x);
            scissor.extent.height = (uint32_t)(cmd.ClipRect.w - cmd.ClipRect.y);

            _cmdList.setScissorRect(0, 1, &scissor);
            _cmdList.drawIndexed(cmd.ElemCount, 1, idxOffset, vtxOffset, 0);

            idxOffset += cmd.ElemCount;
        }
        vtxOffset += imCmdList->VtxBuffer.Size;
    }

    _cmdList.endRendering();
}

void vkz_prepareUI(UIRendering& _ui, vkz::ImageHandle _color, vkz::ImageHandle _depth, float _scale /*= 1.f*/, bool _useChinese /*= false*/)
{
    VKZ_ZoneScopedC(vkz::Color::blue);

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = _scale;

    vkz::ShaderHandle vs = vkz::registShader("ui.vs", "shaders/ui.vert.spv");
    vkz::ShaderHandle fs = vkz::registShader("ui.fs", "shaders/ui.frag.spv");

    vkz::ProgramHandle program = vkz::registProgram("ui.program", { vs, fs }, sizeof(PushConstBlock));

    vkz::VertexBindingDesc bindings[] =
    {
        {0, sizeof(ImDrawVert), vkz::VertexInputRate::vertex},
    };

    vkz::VertexAttributeDesc attributes[] =
    {
        {0, 0, vkz::ResourceFormat::r32g32_sfloat, offsetof(ImDrawVert, pos)},
        {1, 0, vkz::ResourceFormat::r32g32_sfloat, offsetof(ImDrawVert, uv)},
        {2, 0, vkz::ResourceFormat::r8g8b8a8_unorm, offsetof(ImDrawVert, col)},
    };

    const vkz::Memory* vtxBindingMem = vkz::alloc(uint32_t(sizeof(vkz::VertexBindingDesc) * COUNTOF(bindings)));
    memcpy(vtxBindingMem->data, bindings, sizeof(vkz::VertexBindingDesc) * COUNTOF(bindings));

    const vkz::Memory* vtxAttributeMem = vkz::alloc(uint32_t(sizeof(vkz::VertexAttributeDesc) * COUNTOF(attributes)));
    memcpy(vtxAttributeMem->data, attributes, sizeof(vkz::VertexAttributeDesc) * COUNTOF(attributes));

    vkz::PassDesc passDesc;
    passDesc.programId = program.id;
    passDesc.queue = vkz::PassExeQueue::graphics;
    passDesc.vertexBindingNum = (uint32_t)COUNTOF(bindings);
    passDesc.vertexBindings = vtxBindingMem->data;
    passDesc.vertexAttributeNum = (uint32_t)COUNTOF(attributes);
    passDesc.vertexAttributes = vtxAttributeMem->data;
    passDesc.pipelineConfig = { true, true, vkz::CompareOp::always };
    
    passDesc.passConfig.colorLoadOp = vkz::AttachmentLoadOp::load;
    passDesc.passConfig.colorStoreOp = vkz::AttachmentStoreOp::store;
    passDesc.passConfig.depthLoadOp = vkz::AttachmentLoadOp::load;
    passDesc.passConfig.depthStoreOp = vkz::AttachmentStoreOp::dont_care;

    vkz::PassHandle pass = vkz::registPass("ui_pass", passDesc);

    // create font texture
    unsigned char* fontData = nullptr;
    int width = 0, height = 0;

    if (_useChinese)
        io.Fonts->AddFontFromFileTTF("../data/fonts/SmileySans-Oblique.ttf", 16.f * io.FontGlobalScale, 0, _useChinese ? io.Fonts->GetGlyphRangesChineseSimplifiedCommon() : 0);
    else
        io.Fonts->AddFontFromFileTTF("../data/fonts/consola.ttf", 16.f * io.FontGlobalScale);

    io.Fonts->GetTexDataAsRGBA32(&fontData, &width, &height);
    assert(fontData);
    size_t uploadSize = width * height * 4 * sizeof(char);

    const vkz::Memory* fontImgMem = vkz::alloc((uint32_t)uploadSize);
    memcpy(fontImgMem->data, fontData, uploadSize);

    vkz::ImageDesc imageDesc{};
    imageDesc.width = width;
    imageDesc.height = height;
    imageDesc.mipLevels = 1;
    imageDesc.format = vkz::ResourceFormat::r8g8b8a8_unorm;
    imageDesc.usage = vkz::ImageUsageFlagBits::sampled | vkz::ImageUsageFlagBits::transfer_dst;
    imageDesc.size = fontImgMem->size;
    imageDesc.data = fontImgMem->data;
    vkz::ImageHandle fontImage = vkz::registTexture("ui.fontImage", imageDesc, vkz::ResourceLifetime::non_transition);

    vkz::BufferDesc vbDesc{};
    vbDesc.size = kInitialVertexBufferSize;
    vbDesc.fillVal = 0;
    vbDesc.usage = vkz::BufferUsageFlagBits::vertex | vkz::BufferUsageFlagBits::transfer_dst;
    vkz::BufferHandle vb = vkz::registBuffer("ui.vb", vbDesc, vkz::ResourceLifetime::non_transition);

    vkz::BufferDesc ibDesc{};
    ibDesc.size = kInitialIndexBufferSize;
    ibDesc.fillVal = 0;
    ibDesc.usage = vkz::BufferUsageFlagBits::index | vkz::BufferUsageFlagBits::transfer_dst;
    vkz::BufferHandle ib = vkz::registBuffer("ui.ib", ibDesc, vkz::ResourceLifetime::non_transition);

    vkz::ImageHandle colorOutAlias = vkz::alias(_color);
    vkz::ImageHandle depthOutAlias = vkz::alias(_depth);

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

    const vkz::Memory* mem = vkz::alloc(sizeof(UIRendering));
    memcpy(mem->data, &_ui, mem->size);

    vkz::bindIndexBuffer(pass, ib); // TODO: bind in the custom func already, this is for frame-graph
    vkz::bindVertexBuffer(pass, vb);
    vkz::sampleImage(pass, fontImage
        , 0
        , vkz::PipelineStageFlagBits::fragment_shader
        , vkz::ImageLayout::general
        , vkz::SamplerReductionMode::weighted_average
    );

    vkz::setCustomRenderFunc(pass, ui_renderFunc, mem);

    vkz::setAttachmentOutput(_ui.pass, _color, 0, colorOutAlias);
    vkz::setAttachmentOutput(_ui.pass, _depth, 0, depthOutAlias);
}

void vkz_destroyUIRendering(UIRendering& _ui)
{
    VKZ_ZoneScopedC(vkz::Color::blue);;

    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }

    _ui = {};
}

void vkz_updateImGuiIO(const Input& input)
{
    VKZ_ZoneScopedC(vkz::Color::blue);;

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)input.width, (float)input.height);

    io.MousePos = ImVec2(input.mousePosx, input.mousePosy);
    io.MouseDown[0] = input.mouseButtons.left;
    io.MouseDown[1] = input.mouseButtons.right;
    io.MouseDown[2] = input.mouseButtons.middle;
}

void vkz_updateImGuiContent(RenderOptionsData& _rod, const ProfilingData& _pd, const LogicData& _ld)
{
    VKZ_ZoneScopedC(vkz::Color::blue);;

    ImGui::NewFrame();
    ImGui::SetNextWindowSize({ 400, 450 }, ImGuiCond_FirstUseEver);
    ImGui::Begin("info:");

    ImGui::Text("cpu: [%.3f]ms", _pd.cpuTime);
    ImGui::Text("avg cpu: [%.3f]ms", _pd.avgCpuTime);
    ImGui::Text("gpu: [%.3f]ms", _pd.gpuTime);
    ImGui::Text("avg gpu: [%.3f]ms", _pd.avgGpuTime);
    ImGui::Text("cull: [%.3f]ms", _pd.cullTime);
    ImGui::Text("draw: [%.3f]ms", _pd.drawTime);

    ImGui::Text("ui: [%.3f]ms", _pd.uiTime);
    ImGui::Text("wait: [%.3f]ms", _pd.waitTime);
    
    if (_rod.ocEnabled)
    {
        ImGui::Text("pyramid: [%.3f]ms", _pd.pyramidTime);
        ImGui::Text("late cull: [%.3f]ms", _pd.lateCullTime);
        ImGui::Text("render L: [%.3f]ms", _pd.lateRender);
    }

    if (ImGui::TreeNode("Static Data:"))
    {
        ImGui::Text("primitives : [%d]", _pd.primitiveCount);
        ImGui::Text("meshlets: [%d]", _pd.meshletCount);
        ImGui::Text("tri E: [%.3f]M", _pd.triangleEarlyCount);
        ImGui::Text("tri L: [%.3f]M", _pd.triangleLateCount);
        ImGui::Text("triangles: [%.3f]M", _pd.triangleCount);
        ImGui::Text("tri/sec: [%.2f]B", _pd.trianglesPerSec);
        ImGui::Text("draw/sec: [%.2f]M", 1000.f / _pd.avgCpuTime * _pd.objCount * 1e-6);
        ImGui::Text("frame: [%.2f]fps", 1000.f / _pd.avgCpuTime);
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
        ImGui::Text("pos: %.2f, %.2f, %.2f", _ld.cameraPos.x, _ld.cameraPos.y, _ld.cameraPos.z);
        ImGui::Text("dir: %.2f, %.2f, %.2f", _ld.cameraFront.x, _ld.cameraFront.y, _ld.cameraFront.z);
        ImGui::TreePop();
    }

    ImGui::End();
}

void vkz_updateImGui(const Input& input, RenderOptionsData& rd, const ProfilingData& pd, const LogicData& ld)
{
    VKZ_ZoneScopedC(vkz::Color::blue);;

    vkz_updateImGuiIO(input);

    vkz_updateImGuiContent(rd, pd, ld);

    ImGui::Render();
}

void vkz_updateUIRenderData(UIRendering& _ui)
{
    VKZ_ZoneScopedC(vkz::Color::blue);;

    ImDrawData* imDrawData = ImGui::GetDrawData();

    if (!imDrawData) {
        return;
    }

    VkDeviceSize vbSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize ibSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    if (vbSize == 0 || ibSize == 0)
        return;

    // TODO: fix the hard-code to fit the atomic no coherent memory.
    vbSize += (0x40 - (vbSize % 0x40));
    ibSize += (0x40 - (ibSize % 0x40));

    assert((vbSize % 0x40 == 0) && (ibSize % 0x40 == 0));
    if ( _ui.vtxCount != imDrawData->TotalVtxCount
         || _ui.idxCount != imDrawData->TotalIdxCount) 
    {
        const vkz::Memory* vbMem = vkz::alloc((uint32_t)vbSize);
        const vkz::Memory* ibMem = vkz::alloc((uint32_t)ibSize);

        uint32_t vbOffset = 0;
        uint32_t ibOffset = 0;
        for (int32_t ii = 0; ii < imDrawData->CmdListsCount; ++ii) {
            const ImDrawList* imCmdList = imDrawData->CmdLists[ii];
            memcpy(vbMem->data + vbOffset, imCmdList->VtxBuffer.Data, imCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(ibMem->data + ibOffset, imCmdList->IdxBuffer.Data, imCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            vbOffset += imCmdList->VtxBuffer.Size * sizeof(ImDrawVert);
            ibOffset += imCmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        }

        if (_ui.vtxCount != imDrawData->TotalVtxCount) {
            vkz::updateBuffer(_ui.vb, vbMem);
            _ui.vtxCount = imDrawData->TotalVtxCount;
        }

        if (_ui.idxCount != imDrawData->TotalIdxCount) {
            vkz::updateBuffer(_ui.ib, ibMem);
            _ui.idxCount = imDrawData->TotalIdxCount;
        }
    }
}