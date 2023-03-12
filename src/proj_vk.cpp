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
    uint16_t    vx, vy, vz, vw;
    uint8_t     nx, ny, nz, nw;
    uint16_t    tu, tv;
};

struct alignas(16) Meshlet
{
    float cone[4];
    uint32_t vertices[64];
    uint8_t  indices[84 * 3]; //this is 84 triangles
    uint8_t  triangleCount;
    uint8_t  vertexCount;
};

struct Mesh
{
    std::vector<Vertex>     vertices;
    std::vector<uint32_t>   indices;
    std::vector<Meshlet>    meshlets;
};

void buildMeshlets(Mesh& mesh)
{
    Meshlet meshlet = {};
    std::vector<uint8_t> meshletVertices(mesh.vertices.size(), (uint8_t)0xff);

    for (size_t i = 0; i < mesh.indices.size(); i += 3)
    {
        uint32_t a = mesh.indices[i + 0];
        uint32_t b = mesh.indices[i + 1];
        uint32_t c = mesh.indices[i + 2];

        uint8_t& av = meshletVertices[a];
        uint8_t& bv = meshletVertices[b];
        uint8_t& cv = meshletVertices[c];
        if (meshlet.vertexCount + (av == 0xff) + (bv == 0xff) + (cv == 0xff) > 64
            || meshlet.triangleCount >= 84) {
            mesh.meshlets.push_back(meshlet);


            for (size_t j = 0; j < meshlet.vertexCount; ++j)
            {
                meshletVertices[meshlet.vertices[j]] = (uint8_t)0xff;
            }

            meshlet = {};
        }

        if (av == 0xff)
        {
            av = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = a;
        }
        if (bv == 0xff)
        {
            bv = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = b;
        }
        if (cv == 0xff)
        {
            cv = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = c;
        }

        meshlet.indices[meshlet.triangleCount * 3 + 0] = av;
        meshlet.indices[meshlet.triangleCount * 3 + 1] = bv;
        meshlet.indices[meshlet.triangleCount * 3 + 2] = cv;
        meshlet.triangleCount++;

    }

    if (meshlet.triangleCount)
    {
        mesh.meshlets.push_back(meshlet);
    }

    // TODO: remove this after implement a proper way to deal with dynamic meshlet count. currently assume all 32 meshlet required in task shaders
    while (mesh.meshlets.size() % 32)
    {
        mesh.meshlets.push_back(Meshlet{});
    }
}

float halfToFloat(uint16_t v)
{
    // according to IEEE 754: half
    uint16_t sign = v >> 15;
    uint16_t exp = (v >> 10) & 0x1f; // 5 bit exp
    uint16_t mant = v & 0x3ff; // 10 bit mant

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


void buildMeshletCones(Mesh& mesh)
{
    for (Meshlet& meshlet : mesh.meshlets)
    {
        float normals[84][3] = {};
        for (uint32_t i = 0; i < meshlet.triangleCount; i++)
        {
            uint32_t a = meshlet.indices[i * 3 + 0];
            uint32_t b = meshlet.indices[i * 3 + 1];
            uint32_t c = meshlet.indices[i * 3 + 2];

            const Vertex& va = mesh.vertices[meshlet.vertices[a]];
            const Vertex& vc = mesh.vertices[meshlet.vertices[b]];
            const Vertex& vb = mesh.vertices[meshlet.vertices[c]];

            float p0[3] = { halfToFloat(va.vx), halfToFloat(va.vy), halfToFloat(va.vz) };
            float p1[3] = { halfToFloat(vb.vx), halfToFloat(vb.vy), halfToFloat(vb.vz) };
            float p2[3] = { halfToFloat(vc.vx), halfToFloat(vc.vy), halfToFloat(vc.vz) }; 
            
            float p10[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
            float p20[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
            
            float normalx = p10[1] * p20[2] - p10[2] * p20[1];
            float normaly = p10[2] * p20[0] - p10[0] * p20[2];
            float normalz = p10[0] * p20[1] - p10[1] * p20[0];

            float area = sqrtf(normalx * normalx + normaly * normaly + normalz * normalz);
            float invaera = (area == 0.f) ? 0.f : 1 / area;

            normals[i][0] = normalx * invaera;
            normals[i][1] = normaly * invaera;
            normals[i][2] = normalz * invaera;
        }

        float avgnormal[3] = {};

        for (uint32_t i = 0; i < meshlet.triangleCount; ++i)
        {
            avgnormal[0] += normals[i][0];
            avgnormal[1] += normals[i][1];
            avgnormal[2] += normals[i][2];
        }

        float avglength = sqrtf(avgnormal[0]* avgnormal[0] + avgnormal[1] * avgnormal[1] + avgnormal[2] * avgnormal[2]);
        
        if (avglength == 0.f)
        {
            avgnormal[0] = 1.f;
            avgnormal[1] = 0.f;
            avgnormal[2] = 0.f;
        }
        else
        {
            avgnormal[0] /= avglength;
            avgnormal[1] /= avglength;
            avgnormal[2] /= avglength;
        }

        float mindp = 1.f;

        for (uint32_t i = 0; i < meshlet.triangleCount; ++i)
        {
            float dp = normals[i][0] * avgnormal[0] + normals[i][1] * avgnormal[1] + normals[i][2] * avgnormal[2];
            mindp = glm::min(mindp, dp);
        }

        // cone back face culling: 
        float conew = mindp <= 0.f ? 1.f : sqrtf(1.f - mindp * mindp);

        meshlet.cone[0] = avgnormal[0];
        meshlet.cone[1] = avgnormal[1];
        meshlet.cone[2] = avgnormal[2];
        meshlet.cone[3] = conew;
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

            v.vx = meshopt_quantizeHalf(obj->positions[gi.p * 3 + 0]);
            v.vy = meshopt_quantizeHalf(obj->positions[gi.p * 3 + 1]);
            v.vz = meshopt_quantizeHalf(obj->positions[gi.p * 3 + 2]);
            v.vw = uint16_t(0);

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
        buildMeshletCones(mesh);
    }

    Buffer scratch = {};
    createBuffer(scratch, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    Buffer vb = {};
    createBuffer(vb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer ib = {};
    createBuffer(ib, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    Buffer mb = {};
    if (meshShadingSupported)
    {
        createBuffer(mb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }

    uploadBuffer(device, cmdPool, cmdBuffer, queue, vb, scratch, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    uploadBuffer(device, cmdPool, cmdBuffer, queue, ib, scratch, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));

    if (meshShadingSupported)
    {
        uploadBuffer(device, cmdPool, cmdBuffer, queue, mb, scratch, mesh.meshlets.data(), mesh.meshlets.size() * sizeof(Meshlet));
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

        bool meshShadingOn = meshShadingEnabled && meshShadingSupported;

        if(meshShadingOn)
        {
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineMS);

            DescriptorInfo descInfos[2] = { vb.buffer, mb.buffer };

            vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, meshProgramMS.updateTemplate, meshProgramMS.layout, 0, descInfos);

            vkCmdDrawMeshTasksEXT(cmdBuffer, (uint32_t)mesh.meshlets.size() / 32, 1, 1);

        }
        else
        {
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

            DescriptorInfo descInfos[1] = { vb.buffer };

            vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, meshProgram.updateTemplate, meshProgram.layout, 0, descInfos);

            vkCmdBindIndexBuffer(cmdBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmdBuffer, (uint32_t)mesh.indices.size(), 1, 0, 0, 0);
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

        pd.avrageCpuTime = (float)avrageTime;
        deltaTime += (frameCpuEnd - frameCpuBegin);
        if(deltaTime >= 500)
        {
            pd.cpuTime = float(frameCpuEnd - frameCpuBegin);
            pd.gpuTime = float(frameGpuEnd - frameGpuBegin);
            pd.waitTime = float(waitTimeEnd - waitTimeBegin);
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
