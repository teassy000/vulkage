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

        inline bool operator == (const BarrierState_vk& rhs) const {
            return accessMask == rhs.accessMask &&
                imgLayout == rhs.imgLayout &&
                stageMask == rhs.stageMask;
        }

        inline bool operator != (const BarrierState_vk& rhs) const {
            return !(*this == rhs);
        }
    };

    struct PassInfo_vk : PassDesc
    {
        VkPipeline pipeline{};

        uint16_t passId{ kInvalidHandle };
        uint16_t vertexBufferId{ kInvalidHandle };
        uint16_t indexBufferId{ kInvalidHandle };
        uint16_t indirectBufferId{ kInvalidHandle };
        uint16_t indirectCountBufferId{ kInvalidHandle };

        uint16_t writeDepthId{ kInvalidHandle };

        uint32_t indirectBufOffset{ 0 };
        uint32_t indirectCountBufOffset{ 0 };
        uint32_t indirectMaxDrawCount{ 0 };
        uint32_t indirectBufStride{ 0 };

        // barrier status expect in current pass
        std::pair<uint16_t, BarrierState_vk> writeDepth{ kInvalidHandle, {} };
        UniDataContainer< uint16_t, BarrierState_vk> writeColors;
        UniDataContainer< uint16_t, BarrierState_vk> readImages;
        UniDataContainer< uint16_t, BarrierState_vk> readBuffers;
        UniDataContainer< uint16_t, BarrierState_vk> writeBuffers;

        // samplers for image
        UniDataContainer<uint16_t, uint16_t>    imageToSamplerIds;

        // specific image views
        UniDataContainer<uint16_t, SpecificImageViewInfo> imageToSpecificImageViews;
        UniDataContainer<uint16_t, VkImageView> imageToImageViews;

        // write op base to alias
        UniDataContainer<CombinedResID, CombinedResID> writeOpInToOut;

        // for one op passes:
        // copy/blit/fill etc.
        // resources used in this pass
        CombinedResID  oneOpReadRes;
        CombinedResID  oneOpWriteRes;

        // bindings
        std::vector<std::pair<uint32_t, CombinedResID>> bindingToColorIds;
        std::vector<std::pair<uint32_t, CombinedResID>> bindingToResIds;

        PassConfig config{};
    };

    class BarrierDispatcher
    {
    public:
        void addBuffer(const VkBuffer _img, BarrierState_vk _barrierState, const VkBuffer _baseBuf = 0);
        void addImage(const VkImage _img, const ImageAspectFlags _aspect, BarrierState_vk _barrierState, const VkImage _baseImg = 0);

        void barrier(const VkBuffer _buf, const BarrierState_vk& _dst);
        void barrier(const VkImage _img, VkImageAspectFlags _dstAspect, const BarrierState_vk& _dstBarrier);

        void dispatch(const VkCommandBuffer& _cmdBuffer);

        const VkImageLayout getDstImageLayout(const VkImage _img) const;
        const BarrierState_vk getBarrierState(const VkImage _img) const;
        const BarrierState_vk getBarrierState(const VkBuffer _buf) const;

        const BarrierState_vk getBaseBarrierState(const VkImage _img) const;
        const BarrierState_vk getBaseBarrierState(const VkBuffer _buf) const;
    private:
        void clear();

        std::vector<VkImage> m_dispatchImages;
        std::vector<ImageAspectFlags> m_imgAspects;
        std::vector<BarrierState_vk> m_srcImgBarriers;
        std::vector<BarrierState_vk> m_dstImgBarriers;
        
        std::vector<VkBuffer> m_dispatchBuffers;
        std::vector<BarrierState_vk> m_srcBufBarriers;
        std::vector<BarrierState_vk> m_dstBufBarriers;

        std::unordered_map<VkImage, VkImage> m_aliasToBaseImages;
        std::unordered_map<VkBuffer, VkBuffer> m_aliasToBaseBuffers;

        std::unordered_map<VkImage, VkImageAspectFlags> m_imgAspectFlags;
        std::unordered_map<VkImage, BarrierState_vk> m_imgBarrierStatus;
        std::unordered_map<VkBuffer, BarrierState_vk> m_bufBarrierStatus;

        std::unordered_map<VkImage, VkImageAspectFlags> m_baseImgAspectFlags;
        std::unordered_map<VkImage, BarrierState_vk> m_baseImgBarrierStatus;
        std::unordered_map<VkBuffer, BarrierState_vk> m_baseBufBarrierStatus;
    };

    class RHIContext_vk : public RHIContext
    {
    public:
        RHIContext_vk(AllocatorI* _allocator, RHI_Config _config, void* _wnd);

        ~RHIContext_vk() override;
        void init(RHI_Config _config, void* _wnd) override;
        void bake() override;
        bool render() override;

        void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ) override;
        void updateBuffer(BufferHandle _hBuf, const Memory* _mem) override;

    private:

        void createShader(MemoryReader& _reader) override;
        void createProgram(MemoryReader& _reader) override;
        void createPass(MemoryReader& _reader) override;
        void createImage(MemoryReader& _reader) override;
        void createBuffer(MemoryReader& _reader) override;
        void createSampler(MemoryReader& _reader) override;
        void setBrief(MemoryReader& _reader) override;

    private:
        void createInstance();
        void createPhysicalDevice();
        
        void createSpecificImageViews();

        // private pass
        // e.g. upload buffer, copy image, etc.
        // 
        void uploadBuffer(const uint16_t _bufId, const void* data, size_t size);
        void fillBuffer(const uint16_t _bufId, const uint32_t _value, size_t _size);

        // barriers
        void checkUnmatchedBarriers(uint16_t _passId);
        void createBarriers(uint16_t _passId);
        void flushWriteBarriers(uint16_t _passId);

        // push descriptor set with templates
        void pushDescriptorSetWithTemplates(const uint16_t _passId);
        void pushConstants(const uint16_t _passId);

        void exeutePass(const uint16_t _passId);
        void exeGraphic(const uint16_t _passId);
        void exeCompute(const uint16_t _passId);
        void exeCopy(const uint16_t _passId);
        void exeBlit(const uint16_t _passId);
        void exeFillBuffer(const uint16_t _passId);

        void copyToSwapchain(uint32_t _swapImgIdx);

        const Buffer_vk getBuffer(const uint16_t _bufId, bool _base = true) const
        {
            if (_base)
            {
                uint16_t baseId = m_aliasToBaseBuffers.getIdToData(_bufId);
                return m_bufferContainer.getIdToData(baseId);
            }
            return m_bufferContainer.getIdToData(_bufId);
        }

        const Image_vk getImage(const uint16_t _imgId, bool _base = true) const
        {
            if (_base)
            {
                uint16_t baseId = m_aliasToBaseImages.getIdToData(_imgId);
                return m_imageContainer.getIdToData(baseId);
            }
            return m_imageContainer.getIdToData(_imgId);
        }

    private:
        UniDataContainer<uint16_t, Buffer_vk> m_bufferContainer;
        UniDataContainer<uint16_t, Image_vk> m_imageContainer;
        UniDataContainer<uint16_t, Shader_vk> m_shaderContainer;
        UniDataContainer<uint16_t, Program_vk> m_programContainer;
        UniDataContainer<uint16_t, PassInfo_vk> m_passContainer;
        UniDataContainer<uint16_t, VkSampler> m_samplerContainer;

        Buffer_vk m_scratchBuffer;

        std::vector<std::vector<uint16_t>> m_programShaderIds;
        std::vector<uint32_t>           m_progThreadCount;

        UniDataContainer< uint16_t, uint16_t> m_aliasToBaseImages;

        UniDataContainer< uint16_t, uint16_t> m_aliasToBaseBuffers;

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
        VkDescriptorPool m_descPool;

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