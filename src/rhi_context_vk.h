#pragma once

#include "common.h"
#include "util.h"

#include "vk_resource.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_shaders.h"
#include "vk_swapchain.h"

#include "vkz_structs_inner.h"

#include "rhi_context.h"

namespace vkz
{
    enum class ShaderStage_vk : uint16_t
    {
        Invalid = 1 << 0,
        Vertex = 1 << 1,
        Mesh = 1 << 2,
        Task = 1 << 3,
        Fragment = 1 << 4,
        Compute = 1 << 5,
    };
    using ShaderStageBits_vk = uint16_t;

    struct PipelineConfig_vk
    {
        uint16_t enableDepthTest;
        uint16_t enableDepthWrite;
        VkCompareOp depthCompOp{ VK_COMPARE_OP_GREATER };
    };


    struct BarrierState_vk
    {
        VkAccessFlags           accessMask;
        VkPipelineStageFlags    stageMask;
        VkImageLayout           imgLayout;
    };

    struct PassInfo_vk : PassDesc
    {
        VkPipeline pipeline{};

        uint16_t passId{ kInvalidHandle };
        uint16_t writeDepthId{ kInvalidHandle };

        std::vector<uint16_t> writeColorIds;
        std::vector<uint16_t> readImageIds;
        std::vector<uint16_t> rwBufferIds;

        // expect barrier state in passes
        UniDataContainer<uint16_t, BarrierState_vk> bufferBarrierStates;
        UniDataContainer<uint16_t, BarrierState_vk> imageBarrierStates;
    };

    class RHIContext_vk : public RHIContext
    {
    public:
        RHIContext_vk(AllocatorI* _allocator);        

        ~RHIContext_vk() override;
        void init() override;
        void render() override;
    private:

        void createShader(MemoryReader& _reader) override;
        void createProgram(MemoryReader& _reader) override;
        void createPass(MemoryReader& _reader) override;
        void createImage(MemoryReader& _reader) override;
        void createBuffer(MemoryReader& _reader) override;

    private:
        void createInstance();
        void createPhysicalDevice();

        void passRender(uint16_t _passId);

    private:
        UniDataContainer<uint16_t, Buffer_vk> m_bufferContainer;
        UniDataContainer<uint16_t, Image_vk> m_imageContainer;
        UniDataContainer<uint16_t, Shader_vk> m_shaderContainer;
        UniDataContainer<uint16_t, Program_vk> m_programContainer;
        UniDataContainer<uint16_t, PassInfo_vk> m_passContainer;

        std::vector<std::vector<uint16_t>> m_programShaderIds;

        // current state for resources, use resource id as id
        UniDataContainer<uint16_t, BarrierState_vk> m_bufferBarrierStates;
        UniDataContainer<uint16_t, BarrierState_vk> m_imageBarrierStates;

        // glfw data
        GLFWwindow* m_pWindow;

        // vulkan context data
        VkInstance m_instance;
        VkDevice m_device;
        VkPhysicalDevice m_phyDevice;
        VkPhysicalDeviceMemoryProperties m_memProps;
        VkSurfaceKHR m_surface;
        Swapchain_vk m_swapchain;

        VkQueue m_queue;
        VkCommandPool   m_cmdPool;
        VkCommandBuffer m_cmdBuffer;

        VkSemaphore m_acquirSemaphore;
        VkSemaphore m_releaseSemaphore;

        VkFormat m_imageFormat;
        VkFormat m_depthFormat;

        uint32_t m_gfxFamilyIdx;

#if _DEBUG
        VkDebugReportCallbackEXT m_debugCallback;
#endif

        // support
        bool m_supportMeshShading{ false };


    };
} // namespace vkz