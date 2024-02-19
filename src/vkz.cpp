#include "common.h"
#include "alloc.h"
#include "handle.h"
#include "memory_operation.h"
#include "vkz.h"

#include "config.h"
#include "name.h"

#include "util.h"

#include "rhi_context.h"
#include "framegraph_2.h"
#include "rhi_context_vk.h"

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>


namespace vkz
{
    static AllocatorI* s_allocator = nullptr;

    static AllocatorI* getAllocator()
    {
        if (s_allocator == nullptr)
        {
            DefaultAllocator allocator;
            s_allocator = VKZ_NEW(&allocator, DefaultAllocator);
        }
        return s_allocator;
    }

    static void shutdownAllocator()
    {
        deleteObject(getAllocator(), s_allocator);
    }

    uint16_t getBytesPerPixel(ResourceFormat _format)
    {
        uint16_t bpp = 0;
        switch (_format)
        {
        case ResourceFormat::undefined:
            bpp = 0;
            break;
        case ResourceFormat::r8_sint:
            bpp = 1;
            break;
        case ResourceFormat::r8_uint:
            bpp = 1;
            break;
        case ResourceFormat::r16_uint:
            bpp = 2;
            break;
        case ResourceFormat::r16_sint:
            bpp = 2;
            break;
        case ResourceFormat::r16_snorm:
            bpp = 2;
            break;
        case ResourceFormat::r16_unorm:
            bpp = 2;
            break;
        case ResourceFormat::r32_uint:
            bpp = 4;
            break;
        case ResourceFormat::r32_sint:
            bpp = 4;
            break;
        case ResourceFormat::r32_sfloat:
            bpp = 4;
            break;
        case ResourceFormat::b8g8r8a8_snorm:
            bpp = 4;
            break;
        case ResourceFormat::b8g8r8a8_unorm:
            bpp = 4;
            break;
        case ResourceFormat::b8g8r8a8_sint:
            bpp = 4;
            break;
        case ResourceFormat::b8g8r8a8_uint:
            bpp = 4;
            break;
        case ResourceFormat::r8g8b8a8_snorm:
            bpp = 4;
            break;
        case ResourceFormat::r8g8b8a8_unorm:
            bpp = 4;
            break;
        case ResourceFormat::r8g8b8a8_sint:
            bpp = 4;
            break;
        case ResourceFormat::r8g8b8a8_uint:
            bpp = 4;
            break;
        case ResourceFormat::unknown_depth:
            bpp = 0;
            break;
        case ResourceFormat::d16:
            bpp = 2;
            break;
        case ResourceFormat::d32:
            bpp = 4;
            break;
        default:
            bpp = 0;
            break;
        }
        
        return bpp;
    }

    struct MemoryRef
    {
        Memory mem;
        ReleaseFn releaseFn;
        void* userData;
    };

    bool isMemoryRef(const Memory* _mem)
    {
        return _mem->data != (uint8_t*)_mem + sizeof(Memory);
    }

    void release(const Memory* _mem)
    {
        assert(nullptr != _mem);
        Memory* mem = const_cast<Memory*>(_mem);
        if (isMemoryRef(mem))
        {
            MemoryRef* memRef = reinterpret_cast<MemoryRef*>(mem);
            if (nullptr != memRef->releaseFn)
            {
                memRef->releaseFn(mem->data, memRef->userData);
            }
        }
        free(getAllocator(), mem);
    }

    class GLFWManager
    {
    public:
        bool init(uint32_t _w, uint32_t _h, const char* _name);
        bool shouldClose();
        void update();
        void shutdown();
        GLFWwindow* getWindow() { return m_pWindow; }

    private:
        GLFWwindow* m_pWindow;
    };

    bool GLFWManager::init(uint32_t _w, uint32_t _h, const char* _name)
    {
        int rc = glfwInit();
        assert(rc);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_pWindow = glfwCreateWindow(_w, _h, _name, nullptr, nullptr);
        assert(m_pWindow);

        /*
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mousekeyCallback);
        glfwSetCursorPosCallback(window, mouseMoveCallback);
        */

        return m_pWindow != nullptr;
    }

    bool GLFWManager::shouldClose()
    {
        return glfwWindowShouldClose(m_pWindow);
    }

    void GLFWManager::update()
    {
        glfwPollEvents();
    }

    void GLFWManager::shutdown()
    {
        glfwDestroyWindow(m_pWindow);
        glfwTerminate();
    }

    constexpr uint8_t kTouched = 1;
    constexpr uint8_t kUnTouched = 0;

    struct Context
    {
        // conditions
        bool isCompute(const PassHandle _hPass);
        bool isGraphics(const PassHandle _hPass);
        bool isFillBuffer(const PassHandle _hPass);
        bool isOneOpPass(const PassHandle _hPass);
        bool isRead(const AccessFlags _access);
        bool isWrite(const AccessFlags _access, const PipelineStageFlags _stage);

        bool isDepthStencil(const ImageHandle _hImg);
        bool isColorAttachment(const ImageHandle _hImg);
        bool isNormalImage(const ImageHandle _hImg);

        bool isAliasFromBuffer(const BufferHandle _hBase, const BufferHandle _hAlias);
        bool isAliasFromImage(const ImageHandle _hBase, const ImageHandle _hAlias);

        // Context API
        void setRenderSize(uint32_t _width, uint32_t _height);
        void setWindowName(const char* _name);

        ShaderHandle registShader(const char* _name, const char* _path);
        ProgramHandle registProgram(const char* _name, const Memory* _shaders, const uint16_t _shaderCount, const uint32_t _sizePushConstants = 0);

        PassHandle registPass(const char* _name, const PassDesc& _desc);

        BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime);
        ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);
        ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);
        ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);

        ImageViewHandle registImageView(const char* _name, const ImageHandle _hImg, const uint32_t _baseMip, const uint32_t _mipLevel);

        SamplerHandle requestSampler(const SamplerDesc& _desc);

        BufferHandle aliasBuffer(const BufferHandle _baseBuf);
        ImageHandle aliasImage(const ImageHandle  _baseImg);

        // read-only data, no barrier required
        void bindVertexBuffer(const PassHandle _hPass, const BufferHandle _hBuf);
        void bindIndexBuffer(const PassHandle _hPass, const BufferHandle _hBuf);

        void setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _defaultMaxCount);
        void setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset);

        void bindBuffer(PassHandle _hPass, BufferHandle _buf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias);
        SamplerHandle sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, ImageLayout _layout, SamplerReductionMode _reductionMode);
        void bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
            , const ImageHandle _outAlias, const ImageViewHandle _hImgView);

        void setCustomRenderFunc(const PassHandle _hPass, RenderFuncPtr _func, const Memory* _dataMem);

        void setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias);

        void fillBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _offset, const uint32_t _size, const uint32_t _value, const BufferHandle _outAlias);

        void setMultiFrame(ImageHandle _img);
        void setMultiFrame(BufferHandle _buf);
        
        void setPresentImage(ImageHandle _rt);

        void updateCustomRenderFuncData(const PassHandle _hPass, const Memory* _dataMem);

        void updateBuffer(const BufferHandle _hbuf, const Memory* _mem);

        void updatePushConstants(const PassHandle _hPass, const Memory* _mem);

        void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ);

        // actual write to memory
        void storeBrief();
        void storeShaderData();
        void storeProgramData();
        void storePassData();
        void storeBufferData();
        void storeImageData();
        void storeAliasData();
        void storeSamplerData();

        void init();
        bool shouldClose();
        void run();
        void bake();
        void render();
        void shutdown();

        void setRenderGraphDataDirty() { m_isRenderGraphDataDirty = true; }
        bool isRenderGraphDataDirty() const { return m_isRenderGraphDataDirty; }

        HandleArrayT<kMaxNumOfShaderHandle> m_shaderHandles;
        HandleArrayT<kMaxNumOfProgramHandle> m_programHandles;
        HandleArrayT<kMaxNumOfPassHandle> m_passHandles;
        HandleArrayT<kMaxNumOfBufferHandle> m_bufferHandles;
        HandleArrayT<kMaxNumOfImageHandle> m_imageHandles;
        HandleArrayT<kMaxNumOfImageViewHandle> m_imageViewHandles;
        HandleArrayT<kMaxNumOfSamplerHandle> m_samplerHandles;
        
        ImageHandle m_presentImage{kInvalidHandle};

        // resource Info
        UniDataContainer<ShaderHandle, std::string> m_shaderDescs;
        UniDataContainer<ProgramHandle, ProgramDesc> m_programDescs;
        UniDataContainer<BufferHandle, BufferMetaData> m_bufferMetas;
        UniDataContainer<ImageHandle, ImageMetaData> m_imageMetas;
        UniDataContainer<PassHandle, PassMetaData> m_passMetas;
        UniDataContainer<SamplerHandle, SamplerDesc> m_samplerDescs;
        UniDataContainer<ImageViewHandle, ImageViewDesc> m_imageViewDesc;

        // alias
        UniDataContainer<BufferHandle, stl::vector<BufferHandle>> m_aliasBuffers;
        UniDataContainer<ImageHandle, stl::vector<ImageHandle>> m_aliasImages;

        // read/write resources
        UniDataContainer<PassHandle, stl::vector<PassResInteract>> m_readBuffers;
        UniDataContainer<PassHandle, stl::vector<PassResInteract>> m_writeBuffers;
        UniDataContainer<PassHandle, stl::vector<PassResInteract>> m_readImages;
        UniDataContainer<PassHandle, stl::vector<PassResInteract>> m_writeImages;
        UniDataContainer<PassHandle, stl::vector<WriteOperationAlias>> m_implicitOutBufferAliases; // add a dependency line to the write resource
        UniDataContainer<PassHandle, stl::vector<WriteOperationAlias>> m_implicitOutImageAliases; // add a dependency line to the write resource

        // binding
        UniDataContainer<PassHandle, stl::vector<uint32_t>> m_usedBindPoints;
        UniDataContainer<PassHandle, stl::vector<uint32_t>> m_usedAttchBindPoints;

        // push constants
        UniDataContainer<ProgramHandle, void *> m_pushConstants;

        // 1-operation pass: blit/copy/fill 
        UniDataContainer<PassHandle, uint8_t> m_oneOpPassTouched;


        // render config
        uint32_t m_renderWidth{ 0 };
        uint32_t m_renderHeight{ 0 };

        // render data status
        bool    m_isRenderGraphDataDirty{ false };

        // frame graph
        MemoryBlockI* m_pFgMemBlock{ nullptr };
        MemoryWriter* m_fgMemWriter{ nullptr };

        RHIContext* m_rhiContext{ nullptr };

        Framegraph2* m_frameGraph{ nullptr };

        AllocatorI* m_pAllocator{ nullptr };

        // glfw
        char m_windowTitle[256];
        GLFWManager m_glfwManager;
    };

    bool Context::isCompute(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas.getIdToData(_hPass);
        return meta.queue == PassExeQueue::compute;
    }

    bool Context::isGraphics(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas.getIdToData(_hPass);
        return meta.queue == PassExeQueue::graphics;
    }

    bool Context::isFillBuffer(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas.getIdToData(_hPass);
        return meta.queue == PassExeQueue::fill_buffer;
    }

    bool Context::isOneOpPass(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas.getIdToData(_hPass);

        return (
            meta.queue == PassExeQueue::fill_buffer
            || meta.queue == PassExeQueue::copy
            );
    }

    bool Context::isRead(const AccessFlags _access)
    {
        return _access & AccessFlagBits::read_only;
    }

    bool Context::isWrite(const AccessFlags _access, const PipelineStageFlags _stage)
    {
        return (_access & AccessFlagBits::write_only) || (_stage & PipelineStageFlagBits::color_attachment_output);
    }

    bool Context::isDepthStencil(const ImageHandle _hImg)
    {
        const ImageMetaData& meta = m_imageMetas.getIdToData(_hImg);
        return meta.usage & ImageUsageFlagBits::depth_stencil_attachment;
    }

    bool Context::isColorAttachment(const ImageHandle _hImg)
    {
        const ImageMetaData& meta = m_imageMetas.getIdToData(_hImg);
        return meta.usage & ImageUsageFlagBits::color_attachment;
    }

    bool Context::isNormalImage(const ImageHandle _hImg)
    {
        return !isDepthStencil(_hImg) && !isColorAttachment(_hImg);
    }

    bool Context::isAliasFromBuffer(const BufferHandle _hBase, const BufferHandle _hAlias)
    {
        bool result = false;

        if (m_aliasBuffers.exist(_hBase))
        {
            const stl::vector<BufferHandle>& aliasVec = m_aliasBuffers.getIdToData(_hBase);
            for (const BufferHandle& alias : aliasVec)
            {
                if ( alias == _hAlias)
                {
                    result = true;
                    break;
                }
            }
        }

        return result;
    }

    bool Context::isAliasFromImage(const ImageHandle _hBase, const ImageHandle _hAlias)
    {
        bool result = false;

        if (m_aliasImages.exist(_hBase))
        {
            const stl::vector<ImageHandle>& aliasVec = m_aliasImages.getIdToData(_hBase);
            for (const ImageHandle& alias : aliasVec)
            {
                if (alias == _hAlias)
                {
                    result = true;
                    break;
                }
            }
        }

        return result;
    }

    void Context::setRenderSize(uint32_t _width, uint32_t _height)
    {
        m_renderWidth = _width;
        m_renderHeight = _height;
    }

    void Context::setWindowName(const char* _name)
    {
        if (nullptr == _name)
        {
            memcpy(m_windowTitle, "vkz", 3);
            m_windowTitle[3] = '\0';
            return;
        }

        size_t length = strlen(_name);
        memcpy(m_windowTitle, _name, length);
        m_windowTitle[length] = '\0';
    }

    ShaderHandle Context::registShader(const char* _name, const char* _path)
    {
        uint16_t idx = m_shaderHandles.alloc();

        ShaderHandle handle = ShaderHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ShaderHandle failed!");
            return ShaderHandle{ kInvalidHandle };
        }

        m_shaderDescs.push_back({ idx }, _path);
        
        setRenderGraphDataDirty();

        return handle;
    }

    ProgramHandle Context::registProgram(const char* _name, const Memory* _mem, const uint16_t _shaderNum, const uint32_t _sizePushConstants /*= 0*/)
    {
        uint16_t idx = m_programHandles.alloc();

        ProgramHandle handle = ProgramHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ProgramHandle failed!");
            return ProgramHandle{ kInvalidHandle };
        }

        ProgramDesc desc{};
        desc.progId = idx;
        desc.shaderNum = _shaderNum;
        desc.sizePushConstants = _sizePushConstants;
        memcpy_s(desc.shaderIds, COUNTOF(desc.shaderIds), _mem->data, _mem->size);
        release(_mem);
        m_programDescs.push_back({ idx }, desc);

        setRenderGraphDataDirty();

        return handle;
    }

    PassHandle Context::registPass(const char* _name, const PassDesc& _desc)
    {
        uint16_t idx = m_passHandles.alloc();
        PassHandle handle = PassHandle{ idx };
        
        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create PassHandle failed!");
            return PassHandle{ kInvalidHandle };
        }

        PassMetaData meta{ _desc };
        meta.passId = idx;
        m_passMetas.push_back({ idx }, meta);

        m_oneOpPassTouched.push_back({ idx }, kUnTouched);

        setRenderGraphDataDirty();

        return handle;
    }

    BufferHandle Context::registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_bufferHandles.alloc();

        BufferHandle handle = BufferHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create BufferHandle failed!");
            return BufferHandle{ kInvalidHandle };
        }

        BufferMetaData meta{ _desc };
        meta.bufId = idx;
        meta.lifetime = _lifetime;
        
        if (_desc.data != nullptr)
        {
            meta.usage |= BufferUsageFlagBits::transfer_dst;
        }

        m_bufferMetas.push_back({ idx }, meta);

        setRenderGraphDataDirty();

        return handle;
    }

    ImageHandle Context::registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        ImageHandle handle = ImageHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create ImageHandle failed!");
            return ImageHandle{ kInvalidHandle };
        }

        ImageMetaData meta = _desc;
        meta.imgId = idx;
        meta.aspectFlags = ImageAspectFlagBits::color;
        meta.lifetime = _lifetime;

        m_imageMetas.push_back({ idx }, meta);

        setRenderGraphDataDirty();

        return handle;
    }

    ImageHandle Context::registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        ImageHandle handle = ImageHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create ImageHandle failed!");
            return ImageHandle{ kInvalidHandle };
        }

        // do not set the texture format
        if (_desc.format != ResourceFormat::undefined)
        {
            message(error, "do not set the resource format for render target");
            return ImageHandle{ kInvalidHandle };
        }

        if (m_renderWidth != _desc.width || m_renderHeight != _desc.height)
        {
            message(warning, "no need to set width and height for render target, will use render size instead");
        }

        ImageMetaData meta = _desc;
        meta.width = m_renderWidth;
        meta.height = m_renderHeight;
        meta.imgId = idx;
        meta.format = ResourceFormat::undefined;
        meta.usage = ImageUsageFlagBits::color_attachment | _desc.usage;
        meta.aspectFlags = ImageAspectFlagBits::color;
        meta.lifetime = _lifetime;

        m_imageMetas.push_back({ idx }, meta);

        setRenderGraphDataDirty();

        return handle;
    }

    ImageHandle Context::registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        ImageHandle handle = ImageHandle{ idx };

        if (m_renderWidth != _desc.width || m_renderHeight != _desc.height)
        {
            message(warning, "no need to set width and height for depth stencil, will use render size instead");
        }

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create ImageHandle failed!");
            return ImageHandle{ kInvalidHandle };
        }

        // do not set the texture format
        if (_desc.format != ResourceFormat::undefined)
        {
            message(error, "do not set the resource format for depth stencil!");
            return ImageHandle{ kInvalidHandle };
        }

        ImageMetaData meta{ _desc };
        meta.width = m_renderWidth;
        meta.height = m_renderHeight;
        meta.imgId = idx;
        meta.format = ResourceFormat::d32;
        meta.usage = ImageUsageFlagBits::depth_stencil_attachment | _desc.usage;
        meta.aspectFlags = ImageAspectFlagBits::depth;
        meta.lifetime = _lifetime;

        m_imageMetas.push_back({ idx }, meta);

        setRenderGraphDataDirty();

        return handle;
    }

    vkz::ImageViewHandle Context::registImageView(const char* _name, const ImageHandle _hImg, const uint32_t _baseMip, const uint32_t _mipLevel)
    {
        uint16_t idx = m_imageViewHandles.alloc();

        ImageViewHandle handle { idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create BufferHandle failed!");
            return ImageViewHandle{ kInvalidHandle };
        }

        ImageViewDesc desc{ _hImg.id, _baseMip, _mipLevel };
        m_imageViewDesc.push_back({ idx }, desc);

        setRenderGraphDataDirty();

        return handle;
    }

    SamplerHandle Context::requestSampler(const SamplerDesc& _desc)
    {
        // find if the sampler already exist
        uint16_t samplerId = kInvalidHandle;
        for (uint16_t ii = 0; ii < m_samplerHandles.getNumHandles(); ++ii)
        {
            SamplerHandle handle{ m_samplerHandles.getHandleAt(ii) };
            const SamplerDesc& desc = m_samplerDescs.getIdToData(handle);

            if (desc == _desc)
            {
                samplerId = handle.id;
                break;
            }
        }

        if (kInvalidHandle == samplerId)
        {
            samplerId = m_samplerHandles.alloc();

            if (kInvalidHandle == samplerId)
            {
                message(error, "create SamplerHandle failed!");
                return SamplerHandle{ kInvalidHandle };
            }
            
            m_samplerDescs.push_back({ samplerId }, _desc);
        }

        return SamplerHandle{ samplerId };
    }

    void Context::storeBrief()
    {
        MagicTag magic{ MagicTag::set_brief };
        write(m_fgMemWriter, magic);

        // set the brief data
        FrameGraphBrief brief;
        brief.version = 1u;
        brief.passNum = m_passHandles.getNumHandles();
        brief.bufNum = m_bufferHandles.getNumHandles();
        brief.imgNum = m_imageHandles.getNumHandles();
        brief.shaderNum = m_shaderHandles.getNumHandles();
        brief.programNum = m_programHandles.getNumHandles();
        brief.samplerNum = m_samplerHandles.getNumHandles();

        brief.presentImage = m_presentImage.id;


        write(m_fgMemWriter, brief);

        write(m_fgMemWriter, MagicTag::magic_body_end);
    }

    void Context::storeShaderData()
    {
        uint16_t shaderCount = m_shaderHandles.getNumHandles();
        
        for (uint16_t ii = 0; ii < shaderCount; ++ii)
        {
            ShaderHandle shader { (m_shaderHandles.getHandleAt(ii)) };
            const std::string& path = m_shaderDescs.getIdToData(shader);

            // magic tag
            MagicTag magic{ MagicTag::register_shader };
            write(m_fgMemWriter, magic);

            ShaderRegisterInfo info;
            info.shaderId = shader.id;
            info.strLen = (uint16_t)strnlen_s(path.c_str(), kMaxPathLen);

            write(m_fgMemWriter, info);
            write(m_fgMemWriter, (void*)path.c_str(), (int32_t)info.strLen);

            write(m_fgMemWriter, MagicTag::magic_body_end);
        }
    }

    void Context::storeProgramData()
    {
        uint16_t programCount = m_programHandles.getNumHandles();

        for (uint16_t ii = 0; ii < programCount; ++ii)
        {
            ProgramHandle program { (m_programHandles.getHandleAt(ii)) };
            const ProgramDesc& desc = m_programDescs.getIdToData(program);

            // magic tag
            MagicTag magic{ MagicTag::register_program };
            write(m_fgMemWriter, magic);

            // info
            ProgramRegisterInfo info;
            info.progId = desc.progId;
            info.sizePushConstants = desc.sizePushConstants;
            info.shaderNum = desc.shaderNum;
            write(m_fgMemWriter, info);

            // actual shader indexes
            write(m_fgMemWriter, desc.shaderIds, sizeof(uint16_t) * desc.shaderNum);

            write(m_fgMemWriter, MagicTag::magic_body_end);
        }
    }

    void Context::storePassData()
    {
        uint16_t passCount = m_passHandles.getNumHandles();

        for (uint16_t ii = 0; ii < passCount; ++ii)
        {
            auto getResInteractNum = [](
                const UniDataContainer<PassHandle, stl::vector<PassResInteract>>& __cont
                , const PassHandle __pass) -> uint32_t
            {
                if (__cont.exist(__pass)) {
                    return (uint32_t)(__cont.getIdToData(__pass).size());
                }
                
                return 0u;
            };

            PassHandle pass{ (m_passHandles.getHandleAt(ii)) };
            const PassMetaData& passMeta = m_passMetas.getIdToData(pass);

            assert(getResInteractNum(m_writeImages, pass) == passMeta.writeImageNum);
            assert(getResInteractNum(m_readImages, pass) == passMeta.readImageNum);
            assert(getResInteractNum(m_writeBuffers, pass) == passMeta.writeBufferNum);
            assert(getResInteractNum(m_readBuffers, pass) == passMeta.readBufferNum);

            // frame graph data
            MagicTag magic{ MagicTag::register_pass };
            write(m_fgMemWriter, magic);

            write(m_fgMemWriter, passMeta);

            if (0 != passMeta.vertexBindingNum)
            {
                write(m_fgMemWriter, passMeta.vertexBindings, sizeof(VertexBindingDesc) * passMeta.vertexBindingNum);
            }
            if (0 != passMeta.vertexAttributeNum)
            {
                write(m_fgMemWriter, passMeta.vertexAttributes, sizeof(VertexAttributeDesc) * passMeta.vertexAttributeNum);
            }
            if (0 != passMeta.pipelineSpecNum)
            {
                write(m_fgMemWriter, passMeta.pipelineSpecData, sizeof(int) * passMeta.pipelineSpecNum);
            }

            // write image
            if (0 != passMeta.writeImageNum)
            {
                write(
                    m_fgMemWriter
                    , (const void*)m_writeImages.getIdToData(pass).data()
                    , (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_writeImages, pass))
                );
            }
            // read image
            if (0 != passMeta.readImageNum)
            {
                write(
                    m_fgMemWriter
                    , (const void*)m_readImages.getIdToData(pass).data()
                    , (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_readImages, pass)));
            }
            // write buffer
            if (0 != passMeta.writeBufferNum)
            {
                write(
                    m_fgMemWriter, 
                    (const void*)m_writeBuffers.getIdToData(pass).data(), 
                    (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_writeBuffers, pass)));
            }
            // read buffer
            if (0 != passMeta.readBufferNum)
            {
                write(
                    m_fgMemWriter
                    , (const void*)m_readBuffers.getIdToData(pass).data()
                    , (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_readBuffers, pass)));
            }

            // implicit alias
            if (0 != passMeta.writeImgAliasNum)
            {
                write(
                    m_fgMemWriter
                    , (const void*)m_implicitOutImageAliases.getIdToData(pass).data()
                    , (uint32_t)(sizeof(WriteOperationAlias) * passMeta.writeImgAliasNum));
            }

            if (0 != passMeta.writeBufAliasNum)
            {
                write(
                    m_fgMemWriter
                    , (const void*)m_implicitOutBufferAliases.getIdToData(pass).data()
                    , (uint32_t)(sizeof(WriteOperationAlias) * passMeta.writeBufAliasNum));
            }


            write(m_fgMemWriter, MagicTag::magic_body_end);
        }
    }

    void Context::storeBufferData()
    {
        uint16_t bufCount = m_bufferHandles.getNumHandles();

        for (uint16_t ii = 0; ii < bufCount; ++ii)
        {
            BufferHandle buf { (m_bufferHandles.getHandleAt(ii)) };
            const BufferMetaData& meta = m_bufferMetas.getIdToData(buf);

            // frame graph data
            MagicTag magic{ MagicTag::register_buffer };
            write(m_fgMemWriter, magic);

            BufRegisterInfo info;
            info.bufId = meta.bufId;
            info.size = meta.size;
            info.data = meta.data;
            info.fillVal = meta.fillVal;
            info.usage = meta.usage;
            info.memFlags = meta.memFlags;
            info.lifetime = meta.lifetime;

            info.initialState.access = AccessFlagBits::none;
            info.initialState.layout = ImageLayout::undefined;
            info.initialState.stage = PipelineStageFlagBits::none;

            write(m_fgMemWriter, info);

            write(m_fgMemWriter, MagicTag::magic_body_end);
        }
    }

    void Context::storeImageData()
    {
        uint16_t imgCount = m_imageHandles.getNumHandles();

        for (uint16_t ii = 0; ii < imgCount; ++ii)
        {
            ImageHandle img { (m_imageHandles.getHandleAt(ii)) };
            const ImageMetaData& meta = m_imageMetas.getIdToData(img);

            // frame graph data
            MagicTag magic{ MagicTag::register_image };
            write(m_fgMemWriter, magic);

            ImgRegisterInfo info;
            info.imgId = meta.imgId;
            info.width = meta.width;
            info.height = meta.height;
            info.depth = meta.depth;
            info.mips = meta.mips;
            info.size = meta.size;
            info.data = meta.data;

            info.type = meta.type;
            info.viewType = meta.viewType;
            info.format = meta.format;
            info.layout = meta.layout;
            info.arrayLayers = meta.arrayLayers;
            info.usage = meta.usage;
            info.lifetime = meta.lifetime;
            info.bpp = meta.bpp;
            info.aspectFlags = meta.aspectFlags;

            info.initialState.access = AccessFlagBits::none;
            info.initialState.layout = ImageLayout::undefined;
            info.initialState.stage = PipelineStageFlagBits::none;

            write(m_fgMemWriter, info);

            write(m_fgMemWriter, MagicTag::magic_body_end);
        }
    }

    void Context::storeAliasData()
    {
        // buffers
        for (uint16_t ii = 0; ii < m_aliasBuffers.size(); ++ii)
        {
            BufferHandle buf = m_aliasBuffers.getIdAt(ii);
            const stl::vector<BufferHandle>& aliasVec = m_aliasBuffers.getIdToData(buf);

            if (aliasVec.empty())
            {
                continue;
            }

            // magic tag
            MagicTag magic{ MagicTag::force_alias_buffer };
            write(m_fgMemWriter, magic);

            // info
            ResAliasInfo info;
            info.aliasNum = (uint16_t)aliasVec.size();
            info.resBase = buf.id;
            write(m_fgMemWriter, info);

            // actual alias indexes
            write(m_fgMemWriter, aliasVec.data(), uint32_t(sizeof(uint16_t) * aliasVec.size()));

            write(m_fgMemWriter, MagicTag::magic_body_end);
            
        }

        // images
        for (uint16_t ii = 0; ii < m_aliasImages.size(); ++ii)
        {
            ImageHandle img  = m_aliasImages.getIdAt(ii);
            const stl::vector<ImageHandle>& aliasVec = m_aliasImages.getIdToData(img);

            if (aliasVec.empty())
            {
                continue;
            }


            const ImageMetaData& meta = m_imageMetas.getIdToData(img);
            MagicTag magic{ MagicTag::force_alias_image };
            write(m_fgMemWriter, magic);

            // info
            ResAliasInfo info;
            info.aliasNum = (uint16_t)aliasVec.size();
            info.resBase = img.id;
            write(m_fgMemWriter, info);

            // actual alias indexes
            write(m_fgMemWriter, aliasVec.data(), uint32_t(sizeof(uint16_t) * aliasVec.size()));

            write(m_fgMemWriter, MagicTag::magic_body_end);
        }
    }

    void Context::storeSamplerData()
    {
        for (uint16_t ii = 0; ii < m_samplerHandles.getNumHandles(); ++ii )
        {
            uint16_t samplerId = m_samplerHandles.getHandleAt(ii);
            const SamplerDesc& desc = m_samplerDescs.getIdToData({ samplerId });

            // magic tag
            write(m_fgMemWriter, MagicTag::register_sampler);

            SamplerMetaData meta{desc};
            meta.samplerId = samplerId;

            write(m_fgMemWriter, meta);

            write(m_fgMemWriter, MagicTag::magic_body_end);
        }
    }

    BufferHandle Context::aliasBuffer(const BufferHandle _baseBuf)
    {
        uint16_t aliasId = m_bufferHandles.alloc();

        if (m_aliasBuffers.exist(_baseBuf)) 
        {
            stl::vector<BufferHandle>& aliasVecRef = m_aliasBuffers.getDataRef(_baseBuf);
            aliasVecRef.emplace_back(BufferHandle{ aliasId });
        }
        else 
        {
            stl::vector<BufferHandle> aliasVec{};
            //aliasVec.push_back(_baseBuf); // first one is the base
            aliasVec.emplace_back(BufferHandle{ aliasId });

            m_aliasBuffers.push_back({ _baseBuf }, aliasVec);
        }

        BufferMetaData meta = { m_bufferMetas.getIdToData(_baseBuf) };
        meta.bufId = aliasId;
        m_bufferMetas.push_back({ aliasId }, meta);

        setRenderGraphDataDirty();

        return { aliasId };
    }

    ImageHandle Context::aliasImage(const ImageHandle _baseImg)
    {
        uint16_t aliasId = m_imageHandles.alloc();

        if (m_aliasImages.exist(_baseImg))
        {
            stl::vector<ImageHandle>& aliasVecRef = m_aliasImages.getDataRef(_baseImg);
            aliasVecRef.emplace_back(ImageHandle{ aliasId });
        }
        else
        {
            stl::vector<ImageHandle> aliasVec{};
            //aliasVec.push_back(_baseImg);
            aliasVec.emplace_back(ImageHandle{ aliasId });

            m_aliasImages.push_back({ _baseImg }, aliasVec);
        }

        ImageMetaData meta = { m_imageMetas.getIdToData(_baseImg) };
        meta.imgId = aliasId;
        m_imageMetas.push_back({ aliasId }, meta);

        setRenderGraphDataDirty();

        return { aliasId };
    }

    uint32_t availableBinding(UniDataContainer<PassHandle, stl::vector<uint32_t>>& _container, const PassHandle _hPass, const uint32_t _binding)
    {
        bool conflict = false;
        if (_container.exist(_hPass))
        {
            const stl::vector<uint32_t>& bindingVec = _container.getDataRef(_hPass);
            for (const uint32_t& binding : bindingVec)
            {
                if (binding == _binding)
                {
                    conflict = true;
                }
            }
        }
        else
        {
            stl::vector<uint32_t> bindingVec{};
            bindingVec.emplace_back(_binding);
            _container.push_back({ _hPass }, bindingVec);
        }

        return !conflict;
    }

    uint32_t insertResInteract(UniDataContainer<PassHandle, stl::vector<PassResInteract>>& _container
        , const PassHandle _hPass, const uint16_t _resId, const SamplerHandle _hSampler, const ResInteractDesc& _interact
        , const ImageViewDesc& _specImgViewInfo
        )
    {
        uint32_t vecSize = 0u;

        PassResInteract pri;
        pri.passId = _hPass.id;
        pri.resId = _resId;
        pri.interact = _interact;
        pri.samplerId = _hSampler.id;
        pri.specImgViewInfo = _specImgViewInfo;


        if (_container.exist(_hPass))
        {
            stl::vector<PassResInteract>& prInteractVec = _container.getDataRef(_hPass);
            prInteractVec.emplace_back(pri);

            vecSize = (uint32_t)prInteractVec.size();
        }
        else
        {
            stl::vector<PassResInteract> prInteractVec{};
            prInteractVec.emplace_back(pri);
            _container.push_back({ _hPass }, prInteractVec);

            vecSize = (uint32_t)prInteractVec.size();
        }

        return vecSize;
    }
    

    uint32_t insertResInteract(UniDataContainer<PassHandle, stl::vector<PassResInteract>>& _container
        , const PassHandle _hPass, const uint16_t _resId, const SamplerHandle _hSampler, const ResInteractDesc& _interact
        )
    {
        return insertResInteract(_container, _hPass, _resId, _hSampler, _interact, defaultImageView({ _resId }));
    }

    uint32_t insertWriteResAlias(UniDataContainer<PassHandle, stl::vector<WriteOperationAlias>>& _container, const PassHandle _hPass, const uint16_t _resIn, const uint16_t _resOut)
    {
        uint32_t vecSize = 0u;

        if (_container.exist(_hPass))
        {
            stl::vector<WriteOperationAlias>& aliasVec = _container.getDataRef(_hPass);
            aliasVec.emplace_back(WriteOperationAlias{ _resIn, _resOut });

            vecSize = (uint32_t)aliasVec.size();
        }
        else
        {
            stl::vector<WriteOperationAlias> aliasVec{};
            aliasVec.emplace_back(WriteOperationAlias{ _resIn, _resOut });
            _container.push_back({ _hPass }, aliasVec);

            vecSize = (uint32_t)aliasVec.size();
        }

        return vecSize;
    }

    void Context::bindVertexBuffer(const PassHandle _hPass, const BufferHandle _hBuf)
    {
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.vertexBufferId = _hBuf.id;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::none;
        interact.access = AccessFlagBits::none;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);

        setRenderGraphDataDirty();
    }

    void Context::bindIndexBuffer(const PassHandle _hPass, const BufferHandle _hBuf)
    {
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.indexBufferId = _hBuf.id;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::none;
        interact.access = AccessFlagBits::none;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);

        setRenderGraphDataDirty();
    }

    void Context::setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _defaultMaxCount)
    {
        assert(isGraphics(_hPass));

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.indirectBufferId = _hBuf.id;
        passMeta.indirectBufOffset = _offset;
        passMeta.indirectBufStride = _stride;
        passMeta.indirectMaxDrawCount = _defaultMaxCount;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::draw_indirect;
        interact.access = AccessFlagBits::indirect_command_read;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);

        setRenderGraphDataDirty();
    }

    void Context::setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset)
    {
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.indirectCountBufferId = _hBuf.id;
        passMeta.indirectCountBufOffset = _offset;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::draw_indirect;
        interact.access = AccessFlagBits::indirect_command_read;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);

        setRenderGraphDataDirty();
    }

    void Context::bindBuffer(PassHandle _hPass, BufferHandle _buf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias)
    {
        if (!availableBinding(m_usedBindPoints, _hPass, _binding))
        {
            message(DebugMessageType::error, "trying to binding the same point: %d", _binding);
            return;
        }

        ResInteractDesc interact{};
        interact.binding = _binding;
        interact.stage = _stage;
        interact.access = _access;

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        if (isRead(_access))
        {
            passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _buf.id, { kInvalidHandle }, interact);
            setRenderGraphDataDirty();
        }

        if (isWrite(_access, _stage))
        {
            if (!isValid(_outAlias))
            {
                message(DebugMessageType::error, "the _outAlias must be valid if trying to write to resource in pass");
                return;
            }

            passMeta.writeBufferNum = insertResInteract(m_writeBuffers, _hPass, _buf.id, {kInvalidHandle}, interact);
            passMeta.writeBufAliasNum = insertWriteResAlias(m_implicitOutBufferAliases, _hPass, _buf.id, _outAlias.id);

            assert(passMeta.writeBufferNum == passMeta.writeBufAliasNum);
            setRenderGraphDataDirty();
        }
    }

    SamplerHandle Context::sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, ImageLayout _layout, SamplerReductionMode _reductionMode)
    {
        if (!availableBinding(m_usedBindPoints, _hPass, _binding))
        {
            message(DebugMessageType::error, "trying to binding the same point: %d", _binding);
            return { kInvalidHandle };
        }

        // get the sampler id
        SamplerDesc desc;
        desc.reductionMode = _reductionMode;

        SamplerHandle sampler = requestSampler(desc);

        ResInteractDesc interact{};
        interact.binding = _binding;
        interact.stage = _stage;
        interact.access = AccessFlagBits::shader_read;
        interact.layout = _layout;

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.readImageNum = insertResInteract(m_readImages, _hPass, _hImg.id, sampler, interact);
        passMeta.sampleImageNum++; // Note: is this the only way to read texture?
        setRenderGraphDataDirty();

        return sampler;
    }

    void Context::bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
        , const ImageHandle _outAlias, const ImageViewHandle _hImgView)
    {
        if (!availableBinding(m_usedBindPoints, _hPass, _binding))
        {
            message(DebugMessageType::error, "trying to binding the same point: %d", _binding);
            return;
        }

        ImageMetaData& meta = m_imageMetas.getDataRef(_hImg);
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);

        ResInteractDesc interact{};
        interact.binding = _binding; 
        interact.stage = _stage;
        interact.access = _access;
        interact.layout = _layout;


        if (isRead(_access))
        {
            if (isGraphics(_hPass) && isDepthStencil(_hImg))
            {
                assert((meta.aspectFlags & ImageAspectFlagBits::depth) > 0);
                interact.access |= AccessFlagBits::depth_stencil_attachment_read;
            }
            else if (isGraphics(_hPass) && isColorAttachment(_hImg))
            {
                assert((meta.aspectFlags & ImageAspectFlagBits::color) > 0);
                interact.access |= AccessFlagBits::color_attachment_read;
            }

            passMeta.readImageNum = insertResInteract(m_readImages, _hPass, _hImg.id, { kInvalidHandle }, interact);
            setRenderGraphDataDirty();
        }

        if (isWrite(_access, _stage))
        {
            if (!isValid(_outAlias))
            {
                message(DebugMessageType::error, "the _outAlias must be valid if trying to write to resource in pass");
            }

            if (isGraphics(_hPass))
            {
                message(DebugMessageType::error, "Graphics pass not allowed to write to texture via bind! try to use setAttachmentOutput instead.");
            }
            else if (isCompute(_hPass) && isNormalImage(_hImg))
            {
                const ImageViewDesc& imgViewDesc = isValid(_hImgView) ? m_imageViewDesc.getIdToData(_hImgView) : defaultImageView(_hImg);

                passMeta.writeImageNum = insertResInteract(m_writeImages, _hPass, _hImg.id, { kInvalidHandle }, interact, imgViewDesc);
                passMeta.specImageViewNum++;
                passMeta.writeImgAliasNum = insertWriteResAlias(m_implicitOutImageAliases, _hPass, _hImg.id, _outAlias.id);

                assert(passMeta.writeImageNum == passMeta.writeImgAliasNum);

                setRenderGraphDataDirty();
            }
            else
            {
                message(DebugMessageType::error, "write to an attachment via binding? nope!");
                assert(0);
            }
        }
    }

    void Context::setCustomRenderFunc(const PassHandle _hPass, RenderFuncPtr _func, const Memory* _dataMem)
    {
        assert(_func != nullptr);

        void * mem = alloc(getAllocator(), _dataMem->size);
        memcpy(mem, _dataMem->data, _dataMem->size);

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.renderFunc = _func;
        passMeta.renderFuncDataPtr = mem;
        passMeta.renderFuncDataSize = _dataMem->size;

        release(_dataMem);
    }

    void Context::setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias)
    {
        if (isColorAttachment(_hImg) && !availableBinding(m_usedAttchBindPoints, _hPass, _attachmentIdx))
        {
            message(DebugMessageType::error, "trying to binding the same point: %d", _attachmentIdx);
            return;
        }

        assert(isGraphics(_hPass));
        assert(isValid(_outAlias));

        ImageMetaData& meta = m_imageMetas.getDataRef(_hImg);
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);

        ResInteractDesc interact{};
        interact.binding = _attachmentIdx;
        interact.stage = PipelineStageFlagBits::color_attachment_output;
        interact.access = AccessFlagBits::none;

        if (isDepthStencil(_hImg))
        {
            assert((meta.aspectFlags & ImageAspectFlagBits::depth) > 0);
            assert(kInvalidHandle == passMeta.writeDepthId);

            passMeta.writeDepthId = _hImg.id;
            interact.layout = ImageLayout::depth_stencil_attachment_optimal;
        }
        else if (isColorAttachment(_hImg))
        {
            assert((meta.aspectFlags & ImageAspectFlagBits::color) > 0);
            interact.layout = ImageLayout::color_attachment_optimal;
        }
        else
        {
            assert(0);
        }

        passMeta.writeImageNum = insertResInteract(m_writeImages, _hPass, _hImg.id, { kInvalidHandle }, interact);
        passMeta.writeImgAliasNum = insertWriteResAlias(m_implicitOutImageAliases, _hPass, _hImg.id, _outAlias.id);
        assert(passMeta.writeImageNum == passMeta.writeImgAliasNum);

        setRenderGraphDataDirty();
    }

    void Context::fillBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _offset, const uint32_t _size, const uint32_t _value, const BufferHandle _outAlias)
    {

        if (!isFillBuffer(_hPass))
        {
            message(DebugMessageType::error, "pass type is not match to operation");
            return;
        }

        if (m_oneOpPassTouched.getIdToData(_hPass) == kTouched)
        {
            message(DebugMessageType::error, "one op pass already touched!");
            return;
        }

        BufferMetaData& bufMeta = m_bufferMetas.getDataRef(_hBuf);
        bufMeta.fillVal = _value;

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::transfer;
        interact.access = AccessFlagBits::transfer_write;

        passMeta.writeBufferNum = insertResInteract(m_writeBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);
        passMeta.writeBufAliasNum = insertWriteResAlias(m_implicitOutBufferAliases, _hPass, _hBuf.id, _outAlias.id);
        assert(passMeta.writeBufferNum == passMeta.writeBufAliasNum);

        m_oneOpPassTouched.update_data(_hPass, kTouched);
        setRenderGraphDataDirty();
    }

    void Context::setMultiFrame(ImageHandle _img)
    {
        ImageMetaData meta = m_imageMetas.getIdToData(_img);
        meta.lifetime = ResourceLifetime::non_transition;

        m_imageMetas.update_data(_img, meta);

        setRenderGraphDataDirty();
    }

    void Context::setMultiFrame(BufferHandle _buf)
    {
        BufferMetaData meta = m_bufferMetas.getIdToData(_buf);
        meta.lifetime = ResourceLifetime::non_transition;

        m_bufferMetas.update_data(_buf, meta);

        setRenderGraphDataDirty();
    }

    void Context::setPresentImage(ImageHandle _img)
    {
        m_presentImage = _img;

        setRenderGraphDataDirty();
    }

    void Context::updateCustomRenderFuncData(const PassHandle _hPass, const Memory* _dataMem)
    {
        m_rhiContext->updateCustomFuncData(_hPass, _dataMem->data, _dataMem->size);
        release(_dataMem);
    }

    void Context::updateBuffer(const BufferHandle _hbuf, const Memory* _mem)
    {
        if (_mem == nullptr)
        {
            return;
        }

        m_rhiContext->updateBuffer(_hbuf, _mem->data, _mem->size);
        release(_mem);
    }

    void Context::updatePushConstants(const PassHandle _hPass, const Memory* _mem)
    {
        if (isRenderGraphDataDirty())
        {
            message(warning, "render graph is not baked yet!");
            return;
        }

        m_rhiContext->updatePushConstants(_hPass, _mem->data, _mem->size);
        release(_mem);
    }

    void Context::updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ)
    {
        if (1 > _threadCountX || 1 > _threadCountY || 1 > _threadCountZ)
        {
            message(warning, "thread count in any dimension should not less than 1! update failed");
            return;
        }

        m_rhiContext->updateThreadCount(_hPass, _threadCountX, _threadCountY, _threadCountZ);
    }

    static void* glfwNativeWindowHandle(GLFWwindow* _window)
    {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        return glfwGetWin32Window(_window);
#else
        #error unsupport platform
#endif
    }

    void Context::init()
    {
        m_pAllocator = getAllocator();

        // init glfw
        m_glfwManager.init(m_renderWidth, m_renderHeight, m_windowTitle);
        
        void* wnd = glfwNativeWindowHandle(m_glfwManager.getWindow());

        RHI_Config rhiConfig{};
        rhiConfig.windowWidth = m_renderWidth;
        rhiConfig.windowHeight = m_renderHeight;
        m_rhiContext = VKZ_NEW(m_pAllocator, RHIContext_vk(m_pAllocator, rhiConfig, wnd));

        m_frameGraph = VKZ_NEW(m_pAllocator, Framegraph2(m_pAllocator, m_rhiContext->getMemoryBlock()));

        m_pFgMemBlock = m_frameGraph->getMemoryBlock();
        m_fgMemWriter = VKZ_NEW(m_pAllocator, MemoryWriter(m_pFgMemBlock));

        setRenderGraphDataDirty();
    }

    bool Context::shouldClose()
    {
        return m_glfwManager.shouldClose();
    }

    void Context::run()
    {
        m_glfwManager.update();

        if (kInvalidHandle == m_presentImage.id)
        {
            message(error, "result render target is not set!");
            return;
        }

        if (isRenderGraphDataDirty())
        {
            bake();
        }

        render();
    }

    void Context::bake()
    {
        // store all resources
        storeBrief();
        storeShaderData();
        storeProgramData();
        storeImageData();
        storeBufferData();
        storeSamplerData();
        storePassData();
        storeAliasData();

        // write the finish tag
        MagicTag magic{ MagicTag::end };
        write(m_fgMemWriter, magic);

        m_frameGraph->setMemoryBlock(m_pFgMemBlock);
        m_frameGraph->bake();

        m_isRenderGraphDataDirty = false;

        m_rhiContext->bake();
    }

    void Context::render()
    {
        m_rhiContext->render();
    }

    void Context::shutdown()
    {
        deleteObject(m_pAllocator, m_frameGraph);
        deleteObject(m_pAllocator, m_rhiContext);
        deleteObject(m_pAllocator, m_fgMemWriter);

        m_pFgMemBlock = nullptr;
        m_pAllocator = nullptr;
    }

    // ================================================
    static Context* s_ctx = nullptr;


    ShaderHandle registShader(const char* _name, const char* _path)
    {
        return s_ctx->registShader(_name, _path);
    }

    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants /*= 0*/)
    {
        const uint16_t shaderNum = static_cast<const uint16_t>(_shaders.size());
        const Memory* mem = alloc(shaderNum * sizeof(uint16_t));

        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < shaderNum; ++ii)
        {
            const ShaderHandle& handle = begin(_shaders)[ii];
            write(&writer, handle.id);
        }

        return s_ctx->registProgram(_name, mem, shaderNum, _sizePushConstants);
    }

    BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::transision*/)
    {
        return s_ctx->registBuffer(_name, _desc, _lifetime);
    }

    ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registTexture(_name, _desc, _lifetime);
    }

    ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registRenderTarget(_name, _desc, _lifetime);
    }

    ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registDepthStencil(_name, _desc, _lifetime);
    }

    vkz::ImageViewHandle registImageView(const char* _name, const ImageHandle _hImg, const uint32_t _baseMip, const uint32_t _mipLevel)
    {
        return s_ctx->registImageView(_name, _hImg, _baseMip, _mipLevel);
    }

    PassHandle registPass(const char* _name, PassDesc _desc)
    {
        return s_ctx->registPass(_name, _desc);
    }

    BufferHandle alias(const BufferHandle _baseBuf)
    {
        return s_ctx->aliasBuffer(_baseBuf);
    }

    ImageHandle alias(const ImageHandle _baseImg)
    {
        return s_ctx->aliasImage(_baseImg);
    }

    void bindVertexBuffer(PassHandle _hPass, BufferHandle _buf)
    {
        s_ctx->bindVertexBuffer(_hPass, _buf);
    }

    void bindIndexBuffer(PassHandle _hPass, BufferHandle _buf)
    {
        s_ctx->bindIndexBuffer(_hPass, _buf);
    }

    void setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _maxCount)
    {
        s_ctx->setIndirectBuffer(_hPass, _hBuf, _offset, _stride, _maxCount);
    }

    void setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset)
    {
        s_ctx->setIndirectCountBuffer(_hPass, _hBuf, _offset);
    }

    void bindBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias /*= {kInvalidHandle}*/)
    {
        s_ctx->bindBuffer(_hPass, _hBuf, _binding, _stage, _access, _outAlias);
    }

    SamplerHandle sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, ImageLayout _layout, SamplerReductionMode _reductionMode)
    {
        return s_ctx->sampleImage(_hPass, _hImg, _binding, _stage, _layout, _reductionMode);
    }

    void bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
        , const ImageHandle _outAlias /*= {kInvalidHandle}*/, const ImageViewHandle _hImgView/* = {kInvalidHandle}*/)
    {
        s_ctx->bindImage(_hPass, _hImg, _binding, _stage, _access, _layout, _outAlias, _hImgView);
    }

    void setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias)
    {
        s_ctx->setAttachmentOutput(_hPass, _hImg, _attachmentIdx, _outAlias);
    }

    void fillBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _offset, const uint32_t _size, const uint32_t _value, const BufferHandle _outAlias)
    {
        s_ctx->fillBuffer(_hPass, _hBuf, _offset, _size, _value, _outAlias);
    }

    void copyBuffer(const PassHandle _hPass, const BufferHandle _hSrc, const BufferHandle _hDst, const uint32_t _size)
    {
        ;
    }

    void blitImage(const PassHandle _hPass, const ImageHandle _hSrc, const ImageHandle _hDst)
    {
        ;
    }

    void setCustomRenderFunc(const PassHandle _hPass, RenderFuncPtr _func, const Memory* _dataMem)
    {
        s_ctx->setCustomRenderFunc(_hPass, _func, _dataMem);
    }

    void setPresentImage(ImageHandle _rt)
    {
        s_ctx->setPresentImage(_rt);
    }
    
    void updatePushConstants(const PassHandle _hPass, const Memory* _mem)
    {
        s_ctx->updatePushConstants(_hPass, _mem);
    }

    void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ)
    {
        s_ctx->updateThreadCount(_hPass, _threadCountX, _threadCountY, _threadCountZ);
    }

    void updateBuffer(const BufferHandle _hbuf, const Memory* _mem)
    {
        s_ctx->updateBuffer(_hbuf, _mem);
    }


    void updateCustomRenderFuncData(const PassHandle _hPass, const Memory* _dataMem)
    {
        s_ctx->updateCustomRenderFuncData(_hPass, _dataMem);
    }

    const Memory* alloc(uint32_t _sz)
    {
        if (_sz < 0)
        {
            message(error, "_sz < 0");
        }

        Memory* mem = (Memory*)alloc(getAllocator(), sizeof(Memory) + _sz);
        mem->size = _sz;
        mem->data = (uint8_t*)mem + sizeof(Memory);
        return mem;
    }

    const Memory* copy(const void* _data, uint32_t _sz)
    {
        if (_sz < 0)
        {
            message(error, "_sz < 0");
        }

        const Memory* mem = alloc(_sz);
        ::memcpy(mem->data, _data, _sz);
        return mem;
    }

    const Memory* copy(const Memory* _mem)
    {
        return copy(_mem->data, _mem->size);
    }

    const Memory* makeRef(const void* _data, uint32_t _sz, ReleaseFn _releaseFn /*= nullptr*/, void* _userData /*= nullptr*/)
    {
        MemoryRef* memRef = (MemoryRef*)alloc(getAllocator(), sizeof(MemoryRef));
        memRef->mem.size = _sz;
        memRef->mem.data = (uint8_t*)_data;
        memRef->releaseFn = _releaseFn;
        memRef->userData = _userData;
        return &memRef->mem;
    }

    // main data here
    bool init(VKZInitConfig _config /*= {}*/)
    {
        s_ctx = VKZ_NEW(getAllocator(), Context);
        s_ctx->setRenderSize(_config.windowWidth, _config.windowHeight);
        s_ctx->setWindowName(_config.name);
        s_ctx->init();

        return true;
    }

    bool shouldClose()
    {
        return s_ctx->shouldClose();
    }

    void bake()
    {
        s_ctx->bake();
    }

    void run()
    {
        s_ctx->run();
    }

    void shutdown()
    {
        deleteObject(getAllocator(), s_ctx);
        shutdownAllocator();
    }
} // namespace vkz