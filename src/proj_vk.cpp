// proj_vk.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "common.h"

#include "resources.h"
#include "swapchain.h"
#include "shaders.h"
#include "device.h"

// for ui
#include "glm/glm.hpp"
#include "imgui.h"
#include "ui.h"

#include <stdio.h>

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <vector>
#include <algorithm>

#include <fast_obj.h>
#include <meshoptimizer.h>
#include <string>


static bool meshShadingEnabled = true;

static Input input = {};

VkSemaphore createSemaphore(VkDevice device)
{

    VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore semaphore = 0;
    VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

    return semaphore;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t familyIndex)
{

    VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = familyIndex;

    VkCommandPool cmdPool = 0;
    VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &cmdPool));

    return cmdPool;
}

VkRenderPass createRenderPass(VkDevice device, VkFormat format)
{
    VkAttachmentDescription attachments[1] = {};
    attachments[0].format = format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkAttachmentReference colorAttachments = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachments;

    VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    createInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    VkRenderPass renderPass = 0;
    VK_CHECK(vkCreateRenderPass(device, &createInfo, 0, &renderPass));

    return renderPass;
}


bool checkExtSupportness(std::vector<VkExtensionProperties>& props, const char* extName)
{
    bool extSupported = false;
    for (const auto& extension : props) {
        if (std::string(extension.extensionName) == extName) {
            extSupported = true;
            break;
        }
    }

    printf("%s : %s\n", extName, extSupported ? "true" : "false");

    return extSupported;
}

VkQueryPool createQueryPool(VkDevice device, uint32_t queryCount)
{
    VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = queryCount;

    VkQueryPool queryPool = nullptr;
    VK_CHECK(vkCreateQueryPool(device, &createInfo, 0, &queryPool));
    return queryPool;
}


struct Vertex
{
    float       vx, vy, vz;
    uint8_t     nx, ny, nz, nw;
    uint16_t    tu, tv;
};

struct alignas(16) Meshlet
{
    float cone[4];
    uint32_t dataOffset;
    uint8_t  triangleCount;
    uint8_t  vertexCount;
};

struct Mesh
{
    std::vector<Vertex>     vertices;
    std::vector<uint32_t>   indices;
    std::vector<Meshlet>    meshlets;
    std::vector<uint32_t>   meshletdata;
};

void buildMeshlets(Mesh& mesh)
{
    const size_t max_vertices = 64;
    const size_t max_triangles = 84;
    const float cone_weight = 1.f;

    std::vector<meshopt_Meshlet> meshlets(meshopt_buildMeshletsBound(mesh.indices.size(), max_vertices, max_triangles));
    std::vector<unsigned int> meshlet_vertices(meshlets.size() * max_vertices);
    std::vector<unsigned char> meshlet_triangles(meshlets.size() * max_triangles * 3);

    meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), mesh.indices.data(), mesh.indices.size()
        , &mesh.vertices[0].vx, mesh.vertices.size(), sizeof(Vertex), max_vertices, max_triangles, cone_weight);

    while (meshlets.size() % 32)
    {
        meshlets.push_back(meshopt_Meshlet{});
    }
    
    for (const meshopt_Meshlet meshlet : meshlets)
    {
        Meshlet m = {};
        size_t dataOffset = mesh.meshletdata.size();

        for (unsigned int i = 0; i < meshlet.vertex_count; ++i)
        {
            mesh.meshletdata.push_back(meshlet_vertices[meshlet.vertex_offset + i]);
        }

        const uint32_t* idxGroup = reinterpret_cast<const unsigned int*>(&meshlet_triangles[0] + meshlet.triangle_offset);
        uint32_t idxGroupCount = (meshlet.triangle_count * 3 + 3) / 4;

        for (uint32_t i = 0; i < idxGroupCount; ++i)
        {
            mesh.meshletdata.push_back(idxGroup[i]);
        }

        meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset]
            , meshlet.triangle_count, &mesh.vertices[0].vx, mesh.vertices.size(), sizeof(Vertex));


        m.dataOffset = uint32_t(dataOffset);
        m.triangleCount = meshlet.triangle_count;
        m.vertexCount = meshlet.vertex_count;

        m.cone[0] = bounds.cone_axis_s8[0];
        m.cone[1] = bounds.cone_axis_s8[1];
        m.cone[2] = bounds.cone_axis_s8[2];
        m.cone[3] = bounds.cone_cutoff_s8;

        mesh.meshlets.push_back(m);
    }
 }

float halfToFloat(uint16_t v)
{
    // according to IEEE 754: half
    uint16_t sign = v >> 15;
    uint16_t exp = (v >> 10) & 31; // 5 bit exp
    uint16_t mant = v & 1023; // 10 bit mant

    assert(exp != 31);

    if (exp == 0)
    {
        assert(mant == 0);
        return 0.f;
    }
    else
    {
        return(sign ? -1.f : 1.f) * ldexpf(float(mant + 1024) / 1024.f, exp - 15);
    }
}


void enumrateDeviceExtPorps(VkPhysicalDevice physicalDevice, std::vector<VkExtensionProperties>& availableExtensions)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    availableExtensions.resize(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
}


void dumpExtensionSupport(VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    checkExtSupportness(availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_8BIT_STORAGE_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_SPIRV_1_4_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}


bool loadMesh(Mesh& result, const char* path)
{
    fastObjMesh* obj =  fast_obj_read(path);
    if (!obj)
        return false;

    size_t index_count = obj->index_count;

    std::vector<Vertex> triangle_vertices;
    triangle_vertices.resize(index_count);

    size_t vertex_offset = 0, index_offset = 0;

    size_t texcoord_count = obj->texcoord_count;

    for (uint32_t i = 0; i < obj->face_count; ++i)
    {
        for (uint32_t j = 0; j < obj->face_vertices[i]; ++j)
        {
            fastObjIndex gi = obj->indices[index_offset + j];

            assert(j < 3);

            Vertex& v = triangle_vertices[vertex_offset++];

            v.vx = (obj->positions[gi.p * 3 + 0]);
            v.vy = (obj->positions[gi.p * 3 + 1]);
            v.vz = (obj->positions[gi.p * 3 + 2]);

            v.nx = uint8_t(obj->normals[gi.n * 3 + 0] * 127.f + 127.5f);
            v.ny = uint8_t(obj->normals[gi.n * 3 + 1] * 127.f + 127.5f);
            v.nz = uint8_t(obj->normals[gi.n * 3 + 2] * 127.f + 127.5f);
            v.nw = uint8_t(0);

            v.tu = meshopt_quantizeHalf(obj->texcoords[gi.t * 2 + 0]);
            v.tv = meshopt_quantizeHalf(obj->texcoords[gi.t * 2 + 1]);
        }

        index_offset += obj->face_vertices[i];
    }

    assert(vertex_offset == index_count);

    fast_obj_destroy(obj);

    std::vector<uint32_t> remap(index_count);
    size_t vertex_count = meshopt_generateVertexRemap(remap.data(), 0, index_count, triangle_vertices.data(), index_count, sizeof(Vertex));

    std::vector<Vertex> vertices(vertex_count);
    std::vector<uint32_t> indices(index_count);

    meshopt_remapVertexBuffer(vertices.data(), triangle_vertices.data(), index_count, sizeof(Vertex), remap.data());
    meshopt_remapIndexBuffer(indices.data(), 0, index_count, remap.data());

    meshopt_optimizeVertexCache(indices.data(), indices.data(), index_count, vertex_count);
    meshopt_optimizeVertexFetch(vertices.data(), indices.data(), index_count, vertices.data(), vertex_count, sizeof(Vertex));

    result.vertices.insert(result.vertices.end(), vertices.begin(), vertices.end());
    result.indices.insert(result.indices.end(), indices.begin(), indices.end());

    return true;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) 
    {
        if (key == GLFW_KEY_M)
        {
            meshShadingEnabled = !meshShadingEnabled;
        }
        if (key == GLFW_KEY_ESCAPE)
        {
            exit(0);
        }
    }
}

void mousekeyCallback(GLFWwindow* window, int key, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_MOUSE_BUTTON_LEFT)
        {
            input.mouseButtons.left = true;
        }
        if (key == GLFW_MOUSE_BUTTON_RIGHT)
        {
            input.mouseButtons.right = true;
        }
        if (key == GLFW_MOUSE_BUTTON_MIDDLE)
        {
            input.mouseButtons.middle = true;
        }
    }
    if (action == GLFW_RELEASE)
    {
        if (key == GLFW_MOUSE_BUTTON_LEFT)
        {
            input.mouseButtons.left = false;
        }
        if (key == GLFW_MOUSE_BUTTON_RIGHT)
        {
            input.mouseButtons.right = false;
        }
        if (key == GLFW_MOUSE_BUTTON_MIDDLE)
        {
            input.mouseButtons.middle = false;
        }
    }
}

void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    input.mousePosx = (float)xpos;
    input.mousePosy = (float)ypos;
}

struct ProfilingData
{
    float cpuTime;
    float avrageCpuTime;
    float gpuTime;
    float waitTime;
    float trianlesPerSecond;
    uint32_t primitiveCount;
    uint32_t meshletCount;
    bool usingMS;
};

int main(int argc, const char** argv)
{
    if (argc < 2) {
        printf("Usage: %s [mesh]\n", argv[0]);
        return 1;
    }

	int rc = glfwInit();
	assert(rc);

    VK_CHECK(volkInitialize());

	VkInstance instance = createInstance();
	assert(instance);

    volkLoadInstance(instance);

    VkDebugReportCallbackEXT debugCallback = registerDebugCallback(instance);

	VkPhysicalDevice physicalDevices[16];
	uint32_t deviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
	VK_CHECK( vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices));


	VkPhysicalDevice physicalDevice = pickPhysicalDevice(physicalDevices, deviceCount);
	assert(physicalDevice);

    std::vector<VkExtensionProperties> supportedExtensions;
    enumrateDeviceExtPorps(physicalDevice, supportedExtensions);

    
    bool meshShadingSupported = false;
    meshShadingSupported = checkExtSupportness(supportedExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME);

    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    assert(props.limits.timestampPeriod);

    uint32_t familyIndex = getGraphicsFamilyIndex(physicalDevice);
    assert(familyIndex != VK_QUEUE_FAMILY_IGNORED);

	VkDevice device = createDevice(instance, physicalDevice, familyIndex, meshShadingSupported);
	assert(device);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "proj_vk", 0, 0);
	assert(window);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mousekeyCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);

	VkSurfaceKHR surface = createSurface(instance, window);
	assert(surface);

    VkBool32 presentSupported = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &presentSupported));
    assert(presentSupported);

    VkFormat swapchainFormat = getSwapchainFormat(physicalDevice, surface);
    assert(swapchainFormat);
    
    VkSemaphore acquirSemaphore = createSemaphore(device);
    assert(acquirSemaphore);

    VkSemaphore releaseSemaphore = createSemaphore(device);
    assert(releaseSemaphore);

    VkQueue queue = 0;
    vkGetDeviceQueue(device, familyIndex, 0, &queue);
    assert(queue);

    VkRenderPass renderPass = createRenderPass(device, swapchainFormat);
    assert(renderPass);

    bool lsr = false;

    Shader meshVS = {};
    lsr = loadShader(meshVS, device, "shaders/mesh.vert.spv");
    assert(lsr);

    Shader meshFS = {};
    lsr = loadShader(meshFS, device, "shaders/mesh.frag.spv");
    assert(lsr);

    Shader meshletMS = {};
    Shader meshletTS = {};
    
    if (meshShadingSupported)
    {
        lsr = loadShader(meshletMS, device, "shaders/meshlet.mesh.spv");
        assert(lsr);

        lsr = loadShader(meshletTS, device, "shaders/meshlet.task.spv");
        assert(lsr);
    }

    Program meshProgram = createProgram(device, VK_PIPELINE_BIND_POINT_GRAPHICS, { &meshVS, &meshFS });
    
    Program meshProgramMS = {};
    if (meshShadingSupported)
    {
        meshProgramMS = createProgram(device, VK_PIPELINE_BIND_POINT_GRAPHICS, { &meshletTS, &meshletMS, &meshFS });
    }

    VkPipelineCache pipelineCache = 0;
    VkPipeline meshPipeline = createGraphicsPipeline(device, pipelineCache, meshProgram.layout, renderPass, { &meshVS, &meshFS}, nullptr);
    assert(meshPipeline);


    VkPipeline meshPipelineMS = 0;
    if (meshShadingSupported)
    {
        meshPipelineMS = createGraphicsPipeline(device, pipelineCache, meshProgramMS.layout, renderPass, { &meshletTS, &meshletMS, &meshFS }, nullptr);
        assert(meshPipelineMS);
    }

    Swapchain swapchain = {};
    createSwapchain(swapchain, physicalDevice, device, surface, &familyIndex, swapchainFormat, renderPass);

    VkQueryPool queryPool = createQueryPool(device, 128);
    assert(queryPool);

    VkCommandPool cmdPool = createCommandPool(device, familyIndex);
    assert(cmdPool);

    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = cmdPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer = 0;
    VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &cmdBuffer));

    
    VkDescriptorPool descPool = 0;
    {
        uint32_t descriptorCount = 512;

        VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, descriptorCount },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorCount },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorCount },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorCount },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorCount },
        };

        VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolCreateInfo.maxSets = descriptorCount;
        poolCreateInfo.poolSizeCount = COUNTOF(poolSizes);
        poolCreateInfo.pPoolSizes = poolSizes;

        VK_CHECK(vkCreateDescriptorPool(device, &poolCreateInfo, 0, &descPool));
    }


    VkPhysicalDeviceMemoryProperties memoryProps = {};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProps);
    
    Mesh mesh;
    bool rcm = loadMesh(mesh, argv[1]);
    assert(rcm);
    if (meshShadingSupported)
    {
        buildMeshlets(mesh);

        printf("mesh data size: %d\n", (uint32_t)mesh.meshletdata.size());
        printf("meshlet size: %d\n", (uint32_t)mesh.meshlets.size());
    }

    Buffer scratch = {};
    createBuffer(scratch, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    Buffer vb = {};
    createBuffer(vb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer ib = {};
    createBuffer(ib, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    Buffer mb = {};
    Buffer mdb = {};
    if (meshShadingSupported)
    {
        createBuffer(mb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        createBuffer(mdb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }

    uploadBuffer(device, cmdPool, cmdBuffer, queue, vb, scratch, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    uploadBuffer(device, cmdPool, cmdBuffer, queue, ib, scratch, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));

    if (meshShadingSupported)
    {
        uploadBuffer(device, cmdPool, cmdBuffer, queue, mb, scratch, mesh.meshlets.data(), mesh.meshlets.size() * sizeof(Meshlet));
        uploadBuffer(device, cmdPool, cmdBuffer, queue, mdb, scratch, mesh.meshletdata.data(), mesh.meshletdata.size() * sizeof(uint32_t));

    }


    // imgui
    UI ui = {};
    initializeUI(ui, device, queue, 1.3f);
    prepareUIPipeline(ui, pipelineCache, renderPass);
    prepareUIResources(ui, memoryProps, cmdPool);


    ProfilingData pd = {};
    pd.meshletCount = (uint32_t)mesh.meshlets.size();
    pd.primitiveCount = (uint32_t)(mesh.indices.size() / 3);

    double deltaTime = 0.f;
    double avrageTime = 0.f;

    while (!glfwWindowShouldClose(window))
    {

        double frameCpuBegin = glfwGetTime() * 1000.0;

        glfwPollEvents();

        int newWindowWidth = 0, newWindowHeight = 0;
        glfwGetWindowSize(window, &newWindowWidth, &newWindowHeight);

        resizeSwapchainIfNecessary(swapchain, physicalDevice, device, surface, &familyIndex, swapchainFormat, renderPass);

        // set input data
        input.width = (float)newWindowWidth;
        input.height = (float)newWindowHeight;

        updateImguiIO(input);
        {
            ImGuiIO& io = ImGui::GetIO();

            io.DisplaySize = ImVec2((float)newWindowWidth, (float)newWindowHeight);
            ImGui::NewFrame();
            ImGui::SetNextWindowSize({400, 400});
            ImGui::Begin("info:");
            
            ImGui::Text("cpu: [%.2f]ms", pd.cpuTime);
            ImGui::Text("avg cpu: [%.2f]ms", pd.avrageCpuTime);
            ImGui::Text("gpu: [%.2f]ms", pd.gpuTime);
            ImGui::Text("wait: [%.2f]ms", pd.waitTime);
            ImGui::Text("primitive count: [%d]", pd.primitiveCount);
            ImGui::Text("primitive count: [%d]", pd.meshletCount);
            ImGui::Text("tri/sec: [%.2f]B", pd.trianlesPerSecond);
            ImGui::Text("frame: [%.2f]fps", 1000.f / pd.avrageCpuTime);
            ImGui::Checkbox("Mesh Shading", &pd.usingMS);

            ImGui::End();
            ImGui::Render();
        }

        updateUI(ui, memoryProps);
        
        
        uint32_t imageIndex = 0;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain, ~0ull, acquirSemaphore, VK_NULL_HANDLE, &imageIndex));

        VK_CHECK(vkResetCommandPool(device, cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

        vkCmdResetQueryPool(cmdBuffer, queryPool, 0, 128);
        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 0);

        VkImageMemoryBarrier renderBeginBarrier = imageBarrier(swapchain.images[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
            , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // check https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#combined-graphicspresent-queue
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            , VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &renderBeginBarrier);
        
        // jinzi - from http://zhongguose.com
        VkClearColorValue color = { 128.f/255.f, 109.f/255.f, 158.f/255.f, 1 };
        VkClearValue clearColor = { color };

        VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        passBeginInfo.renderPass = renderPass;
        passBeginInfo.framebuffer = swapchain.framebuffers[imageIndex];
        passBeginInfo.renderArea.extent.width = swapchain.width;
        passBeginInfo.renderArea.extent.height = swapchain.height;
        passBeginInfo.clearValueCount = 1;
        passBeginInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmdBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // drawcalls
        VkViewport viewport = { 0.f, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.f, 1.f };
        VkRect2D scissor = { {0, 0}, {uint32_t(swapchain.width), uint32_t(swapchain.height)}};

        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);


        uint32_t drawCount = 1;
        bool meshShadingOn = meshShadingEnabled && meshShadingSupported;

        if(meshShadingOn)
        {
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineMS);

            DescriptorInfo descInfos[3] = { vb.buffer, mb.buffer, mdb.buffer };

            vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, meshProgramMS.updateTemplate, meshProgramMS.layout, 0, descInfos);
            
            for (uint32_t i = 0; i < drawCount; i++)
            {
                vkCmdDrawMeshTasksEXT(cmdBuffer, (uint32_t)mesh.meshlets.size() / 32, 1, 1);
            }
        }
        else
        {
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

            DescriptorInfo descInfos[1] = { vb.buffer };

            vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, meshProgram.updateTemplate, meshProgram.layout, 0, descInfos);

            vkCmdBindIndexBuffer(cmdBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);

            
            for (uint32_t i = 0; i < drawCount; i++)
            {
                vkCmdDrawIndexed(cmdBuffer, (uint32_t)mesh.indices.size(), 1, 0, 0, 0);
            }
        }

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 1);
        drawUI(ui, cmdBuffer);

        vkCmdEndRenderPass(cmdBuffer);

        VkImageMemoryBarrier renderEndBarrier = imageBarrier(swapchain.images[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0
            , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
            , VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &renderEndBarrier);

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 2);

        VK_CHECK(vkEndCommandBuffer(cmdBuffer));

 
        VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &acquirSemaphore;
        submitInfo.pWaitDstStageMask = &submitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &releaseSemaphore;

        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pWaitSemaphores = &releaseSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

        double waitTimeBegin = glfwGetTime() * 1000.0;

        VK_CHECK(vkDeviceWaitIdle(device));
        
        double waitTimeEnd = glfwGetTime() * 1000.0;
        
        uint64_t queryResults[3] = {};
        vkGetQueryPoolResults(device, queryPool, 0, ARRAYSIZE(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT);

        double frameCpuEnd = glfwGetTime() * 1000.0;

        double frameGpuBegin = double(queryResults[0]) * props.limits.timestampPeriod * 1e-6;
        double frameUIBegin = double(queryResults[1]) * props.limits.timestampPeriod * 1e-6;
        double frameGpuEnd = double(queryResults[2]) * props.limits.timestampPeriod * 1e-6;

        avrageTime = avrageTime * 0.9995 + (frameCpuEnd - frameCpuBegin) * 0.0005;
        double tps = double(mesh.indices.size() * drawCount / 3) / double((frameGpuEnd - frameGpuBegin) * 1e-3);
        pd.avrageCpuTime = (float)avrageTime;
        deltaTime += (frameCpuEnd - frameCpuBegin);
        if(deltaTime >= 100)
        {
            pd.cpuTime = float(frameCpuEnd - frameCpuBegin);
            pd.gpuTime = float(frameGpuEnd - frameGpuBegin);
            pd.waitTime = float(waitTimeEnd - waitTimeBegin);
            pd.trianlesPerSecond = float(tps * 1e-9);
            pd.usingMS = meshShadingOn;
            deltaTime = 0.0;
        }
	}
    VK_CHECK(vkDeviceWaitIdle(device));

    destroyUI(ui);
   
    destroyBuffer(device, scratch);
    destroyBuffer(device, ib); 
    destroyBuffer(device, vb);
    if (meshShadingSupported)
    {
        destroyBuffer(device, mb);
        destroyBuffer(device, mdb);
    }

    vkDestroyCommandPool(device, cmdPool, 0);

    vkDestroyDescriptorPool(device, descPool, 0);

    destroySwapchain(device, swapchain);

    vkDestroyQueryPool(device, queryPool, 0);

    vkDestroyPipeline(device, meshPipeline, 0);
    destroyProgram(device, meshProgram);
    if (meshShadingSupported)
    {
        vkDestroyPipeline(device, meshPipelineMS, 0);
        destroyProgram(device, meshProgramMS);
    }

    vkDestroyShaderModule(device, meshFS.module, 0);
    vkDestroyShaderModule(device, meshVS.module, 0);

    if (meshShadingSupported)
    {
        vkDestroyShaderModule(device, meshletMS.module, 0);
        vkDestroyShaderModule(device, meshletTS.module, 0);
    }

    vkDestroyRenderPass(device, renderPass, 0);

    vkDestroySemaphore(device, releaseSemaphore, 0);
    vkDestroySemaphore(device, acquirSemaphore, 0);
    vkDestroySurfaceKHR(instance, surface, 0);

	glfwDestroyWindow(window);

    vkDestroyDevice(device, 0);

    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(instance, debugCallback, 0);


    vkDestroyInstance(instance, 0);
}
