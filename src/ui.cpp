#include "common.h"
#include "imgui.h"
#include "resources.h"
#include "shaders.h"
#include "glm/glm.hpp"

#include "ui.h"

void initializeUI(UI& ui, VkDevice device, VkQueue queue, float scale /* = 1.f*/)
{
    ui.device = device;
    ui.queue = queue;
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;
}

void destroyUI(UI& ui)
{
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }

    destroyBuffer(ui.device, ui.vb);
    destroyBuffer(ui.device, ui.ib);

    vkDestroySampler(ui.device, ui.sampler, 0);
    destroyImage(ui.device, ui.fontImage);

    vkDestroyPipeline(ui.device, ui.pipeline, 0);
    destroyProgram(ui.device, ui.program);

    vkDestroyShaderModule(ui.device, ui.vs.module, 0);
    vkDestroyShaderModule(ui.device, ui.fs.module, 0);

}

void prepareUIPipeline(UI& ui, const VkPipelineCache pipelineCache, const VkRenderPass renderPass)
{
    VkDevice device = ui.device;

    bool lsr = false;
    lsr = loadShader(ui.vs, device, "shaders/ui.vert.spv");
    assert(lsr);

    lsr = loadShader(ui.fs, device, "shaders/ui.frag.spv");
    assert(lsr);

    Program program = createProgram(device, VK_PIPELINE_BIND_POINT_GRAPHICS, { &ui.vs, &ui.fs }, sizeof(PushConstBlock));
    
    
    std::vector<VkVertexInputBindingDescription> bindings =
    {
        {0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX},
    };

    std::vector<VkVertexInputAttributeDescription> attributes =
    {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)},
        {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)},
    };

    VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInput.vertexBindingDescriptionCount = (uint32_t)bindings.size();
    vertexInput.pVertexBindingDescriptions = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
    vertexInput.pVertexAttributeDescriptions = attributes.data();

    VkPipeline pipeline = createGraphicsPipeline(device, pipelineCache, program.layout, renderPass, { &ui.vs, &ui.fs }, &vertexInput);
    assert(pipeline);

    ui.program = program;
    ui.pipeline = pipeline;
}

void prepareUIResources(UI& ui, const VkPhysicalDeviceMemoryProperties& memoryProps, VkCommandPool cmdPool)
{
    VkDevice device = ui.device;
    VkQueue queue = ui.queue;
    ImGuiIO& io = ImGui::GetIO();

    // create font texture
    unsigned char* fontData = nullptr;
    int width = 0, height = 0;
    
    io.Fonts->AddFontFromFileTTF("../data/fonts/consola.ttf", 16.f * io.FontGlobalScale);
    io.Fonts->GetTexDataAsRGBA32(&fontData, &width, &height);
    assert(fontData);
    size_t uploadSize = width * height * 4 * sizeof(char);

    const VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    const uint32_t mipLevels = 1;
    Image image = {};
    createImage(image, device, memoryProps, width, height, mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    // buffer use to upload image
    Buffer scratch = {};
    createBuffer(scratch, memoryProps, device, uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = cmdPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer uploadCmd = 0;
    VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &uploadCmd));
    assert(uploadCmd);

    uploadImage(device, cmdPool, uploadCmd, queue, image, scratch, fontData, uploadSize);
    
    destroyBuffer(device, scratch);

    VkSampler sampler = createSampler(device);
    assert(sampler);

    // update ui image
    ui.fontImage.image = image.image;
    ui.fontImage.memory = image.memory;
    ui.fontImage.imageView = image.imageView;
    ui.fontImage.width = image.width;
    ui.fontImage.height = image.height;

    ui.sampler = sampler;
}

void updateImguiIO(const Input& input)
{
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)input.width, (float)input.height);

    io.MousePos = ImVec2(input.mousePosx, input.mousePosy);
    io.MouseDown[0] = input.mouseButtons.left;
    io.MouseDown[1] = input.mouseButtons.right;
    io.MouseDown[2] = input.mouseButtons.middle;
}

void updateUI(UI& ui, const VkPhysicalDeviceMemoryProperties& memoryProps)
{
    Buffer& vb = ui.vb;
    Buffer& ib = ui.ib;
    VkDevice& device = ui.device;
    ImDrawData* imDrawData = ImGui::GetDrawData();

    if (!imDrawData) {
        return;
    }

    VkDeviceSize vbSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize ibSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    assert(vbSize && ibSize);

    if (vb.buffer == VK_NULL_HANDLE || ui.vtxCount != imDrawData->TotalVtxCount) {
        destroyBuffer(device, vb);
        createBuffer(vb, memoryProps, device, vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        ui.vtxCount = imDrawData->TotalVtxCount;
    }

    if (ib.buffer == VK_NULL_HANDLE || ui.idxCount != imDrawData->TotalIdxCount) {
        destroyBuffer(device, ib);
        createBuffer(ib, memoryProps, device, ibSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        ui.idxCount = imDrawData->TotalIdxCount;
    }

    ImDrawVert* vtxDst = (ImDrawVert*)vb.data;
    ImDrawIdx* idxDst = (ImDrawIdx*)ib.data;

    for (int32_t i = 0; i < imDrawData->CmdListsCount; ++i) {
        const ImDrawList* imCmdList = imDrawData->CmdLists[i];
        memcpy(vtxDst, imCmdList->VtxBuffer.Data, imCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, imCmdList->IdxBuffer.Data, imCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += imCmdList->VtxBuffer.Size;
        idxDst += imCmdList->IdxBuffer.Size;
    }

    flushBuffer(device, vb);
    flushBuffer(device, ib);
}

void drawUI(UI& ui, const VkCommandBuffer cmdBuffer)
{
    ImDrawData* imDrawData = ImGui::GetDrawData();
    int32_t vtxOffset = 0;
    int32_t idxOffset = 0;

    if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    VkViewport viewport = { 0.f, 0.f, imDrawData->DisplaySize.x, imDrawData->DisplaySize.y, 0.f, 1.f };
    VkRect2D scissor = { {0, 0}, {uint32_t(imDrawData->DisplaySize.x), uint32_t(imDrawData->DisplaySize.y)} };

    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ui.pipeline);

    // descriptor set
    DescriptorInfo descInfos[1] = { {ui.sampler, ui.fontImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL} };
    vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, ui.program.updateTemplate, ui.program.layout, 0, descInfos);

    // constants
    PushConstBlock constantBlock = {};
    constantBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    constantBlock.translate = glm::vec2(-1.f, -1.0f);
    vkCmdPushConstants(cmdBuffer, ui.program.layout, ui.program.pushConstantStages, 0, sizeof(PushConstBlock), &constantBlock);

    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &ui.vb.buffer, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, ui.ib.buffer, 0, VK_INDEX_TYPE_UINT16);

    for (int32_t i = 0; i < imDrawData->CmdListsCount; ++i) 
    {
        const ImDrawList* imCmdList = imDrawData->CmdLists[i];
        for (int32_t j = 0; j < imCmdList->CmdBuffer.Size; ++j) 
        {
            const ImDrawCmd& cmd = imCmdList->CmdBuffer[j];

            VkRect2D scissor;
            scissor.offset.x = glm::max((int32_t)(cmd.ClipRect.x), 0);
            scissor.offset.y = glm::max((int32_t)(cmd.ClipRect.y), 0);
            scissor.extent.width = (uint32_t)(cmd.ClipRect.z - cmd.ClipRect.x);
            scissor.extent.height = (uint32_t)(cmd.ClipRect.w - cmd.ClipRect.y);

            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
            vkCmdDrawIndexed(cmdBuffer, cmd.ElemCount, 1, idxOffset, vtxOffset, 0);
            idxOffset += cmd.ElemCount;
        }
        vtxOffset += imCmdList->VtxBuffer.Size;
    }
}
