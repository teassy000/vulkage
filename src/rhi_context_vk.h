#pragma once

#include "common.h"

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

    struct PassInfo_vk : PassDesc
    {
        VkPipeline pipeline{};

        uint16_t passId{ kInvalidHandle };
        uint16_t writeDepthId{ kInvalidHandle };

        std::vector<uint16_t> writeColorIds;
        std::vector<uint16_t> readImageIds;
        std::vector<uint16_t> rwBufferIds;
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
        std::vector<uint16_t>   m_bufferIds;
        std::vector<Buffer_vk>  m_buffers;

        std::vector<uint16_t>   m_imageIds;
        std::vector<Image_vk>   m_images;

        std::vector<uint16_t>   m_shaderIds;
        std::vector<Shader_vk>  m_shaders;

        std::vector<uint16_t>   m_programIds;
        std::vector<std::vector<uint16_t>> m_programShaderIds;
        std::vector<Program_vk> m_programs;

        std::vector<PassInfo_vk> m_passInfos;

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