#pragma once

#include "common.h"
#include "util.h"

#include "volk.h"

#include "vk_resource.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_shaders.h"
#include "vk_swapchain.h"

#include "kage_inner.h"
#include "kage_rhi_vk.h"

#include "rhi_context.h"
#include "command_buffer.h"

namespace kage { namespace vk
{
    template<typename Ty>
    void release(Ty)
    {
    }

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

        VkAccessFlags           accessMask;
        VkImageLayout           imgLayout;
        VkPipelineStageFlags    stageMask;
    };

    struct Bindless_vk
    {
        VkDescriptorSetLayout   layout{ VK_NULL_HANDLE };
        VkDescriptorSet         set{ VK_NULL_HANDLE };
        VkDescriptorPool        pool{ VK_NULL_HANDLE };
        uint32_t                setIdx{ 0 };
        uint32_t                setCount{ 0 };
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

        uint32_t indexCount{ 0 };
        uint32_t vertexCount{ 0 };

        // barrier status expect in current pass
        std::pair<uint16_t, BarrierState_vk> writeDepth{ kInvalidHandle, {} };

        ContinuousMap< uint16_t, BarrierState_vk> writeColors;
        ContinuousMap< uint16_t, BarrierState_vk> readImages;
        ContinuousMap< uint16_t, BarrierState_vk> readBuffers;
        ContinuousMap< uint16_t, BarrierState_vk> writeBuffers;

        // write op base to alias
        ContinuousMap<CombinedResID, CombinedResID> writeOpInToOut;

        bool recorded{ false };
    };

    struct BarrierDispatcher
    {
        struct ImageStatus
        {
            ImageStatus()
                : image(VK_NULL_HANDLE)
                , aspect(0)
                , srcState()
                , dstState()
            {

            }

            ImageStatus(const VkImage _vk, VkImageAspectFlags _aspect, BarrierState_vk _state)
                : image(_vk)
                , aspect(_aspect)
                , srcState(_state)
                , dstState()
            {
            }

            VkImage image;
            VkImageAspectFlags aspect;
            BarrierState_vk srcState;
            BarrierState_vk dstState;
        };

        struct BufferStatus
        {
            BufferStatus()
                : buffer(VK_NULL_HANDLE)
                , srcState()
                , dstState()
            {
            }

            BufferStatus(const VkBuffer _vk, BarrierState_vk _state)
                : buffer(_vk)
                , srcState(_state)
                , dstState()
            {
            }

            VkBuffer buffer;
            BarrierState_vk srcState;
            BarrierState_vk dstState;
        };

        ~BarrierDispatcher();

        void track(
            const VkBuffer _buf
            , BarrierState_vk _state
            , const VkBuffer _baseBuf = { VK_NULL_HANDLE }
        );

        void track(
            const VkImage _img
            , const VkImageAspectFlags _aspect
            , BarrierState_vk _state
            , const VkImage _baseBuf = { VK_NULL_HANDLE }
        );

        void untrack(const VkBuffer _hBuf);
        void untrack(const VkImage _hImg);


        void validate(
            const VkBuffer _hBuf
            , const BarrierState_vk& _state
        );

        void validate(
            const VkImage _hImg
            , VkImageAspectFlags _dstAspect
            , const BarrierState_vk& _dstBarrier
        );


        void barrier(
            const VkBuffer _hBuf
            , const BarrierState_vk& _dst
        );

        void barrier(
            const VkImage _hImg
            , VkImageAspectFlags _dstAspect
            , const BarrierState_vk& _dstBarrier
        );

        void dispatch(const VkCommandBuffer& _cmdBuffer);
        void dispatchGlobalBarrier(
            const VkCommandBuffer& _cmdBuffer
            , const BarrierState_vk& _src
            , const BarrierState_vk& _dst
        );

        VkImageLayout getCurrentImageLayout(const VkImage _img) const;
        BarrierState_vk getBarrierState(const VkImage _img) const;
        BarrierState_vk getBarrierState(const VkBuffer _buf) const;

        BarrierState_vk getBaseBarrierState(const VkImage _img) const;
        BarrierState_vk getBaseBarrierState(const VkBuffer _buf) const;
    private:
        void clearPending();

        stl::unordered_set<VkBuffer> m_pendingBuffers;
        stl::unordered_set<VkImage> m_pendingImages;

        stl::unordered_map<VkBuffer, VkBuffer> m_aliasToBaseBuffers;
        stl::unordered_map<VkImage, VkImage> m_aliasToBaseImages;

        stl::unordered_map<VkBuffer, BufferStatus> m_trackingBuffers;
        stl::unordered_map<VkImage, ImageStatus> m_trackingImages;

        stl::unordered_map<VkBuffer, BufferStatus> m_baseBufferStatus;
        stl::unordered_map<VkImage, ImageStatus> m_baseImageStatus;
    };

    struct CommandQueue_vk
    {
        void init(uint32_t _familyIdx, VkQueue _queue, uint32_t _numFramesInFlight);
        void reset();
        void shutdown();

        void alloc(VkCommandBuffer* _cmdBuf);
        void addWaitSemaphore(VkSemaphore _semaphore, VkPipelineStageFlags _stage);
        void addSignalSemaphore(VkSemaphore _semaphore);

        void kick(bool _wait = false);
        void finish(bool _finishAll = false);

        void release(uint64_t _handle, VkObjectType _type);
        void consume();

        uint32_t m_queueFamilyIdx;
        VkQueue m_queue;

        uint32_t m_numFramesInFlight;

        uint32_t m_currentFrameInFlight;
        uint32_t m_consumeIndex;

        VkCommandBuffer m_activeCommandBuffer;

        VkFence m_currentFence;
        VkFence m_completedFence;

        uint64_t m_submitted;

        struct CommandList
        {
            VkCommandPool m_commandPool = VK_NULL_HANDLE;
            VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
            VkFence m_fence = VK_NULL_HANDLE;
        };

        CommandList m_commandList[kMaxNumFrameLatency];

        uint32_t             m_numWaitSemaphores;
        VkSemaphore          m_waitSemaphores[kMaxNumFrameBuffers];
        VkPipelineStageFlags m_waitSemaphoreStages[kMaxNumFrameBuffers];
        uint32_t             m_numSignalSemaphores;
        VkSemaphore          m_signalSemaphores[kMaxNumFrameBuffers];

        struct Resource
        {
            VkObjectType m_type;
            uint64_t m_handle;
        };

        typedef stl::vector<Resource> ResourceArray;
        ResourceArray m_release[kMaxNumFrameLatency];

    private:
        template<typename Ty>
        void destroy(uint64_t _handle)
        {
            using vk_t = decltype(Ty::vk);
            Ty obj = vk_t(_handle);
            vkDestroy(obj);
        }
    };

    struct ScratchBuffer
    {
        void create(uint32_t _size, uint32_t _count);
        bool occupy(VkDeviceSize& _offset, const void* _data, uint32_t _size);
        void flush();

        void reset();
        void destroy();

        VkBuffer get() const
        {
            return m_buf.buffer;
        }

        Buffer_vk m_buf;
        uint32_t m_offset;
    };

    struct FrameRecCmds
    {
        struct RecCmdRange
        {
            uint32_t startIdx;
            uint32_t endIdx;
        };

        void init();

        void record(const PassHandle _hPass, const CommandQueue& queue, uint32_t _offset, uint32_t _count);

        CommandQueue& actPass(const PassHandle _hPass);

        void finish();
        void start();

        using  RecCmdRangeMap = stl::unordered_map<PassHandle, RecCmdRange>;
        RecCmdRangeMap m_recCmdRange;

        CommandQueue m_cmdQueue;
    };

    struct RHIContext_vk : public RHIContext
    {
        RHIContext_vk(bx::AllocatorI* _allocator);
        ~RHIContext_vk() override;

        void init(const Resolution& _resolution, void* _wnd) override;
        void bake() override;
        bool run() override;

        bool render();
        void kick(bool _finishAll = false);
        void shutdown();

        bool checkSupports(VulkanSupportExtension _ext) override;

        void updateResolution(const Resolution& _resolution) override;

        void updateBuffer(
            const BufferHandle _hBuf
            , const Memory* _mem
            , const uint32_t _offset
            , const uint32_t _size
        ) override;

        void updateImage(
            const ImageHandle _hImg
            , const uint16_t _width
            , const uint16_t _height
            , const Memory* _mem
        ) override;

        void updateImageWithAlias(
            const ImageHandle _hImg
            , const uint16_t _width
            , const uint16_t _height
            , const Memory* _mem
            , const stl::vector<ImageHandle>& _alias
        );

        VkBuffer getVkBuffer(const BufferHandle _hBuf) const
        {
            assert(m_bufferContainer.exist(_hBuf.id));

            return m_bufferContainer.getIdToData(_hBuf.id).buffer;
        }
        VkImage getVkImage(const ImageHandle _hImg) const
        {
            assert(m_imageContainer.exist(_hImg.id));

            return m_imageContainer.getIdToData(_hImg.id).image;
        }

        void pushDescriptorSetWithTemplates(const VkCommandBuffer& _cmdBuf, const uint16_t _passId) const;

        const Shader_vk& getShader(const ShaderHandle _hShader) const;
        const Program_vk& getProgram(const PassHandle _hPass) const;

        void beginRendering(const VkCommandBuffer& _cmdBuf, const uint16_t _passId) const;
        void endRendering(const VkCommandBuffer& _cmdBuf) const;

        const DescriptorInfo getImageDescInfo(const ImageHandle _hImg, uint16_t _mip, const SamplerHandle _hSampler);
        const DescriptorInfo getBufferDescInfo(const BufferHandle _hBuf) const;

        // barriers
        void dispatchBarriers();

        double getPassTime(const PassHandle _hPass) override;
        double getGPUTime() override;
        uint64_t getPassClipping(const PassHandle _hPass) override;


        void createShader(bx::MemoryReader& _reader) override;
        void createProgram(bx::MemoryReader& _reader) override;
        void createPass(bx::MemoryReader& _reader) override;
        void createImage(bx::MemoryReader& _reader) override;
        void createBuffer(bx::MemoryReader& _reader) override;
        void createSampler(bx::MemoryReader& _reader) override;
        void createBindless(bx::MemoryReader& _reader) override;
        void setBrief(bx::MemoryReader& _reader) override;

        void setName(Handle _h, const char* _name, uint32_t _len) override;

        // rendering command start
        void setRecord(
            PassHandle _hPass
            , const CommandQueue& _cq
            , const uint32_t _offset
            , const uint32_t _size
        ) override;

        void setConstants(
            PassHandle _hPass
            , const Memory* _mem
        );

        void pushDescriptorSet(
            PassHandle _hPass
            , const Memory* _mem
        );

        void setColorAttachments(
            PassHandle _hPass
            , const Memory* _mem
        );

        void setDepthAttachment(
            PassHandle _hPass
            , Attachment _depth
        );

        void setBindless(
            PassHandle _hPass
            , BindlessHandle _hBindless
        );

        void setViewport(
            PassHandle _hPass
            , int32_t _x
            , int32_t _y
            , uint32_t _w
            , uint32_t _h
        );

        void setScissor(
            PassHandle _hPass
            , int32_t _x
            , int32_t _y
            , uint32_t _w
            , uint32_t _h
        );

        void setVertexBuffer(
            PassHandle _hPass
            , BufferHandle _hBuf
        );

        void setIndexBuffer(
            PassHandle _hPass
            , BufferHandle _hBuf
            , uint32_t _offset
            , IndexType _type
        );

        void fillBuffer(
            PassHandle _hPass
            , BufferHandle _hBuf
            , uint32_t _value
        );

        void dispatch(
            PassHandle _hPass
            , uint32_t _x
            , uint32_t _y
            , uint32_t _z
        );

        void draw(
            PassHandle _hPass
            , uint32_t _vtxCount
            , uint32_t _instCount
            , uint32_t _firstIdx
            , uint32_t _firstInst
        );

        void drawIndexed(
            PassHandle _hPass
            , uint32_t _idxCount
            , uint32_t _instCount
            , uint32_t _firstIdx
            , uint32_t _vtxOffset
            , uint32_t _firstInst
        );

        void drawIndirect(
            PassHandle _hPass
            , BufferHandle _hIndirectBuf
            , uint32_t _offset
            , uint32_t _drawCount
        );

        void drawIndexedIndirect(
            PassHandle _hPass
            , BufferHandle _hIndirectBuf
            , uint32_t _offset
            , uint32_t _drawCount
            , uint32_t _stride
        );

        void drawIndexedIndirectCount(
            PassHandle _hPass
            , BufferHandle _hIndirectBuf
            , uint32_t _indirectOffset
            , BufferHandle _hIndirectCountBuf
            , uint32_t _countOffset
            , uint32_t _drawCount
            , uint32_t _stride
        );

        void drawMeshTaskIndirect(
            PassHandle _hPass
            , BufferHandle _hBuf
            , uint32_t _offset
            , uint32_t _drawCount
            , uint32_t _stride
        );
        
        // rendering command end
        VkSampler getCachedSampler(SamplerFilter _filter, SamplerMipmapMode _mipmapMode, SamplerAddressMode _addrMd, SamplerReductionMode _reduMd);
        VkImageView getCachedImageView(const ImageHandle _hImg, uint16_t _mip, uint16_t _numMips, uint16_t _numLayers, VkImageViewType _type);

        void createInstance();
        void createPhysicalDevice();

        // private pass
        // e.g. upload buffer, copy image, etc.
        // 
        void uploadBuffer(const uint16_t _bufId, const void* data, uint32_t size);
        void fillBuffer(const uint16_t _bufId, const uint32_t _value, uint32_t _size);
        void uploadImage(const uint16_t _imgId, const void* data, uint32_t size);

        // barriers
        void checkUnmatchedBarriers(uint16_t _passId);
        void createBarriers(uint16_t _passId);
        void flushWriteBarriers(uint16_t _passId);

        // barriers for rec
        void createBarriersRec(const PassHandle _hPass);
        void flushWriteBarriersRec(const PassHandle _hPass);

        // lazy set desc after rec barrier
        void lazySetDescriptorSet(const PassHandle _hPass);

        // push descriptor set with templates
        void executePass(const uint16_t _passId);

        bool checkCopyableToSwapchain(const ImageHandle _hImg) const;
        bool checkBlitableToSwapchain(const ImageHandle _hImg) const;
        void drawToSwapchain(uint32_t _swapImgIdx);
        void blitToSwapchain(uint32_t _swapImgIdx);
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

        const VkDescriptorPool getDescriptorPool(const VkDescriptorSet _descSet) const
        {
            if (m_descSetToPool.find(_descSet) != m_descSetToPool.end())
            {
                const VkDescriptorPool pool = m_descSetToPool.find(_descSet)->second;
                return pool;
            }
            return VK_NULL_HANDLE;
        }

        template<typename Ty>
        void release(Ty& _object);

        // rec
        void execRecQueue(PassHandle _hPass);
        stl::vector<Binding> m_descBindingSetsPerPass;
        stl::vector<Attachment> m_colorAttachPerPass;
        Attachment m_depthAttachPerPass;
        // rec end

        ContinuousMap<uint16_t, Buffer_vk>      m_bufferContainer;
        ContinuousMap<uint16_t, Image_vk>       m_imageContainer;
        ContinuousMap<uint16_t, Shader_vk>      m_shaderContainer;
        ContinuousMap<uint16_t, Program_vk>     m_programContainer;
        ContinuousMap<uint16_t, PassInfo_vk>    m_passContainer;
        ContinuousMap<uint16_t, VkSampler>      m_samplerContainer;
        ContinuousMap<uint16_t, Bindless_vk>    m_bindlessContainer;

        StateCacheT<VkSampler> m_samplerCache;
        StateCacheLru<VkImageView, 1024> m_imgViewCache;

        ContinuousMap<uint16_t, BufferCreateInfo> m_bufferCreateInfos;
        ContinuousMap<uint16_t, ImageCreateInfo> m_imgCreateInfos;

        stl::vector<stl::vector<uint16_t>> m_programShaderIds;
        stl::vector<uint32_t>           m_progThreadCount;

        ContinuousMap< uint16_t, uint16_t> m_aliasToBaseImages;
        ContinuousMap< uint16_t, uint16_t> m_aliasToBaseBuffers;

        stl::vector<ImageHandle> m_colorAttchBase;
        stl::vector<ImageHandle> m_depthAttchBase;
        stl::vector<ImageHandle> m_storageImageBase;

        stl::unordered_map<ImageHandle, stl::vector<ImageHandle>> m_imgToAliases;
        stl::vector<VkDescriptorPool> m_descPools;
        stl::unordered_map<const VkDescriptorSet, const VkDescriptorPool> m_descSetToPool;

        RHIBrief m_brief;

        void* m_nwh;

        ScratchBuffer m_scratchBuffer[kMaxNumFrameLatency];

        // vulkan context data
        VkAllocationCallbacks* m_allocatorCb;

        VkInstance m_instance;
        VkDevice m_device;
        VkPhysicalDevice m_physicalDevice;
        VkPhysicalDeviceMemoryProperties m_memProps;
        VkSurfaceKHR m_surface;
        VkPhysicalDeviceProperties m_phyDeviceProps;

        Swapchain_vk m_swapchain;

        uint32_t m_numFramesInFlight{ kMaxNumFrameLatency };
        CommandQueue_vk m_cmd;
        VkCommandBuffer m_cmdBuffer;

        VkQueue m_queue;

        Resolution m_resolution;

        bool m_updated = false;

        VkFormat m_imageFormat;
        VkFormat m_depthFormat;

        uint32_t m_gfxFamilyIdx;
        // support
        bool m_supportMeshShading{ false };

        // barrier dispatcher
        BarrierDispatcher m_barrierDispatcher;

        // naive profiling
        VkQueryPool m_queryPoolTimeStamp;
        uint32_t m_queryTimeStampCount{ 0 };
        stl::unordered_map<uint16_t, double> m_passTime;
        double m_gpuTime{ 0.0 };

        VkQueryPool m_queryPoolStatistics;
        uint32_t m_queryStatisticsCount{ 0 };
        stl::unordered_map<uint16_t, uint64_t> m_passStatistics;

        FrameRecCmds m_frameRecCmds;
        VkDebugReportCallbackEXT m_debugCallback;

        // tracy
        void initTracy(VkQueue _queue, uint32_t _familyIdx);
        void destroyTracy();
#if TRACY_ENABLE
        KG_ProfCtxType* m_tracyVkCtx = nullptr;
        VkCommandPool m_tracyCmdPool = VK_NULL_HANDLE;
        VkCommandBuffer m_tracyCmdBuf = VK_NULL_HANDLE;
#endif //TRACY_ENABLE
    };


} // namespace vk
} // namespace kage