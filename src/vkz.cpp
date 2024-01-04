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
#include <array>
#include "rhi_context_vk.h"


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
        case vkz::ResourceFormat::undefined:
            bpp = 0;
            break;
        case vkz::ResourceFormat::r8_sint:
            bpp = 1;
            break;
        case vkz::ResourceFormat::r8_uint:
            bpp = 1;
            break;
        case vkz::ResourceFormat::r16_uint:
            bpp = 2;
            break;
        case vkz::ResourceFormat::r16_sint:
            bpp = 2;
            break;
        case vkz::ResourceFormat::r16_snorm:
            bpp = 2;
            break;
        case vkz::ResourceFormat::r16_unorm:
            bpp = 2;
            break;
        case vkz::ResourceFormat::r32_uint:
            bpp = 4;
            break;
        case vkz::ResourceFormat::r32_sint:
            bpp = 4;
            break;
        case vkz::ResourceFormat::r32_sfloat:
            bpp = 4;
            break;
        case vkz::ResourceFormat::b8g8r8a8_snorm:
            bpp = 4;
            break;
        case vkz::ResourceFormat::b8g8r8a8_unorm:
            bpp = 4;
            break;
        case vkz::ResourceFormat::b8g8r8a8_sint:
            bpp = 4;
            break;
        case vkz::ResourceFormat::b8g8r8a8_uint:
            bpp = 4;
            break;
        case vkz::ResourceFormat::r8g8b8a8_snorm:
            bpp = 4;
            break;
        case vkz::ResourceFormat::r8g8b8a8_unorm:
            bpp = 4;
            break;
        case vkz::ResourceFormat::r8g8b8a8_sint:
            bpp = 4;
            break;
        case vkz::ResourceFormat::r8g8b8a8_uint:
            bpp = 4;
            break;
        case vkz::ResourceFormat::unknown_depth:
            bpp = 0;
            break;
        case vkz::ResourceFormat::d16:
            bpp = 2;
            break;
        case vkz::ResourceFormat::d32:
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
        vkz::free(getAllocator(), mem);
    }

    struct Context
    {
        // conditions
        bool isCompute(const PassHandle _hPass);
        bool isGraphics(const PassHandle _hPass);

        bool isDepthStencil(const ImageHandle _hImg);
        bool isColorAttachment(const ImageHandle _hImg);
        bool isNormalImage(const ImageHandle _hImg);

        // Context API
        ShaderHandle registShader(const char* _name, const char* _path);
        ProgramHandle registProgram(const char* _name, const Memory* _shaders, const uint16_t _shaderCount, const uint32_t _sizePushConstants = 0);

        PassHandle registPass(const char* _name, const PassDesc& _desc);

        BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime);
        ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);
        ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);
        ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);

        BufferHandle aliasBuffer(const BufferHandle _baseBuf);
        ImageHandle aliasImage(const ImageHandle  _baseImg);

        void readBuffer(const PassHandle _hPass, const BufferHandle _hRes, ResInteractDesc _interact);
        void writeBuffer(const PassHandle _hPass, const BufferHandle _hRes, ResInteractDesc _interact);
        void readImage(const PassHandle _hPass, const ImageHandle _hRes, ResInteractDesc _interact);
        void writeImage(const PassHandle _hPass, const ImageHandle _hRes, ResInteractDesc _interact);

        // new
        void setMultiFrame(ImageHandle _img);
        void setMultiFrame(BufferHandle _buf);
        
        void setPresentImage(ImageHandle _rt);

        // actual write to memory
        void storeBrief();
        void storeShaderData();
        void storeProgramData();
        void storePassData();
        void storeBufferData();
        void storeImageData();
        void storeAliasData();

        void init();
        void loop();
        void update();
        void render();
        void shutdown();

        void setRenderDataDirty() { m_isRenderDataDirty = true; }
        bool isRenderDataDirty() const { return m_isRenderDataDirty; }

        HandleArrayT<kMaxNumOfShaderHandle> m_shaderHandles;
        HandleArrayT<kMaxNumOfProgramHandle> m_programHandles;
        HandleArrayT<kMaxNumOfPassHandle> m_passHandles;
        HandleArrayT<kMaxNumOfImageHandle> m_imageHandles;
        HandleArrayT<kMaxNumOfBufferHandle> m_bufferHandles;

        ImageHandle m_presentImage{kInvalidHandle};

        // resource Info
        UniDataContainer<ShaderHandle, std::string> m_shaderDescs;
        UniDataContainer<ProgramHandle, ProgramDesc> m_programDescs;
        UniDataContainer<BufferHandle, BufferMetaData> m_bufferMetas;
        UniDataContainer<ImageHandle, ImageMetaData> m_imageMetas;
        UniDataContainer<PassHandle, PassMetaData> m_passMetas;

        // alias
        UniDataContainer<BufferHandle, std::vector<BufferHandle>> m_aliasBuffers;
        UniDataContainer<ImageHandle, std::vector<ImageHandle>> m_aliasImages;

        // read/write resources
        UniDataContainer<PassHandle, std::vector<PassResInteract>> m_readBuffers;
        UniDataContainer<PassHandle, std::vector<PassResInteract>> m_writeBuffers;
        UniDataContainer<PassHandle, std::vector<PassResInteract>> m_readImages;
        UniDataContainer<PassHandle, std::vector<PassResInteract>> m_writeImages;

        // render data status
        bool    m_isRenderDataDirty{ false };

        // frame graph
        MemoryBlockI* m_pFgMemBlock{ nullptr };
        MemoryWriter* m_fgMemWriter{ nullptr };

        RHIContext* m_rhiContext{ nullptr };

        Framegraph2* m_frameGraph{ nullptr };

        AllocatorI* m_pAllocator{ nullptr };
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

    ShaderHandle Context::registShader(const char* _name, const char* _path)
    {
        uint16_t idx = m_shaderHandles.alloc();

        ShaderHandle handle = ShaderHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ShaderHandle failed!");
            return ShaderHandle{ kInvalidHandle };
        }

        m_shaderDescs.addData({ idx }, _path);
        
        setRenderDataDirty();

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
        m_programDescs.addData({ idx }, desc);

        setRenderDataDirty();

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
        m_passMetas.addData({ idx }, meta);

        setRenderDataDirty();

        return handle;
    }

    vkz::BufferHandle Context::registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime)
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
        m_bufferMetas.addData({ idx }, meta);

        setRenderDataDirty();

        return handle;
    }

    vkz::ImageHandle Context::registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
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
        meta.format = _desc.format;
        meta.usage = _desc.usage;
        meta.aspectFlags = ImageAspectFlagBits::color;
        meta.lifetime = _lifetime;

        m_imageMetas.addData({ idx }, meta);

        setRenderDataDirty();

        return handle;
    }

    vkz::ImageHandle Context::registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
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

        ImageMetaData meta = _desc;
        meta.imgId = idx;
        meta.format = ResourceFormat::undefined;
        meta.usage = ImageUsageFlagBits::color_attachment | _desc.usage;
        meta.aspectFlags = ImageAspectFlagBits::color;
        meta.lifetime = _lifetime;

        m_imageMetas.addData({ idx }, meta);

        setRenderDataDirty();

        return handle;
    }

    vkz::ImageHandle Context::registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
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
            message(error, "do not set the resource format for depth stencil!");
            return ImageHandle{ kInvalidHandle };
        }

        ImageMetaData meta{ _desc };
        meta.imgId = idx;
        meta.format = ResourceFormat::unknown_depth;
        meta.usage = ImageUsageFlagBits::depth_stencil_attachment | _desc.usage;
        meta.aspectFlags = ImageAspectFlagBits::depth;
        meta.lifetime = _lifetime;

        m_imageMetas.addData({ idx }, meta);

        setRenderDataDirty();

        return handle;
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
            PassHandle pass{ (m_passHandles.getHandleAt(ii)) };
            const PassMetaData& meta = m_passMetas.getIdToData(pass);

            // frame graph data
            MagicTag magic{ MagicTag::register_pass };
            write(m_fgMemWriter, magic);

            PassRegisterInfo info;
            info.passId = meta.passId;
            info.programId = meta.programId;
            info.queue = meta.queue;
            info.vertexBindingNum = meta.vertexBindingNum; // binding struct
            info.vertexAttributeNum = meta.vertexAttributeNum; // attribute struct 
            info.vertexAttributeInfos = meta.vertexAttributeInfos;
            info.vertexBindingInfos = meta.vertexBindingInfos;
            info.pushConstantNum = meta.pushConstantNum; // int
            info.pipelineConfig = meta.pipelineConfig;

            info.writeDepthId = meta.writeDepthId;
            info.writeImageNum = meta.writeImageNum; // include depth and color attachment
            info.readImageNum = meta.readImageNum;
            info.readBufferNum = meta.readBufferNum;
            info.writeBufferNum = meta.writeBufferNum;

            write(m_fgMemWriter, info);

            if (0 != meta.vertexBindingNum)
            {
                write(m_fgMemWriter, meta.vertexBindingInfos, sizeof(VertexBindingDesc) * meta.vertexBindingNum);
            }
            if (0 != meta.vertexAttributeNum)
            {
                write(m_fgMemWriter, meta.vertexAttributeInfos, sizeof(VertexAttributeDesc) * meta.vertexAttributeNum);
            }
            if (0 != meta.pushConstantNum)
            {
                write(m_fgMemWriter, meta.pushConstants, sizeof(int) * meta.pushConstantNum);
            }

            // read/write resources
            // write image
            // read image
            // write buffer
            // read buffer
            if (0 != info.writeImageNum)
            {
                write(m_fgMemWriter, m_writeImages.getDataRef(pass).data(), sizeof(PassResInteract) * (info.writeImageNum));
            }
            if (0 != info.readImageNum)
            {
                write(m_fgMemWriter, (void*)m_readImages.getDataRef(pass).data(), sizeof(PassResInteract) * info.readImageNum);
            }
            if (0 != info.writeBufferNum)
            {
                write(m_fgMemWriter, (void*)m_writeBuffers.getDataRef(pass).data(), sizeof(PassResInteract) * info.writeBufferNum);
            }
            if (0 != info.readBufferNum)
            {
                write(m_fgMemWriter, (void*)m_readBuffers.getDataRef(pass).data(), sizeof(PassResInteract) * info.readBufferNum);
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
            info.format = meta.format;
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
            BufferHandle buf { (m_bufferHandles.getHandleAt(ii)) };
            const std::vector<BufferHandle>& aliasVec = m_aliasBuffers.getIdToData(buf);

            if (aliasVec.size() > 1)
            {
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
        }

        // images
        for (uint16_t ii = 0; ii < m_aliasImages.size(); ++ii)
        {
            ImageHandle img { (m_imageHandles.getHandleAt(ii)) };
            const std::vector<ImageHandle>& aliasVec = m_aliasImages.getIdToData(img);

            if (aliasVec.size() > 1)
            {
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
    }

    vkz::BufferHandle Context::aliasBuffer(const BufferHandle _baseBuf)
    {
        uint16_t aliasId = m_bufferHandles.alloc();

        if (m_aliasBuffers.exist(_baseBuf)) 
        {
            std::vector<BufferHandle>& aliasVecRef = m_aliasBuffers.getDataRef(_baseBuf);
            aliasVecRef.emplace_back(BufferHandle{ aliasId });
        }
        else 
        {
            std::vector<BufferHandle> aliasVec{};
            aliasVec.push_back(_baseBuf);
            aliasVec.emplace_back(BufferHandle{ aliasId });

            m_aliasBuffers.addData({ _baseBuf }, aliasVec);
        }

        setRenderDataDirty();

        return { aliasId };
    }

    vkz::ImageHandle Context::aliasImage(const ImageHandle _baseImg)
    {
        uint16_t aliasId = m_imageHandles.alloc();

        if (m_aliasImages.exist(_baseImg))
        {
            std::vector<ImageHandle>& aliasVecRef = m_aliasImages.getDataRef(_baseImg);
            aliasVecRef.emplace_back(ImageHandle{ aliasId });
        }
        else
        {
            std::vector<ImageHandle> aliasVec{};
            aliasVec.push_back(_baseImg);
            aliasVec.emplace_back(ImageHandle{ aliasId });

            m_aliasImages.addData({ _baseImg }, aliasVec);
        }

        setRenderDataDirty();

        return { aliasId };
    }

    void addResInteract(UniDataContainer<PassHandle, std::vector<PassResInteract>>& _container, const PassHandle _hPass, const uint16_t _resId, const ResInteractDesc& _interact)
    {
        PassResInteract pri;
        pri.passId = _hPass.id;
        pri.resId = _resId;
        pri.interact = _interact;

        if (_container.exist(_hPass))
        {
            std::vector<PassResInteract>& prInteractVec = _container.getDataRef(_hPass);
            prInteractVec.emplace_back(pri);
        }
        else
        {
            std::vector<PassResInteract> prInteractVec{};
            prInteractVec.emplace_back(pri);
            _container.addData({ _hPass }, prInteractVec);
        }
    }

    void Context::readBuffer(const PassHandle _hPass, const BufferHandle _hBuf, ResInteractDesc _interact)
    {
        addResInteract(m_readBuffers, _hPass, _hBuf.id, _interact);
        setRenderDataDirty();
    }

    void Context::writeBuffer(const PassHandle _hPass, const BufferHandle _hBuf, ResInteractDesc _interact)
    {
        addResInteract(m_writeBuffers, _hPass, _hBuf.id, _interact);
        setRenderDataDirty();
    }

    void Context::readImage(const PassHandle _hPass, const ImageHandle _hImg, ResInteractDesc _interact)
    {
        ImageMetaData& meta = m_imageMetas.getDataRef(_hImg);
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        if (isGraphics(_hPass) && isDepthStencil(_hImg))
        {
            assert((meta.aspectFlags & ImageAspectFlagBits::depth) > 0);

            _interact.access |= AccessFlagBits::depth_stencil_attachment_read;
            addResInteract(m_readImages, _hPass, _hImg.id, _interact);
            passMeta.readImageNum += 1;
        }
        else if (isGraphics(_hPass) && isColorAttachment(_hImg))
        {
            assert((meta.aspectFlags & ImageAspectFlagBits::color) > 0);

            _interact.access |= AccessFlagBits::color_attachment_read;
            addResInteract(m_readImages, _hPass, _hImg.id, _interact);
            passMeta.readImageNum += 1;
        }
        else
        {
            addResInteract(m_readImages, _hPass, _hImg.id, _interact);
            passMeta.readImageNum += 1;
        }

        setRenderDataDirty();
    }

    void Context::writeImage(const PassHandle _hPass, const ImageHandle _hImg, ResInteractDesc _interact)
    {
        ImageMetaData& meta = m_imageMetas.getDataRef(_hImg);
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);

        if (isGraphics(_hPass) && isDepthStencil(_hImg) && kInvalidHandle == passMeta.writeDepthId)
        {
            assert((meta.aspectFlags & ImageAspectFlagBits::depth) > 0);

            passMeta.writeDepthId = _hImg.id;

            _interact.access |= AccessFlagBits::depth_stencil_attachment_write;
            addResInteract(m_writeImages, _hPass, _hImg.id, _interact);
            passMeta.writeImageNum += 1;
        }
        else if (isGraphics(_hPass) && isColorAttachment(_hImg))
        {
            assert((meta.aspectFlags & ImageAspectFlagBits::color) > 0);

            _interact.access |= AccessFlagBits::color_attachment_write;
            addResInteract(m_writeImages, _hPass, _hImg.id, _interact);
            passMeta.writeImageNum += 1;
        }

        else if (isCompute(_hPass) && isNormalImage(_hImg))
        {
            assert((meta.aspectFlags & ImageAspectFlagBits::color) > 0);

            addResInteract(m_writeImages, _hPass, _hImg.id, _interact);
            passMeta.writeImageNum += 1;
        }
        else
        {
            assert(0);
        }

        setRenderDataDirty();
    }

    void Context::setMultiFrame(ImageHandle _img)
    {
        ImageMetaData meta = m_imageMetas.getIdToData(_img);
        meta.lifetime = ResourceLifetime::non_transition;

        m_imageMetas.updateData(_img, meta);

        setRenderDataDirty();
    }

    void Context::setMultiFrame(BufferHandle _buf)
    {
        BufferMetaData meta = m_bufferMetas.getIdToData(_buf);
        meta.lifetime = ResourceLifetime::non_transition;

        m_bufferMetas.updateData(_buf, meta);

        setRenderDataDirty();
    }

    void Context::setPresentImage(ImageHandle _img)
    {
        m_presentImage = _img;

        setRenderDataDirty();
    }

    void Context::init()
    {
        m_pAllocator = getAllocator();
        
        m_rhiContext = VKZ_NEW(m_pAllocator, RHIContext_vk(m_pAllocator));

        m_frameGraph = VKZ_NEW(m_pAllocator, Framegraph2(m_pAllocator, m_rhiContext->getMemoryBlock()));

        m_pFgMemBlock = m_frameGraph->getMemoryBlock();
        m_fgMemWriter = VKZ_NEW(m_pAllocator, MemoryWriter(m_pFgMemBlock));

        setRenderDataDirty();
    }

    void Context::loop()
    {
        update();
        render();
    }

    void Context::update()
    {
        if (kInvalidHandle == m_presentImage.id)
        {
            message(error, "result render target is not set!");
            return;
        }

        if (!isRenderDataDirty())
        {
            return;
        }

        // store all resources
        storeBrief();
        storeShaderData();
        storeProgramData();
        storeImageData();
        storeBufferData();
        storePassData();
        storeAliasData();

        // write the finish tag
        MagicTag magic{ MagicTag::end };
        write(m_fgMemWriter, magic);

        m_frameGraph->setMemoryBlock(m_pFgMemBlock);
        m_frameGraph->bake();

        m_rhiContext->update();

        m_isRenderDataDirty = false;
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


    vkz::ShaderHandle registShader(const char* _name, const char* _path)
    {
        return s_ctx->registShader(_name, _path);
    }

    vkz::ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants /*= 0*/)
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

    vkz::BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registBuffer(_name, _desc, _lifetime);
    }

    vkz::ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/, const Memory* _mem /*= nullptr*/)
    {
        return s_ctx->registTexture(_name, _desc, _lifetime);
    }

    vkz::ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registRenderTarget(_name, _desc, _lifetime);
    }

    vkz::ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registDepthStencil(_name, _desc, _lifetime);
    }

    PassHandle registPass(const char* _name, PassDesc _desc)
    {
        return s_ctx->registPass(_name, _desc);
    }

    vkz::BufferHandle alias(const BufferHandle _baseBuf)
    {
        return s_ctx->aliasBuffer(_baseBuf);
    }

    vkz::ImageHandle alias(const ImageHandle _baseImg)
    {
        return s_ctx->aliasImage(_baseImg);
    }

    void passReadBuffer(PassHandle _pass, BufferHandle _buf, ResInteractDesc _interact)
    {
        s_ctx->readBuffer(_pass, _buf, _interact);
    }

    void passWriteBuffer(PassHandle _pass, BufferHandle _buf, ResInteractDesc _interact)
    {
        s_ctx->writeBuffer(_pass, _buf, _interact);
    }

    void passReadImage(PassHandle _pass, ImageHandle _img, ResInteractDesc _interact)
    {
        s_ctx->readImage(_pass, _img, _interact);
    }

    void passWriteImage(PassHandle _pass, ImageHandle _img, ResInteractDesc _interact)
    {
        s_ctx->writeImage(_pass, _img, _interact);
    }

    void setPresentImage(ImageHandle _rt)
    {
        s_ctx->setPresentImage(_rt);
    }

    const Memory* alloc(uint32_t _sz)
    {
        if (_sz < 0)
        {
            message(error, "_sz < 0");
        }

        Memory* mem = (Memory*)vkz::alloc(getAllocator(), sizeof(Memory) + _sz);
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
        MemoryRef* memRef = (MemoryRef*)vkz::alloc(getAllocator(), sizeof(MemoryRef));
        memRef->mem.size = _sz;
        memRef->mem.data = (uint8_t*)_data;
        memRef->releaseFn = _releaseFn;
        memRef->userData = _userData;
        return &memRef->mem;
    }

    // main data here
    bool init()
    {
        s_ctx = VKZ_NEW(getAllocator(), Context);
        s_ctx->init();

        return true;
    }

    void loop()
    {
        s_ctx->loop();
    }

    void shutdown()
    {
        deleteObject(getAllocator(), s_ctx);
        shutdownAllocator();
    }
}