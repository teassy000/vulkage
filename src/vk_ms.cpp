// proj_vk.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "common.h"
#include "math.h"

#include "resources.h"
#include "swapchain.h"
#include "shaders.h"
#include "device.h"

// for ui
#include <imgui.h>
#include "ui.h"

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <stdio.h>
#include <vector>
#include <algorithm>
#include <string>

#include <fast_obj.h>
#include <meshoptimizer.h>


struct Camera
{
    vec3 dir{ 0.f, 0.f, -1.f };
    vec3 lookAt{ 0.f, 0.f, -1.f };
    vec3 up{ 0.f, 1.f, 0.f };
    vec3 pos{ 0.f, 0.f, 0.f };
};

static Camera camera = {};

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

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
{
    VkAttachmentDescription attachments[2] = {};
    attachments[0].format = colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ;

    VkAttachmentReference colorAttachment = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthAttachment = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachment;
    subpass.pDepthStencilAttachment = &depthAttachment;

    VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    createInfo.attachmentCount = ARRAYSIZE(attachments);
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

struct alignas(16) Globals
{
    mat4 vp;
    
    vec3 cameraPos;
};

struct MeshDrawCull
{
    float znear;
    float zfar;
    float frustum[4];
};

struct alignas(16) MeshDraw
{
    vec3 pos;
    float scale;
    quat orit;
    
    uint32_t meshIdx;
    uint32_t vertexOffset;
};

struct MeshDrawCommand
{
    uint32_t drawId;
    VkDrawIndexedIndirectCommand indirect; // 5 uint32_t
    VkDrawMeshTasksIndirectCommandEXT indirectMS; // 3 uint32_t
};


struct Vertex
{
    float       vx, vy, vz;
    uint8_t     nx, ny, nz, nw;
    uint16_t    tu, tv;
};

struct alignas(16) Meshlet
{
    vec3 center;
    float radians;
    int8_t cone_axis[3];
    int8_t cone_cutoff;

    uint32_t dataOffset;
    uint8_t triangleCount;
    uint8_t vertexCount;
};


struct alignas(16) Mesh
{
    vec3 center;
    float   radius;

    uint32_t vertexOffset;
    uint32_t meshletOffset;
    uint32_t meshletCount;
    uint32_t indexOffset;
    uint32_t indexCount;
};

struct Geometry
{
    std::vector<Vertex>     vertices;
    std::vector<uint32_t>   indices;
    std::vector<Meshlet>    meshlets;
    std::vector<uint32_t>   meshletdata;
    std::vector<Mesh>       meshes;
};


bool loadMesh(Geometry& result, const char* path, bool buildMeshlets)
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



    uint32_t indexCount = uint32_t(indices.size());
    uint32_t indexOffset = uint32_t(result.indices.size());
    uint32_t vertexOffset = uint32_t(result.vertices.size());


    uint32_t meshletCount = 0u, meshletOffset = 0u;
    if (buildMeshlets)
    {
        const size_t max_vertices = 64;
        const size_t max_triangles = 84;
        const float cone_weight = 1.f;

        std::vector<meshopt_Meshlet> meshlets(meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles));
        std::vector<unsigned int> meshlet_vertices(meshlets.size() * max_vertices);
        std::vector<unsigned char> meshlet_triangles(meshlets.size() * max_triangles * 3);

        meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), indices.data(), indices.size()
            , &vertices[0].vx, vertices.size(), sizeof(Vertex), max_vertices, max_triangles, cone_weight);

        meshletCount = (uint32_t)meshlets.size();
        meshletOffset = (uint32_t)result.meshlets.size();

        for (const meshopt_Meshlet meshlet : meshlets)
        {
            Meshlet m = {};
            size_t dataOffset = result.meshletdata.size();

            for (unsigned int i = 0; i < meshlet.vertex_count; ++i)
            {
                result.meshletdata.push_back(meshlet_vertices[meshlet.vertex_offset + i] + vertexOffset);
            }

            const uint32_t* idxGroup = reinterpret_cast<const unsigned int*>(&meshlet_triangles[0] + meshlet.triangle_offset);
            uint32_t idxGroupCount = (meshlet.triangle_count * 3 + 3) / 4;

            for (uint32_t i = 0; i < idxGroupCount; ++i)
            {
                result.meshletdata.push_back(idxGroup[i]);
            }

            meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset]
                , meshlet.triangle_count, &vertices[0].vx, vertices.size(), sizeof(Vertex));

            m.dataOffset = uint32_t(dataOffset);
            m.triangleCount = meshlet.triangle_count;
            m.vertexCount = meshlet.vertex_count;

            m.center[0] = bounds.center[0];
            m.center[1] = bounds.center[1];
            m.center[2] = bounds.center[2];
            m.radians = bounds.radius;

            m.cone_axis[0] = bounds.cone_axis_s8[0];
            m.cone_axis[1] = bounds.cone_axis_s8[1];
            m.cone_axis[2] = bounds.cone_axis_s8[2];
            m.cone_cutoff = bounds.cone_cutoff_s8;

            result.meshlets.push_back(m);
        }
    }

    vec3 meshCenter = {};

    for (Vertex& v : vertices) {
        meshCenter += vec3(v.vx, v.vy, v.vz);
    }

    meshCenter /= float(vertices.size());

    float radius = 0.0;
    for (Vertex& v : vertices) {
        radius = glm::max(radius, glm::distance(meshCenter, vec3(v.vx, v.vy, v.vz)));
    }


    Mesh mesh = {};
    mesh.center = meshCenter;
    mesh.radius = radius;
    mesh.indexCount = indexCount;
    mesh.indexOffset = indexOffset;
    mesh.meshletCount = meshletCount;
    mesh.meshletOffset = meshletOffset;
    mesh.vertexOffset = vertexOffset;

    result.meshes.push_back(mesh);
    
    result.vertices.insert(result.vertices.end(), vertices.begin(), vertices.end());
    result.indices.insert(result.indices.end(), indices.begin(), indices.end());

    return true;
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
        if (key == GLFW_KEY_W)
        {
            camera.pos.z += 0.5f;
            camera.lookAt = camera.pos + camera.dir;
        }
        if (key == GLFW_KEY_S)
        {
            camera.pos.z -= 0.5f;
            camera.lookAt = camera.pos + camera.dir;
        }
        if (key == GLFW_KEY_A)
        {
            camera.pos.x -= 0.5;
            camera.lookAt = camera.pos + camera.dir;
        }
        if (key == GLFW_KEY_D)
        {
            camera.pos.x += 0.5;
            camera.lookAt = camera.pos + camera.dir;
        }
        if (key == GLFW_KEY_R)
        {
            camera = Camera{};
        }
    }
    if(action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_W)
        {
            camera.pos.z += 0.5f;
            camera.lookAt = camera.pos + camera.dir;
        }
        if (key == GLFW_KEY_S)
        {
            camera.pos.z -= 0.5f;
            camera.lookAt = camera.pos + camera.dir;
        }
        if (key == GLFW_KEY_A)
        {
            camera.pos.x -= 0.5;
            camera.lookAt = camera.pos + camera.dir;
        }
        if (key == GLFW_KEY_D)
        {
            camera.pos.x += 0.5;
            camera.lookAt = camera.pos + camera.dir;
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
    float avgCpuTime;
    float gpuTime;
    float avgGpuTime;
    float assemplyCmdTime;
    float uiTime;
    float waitTime;
    float trangleCount;
    float trianglesPerSecond;
    uint32_t primitiveCount;
    uint32_t meshletCount;
};

mat4 persectiveProjection(float fovY, float aspectWbyH, float zNear)
{
    float f = 1.0f / tanf(fovY/ 2.0f);

    return mat4(
        f / aspectWbyH, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, zNear, 0.0f);
}

vec4 normalizePlane(vec4 p)
{
    return p / glm::length(p);
}

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
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    
    VkSemaphore acquirSemaphore = createSemaphore(device);
    assert(acquirSemaphore);

    VkSemaphore releaseSemaphore = createSemaphore(device);
    assert(releaseSemaphore);

    VkQueue queue = 0;
    vkGetDeviceQueue(device, familyIndex, 0, &queue);
    assert(queue);

    VkRenderPass renderPass = createRenderPass(device, swapchainFormat, depthFormat);
    assert(renderPass);

    bool lsr = false;

    Shader meshVS = {};
    lsr = loadShader(meshVS, device, "shaders/mesh.vert.spv");
    assert(lsr);

    Shader meshFS = {};
    lsr = loadShader(meshFS, device, "shaders/mesh.frag.spv");
    assert(lsr);

    Shader drawcmdCS = {};
    lsr = loadShader(drawcmdCS, device, "shaders/drawcmd.comp.spv");

    Shader meshletMS = {};
    Shader meshletTS = {};
    
    if (meshShadingSupported)
    {
        lsr = loadShader(meshletMS, device, "shaders/meshlet.mesh.spv");
        assert(lsr);

        lsr = loadShader(meshletTS, device, "shaders/meshlet.task.spv");
        assert(lsr);
    }

    Program drawcmdProgram = createProgram(device, VK_PIPELINE_BIND_POINT_COMPUTE, { &drawcmdCS}, sizeof(MeshDrawCull));
    VkPipelineCache pipelineCache = 0;
    VkPipeline drawcmdPipeline = createComputePipeline(device, pipelineCache, drawcmdProgram.layout, drawcmdCS );

    Program meshProgram = createProgram(device, VK_PIPELINE_BIND_POINT_GRAPHICS, { &meshVS, &meshFS }, sizeof(Globals));
    
    Program meshProgramMS = {};
    if (meshShadingSupported)
    {
        meshProgramMS = createProgram(device, VK_PIPELINE_BIND_POINT_GRAPHICS, { &meshletTS, &meshletMS, &meshFS }, sizeof(Globals));
    }


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

    Geometry geometry;
    bool rcm = loadMesh(geometry, argv[1], meshShadingSupported);
    assert(rcm);

    loadMesh(geometry, "../data/venus.obj", meshShadingSupported);
    assert(rcm);

    Buffer scratch = {};
    createBuffer(scratch, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    Buffer mb = {};
    createBuffer(mb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer vb = {};
    createBuffer(vb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer ib = {};
    createBuffer(ib, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer mlb = {}; // meshlet buffer
    Buffer mdb = {}; // meshlet data buffer
    if (meshShadingSupported)
    {
        createBuffer(mlb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        createBuffer(mdb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    uploadBuffer(device, cmdPool, cmdBuffer, queue, mb, scratch, geometry.meshes.data(), geometry.meshes.size() * sizeof(Mesh));

    uploadBuffer(device, cmdPool, cmdBuffer, queue, vb, scratch, geometry.vertices.data(), geometry.vertices.size() * sizeof(Vertex));
    uploadBuffer(device, cmdPool, cmdBuffer, queue, ib, scratch, geometry.indices.data(), geometry.indices.size() * sizeof(uint32_t));

    if (meshShadingSupported)
    {
        uploadBuffer(device, cmdPool, cmdBuffer, queue, mlb, scratch, geometry.meshlets.data(), geometry.meshlets.size() * sizeof(Meshlet));
        uploadBuffer(device, cmdPool, cmdBuffer, queue, mdb, scratch, geometry.meshletdata.data(), geometry.meshletdata.size() * sizeof(uint32_t));
    }

    Image colorTarget = {};
    Image depthTarget = {};
    VkFramebuffer targetFrameBuffer = 0;

    // imgui
    UI ui = {};
    initializeUI(ui, device, queue, 1.3f);
    prepareUIPipeline(ui, pipelineCache, renderPass);
    prepareUIResources(ui, memoryProps, cmdPool);


    uint32_t drawCount = 1000;
    double triangleCount = 0.0;
    std::vector<MeshDraw> meshDraws(drawCount);
    
    srand(42);
    for (uint32_t i = 0; i < drawCount; i++)
    {
        uint32_t meshIdx = rand() % geometry.meshes.size();
        Mesh& mesh = geometry.meshes[meshIdx];

        meshDraws[i].pos[0] = (float(rand()) / RAND_MAX) * 40 -20;
        meshDraws[i].pos[1] = (float(rand()) / RAND_MAX) * 40 -20;
        meshDraws[i].pos[2] = (float(rand()) / RAND_MAX) * 40 -20;
        meshDraws[i].scale = (float(rand()) / RAND_MAX)+ 1.f;
        meshDraws[i].orit = glm::rotate(
            quat(1, 0, 0, 0)
            , glm::radians((float(rand()) / RAND_MAX) * 90.f)
            , vec3((float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1));
       
        meshDraws[i].meshIdx = meshIdx;
        meshDraws[i].vertexOffset = mesh.vertexOffset;


        triangleCount += double(mesh.indexCount) / 3.;
    }

    Buffer mdrb = {}; //mesh draw buffer
    createBuffer(mdrb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    uploadBuffer(device, cmdPool, cmdBuffer, queue, mdrb, scratch, meshDraws.data(), meshDraws.size() * sizeof(MeshDraw));

    Buffer mdcb = {}; // mesh draw command buffer
    createBuffer(mdcb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer dccb = {}; // draw command count buffer
    createBuffer(dccb, memoryProps, device, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    ProfilingData pd = {};
    pd.meshletCount = (uint32_t)geometry.meshlets.size();
    pd.primitiveCount = (uint32_t)(geometry.indices.size() / 3);

    double deltaTime = 0.;
    double avrageCpuTime = 0.;
    double avrageGpuTime = 0.;
    bool renderUI = false;

    while (!glfwWindowShouldClose(window))
    {

        double frameCpuBegin = glfwGetTime() * 1000.0;

        glfwPollEvents();

        int newWindowWidth = 0, newWindowHeight = 0;
        glfwGetWindowSize(window, &newWindowWidth, &newWindowHeight);

        SwapchainStatus swapchainStatus = resizeSwapchainIfNecessary(swapchain, physicalDevice, device, surface, &familyIndex, swapchainFormat, renderPass);
        if(swapchainStatus == NotReady)
            continue;

        if (swapchainStatus == Resized || !targetFrameBuffer)
        {
            if (colorTarget.image)
                destroyImage(device, colorTarget);
            if (depthTarget.image)
                destroyImage(device, depthTarget);
            if (targetFrameBuffer)
                vkDestroyFramebuffer(device, targetFrameBuffer, 0);

            createImage(colorTarget, device, memoryProps, swapchain.width, swapchain.height, 1 
                , swapchainFormat
                , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT );
            createImage(depthTarget, device, memoryProps, swapchain.width, swapchain.height, 1
                , depthFormat 
                , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

            targetFrameBuffer = createFramebuffer(device, renderPass, colorTarget.imageView, depthTarget.imageView, swapchain.width, swapchain.height);
        }

        // set input data
        input.width = (float)newWindowWidth;
        input.height = (float)newWindowHeight;

        if(deltaTime > 33.3333)
        {
            updateImguiIO(input);
            ImGuiIO& io = ImGui::GetIO();

            io.DisplaySize = ImVec2((float)newWindowWidth, (float)newWindowHeight);
            ImGui::NewFrame();
            ImGui::SetNextWindowSize({400, 700});
            ImGui::Begin("info:");
            
            ImGui::Text("cpu: [%.3f]ms", pd.cpuTime);
            ImGui::Text("avg cpu: [%.3f]ms", pd.avgCpuTime);
            ImGui::Text("gpu: [%.3f]ms", pd.gpuTime);
            ImGui::Text("avg gpu: [%.3f]ms", pd.avgGpuTime);
            ImGui::Text("cs: [%.3f]ms", pd.assemplyCmdTime);
            ImGui::Text("ui: [%.3f]ms", pd.uiTime);
            ImGui::Text("wait: [%.3f]ms", pd.waitTime);
            ImGui::Text("primitives : [%d]", pd.primitiveCount);
            ImGui::Text("meshlets: [%d]", pd.meshletCount);
            ImGui::Text("triangles: [%.2f]M", pd.trangleCount);
            ImGui::Text("tri/sec: [%.2f]B", pd.trianglesPerSecond);
            ImGui::Text("draw/src: [%.2f]M", 1000.f / pd.avgCpuTime * drawCount * 1e-6);
            ImGui::Text("frame: [%.2f]fps", 1000.f / pd.avgCpuTime);
            ImGui::Checkbox("Mesh Shading", &meshShadingEnabled);
            ImGui::Spacing();
            ImGui::Text("camera: ");
            ImGui::Text("pos: %.2f, %.2f, %.2f", camera.pos.x, camera.pos.y, camera.pos.z);
            ImGui::Text("dir: %.2f, %.2f, %.2f", camera.lookAt.x, camera.lookAt.y, camera.lookAt.z);

            ImGui::End();

            ImGui::Render();
            updateUI(ui, memoryProps);
            deltaTime = 0.0;
            renderUI = true;
        }
        
        uint32_t imageIndex = 0;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain, ~0ull, acquirSemaphore, VK_NULL_HANDLE, &imageIndex));

        VK_CHECK(vkResetCommandPool(device, cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

        vkCmdResetQueryPool(cmdBuffer, queryPool, 0, 128);
        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 0);
        
        mat4 view = glm::lookAt(camera.pos, camera.lookAt, camera.up);
        mat4 projection = persectiveProjection(glm::radians(70.f), (float)swapchain.width / (float)swapchain.height, 0.5f);
        Globals globals = {};
        globals.vp = projection * view;
        globals.cameraPos = camera.pos;

        mat4 projectionT = glm::transpose(projection);
        vec4 frustumX = normalizePlane(projectionT[3] - projectionT[0]);
        vec4 frustumY = normalizePlane(projectionT[3] - projectionT[1]);
        MeshDrawCull drawCull = {};
        drawCull.zfar = 200.f;
        drawCull.znear = 0.5f;
        drawCull.frustum[0] = frustumX.x;
        drawCull.frustum[1] = frustumX.z;
        drawCull.frustum[2] = frustumY.y;
        drawCull.frustum[3] = frustumY.z;

        // culling 
        {
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, drawcmdPipeline);
            
            vkCmdFillBuffer(cmdBuffer, dccb.buffer, 0, dccb.size, 0u);
            VkBufferMemoryBarrier initEndBarrier = bufferBarrier(dccb.buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT);
            vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
                , 0, 0, 0, 1, &initEndBarrier, 0, 0);

            vkCmdPushConstants(cmdBuffer, drawcmdProgram.layout, drawcmdProgram.pushConstantStages, 0, sizeof(drawCull), &drawCull);
            
            DescriptorInfo descInfos[] = {mb.buffer, mdrb.buffer, mdcb.buffer, dccb.buffer };
            vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, drawcmdProgram.updateTemplate, drawcmdProgram.layout, 0, descInfos);

            vkCmdDispatch(cmdBuffer, uint32_t((meshDraws.size() + 63) / 64), 1, 1);

            VkBufferMemoryBarrier cullEndBarrier = bufferBarrier(mdcb.buffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
            vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT , VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
                , 0, 0, 0, 1, &cullEndBarrier, 0, 0);

            vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, 1);
        }

        VkImageMemoryBarrier renderBeginBarriers[] = {
            imageBarrier(colorTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),           
            imageBarrier(depthTarget.image, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
        };
        // check https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#combined-graphicspresent-queue
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            , VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ARRAYSIZE(renderBeginBarriers), renderBeginBarriers);
        
        VkClearValue clearValues[2] = {};
        clearValues[0].color = { 128.f/255.f, 109.f/255.f, 158.f/255.f, 1 };
        clearValues[1].depthStencil =  { 0.f, 0 };

        VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        passBeginInfo.renderPass = renderPass;
        passBeginInfo.framebuffer = targetFrameBuffer;
        passBeginInfo.renderArea.extent.width = swapchain.width;
        passBeginInfo.renderArea.extent.height = swapchain.height;
        passBeginInfo.clearValueCount = ARRAYSIZE(clearValues);
        passBeginInfo.pClearValues = clearValues;

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
 
            DescriptorInfo descInfos[] = { mdcb.buffer, mb.buffer, mdrb.buffer, mlb.buffer, mdb.buffer, vb.buffer};
 
            vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, meshProgramMS.updateTemplate, meshProgramMS.layout, 0, descInfos);
            vkCmdPushConstants(cmdBuffer, meshProgramMS.layout, meshProgramMS.pushConstantStages, 0, sizeof(globals), &globals);

            vkCmdDrawMeshTasksIndirectCountEXT(cmdBuffer, mdcb.buffer, offsetof(MeshDrawCommand, indirectMS), dccb.buffer, 0, (uint32_t)meshDraws.size(), sizeof(MeshDrawCommand));
        }
        else
        {
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);
 
            DescriptorInfo descInfos[] = { mdcb.buffer, mdrb.buffer, vb.buffer};
 
            vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, meshProgram.updateTemplate, meshProgram.layout, 0, descInfos);
            vkCmdPushConstants(cmdBuffer, meshProgram.layout, meshProgram.pushConstantStages, 0, sizeof(globals), &globals);

            vkCmdBindIndexBuffer(cmdBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexedIndirectCount(cmdBuffer, mdcb.buffer, offsetof(MeshDrawCommand, indirect), dccb.buffer, 0, (uint32_t)meshDraws.size(), sizeof(MeshDrawCommand));
        }

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 2);
        drawUI(ui, cmdBuffer);
        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 3);

        vkCmdEndRenderPass(cmdBuffer);
       
        VkImageMemoryBarrier copyBarriers[] = {
            imageBarrier(colorTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
            imageBarrier(swapchain.images[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT,  0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
        };
        // check https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#combined-graphicspresent-queue
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
            , VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, ARRAYSIZE(copyBarriers), copyBarriers);

        VkImageCopy copyRegion = {};
        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.extent = { swapchain.width, swapchain.height, 1 };

        vkCmdCopyImage(cmdBuffer, colorTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        
        
        VkImageMemoryBarrier presentBarrier = imageBarrier(swapchain.images[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
            , VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &presentBarrier);
        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 4);

        VK_CHECK(vkEndCommandBuffer(cmdBuffer));

 
        VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

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
        
        uint64_t queryResults[5] = {};
        vkGetQueryPoolResults(device, queryPool, 0, ARRAYSIZE(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT);

        double frameCpuEnd = glfwGetTime() * 1000.0;

        double frameGpuBegin = double(queryResults[0]) * props.limits.timestampPeriod * 1e-6;
        double frameDrawCmdEnd = double(queryResults[1]) * props.limits.timestampPeriod * 1e-6;;
        double frameUIBegin = double(queryResults[2]) * props.limits.timestampPeriod * 1e-6;
        double frameUIEnd = double(queryResults[3]) * props.limits.timestampPeriod * 1e-6;
        double frameGpuEnd = double(queryResults[4]) * props.limits.timestampPeriod * 1e-6;

        avrageCpuTime = avrageCpuTime * 0.95 + (frameCpuEnd - frameCpuBegin) * 0.05;
        avrageGpuTime = avrageGpuTime * 0.95 + (frameGpuEnd - frameGpuBegin) * 0.05;
        pd.avgCpuTime = (float)avrageCpuTime;
        deltaTime += (frameCpuEnd - frameCpuBegin);
        pd.cpuTime = float(frameCpuEnd - frameCpuBegin);
        pd.gpuTime = float(frameGpuEnd - frameGpuBegin);
        pd.avgGpuTime = float(avrageGpuTime);
        pd.assemplyCmdTime = float(frameDrawCmdEnd - frameGpuBegin);
        pd.uiTime = float(frameUIEnd - frameUIBegin);
        pd.waitTime = float(waitTimeEnd - waitTimeBegin);
        pd.trianglesPerSecond = float(triangleCount * 1e-9 *  double(1000 / avrageGpuTime));
        pd.trangleCount = float(triangleCount * 1e-6);
	}

    VK_CHECK(vkDeviceWaitIdle(device));

    destroyImage(device, colorTarget);
    destroyImage(device, depthTarget);
    vkDestroyFramebuffer(device, targetFrameBuffer, 0);
    
    destroyBuffer(device, dccb);
    destroyBuffer(device, mdcb);
    destroyBuffer(device, mdrb);

    destroyUI(ui);
   
    destroyBuffer(device, scratch);
    destroyBuffer(device, ib); 
    destroyBuffer(device, vb);
    destroyBuffer(device, mb);
    if (meshShadingSupported)
    {
        destroyBuffer(device, mlb);
        destroyBuffer(device, mdb);
    }

    vkDestroyCommandPool(device, cmdPool, 0);

    vkDestroyDescriptorPool(device, descPool, 0);

    destroySwapchain(device, swapchain);

    vkDestroyQueryPool(device, queryPool, 0);

    vkDestroyPipeline(device, drawcmdPipeline, 0);
    destroyProgram(device, drawcmdProgram);

    vkDestroyPipeline(device, meshPipeline, 0);
    destroyProgram(device, meshProgram);
    if (meshShadingSupported)
    {
        vkDestroyPipeline(device, meshPipelineMS, 0);
        destroyProgram(device, meshProgramMS);
    }

    vkDestroyShaderModule(device, drawcmdCS.module, 0);
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