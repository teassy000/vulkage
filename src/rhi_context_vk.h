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


    struct PassInfo_vk : PassDesc
    {
        VkPipeline pipeline{};

        uint16_t passId{ kInvalidHandle };
        uint16_t writeDepthId{ kInvalidHandle };

        std::pair<uint16_t, ResInteractDesc> writeDepth;
        UniDataContainer< uint16_t, ResInteractDesc> writeColors;
        UniDataContainer< uint16_t, ResInteractDesc> readImages;
        UniDataContainer< uint16_t, ResInteractDesc> readBuffers;
        UniDataContainer< uint16_t, ResInteractDesc> writeBuffers;
    };

    struct BarrierState_vk
    {
        VkAccessFlags           accessMask;
        VkImageLayout           imgLayout;
        VkPipelineStageFlags    stageMask;

        inline BarrierState_vk()
            : accessMask(0)
            , imgLayout(VK_IMAGE_LAYOUT_UNDEFINED)
            , stageMask(0)
        {}
        
        // image
        inline BarrierState_vk(const VkAccessFlags _accessMask, const VkImageLayout _imgLayout, const VkPipelineStageFlags _stageMask)
            : accessMask(_accessMask)
            , imgLayout(_imgLayout)
            , stageMask(_stageMask)
        {}

        // buffer
        inline BarrierState_vk(const VkAccessFlags _accessMask, const VkPipelineStageFlags _stageMask)
            : accessMask(_accessMask)
            , imgLayout(VK_IMAGE_LAYOUT_UNDEFINED)
            , stageMask(_stageMask)
        {}
    };

    class BarrierDispatcher
    {
    public:
        void addBufBarrier(const VkBuffer _buf, const BarrierState_vk& _src, const BarrierState_vk& _dst);
        void addImgBarrier(const VkImage _img, VkImageAspectFlags _aspect, const BarrierState_vk& _src, const BarrierState_vk& _dst);
        
        void dispatch(const VkCommandBuffer& _cmdBuffer);

    private:
        void clear();

        std::vector<VkImage> m_imgs;
        std::vector<ImageAspectFlags> m_imgAspects;
        std::vector<BarrierState_vk> m_srcImgBarriers;
        std::vector<BarrierState_vk> m_dstImgBarriers;
        
        std::vector<VkBuffer> m_bufs;
        std::vector<BarrierState_vk> m_srcBufBarriers;
        std::vector<BarrierState_vk> m_dstBufBarriers;
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
        void setBrief(MemoryReader& _reader) override;

    private:
        void createInstance();
        void createPhysicalDevice();

        // barriers
        void addBarriers(uint16_t _passId, bool _flush = false);

        void exeutePass(const uint16_t _passId);
        void exeGraphic(const uint16_t _passId);
        void exeCompute(const uint16_t _passId);
        void exeCopy(const uint16_t _passId);

        void copyToSwapchain(uint32_t _swapImgIdx);

    private:
        UniDataContainer<uint16_t, Buffer_vk> m_bufferContainer;
        UniDataContainer<uint16_t, Image_vk> m_imageContainer;
        UniDataContainer<uint16_t, Shader_vk> m_shaderContainer;
        UniDataContainer<uint16_t, Program_vk> m_programContainer;
        UniDataContainer<uint16_t, PassInfo_vk> m_passContainer;

        std::vector<std::vector<uint16_t>> m_programShaderIds;

        // current state for resources, use resource id as id
        UniDataContainer< uint16_t, ResInteractDesc> m_bufBarrierStatus;
        UniDataContainer< uint16_t, ResInteractDesc> m_imgBarrierStatus;

        RHIBrief m_brief;

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

        uint32_t m_swapChainImageIndex{0};

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

        // barrier dispatcher
        BarrierDispatcher m_barrierDispatcher;
    };
} // namespace vkz