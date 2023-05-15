// proj_vk.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "common.h"
#include "math.h"

#include "resources.h"
#include "swapchain.h"
#include "shaders.h"
#include "device.h"

// ui
#include <imgui.h>
#include "uidata.h"
#include "ui.h"

// camera
#include "camera.h"

#include "mesh.h"
#include "scene.h"

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <stdio.h>
#include <vector>
#include <algorithm>
#include <string>

static float deltaFrameTime = 0.f;
static FreeCamera freeCamera = {};

static RenderOptionsData rod = {
    /* bool enableMeshShading = */  true,
    /* bool enableCull = */         true,
    /* bool enableLod = */          true,
    /* bool enableOcclusion = */    true,
    /* bool enableMeshletOC = */    true,
    /* bool enableTaskSubmit =*/    true,
    /* bool showPyramid = */        false,
    /* int  debugPyramidLevel = */  0,
};
static int s_depthPyramidCount = 0;
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

bool checkExtSupportness(const std::vector<VkExtensionProperties>& props, const char* extName, bool print = true)
{
    bool extSupported = false;
    for (const auto& extension : props) {
        if(strcmp(extension.extensionName, extName)){
        //if (std::string(extension.extensionName) == extName) {
            extSupported = true;
            break;
        }
    }

    if (print)
    {
        printf("%s : %s\n", extName, extSupported ? "true" : "false");
    }


    return extSupported;
}

VkQueryPool createQueryPool(VkDevice device, uint32_t queryCount, VkQueryType queryType)
{
    VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    createInfo.queryType = queryType;
    createInfo.queryCount = queryCount;

    if (queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS)
    {
        createInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;
    }


    VkQueryPool queryPool = nullptr;
    VK_CHECK(vkCreateQueryPool(device, &createInfo, 0, &queryPool));
    return queryPool;
}

struct alignas(16) Globals
{
    mat4 projection;

    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
    float screenWidth, screenHeight;
    int enableMeshletOcclusion;
};

struct alignas(16) MeshDrawCull
{
    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float lodBase, lodStep;
    float pyramidWidth, pyramidHeight;

    int32_t enableCull;
    int32_t enableLod;
    int32_t enableOcclusion;
    int32_t enableMeshletOcclusion;
};

struct alignas(16) TransformData
{
    mat4 view;
    vec3 cameraPos;
};

struct MeshTaskCommand
{
    uint32_t drawId;
    uint32_t taskOffset;
    uint32_t taskCount;
    uint32_t lateDrawVisibility;
    uint32_t meshletVisibilityOffset;
};

struct MeshDrawCommand
{
    uint32_t drawId;
    uint32_t taskOffset;
    uint32_t taskCount;
    uint32_t lateDrawVisibility;
    VkDrawIndexedIndirectCommand indirect; // 5 uint32_t
    VkDrawMeshTasksIndirectCommandEXT indirectMS; // 3 uint32_t
};

void enumrateDeviceExtPorps(VkPhysicalDevice physicalDevice, std::vector<VkExtensionProperties>& availableExtensions)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    availableExtensions.resize(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
}


void dumpExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<VkExtensionProperties>& availableExtensions)
{
    checkExtSupportness(availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_8BIT_STORAGE_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_SPIRV_1_4_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
    checkExtSupportness(availableExtensions, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);

}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) 
    {
        freeCameraProcessKeyboard(freeCamera, key, deltaFrameTime);

        if (key == GLFW_KEY_M)
        {
            rod.meshShadingEnabled = !rod.meshShadingEnabled;
        }
        if (key == GLFW_KEY_ESCAPE)
        {
            exit(0);
        }
        if (key == GLFW_KEY_L)
        {
            rod.lodEnabled = !rod.lodEnabled;
        }
        if (key == GLFW_KEY_C)
        {
            rod.objCullEnabled = !rod.objCullEnabled;
        }
        if (key == GLFW_KEY_T)
        {
            rod.taskSubmitEnabled = !rod.taskSubmitEnabled;
        }
        if (key == GLFW_KEY_P)
        {
            rod.showPyramid = !rod.showPyramid;
        }
        if (key == GLFW_KEY_O)
        {
            rod.ocEnabled = !rod.ocEnabled;
        }
        if (key == GLFW_KEY_UP)
        {
            rod.debugPyramidLevel = glm::min(s_depthPyramidCount - 1, ++rod.debugPyramidLevel);
        }
        if (key == GLFW_KEY_DOWN)
        {
            rod.debugPyramidLevel = glm::max(--rod.debugPyramidLevel, 0);
        }
    }
    
    if(action == GLFW_REPEAT)
    {
        freeCameraProcessKeyboard(freeCamera, key, deltaFrameTime);
    }
}

void mousekeyCallback(GLFWwindow* window, int key, int action, int mods)
{
    freeCameraProcessMouseKey(freeCamera, key, action, mods);

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

    freeCameraProcessMouseMovement(freeCamera, (float)xpos, (float)ypos);
}

// left handed?
mat4 perspectiveProjection(float fovY, float aspectWbyH, float zNear)
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

uint32_t previousPow2(uint32_t v)
{
    uint32_t r = 1;

    while (r < v)
        r <<= 1;
    r >>= 1;
    return r;
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

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	VkInstance instance = createInstance();
	assert(instance);

    volkLoadInstanceOnly(instance);

#if _DEBUG
    VkDebugReportCallbackEXT debugCallback = registerDebugCallback(instance);
#endif

	VkPhysicalDevice physicalDevices[16];
	uint32_t deviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices));


	VkPhysicalDevice physicalDevice = pickPhysicalDevice(physicalDevices, deviceCount);
	assert(physicalDevice);

    std::vector<VkExtensionProperties> supportedExtensions;
    enumrateDeviceExtPorps(physicalDevice, supportedExtensions);

    dumpExtensionSupport(physicalDevice, supportedExtensions);

    bool meshShadingSupported = false;
    meshShadingSupported = checkExtSupportness(supportedExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME, false);

    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    assert(props.limits.timestampPeriod);

    uint32_t familyIndex = getGraphicsFamilyIndex(physicalDevice);
    assert(familyIndex != VK_QUEUE_FAMILY_IGNORED);

	VkDevice device = createDevice(instance, physicalDevice, familyIndex, meshShadingSupported);
	assert(device);

    volkLoadDevice(device);

	GLFWwindow* window = glfwCreateWindow(2560, 1440, "mesh_shading_demo", 0, 0);
	assert(window);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mousekeyCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);

	VkSurfaceKHR surface = createSurface(instance, window);
	assert(surface);

    VkBool32 presentSupported = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &presentSupported));
    assert(presentSupported);
    
    VkSemaphore acquirSemaphore = createSemaphore(device);
    assert(acquirSemaphore);

    VkSemaphore releaseSemaphore = createSemaphore(device);
    assert(releaseSemaphore);

    VkFormat imageFormat = getSwapchainFormat(physicalDevice, surface);
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    Swapchain swapchain = {};
    createSwapchain(swapchain, physicalDevice, device, surface, familyIndex, imageFormat);

    VkQueue queue = 0;
    vkGetDeviceQueue(device, familyIndex, 0, &queue);
    assert(queue);

    bool lsr = false;

    Shader meshVS = {};
    lsr = loadShader(meshVS, device, "shaders/mesh.vert.spv");
    assert(lsr);

    Shader meshFS = {};
    lsr = loadShader(meshFS, device, "shaders/mesh.frag.spv");
    assert(lsr);

    Shader drawcmdCS = {};
    lsr = loadShader(drawcmdCS, device, "shaders/drawcmd.comp.spv");
    assert(lsr);

    Shader taskModifiyCS = {};
    lsr = loadShader(taskModifiyCS, device, "shaders/taskModify.comp.spv");
    assert(lsr);

	Shader depthPyramidCS = {};
	lsr = loadShader(depthPyramidCS, device, "shaders/depthpyramid.comp.spv");
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

	VkPipelineCache pipelineCache = 0;

    Program drawcmdProgram = createProgram(device, VK_PIPELINE_BIND_POINT_COMPUTE, { &drawcmdCS}, sizeof(MeshDrawCull));
    VkPipeline drawcmdPipeline = createComputePipeline(device, pipelineCache, drawcmdProgram.layout, drawcmdCS );
    VkPipeline drawcmdLatePipeline = createComputePipeline(device, pipelineCache, drawcmdProgram.layout, drawcmdCS, {/* late = */true});

    VkPipeline taskCullPipeline = createComputePipeline(device, pipelineCache, drawcmdProgram.layout, drawcmdCS, {/* late = */true, /*task = */ true});
    VkPipeline taskCullLatePipeline = createComputePipeline(device, pipelineCache, drawcmdProgram.layout, drawcmdCS, {/* late = */true, /*task = */ true});

    Program taskModifyProgram = createProgram(device, VK_PIPELINE_BIND_POINT_COMPUTE, { &taskModifiyCS});
    VkPipeline taskModifyPipeline = createComputePipeline(device, pipelineCache, taskModifyProgram.layout, taskModifiyCS );

	Program depthPyramidProgram = createProgram(device, VK_PIPELINE_BIND_POINT_COMPUTE, { &depthPyramidCS}, sizeof(vec2));
	VkPipeline depthPyramidPipeline = createComputePipeline(device, pipelineCache, depthPyramidProgram.layout, depthPyramidCS );

    Program meshProgram = createProgram(device, VK_PIPELINE_BIND_POINT_GRAPHICS, { &meshVS, &meshFS }, sizeof(Globals));
    
    Program meshProgramMS = {};
    if (meshShadingSupported)
    {
        meshProgramMS = createProgram(device, VK_PIPELINE_BIND_POINT_GRAPHICS, { &meshletTS, &meshletMS, &meshFS }, sizeof(Globals));
    }

    VkQueryPool queryPoolTimeStemp = createQueryPool(device, 128, VK_QUERY_TYPE_TIMESTAMP);
    assert(queryPoolTimeStemp);

    VkQueryPool queryPoolStatistics = createQueryPool(device, 6, VK_QUERY_TYPE_PIPELINE_STATISTICS);
    assert(queryPoolStatistics);

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

    Scene scene = {};
    {
        bool lsr = loadScene(scene, &argv[1], argc - 1, meshShadingSupported);
        assert(lsr);
    }

    Buffer scratch = {};
    createBuffer(scratch, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    Buffer mb = {};
    createBuffer(mb, memoryProps, device, scene.geometry.meshes.size() * sizeof(Mesh), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer vb = {};
    createBuffer(vb, memoryProps, device, scene.geometry.vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer ib = {};
    createBuffer(ib, memoryProps, device, scene.geometry.indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


    Buffer mlb = {}; // meshlet buffer
    Buffer mdb = {}; // meshlet data buffer
    if (meshShadingSupported)
    {
        createBuffer(mlb, memoryProps, device, scene.geometry.meshlets.size() * sizeof(Meshlet), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        createBuffer(mdb, memoryProps, device, scene.geometry.meshletdata.size() * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    uploadBuffer(device, cmdPool, cmdBuffer, queue, mb, scratch, scene.geometry.meshes.data(), scene.geometry.meshes.size() * sizeof(Mesh));

    uploadBuffer(device, cmdPool, cmdBuffer, queue, vb, scratch, scene.geometry.vertices.data(), scene.geometry.vertices.size() * sizeof(Vertex));
    uploadBuffer(device, cmdPool, cmdBuffer, queue, ib, scratch, scene.geometry.indices.data(), scene.geometry.indices.size() * sizeof(uint32_t));

    if (meshShadingSupported)
    {
        uploadBuffer(device, cmdPool, cmdBuffer, queue, mlb, scratch, scene.geometry.meshlets.data(), scene.geometry.meshlets.size() * sizeof(Meshlet));
        uploadBuffer(device, cmdPool, cmdBuffer, queue, mdb, scratch, scene.geometry.meshletdata.data(), scene.geometry.meshletdata.size() * sizeof(uint32_t));
    }

    Image colorTarget = {};
    Image depthTarget = {};
    Image renderTarget = {};
    
    Image depthPyramid = {};

    uint32_t depthPyramidLevels = 0;
    VkImageView depthPyramidMips[16] = {};

    VkSampler depthSampler = createSampler(device, VK_SAMPLER_REDUCTION_MODE_MIN);
    assert(depthSampler);

    VkPipelineRenderingCreateInfo renderInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachmentFormats = &imageFormat;
    renderInfo.depthAttachmentFormat = depthFormat;

    VkPipeline meshPipeline = createGraphicsPipeline(device, pipelineCache, meshProgram.layout, renderInfo, { &meshVS, &meshFS }, nullptr);
    assert(meshPipeline);

    VkPipeline meshPipelineMS = 0;
    VkPipeline meshLatePipelineMS = 0;
    VkPipeline taskPipelineMS = 0;
    VkPipeline taskLatePipelineMS = 0;
    if (meshShadingSupported)
    {
        meshPipelineMS = createGraphicsPipeline(device, pipelineCache, meshProgramMS.layout, renderInfo, { &meshletTS, &meshletMS, &meshFS }, nullptr);
        assert(meshPipelineMS);
        meshLatePipelineMS = createGraphicsPipeline(device, pipelineCache, meshProgramMS.layout, renderInfo, { &meshletTS, &meshletMS, &meshFS }, nullptr, {/* late = */true});
        assert(meshLatePipelineMS);
        taskPipelineMS = createGraphicsPipeline(device, pipelineCache, meshProgramMS.layout, renderInfo, { &meshletTS, &meshletMS, &meshFS }, nullptr, {/* late = */false, /*task = */ true});
        assert(taskPipelineMS);
        taskLatePipelineMS = createGraphicsPipeline(device, pipelineCache, meshProgramMS.layout, renderInfo, { &meshletTS, &meshletMS, &meshFS }, nullptr, {/* late = */true, /*task = */ true});
        assert(taskLatePipelineMS);
    }

    Buffer mdrb = {}; //mesh draw buffer
    createBuffer(mdrb, memoryProps, device, scene.meshDraws.size() * sizeof(MeshDraw), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    uploadBuffer(device, cmdPool, cmdBuffer, queue, mdrb, scratch, scene.meshDraws.data(), scene.meshDraws.size() * sizeof(MeshDraw));

    Buffer mdcb = {}; // mesh draw command buffer
    createBuffer(mdcb, memoryProps, device, 128 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer mdccb = {}; // draw command count buffer
    createBuffer(mdccb, memoryProps, device, 16, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer tb = {}; // transform data buffer
    createBuffer(tb, memoryProps, device, sizeof(TransformData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer mdvb = {}; // mesh draw visibility buffer
    createBuffer(mdvb, memoryProps, device, scene.drawCount * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    uint32_t meshletVisibilityDataSize = (scene.meshletVisibilityCount + 31) / 32 * sizeof(uint32_t);
    Buffer mlvb = {}; // meshlet visibility buffer
    createBuffer(mlvb, memoryProps, device, meshletVisibilityDataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    // camera
    freeCameraInit(freeCamera, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, 90.f, 0.f);

    // imgui
    UIRendering ui = {};
    initializeUIRendering(ui, device, queue, /* scale = */ 1.3f);
    prepareUIPipeline(ui, pipelineCache, renderInfo);
    prepareUIResources(ui, memoryProps, cmdPool);

    ProfilingData pd = {};
    pd.meshletCount = (uint32_t)scene.geometry.meshlets.size();
    pd.primitiveCount = (uint32_t)(scene.geometry.indices.size() / 3);

    double deltaTime = 0.;
    double avrageCpuTime = 0.;
    double avrageGpuTime = 0.;

    uint32_t pyramidLevelWidth = previousPow2(swapchain.width);
    uint32_t pyramidLevelHeight = previousPow2(swapchain.height);

    bool mdvbCleared = false, mlvbCleared = false;

    while (!glfwWindowShouldClose(window))
    {

        double frameCpuBegin = glfwGetTime() * 1000.0;

        glfwPollEvents();

        int newWindowWidth = 0, newWindowHeight = 0;
        glfwGetWindowSize(window, &newWindowWidth, &newWindowHeight);

        SwapchainStatus swapchainStatus = resizeSwapchainIfNecessary(swapchain, physicalDevice, device, surface, familyIndex, imageFormat);
        if(swapchainStatus == NotReady)
            continue;

        // update/resize render targets
        if (swapchainStatus == Resized || !colorTarget.image)
        {
            if (colorTarget.image)
                destroyImage(device, colorTarget);
            if (depthTarget.image)
                destroyImage(device, depthTarget);
            if (renderTarget.image)
                destroyImage(device, renderTarget);
            if (depthPyramid.image)
            {
                for (uint32_t i = 0; i < depthPyramidLevels; ++i)
                {
                    vkDestroyImageView(device, depthPyramidMips[i], 0);
                }
                destroyImage(device, depthPyramid);
            }

            createImage(colorTarget, device, memoryProps, swapchain.width, swapchain.height, 1, imageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            createImage(depthTarget, device, memoryProps, swapchain.width, swapchain.height, 1, depthFormat , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            createImage(renderTarget, device, memoryProps, swapchain.width, swapchain.height, 1, imageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

            pyramidLevelWidth = previousPow2(swapchain.width);
            pyramidLevelHeight = previousPow2(swapchain.height);
            depthPyramidLevels = calculateMipLevelCount(pyramidLevelWidth, pyramidLevelHeight);
            s_depthPyramidCount = depthPyramidLevels;
            createImage(depthPyramid, device, memoryProps, pyramidLevelWidth, pyramidLevelHeight, depthPyramidLevels, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
            
            for (uint32_t i = 0; i < depthPyramidLevels; ++i)
            {
                depthPyramidMips[i] = createImageView(device, depthPyramid.image, VK_FORMAT_R32_SFLOAT, i, 1);
            }
        }
        
        // update data
        float znear = .5f;
        mat4 projection = perspectiveProjection(glm::radians(70.f), (float)swapchain.width / (float)swapchain.height, znear);
        mat4 projectionT = glm::transpose(projection);
        vec4 frustumX = normalizePlane(projectionT[3] - projectionT[0]);
        vec4 frustumY = normalizePlane(projectionT[3] - projectionT[1]);

        MeshDrawCull drawCull = {};
        drawCull.P00 = projection[0][0];
        drawCull.P11 = projection[1][1];
        drawCull.zfar = scene.drawDistance;
        drawCull.znear = znear;
        drawCull.pyramidWidth = (float)pyramidLevelWidth;
        drawCull.pyramidHeight = (float)pyramidLevelHeight;
        drawCull.frustum[0] = frustumX.x;
        drawCull.frustum[1] = frustumX.z;
        drawCull.frustum[2] = frustumY.y;
        drawCull.frustum[3] = frustumY.z;
        drawCull.enableCull = rod.objCullEnabled ? 1 : 0;
        drawCull.enableLod = rod.lodEnabled ? 1 : 0;
        drawCull.enableOcclusion = rod.ocEnabled ? 1 : 0;
        drawCull.enableMeshletOcclusion = (rod.meshletOcEnabled && rod.meshShadingEnabled) ? 1 : 0;

        mat4 view = freeCameraGetViewMatrix(freeCamera);
        TransformData trans = {};
        trans.view = view;
        trans.cameraPos = freeCamera.pos;

        uploadBuffer(device, cmdPool, cmdBuffer, queue, tb, scratch, &trans, sizeof(trans));


        uint32_t imageIndex = 0;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain, ~0ull, acquirSemaphore, VK_NULL_HANDLE, &imageIndex));

        VK_CHECK(vkResetCommandPool(device, cmdPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

        vkCmdResetQueryPool(cmdBuffer, queryPoolTimeStemp, 0, 128);
        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 0);


        vkCmdResetQueryPool(cmdBuffer, queryPoolStatistics, 0, 6);
        
        // clear mdvb 
        if(!mdvbCleared)
        {
            vkCmdFillBuffer(cmdBuffer, mdvb.buffer, 0, sizeof(uint32_t) * scene.drawCount, 0);

            VkBufferMemoryBarrier2 barrier = bufferBarrier2(mdvb.buffer, 
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 1, &barrier, 0, 0);
            mdvbCleared = true;
        }

        if (meshShadingSupported && !mlvbCleared)
        {
            vkCmdFillBuffer(cmdBuffer, mlvb.buffer, 0, meshletVisibilityDataSize, 0); 

            VkBufferMemoryBarrier2 barrier = bufferBarrier2(mlvb.buffer,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 1, &barrier, 0, 0);
            mlvbCleared = true;
        }

        bool taskSubmitOn = rod.taskSubmitEnabled && rod.meshShadingEnabled && meshShadingSupported;

        auto culling = [&](bool late, VkPipeline pipeline)
        {
            VkBufferMemoryBarrier2 preFillBarrier = bufferBarrier2(mdccb.buffer,
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
                
            pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 1, &preFillBarrier, 0, 0);

            vkCmdFillBuffer(cmdBuffer, mdccb.buffer, 0, 4, 0u); // fill draw command count or taskSizeX

            VkBufferMemoryBarrier2 postFillBarrier = bufferBarrier2(mdccb.buffer,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 1, &postFillBarrier, 0, 0);
            
            vkCmdPushConstants(cmdBuffer, drawcmdProgram.layout, drawcmdProgram.pushConstantStages, 0, sizeof(drawCull), &drawCull);

            VkImageMemoryBarrier2 cullBeginBarriers[] = {
                imageBarrier(depthPyramid.image, VK_IMAGE_ASPECT_COLOR_BIT,
                    late ? VK_ACCESS_SHADER_READ_BIT : 0, late ? VK_IMAGE_LAYOUT_GENERAL: VK_IMAGE_LAYOUT_UNDEFINED, late ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : 0,
                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT), // just set image layout to VK_IMAGE_LAYOUT_GENERAL
            };

            pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, COUNTOF(cullBeginBarriers), cullBeginBarriers);
            {
                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                DescriptorInfo pyramidInfo = { depthSampler, depthPyramid.imageView, VK_IMAGE_LAYOUT_GENERAL };
                DescriptorInfo descInfos[] = { mb.buffer, mdrb.buffer, tb.buffer, mdcb.buffer, mdccb.buffer, mdvb.buffer, pyramidInfo };
                vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, drawcmdProgram.updateTemplate, drawcmdProgram.layout, 0, descInfos);

                vkCmdDispatch(cmdBuffer, uint32_t((scene.meshDraws.size() + drawcmdCS.localSizeX - 1) / drawcmdCS.localSizeX), drawcmdCS.localSizeY, drawcmdCS.localSizeZ);
            }


            if (taskSubmitOn)
            {
                VkBufferMemoryBarrier2 modifyBarrier[] = {
                    bufferBarrier2(mdccb.buffer,
                        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),
                    bufferBarrier2(mdcb.buffer,
                        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),
                };
                pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, COUNTOF(modifyBarrier), modifyBarrier, 0, 0);

                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, taskModifyPipeline);

                DescriptorInfo descInfos[] = { mdccb.buffer, mdcb.buffer};
                vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, taskModifyProgram.updateTemplate, taskModifyProgram.layout, 0, descInfos);

                vkCmdDispatch(cmdBuffer, 1, 1, 1);
            }

            VkBufferMemoryBarrier2 cullEndBarriers[] = {
                bufferBarrier2(mdcb.buffer,
                    VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT),
                bufferBarrier2(mdccb.buffer,
                    VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT),
            };
            pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, COUNTOF(cullEndBarriers), cullEndBarriers, 0, 0);
        };

        auto pyramid = [&]()
        {
            VkImageMemoryBarrier2 pyrimidBeginBarriers[] = {
                imageBarrier(depthTarget.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),
                imageBarrier(depthPyramid.image, VK_IMAGE_ASPECT_COLOR_BIT,
                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),
            };

            pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, COUNTOF(pyrimidBeginBarriers), pyrimidBeginBarriers);

            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, depthPyramidPipeline);

            for (uint32_t i = 0; i < depthPyramidLevels; ++i)
            {
                uint32_t levelWidth = glm::max(1u, pyramidLevelWidth >> i);
                uint32_t levelHeight = glm::max(1u, pyramidLevelHeight >> i);


                vec2 imageSize = { levelWidth, levelHeight };
                vkCmdPushConstants(cmdBuffer, depthPyramidProgram.layout, depthPyramidProgram.pushConstantStages, 0, sizeof(imageSize), &imageSize);

                DescriptorInfo srcDepth = (i == 0)
                    ? DescriptorInfo{ depthSampler, depthTarget.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
                : DescriptorInfo{ depthSampler, depthPyramidMips[i - 1], VK_IMAGE_LAYOUT_GENERAL };

                DescriptorInfo descInfos[] = {
                    srcDepth,
                    {depthPyramidMips[i], VK_IMAGE_LAYOUT_GENERAL}, };
                vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, depthPyramidProgram.updateTemplate, depthPyramidProgram.layout, 0, descInfos);

                vkCmdDispatch(cmdBuffer, calcGroupCount(levelWidth, depthPyramidCS.localSizeX), calcGroupCount(levelHeight, depthPyramidCS.localSizeY), depthPyramidCS.localSizeZ);

                // add barrier for next loop
                VkImageMemoryBarrier2 reduceBarrier = imageBarrier(depthPyramid.image, VK_IMAGE_ASPECT_COLOR_BIT,
                    VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

                pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &reduceBarrier);
            }

            VkImageMemoryBarrier2 depthWriteBarriers[] = {
                imageBarrier(depthTarget.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT),
                imageBarrier(depthPyramid.image, VK_IMAGE_ASPECT_COLOR_BIT,
                    VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),
            };

            pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, COUNTOF(depthWriteBarriers), depthWriteBarriers);

        };

        auto render = [&](bool late, bool taskSubmit, VkClearColorValue& color, VkClearDepthStencilValue& depth, VkPipeline pipeline, int32_t query)
        {
            vkCmdBeginQuery(cmdBuffer, queryPoolStatistics, query, 0);

            VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            colorAttachment.clearValue.color = color;
            colorAttachment.loadOp = late ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            colorAttachment.imageView = colorTarget.imageView;

            VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            depthAttachment.clearValue.depthStencil = depth;
            depthAttachment.loadOp = late ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR;;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            depthAttachment.imageView = depthTarget.imageView;

            VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
            renderingInfo.renderArea.extent.width = swapchain.width;
            renderingInfo.renderArea.extent.height = swapchain.height;
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;
            renderingInfo.pDepthAttachment = &depthAttachment;

            vkCmdBeginRendering(cmdBuffer, &renderingInfo);

            VkViewport viewport = { 0.f, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.f, 1.f };
            VkRect2D scissor = { {0, 0}, {uint32_t(swapchain.width), uint32_t(swapchain.height)} };

            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

            Globals globals = {};
            globals.projection = projection;
            globals.zfar = scene.drawDistance;
            globals.znear = znear;
            globals.frustum[0] = frustumX.x;
            globals.frustum[1] = frustumX.z;
            globals.frustum[2] = frustumY.y;
            globals.frustum[3] = frustumY.z;
            globals.pyramidWidth = (float)pyramidLevelWidth;
            globals.pyramidHeight = (float)pyramidLevelHeight;
            globals.screenWidth = (float)swapchain.width;
            globals.screenHeight = (float)swapchain.height;
            globals.enableMeshletOcclusion = (rod.meshletOcEnabled && rod.ocEnabled && meshShadingSupported && rod.meshShadingEnabled) ? 1 : 0;

            bool meshShadingOn = rod.meshShadingEnabled && meshShadingSupported;

            if (meshShadingOn)
            {
                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                
                DescriptorInfo pyramidInfo = { depthSampler, depthPyramid.imageView, VK_IMAGE_LAYOUT_GENERAL };
                DescriptorInfo descInfos[] = { mdcb.buffer, mb.buffer, mdrb.buffer, mlb.buffer, mdb.buffer, vb.buffer, tb.buffer, mlvb.buffer, pyramidInfo };

                vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, meshProgramMS.updateTemplate, meshProgramMS.layout, 0, descInfos);
                vkCmdPushConstants(cmdBuffer, meshProgramMS.layout, meshProgramMS.pushConstantStages, 0, sizeof(globals), &globals);
                
                if (taskSubmit)
                    vkCmdDrawMeshTasksIndirectEXT(cmdBuffer, mdccb.buffer, 4, 1, 0);
                else
                    vkCmdDrawMeshTasksIndirectCountEXT(cmdBuffer, mdcb.buffer, offsetof(MeshDrawCommand, indirectMS), mdccb.buffer, 0, (uint32_t)scene.meshDraws.size(), sizeof(MeshDrawCommand));
            }
            else
            {
                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

                DescriptorInfo descInfos[] = { mdcb.buffer, mdrb.buffer, vb.buffer, tb.buffer };

                vkCmdPushDescriptorSetWithTemplateKHR(cmdBuffer, meshProgram.updateTemplate, meshProgram.layout, 0, descInfos);
                vkCmdPushConstants(cmdBuffer, meshProgram.layout, meshProgram.pushConstantStages, 0, sizeof(globals), &globals);

                vkCmdBindIndexBuffer(cmdBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexedIndirectCount(cmdBuffer, mdcb.buffer, offsetof(MeshDrawCommand, indirect), mdccb.buffer, 0, (uint32_t)scene.meshDraws.size(), sizeof(MeshDrawCommand));
            }
            
            vkCmdEndRendering(cmdBuffer);
            vkCmdEndQuery(cmdBuffer, queryPoolStatistics, query);
        };
        
        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 1);

        culling(/* late = */false, taskSubmitOn ? taskCullPipeline : drawcmdPipeline);
        
        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 2);


        VkImageMemoryBarrier2 renderBeginBarriers[] = {
            imageBarrier(colorTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, 
            0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT),
            imageBarrier(depthTarget.image, VK_IMAGE_ASPECT_DEPTH_BIT, 
            0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT),
        };

        pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, COUNTOF(renderBeginBarriers), renderBeginBarriers);
       
        VkClearColorValue clearColor = { 128.f/255.f, 109.f/255.f, 158.f/255.f, 1 };
        VkClearDepthStencilValue clearDepth = { 0.f, 0 };

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 3);

        render(/* late = */false, /*taskSubmit = */ taskSubmitOn, clearColor, clearDepth, taskSubmitOn ? taskPipelineMS : meshPipelineMS, 0);
       
        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 4);


        pyramid();

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 5);

        culling(/* late = */true, taskSubmitOn ? taskCullLatePipeline : drawcmdLatePipeline);

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 6);

        render(/* late = */true, /*taskSubmit = */ taskSubmitOn, clearColor, clearDepth, taskSubmitOn ? taskLatePipelineMS : meshLatePipelineMS, 1);

        
        VkImageMemoryBarrier2 copyBarriers[] = {
            imageBarrier(colorTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, 
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_TRANSFER_READ_BIT , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT),
            imageBarrier(depthPyramid.image, VK_IMAGE_ASPECT_COLOR_BIT, 
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT),
            imageBarrier(renderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT,
                0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT),
        };

        pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, COUNTOF(copyBarriers), copyBarriers);

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 7);
        
        // copy scene image to UI pass
        if (rod.showPyramid)
        {
            uint32_t levelWidth = glm::max(1u, pyramidLevelWidth >> rod.debugPyramidLevel);
            uint32_t levelHeight = glm::max(1u, pyramidLevelHeight >> rod.debugPyramidLevel);

            VkImageBlit regions[1] = {};
            regions[0].srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[0].srcSubresource.mipLevel = rod.debugPyramidLevel;
            regions[0].srcSubresource.layerCount = 1;
            regions[0].dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[0].dstSubresource.layerCount = 1;

            regions[0].srcOffsets[0] = { 0, 0, 0 };
            regions[0].srcOffsets[1] = { int32_t(levelWidth), int32_t(levelHeight), 1};
            regions[0].dstOffsets[0] = { 0, 0, 0 };
            regions[0].dstOffsets[1] = { int32_t(swapchain.width), int32_t(swapchain.height), 1 };

            vkCmdBlitImage(cmdBuffer, depthPyramid.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, renderTarget.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, COUNTOF(regions), regions, VK_FILTER_NEAREST);
        }
        else
        {
            VkImageCopy copyRegion = {};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.extent = { swapchain.width, swapchain.height, 1 };

            vkCmdCopyImage(cmdBuffer, colorTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, renderTarget.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        }

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 8);

        VkImageMemoryBarrier2 uiBarriers[] = {
            imageBarrier(renderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT ,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT),
        };

        pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, COUNTOF(uiBarriers), uiBarriers);

        // draw UI: 
        // actual data from last frame(need the ui rendering time too).
        {
            VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            colorAttachment.imageView = renderTarget.imageView;

            VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            depthAttachment.imageView = depthTarget.imageView;

            VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
            renderingInfo.renderArea.extent.width = swapchain.width;
            renderingInfo.renderArea.extent.height = swapchain.height;
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;
            renderingInfo.pDepthAttachment = &depthAttachment;

            vkCmdBeginRendering(cmdBuffer, &renderingInfo);

            vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 9);
            drawUI(ui, cmdBuffer);
            vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 10);

            vkCmdEndRendering(cmdBuffer);
        }

        VkImageMemoryBarrier2 finalCopyBarriers[] = {
            imageBarrier(renderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT),
            imageBarrier(swapchain.images[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT,
                0, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT),
        };

        pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, COUNTOF(finalCopyBarriers), finalCopyBarriers);

        // copy to swapchain
        {
            VkImageCopy copyRegion = {};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.extent = { swapchain.width, swapchain.height, 1 };

            vkCmdCopyImage(cmdBuffer, renderTarget.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        }
        
        VkImageMemoryBarrier2 presentBarrier = imageBarrier(swapchain.images[imageIndex], VK_IMAGE_ASPECT_COLOR_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

        pipelineBarrier(cmdBuffer, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 1, &presentBarrier);

        vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPoolTimeStemp, 11);
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

        // update profile data
        {
            uint64_t queryResults[13] = {};
            vkGetQueryPoolResults(device, queryPoolTimeStemp, 0, COUNTOF(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT);

            uint64_t pipelineResults[6] = {};
            vkGetQueryPoolResults(device, queryPoolStatistics, 0, COUNTOF(pipelineResults), sizeof(pipelineResults), pipelineResults, sizeof(pipelineResults[0]), VK_QUERY_RESULT_64_BIT);

            uint64_t tcEarly = pipelineResults[0];
            uint64_t tcLate = pipelineResults[1];
            uint64_t triangleCount = tcEarly + tcLate;
            double frameCpuEnd = glfwGetTime() * 1000.0;

            double frameGpuBegin = double(queryResults[0]) * props.limits.timestampPeriod * 1e-6;
            double frameCullBegin = double(queryResults[1]) * props.limits.timestampPeriod * 1e-6;
            double frameCullEnd = double(queryResults[2]) * props.limits.timestampPeriod * 1e-6;
            double frameDrawBegin = double(queryResults[3]) * props.limits.timestampPeriod * 1e-6;
            double frameDrawEnd = double(queryResults[4]) * props.limits.timestampPeriod * 1e-6;
            double framePyramidBegin = double(queryResults[4]) * props.limits.timestampPeriod * 1e-6;
            double framePyramidEnd = double(queryResults[5]) * props.limits.timestampPeriod * 1e-6;
            double frameLateCullEnd = double(queryResults[6]) * props.limits.timestampPeriod * 1e-6;
            double frameCopyBegin = double(queryResults[7]) * props.limits.timestampPeriod * 1e-6;
            double frameCopyEnd = double(queryResults[8]) * props.limits.timestampPeriod * 1e-6;;
            double frameUIBegin = double(queryResults[9]) * props.limits.timestampPeriod * 1e-6;
            double frameUIEnd = double(queryResults[10]) * props.limits.timestampPeriod * 1e-6;
            double frameGpuEnd = double(queryResults[11]) * props.limits.timestampPeriod * 1e-6;

            avrageCpuTime = avrageCpuTime * 0.95 + (frameCpuEnd - frameCpuBegin) * 0.05;
            avrageGpuTime = avrageGpuTime * 0.95 + (frameGpuEnd - frameGpuBegin) * 0.05;
            pd.avgCpuTime = (float)avrageCpuTime;
            pd.cpuTime = float(frameCpuEnd - frameCpuBegin);
            pd.gpuTime = float(frameGpuEnd - frameGpuBegin);
            pd.avgGpuTime = float(avrageGpuTime);
            pd.cullTime = float(frameCullEnd - frameCullBegin);
            pd.drawTime = float(frameDrawEnd - frameDrawBegin);
            pd.pyramidTime = float(framePyramidEnd - framePyramidBegin);
            pd.lateRender = float(frameCopyBegin - frameLateCullEnd);
            pd.lateCullTime = float(frameLateCullEnd - framePyramidEnd);
            pd.uiTime = float(frameUIEnd - frameUIBegin);
            pd.waitTime = float(waitTimeEnd - waitTimeBegin);
            pd.trianglesPerSec = float(triangleCount * 1e-9 * double(1000 / avrageGpuTime));
            pd.triangleEarlyCount = float(tcEarly * 1e-6);
            pd.triangleLateCount = float(tcLate * 1e-6);
            pd.triangleCount = float(triangleCount * 1e-6);

            deltaFrameTime = float(frameCpuEnd - frameCpuBegin);
            deltaTime += deltaFrameTime;
        }

        // update UI
        if (deltaTime > 33.3333)
        {
            // set input data
            input.width = (float)newWindowWidth;
            input.height = (float)newWindowHeight;

            LogicData ld = {};
            ld.cameraPos = freeCamera.pos;
            ld.cameraFront = freeCamera.front;
            updateImGui(input, rod, pd, ld);

            updateUIRendering(ui, memoryProps);
            deltaTime = 0.0;
        }
	}

    VK_CHECK(vkDeviceWaitIdle(device));

    for (uint32_t i = 0; i < depthPyramidLevels; ++i)
    {
        vkDestroyImageView(device, depthPyramidMips[i], 0);
    }
    

    vkDestroySampler(device, depthSampler, 0);
    destroyImage(device, renderTarget);
    destroyImage(device, depthPyramid);
    destroyImage(device, colorTarget);
    destroyImage(device, depthTarget);

    destroyUIRendering(ui);

    destroyBuffer(device, mlvb);
    destroyBuffer(device, mdvb);
    destroyBuffer(device, tb);
    destroyBuffer(device, mdccb);
    destroyBuffer(device, mdcb);
    destroyBuffer(device, mdrb);
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

    vkDestroyQueryPool(device, queryPoolStatistics, 0); 
    vkDestroyQueryPool(device, queryPoolTimeStemp, 0);

    vkDestroyPipeline(device, depthPyramidPipeline, 0);
    destroyProgram(device, depthPyramidProgram);

    vkDestroyPipeline(device, taskModifyPipeline, 0);
    destroyProgram(device, taskModifyProgram);

    vkDestroyPipeline(device, taskCullLatePipeline, 0);
    vkDestroyPipeline(device, taskCullPipeline, 0);
    vkDestroyPipeline(device, drawcmdLatePipeline, 0);
    vkDestroyPipeline(device, drawcmdPipeline, 0);
    destroyProgram(device, drawcmdProgram);

    vkDestroyPipeline(device, meshPipeline, 0);
    destroyProgram(device, meshProgram);
    
    if (meshShadingSupported)
    {
        vkDestroyPipeline(device, taskLatePipelineMS, 0);
        vkDestroyPipeline(device, taskPipelineMS, 0);
        vkDestroyPipeline(device, meshLatePipelineMS, 0);
        vkDestroyPipeline(device, meshPipelineMS, 0);
        destroyProgram(device, meshProgramMS);
    }

    vkDestroyShaderModule(device, depthPyramidCS.module, 0);
    vkDestroyShaderModule(device, taskModifiyCS.module, 0);
    vkDestroyShaderModule(device, drawcmdCS.module, 0);
    vkDestroyShaderModule(device, meshFS.module, 0);
    vkDestroyShaderModule(device, meshVS.module, 0);

    if (meshShadingSupported)
    {
        vkDestroyShaderModule(device, meshletMS.module, 0);
        vkDestroyShaderModule(device, meshletTS.module, 0);
    }

    vkDestroySemaphore(device, releaseSemaphore, 0);
    vkDestroySemaphore(device, acquirSemaphore, 0);
    vkDestroySurfaceKHR(instance, surface, 0);

	glfwDestroyWindow(window);

    vkDestroyDevice(device, 0);

    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
#if _DEBUG
    vkDestroyDebugReportCallbackEXT(instance, debugCallback, 0);
#endif

    vkDestroyInstance(instance, 0);
}
