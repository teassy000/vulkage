#include "common.h"

#include "kage_inner.h"

#include "config.h"

#include "util.h"

#include "rhi_context.h"
#include "framegraph.h"


#include "bx/string.h"

#include "bx/allocator.h"
#include "bx/readerwriter.h"
#include "bx/settings.h"
#include "bx/string.h"
#include "bx/handlealloc.h"
#include "command_buffer.h"


namespace kage
{
    constexpr size_t kDefaultAlign = 8;
    struct AllocatorStub : public bx::AllocatorI
    {
        AllocatorStub() = default;
        ~AllocatorStub() override {}

        void* realloc(
            void* _ptr
            , size_t _sz
            , size_t _align
            , const char* _file
            , uint32_t _line) override
        {
            if (0 == _sz)
            {
                if (nullptr != _ptr)
                {
                    KG_ProfFree(_ptr);
                    if (kDefaultAlign >= _align)
                    {
                        ::free(_ptr);
                    }
                    else
                    {
                        ::_aligned_free(_ptr);
                    }
                }

                return nullptr;
            }

            if (nullptr == _ptr)
            {
                void* ret = nullptr;
                if (kDefaultAlign >= _align)
                {
                    ret = ::malloc(_sz);

                }
                else
                {
                    ret = _aligned_malloc(_sz, _align);
                }

                KG_ProfAlloc(ret, _sz);
                return ret;
            }

            if (kDefaultAlign >= _align)
            {
                KG_ProfFree(_ptr);
                void* ret = ::realloc(_ptr, _sz);
                KG_ProfAlloc(ret, _sz);
                return ret;
            }

            KG_ProfFree(_ptr);
            void* ret = _aligned_realloc(_ptr, _sz, _align);
            KG_ProfAlloc(ret, _sz);
            return ret;
        }

    };

    bx::AllocatorI* g_bxAllocator = nullptr;
    static bx::AllocatorI* getAllocator()
    {
        if (g_bxAllocator == nullptr)
        {
            bx::DefaultAllocator allocator;
            g_bxAllocator = BX_NEW(&allocator, AllocatorStub)();
        }
        return g_bxAllocator;
    }

    void* TinyStlAllocator::static_allocate(size_t _bytes)
    {
        return bx::alloc(getAllocator(), _bytes);
    }

    void TinyStlAllocator::static_deallocate(void* _ptr, size_t /*_bytes*/)
    {
        if (nullptr != _ptr)
        {
            bx::free(getAllocator(), _ptr);
        }
    }

    static void shutdownAllocator()
    {
        g_bxAllocator = nullptr;
    }


    namespace NameTags
    {
        // tag for varies of handle 
        static const char* kShader = "[]";
        static const char* kPass = "[]";
        static const char* kProgram = "[]";
        static const char* kImage = "[]";
        static const char* kBuffer = "[]";
        static const char* kSampler = "[]";

        // tag for alias: buffer, image
        static const char* kAlias = "[alias]";

        static const char* nameTags[] = {
            kShader,
            kProgram,
            kPass,
            kBuffer,
            kImage,
            kSampler,
            kAlias,
        };
    }

    static Handle::TypeName s_aliasName = {"Als", "Alias"};
    static Handle::TypeName s_typeNames[] = {
        { "Sh",     "Shader" },
        { "Pr",     "Program" },
        { "Pa",     "Pass" },
        { "B",      "Buffer" },
        { "I",      "Image" },
        { "?",      "?"},
    };

    const Handle::TypeName& Handle::getTypeName(Handle::Enum _enum)
    {
        BX_ASSERT(_enum < Handle::Count, "Invalid Handle::Enum %d!", _enum);
        return s_typeNames[bx::min(_enum, Handle::Count)];
    }

    struct NameMgr
    {
        NameMgr()
        {
            m_handleToName.clear();
        }

        ~NameMgr()
        {
            m_handleToName.clear();
        }

        void setName(const char* _name, const int32_t _len, const Handle _h, bool _alias)
        {
            String name;
            getPrefix(name, _h, _alias);
            name.append("_");
            name.append(_name);

            m_handleToName.insert({ _h, name });
        }

        const char* getName(const Handle _h) const
        {
            HandleNameMap::const_iterator it = m_handleToName.find(_h);

            BX_ASSERT(m_handleToName.end() != it
                , "Request Name for: %d_%d which is not set yet!"
                , _h.getTypeName()
                , _h.id
            );

            const String& str = it->second;
            return str.getCPtr();
        }

        void getPrefix(String& _outStr, Handle _h, bool _isAlias)
        {
            // if it's aliased resource, just add alias tag since the old name already contains the type tag
            if (_isAlias)
            {
                _outStr.append(s_aliasName.abbrName);
                return;
            }

            const Handle::TypeName& typeName = _h.getTypeName();
            _outStr.append(typeName.abbrName);
        }

        using HandleNameMap = stl::unordered_map<Handle, String>;
        HandleNameMap m_handleToName;
    };

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

    struct RecordingCmd
    {
        uint32_t startIdx;
        uint32_t endIdx;
        uint32_t count;
    };

    struct Context
    {
        // conditions
        bool isCompute(const PassHandle _hPass);
        bool isGraphics(const PassHandle _hPass);
        bool isRead(const AccessFlags _access);
        bool isWrite(const AccessFlags _access, const PipelineStageFlags _stage);

        bool isDepthStencil(const ImageHandle _hImg);
        bool isColorAttachment(const ImageHandle _hImg);

        bool isAliasFromBuffer(const BufferHandle _hBase, const BufferHandle _hAlias);
        bool isAliasFromImage(const ImageHandle _hBase, const ImageHandle _hAlias);

        // Context API
        void setNativeWindowHandle(void* _hWnd);

        bool checkSupports(VulkanSupportExtension _ext);

        ShaderHandle registShader(const char* _name, const char* _path);
        ProgramHandle registProgram(const char* _name, const Memory* _shaders, const uint16_t _shaderCount, const uint32_t _sizePushConstants, const BindlessHandle _bindless);

        PassHandle registPass(const char* _name, const PassDesc& _desc);

        BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const Memory* _mem, const ResourceLifetime _lifetime);

        ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const Memory* _mem, const ResourceLifetime _lifetime);
        ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);
        ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);

        BindlessHandle registBindless(const char* _name, const BindlessDesc& _desc, const ResourceLifetime _lifetime);

        SamplerHandle requestSampler(const SamplerDesc& _desc);

        BufferHandle aliasBuffer(const BufferHandle _baseBuf);
        ImageHandle aliasImage(const ImageHandle  _baseImg);

        void setName(Handle _h, const char* _name, const size_t _len, bool _alias = false);
        const bx::StringView getName(Handle _h);

        // read-only data, no barrier required
        void bindVertexBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _vtxCount);
        void bindIndexBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _idxCount);

        void setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _defaultMaxCount);
        void setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset);

        void bindBuffer(PassHandle _hPass, BufferHandle _buf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias);
        SamplerHandle sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, SamplerFilter _filter, SamplerMipmapMode _mipMode, SamplerAddressMode _addrMode, SamplerReductionMode _reductionMode);

        void setBindlessTextures(BindlessHandle _bindless, const Memory* _mem, uint32_t _texCount, SamplerReductionMode _reductionMode);

        void bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
            , const ImageHandle _outAlias);

        void setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias);

        void setMultiFrame(ImageHandle _img);
        void setMultiFrame(BufferHandle _buf);
        
        void setPresentImage(ImageHandle _rt, uint32_t _mipLv);

        void updateBuffer(const BufferHandle _hbuf, const Memory* _mem, const uint32_t _offset, const uint32_t _size);
        void updateImage(const ImageHandle _hImg 
            , uint32_t _width
            , uint32_t _height
            , const Memory* _mem);

        // renderer execute commands
        void rendererExecCmdQ(CommandQueue& _cmdQ);

        // actual write to memory
        void storeBrief();
        void storeShaderData();
        void storeProgramData();
        void storePassData();
        void storeBufferData();
        void storeImageData();
        void storeAliasData();
        void storeSamplerData();
        void storeBindlessData();

        void init(const Init& _init);

        void render();
        void bake();

        void reset(uint32_t _windth, uint32_t _height, uint32_t _reset);
        void rhi_render();
        void shutdown();

        void setRenderGraphDataDirty() { m_isRenderGraphDataDirty = true; }
        bool isRenderGraphDataDirty() const { return m_isRenderGraphDataDirty; }

        double getPassTime(const PassHandle _hPass);
        uint64_t getPassClipping(const PassHandle _hPass);
        double getGpuTime();

        // Render API Begin
        void startRec(const PassHandle _hPass);

        void setConstants(const Memory* _mem);

        void pushBindings(const Binding* _desc, uint16_t _count);

        void setColorAttachments(const Attachment* _colors, uint16_t _count);

        void setDepthAttachment(const Attachment _depth);

        void setBindless(BindlessHandle _hBindless);

        void setBuffer(
            const BufferHandle _hBuf
            , const uint32_t _binding
            , const PipelineStageFlags _stage
            , const AccessFlags _access
            , const BufferHandle _outAlias
        );

        void setImage(
            const ImageHandle _hImg
            , const uint32_t _binding
            , const PipelineStageFlags _stage
            , const AccessFlags _access
            , const ImageLayout _layout
            , const ImageHandle _outAlias
        );

        void dispatch(
            const uint32_t _groupCountX
            , const uint32_t _groupCountY
            , const uint32_t _groupCountZ
        );

        void setVertexBuffer(BufferHandle _hBuf);

        void setIndexBuffer(BufferHandle _hBuf, uint32_t _offset, IndexType _type);

        void setViewport(
            int32_t _x
            , int32_t _y
            , uint32_t _width
            , uint32_t _height
        );

        void setScissor(
            int32_t _x
            , int32_t _y
            , uint32_t _width
            , uint32_t _height
        );

        void fillBuffer(
            const BufferHandle _hBuf
            , const uint32_t _value
        );

        // draw
        void draw(
            const uint32_t _vertexCount
            , const uint32_t _instanceCount
            , const uint32_t _firstVertex
            , const uint32_t _firstInstance
        );

        // draw indexed
        void drawIndexed(
            const uint32_t _indexCount
            , const uint32_t _instanceCount
            , const uint32_t _firstIndex
            , const int32_t _vertexOffset
            , const uint32_t _firstInstance
        );

        // draw indirect
        void drawIndirect(
            const BufferHandle _hIndirectBuf
            , const uint32_t _offset
            , const uint32_t _count
            , const uint32_t _stride
        );

        // draw indirect count
        void drawIndirect(
            const uint32_t _offset
            , const BufferHandle _countBuf
            , const uint32_t _countOffset
            , const uint32_t _maxCount
            , const uint32_t _stride
        );

        // draw indexed indirect 
        void drawIndexed(
            const BufferHandle _hIndirectBuf
            , const uint32_t _offset
            , const uint32_t _count
            , const uint32_t _stride
        );

        // draw indexed indirect count
        void drawIndexed(
            const BufferHandle _hIndirectBuf
            , const uint32_t _offset
            , const BufferHandle _countBuf
            , const uint32_t _countOffset
            , const uint32_t _maxCount
            , const uint32_t _stride
        );

        void drawMeshTask(
            const uint32_t _groupCountX
            , const uint32_t _groupCountY
            , const uint32_t _groupCountZ
        );

        void drawMeshTask(
            const BufferHandle _hIndirectBuf
            , const uint32_t _offset
            , const uint32_t _count
            , const uint32_t _stride
        );

        void drawMeshTask(
            const BufferHandle _hIndirectBuf
            , const uint32_t _offset
            , const BufferHandle _countBuf
            , const uint32_t _countOffset
            , const uint32_t _maxCount
            , const uint32_t _stride
        );

        void endRec();

        // Render API Ends

        bx::HandleAllocT<kMaxNumOfShaderHandle> m_shaderHandles;
        bx::HandleAllocT<kMaxNumOfProgramHandle> m_programHandles;
        bx::HandleAllocT<kMaxNumOfPassHandle> m_passHandles;
        bx::HandleAllocT<kMaxNumOfBufferHandle> m_bufferHandles;
        bx::HandleAllocT<kMaxNumOfImageHandle> m_imageHandles;
        bx::HandleAllocT<kMaxNumOfSamplerHandle> m_samplerHandles;
        bx::HandleAllocT<kMaxNumOfBindlessResHandle> m_bindlessResHandles;
        
        ImageHandle m_presentImage{kInvalidHandle};
        uint32_t m_presentMipLevel{ 0 };

        NameMgr* m_nameManager;

        // resource Info
        String                  m_shaderPathes[kMaxNumOfShaderHandle];
        ProgramDesc             m_programDescs[kMaxNumOfProgramHandle];
        BufferMetaData          m_bufferMetas[kMaxNumOfBufferHandle];
        ImageMetaData           m_imageMetas[kMaxNumOfImageHandle];
        PassMetaData            m_passMetas[kMaxNumOfPassHandle];
        SamplerDesc             m_samplerDescs[kMaxNumOfSamplerHandle];
        BindlessMetaData        m_bindlessResMetas[kMaxNumOfBindlessResHandle];

        // running Pass
        PassHandle              m_recordingPass{ kInvalidHandle };
        RecordingCmd            m_recCmds[kMaxNumOfPassHandle];

        // alias
        stl::unordered_map<uint16_t, BufferHandle> m_bufferAliasMapToBase;
        stl::unordered_map<uint16_t, ImageHandle> m_imageAliasMapToBase;

        ContinuousMap<BufferHandle, stl::vector<BufferHandle>> m_aliasBuffers;
        ContinuousMap<ImageHandle, stl::vector<ImageHandle>> m_aliasImages;

        // read/write resources
        ContinuousMap<PassHandle, stl::vector<PassResInteract>> m_readBuffers;
        ContinuousMap<PassHandle, stl::vector<PassResInteract>> m_writeBuffers;
        ContinuousMap<PassHandle, stl::vector<PassResInteract>> m_readImages;
        ContinuousMap<PassHandle, stl::vector<PassResInteract>> m_writeImages;
        ContinuousMap<PassHandle, stl::vector<WriteOperationAlias>> m_writeForcedBufferAliases; // add a dependency line to the write resource
        ContinuousMap<PassHandle, stl::vector<WriteOperationAlias>> m_writeForcedImageAliases; // add a dependency line to the write resource

        // binding
        ContinuousMap<PassHandle, stl::vector<uint32_t>> m_usedBindPoints;
        ContinuousMap<PassHandle, stl::vector<uint32_t>> m_usedAttchBindPoints;

        // push constants
        ContinuousMap<ProgramHandle, void *> m_pushConstants;

        // Memories
        stl::vector<const Memory*> m_inputMemories;
        // transient Memories
        stl::vector<const Memory*> m_transientMemories;

        // render data status
        bool    m_isRenderGraphDataDirty{ false };

        // frame graph
        bx::MemoryBlockI* m_pFgMemBlock{ nullptr };
        bx::MemoryWriter* m_fgMemWriter{ nullptr };

        RHIContext* m_rhiContext{ nullptr };

        Framegraph* m_frameGraph{ nullptr };

        Resolution m_resolution{};

        bx::AllocatorI* m_pAllocator{ nullptr };

        void* m_nativeWnd{ nullptr };

        // frame 
        CommandQueue m_cmdQueue{};
    };

    bool Context::isCompute(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas[_hPass.id];
        return meta.queue == PassExeQueue::compute;
    }

    bool Context::isGraphics(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas[_hPass.id];
        return meta.queue == PassExeQueue::graphics;
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
        const ImageMetaData& meta = m_imageMetas[_hImg.id];
        
        return 
            0 != (meta.usage & ImageUsageFlagBits::depth_stencil_attachment);
    }

    bool Context::isColorAttachment(const ImageHandle _hImg)
    {
        const ImageMetaData& meta = m_imageMetas[_hImg.id];
        
        return 
            0 != (meta.usage & ImageUsageFlagBits::color_attachment);
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

    void Context::setNativeWindowHandle(void* _hWnd)
    {
        if (_hWnd == nullptr)
        {
            message(error, "invalid window handle");
            return;
        }

        m_nativeWnd = _hWnd;
    }

    bool Context::checkSupports(VulkanSupportExtension _ext)
    {
        return m_rhiContext->checkSupports(_ext);
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

        String path{ _path };
        m_shaderPathes[idx] = path;

        setName(handle, _name, strlen(_name));

        setRenderGraphDataDirty();

        m_cmdQueue.cmdCreateShader(handle, m_shaderPathes[idx]);

        return handle;
    }

    ProgramHandle Context::registProgram(const char* _name, const Memory* _mem, const uint16_t _shaderNum, const uint32_t _sizePushConstants, const BindlessHandle _bindless)
    {
        uint16_t idx = m_programHandles.alloc();

        ProgramHandle handle = ProgramHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ProgramHandle failed!");
            release(_mem);
            return ProgramHandle{ kInvalidHandle };
        }

        ProgramDesc desc{};
        desc.progId = idx;
        desc.shaderNum = _shaderNum;
        desc.sizePushConstants = _sizePushConstants;
        desc.bindlessId = _bindless.id;
        memcpy_s(desc.shaderIds, COUNTOF(desc.shaderIds), _mem->data, _mem->size);

        m_programDescs[idx] = desc;

        setName(handle, _name, strlen(_name));

        setRenderGraphDataDirty();

        m_cmdQueue.cmdCreateProgram(handle, _mem, _shaderNum, _sizePushConstants, _bindless);

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

        m_passMetas[idx] = meta;

        setName(handle, _name, strlen(_name));

        setRenderGraphDataDirty();

        m_cmdQueue.cmdCreatePass(handle, _desc);

        return handle;
    }

    kage::BufferHandle Context::registBuffer(const char* _name, const BufferDesc& _desc, const Memory* _mem, const ResourceLifetime _lifetime)
    {
        if (0 == _desc.size)
        {
            message(error, "buffer size is 0");
            return BufferHandle{ kInvalidHandle };
        }

        if (_desc.format == ResourceFormat::undefined
            && ((_desc.usage & BufferUsageFlagBits::uniform_texel) 
                || (_desc.usage & BufferUsageFlagBits::storage_texel))
            )
        {
            message(error, "if the format was set! then is should use as uniform_texel | storage_texel");
            return BufferHandle{ kInvalidHandle };
        }

        uint16_t idx = m_bufferHandles.alloc();

        BufferHandle handle = BufferHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create BufferHandle failed!");
            release(_mem);
            return BufferHandle{ kInvalidHandle };
        }

        BufferMetaData meta{ _desc };
        meta.bufId = idx;
        meta.lifetime = _lifetime;

        if (_mem != nullptr)
        {
            meta.pData = _mem->data;
            meta.usage |= BufferUsageFlagBits::transfer_dst;

            m_inputMemories.push_back(_mem);
        }

        m_bufferMetas[idx] = meta;

        setName(handle, _name, strlen(_name));

        setRenderGraphDataDirty();

        m_cmdQueue.cmdCreateBuffer(handle, _desc, _mem, _lifetime);

        return handle;
    }

    ImageHandle Context::registTexture(const char* _name, const ImageDesc& _desc, const Memory* _mem, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        ImageHandle handle = ImageHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ImageHandle failed!");
            return ImageHandle{ kInvalidHandle };
        }

        ImageMetaData meta = _desc;
        meta.imgId = idx;
        meta.aspectFlags = ImageAspectFlagBits::color;
        meta.lifetime = _lifetime;

        if (_mem != nullptr)
        {
            meta.usage |= ImageUsageFlagBits::transfer_dst;
            meta.size = _mem->size;
            meta.pData = _mem->data;

            m_inputMemories.push_back(_mem);
        }

        m_imageMetas[idx] = meta;

        setName(handle, _name, strlen(_name));

        setRenderGraphDataDirty();

        ImageCreate ic{};
        ic.width = _desc.width;
        ic.height = _desc.height;
        ic.depth = _desc.depth;
        ic.numLayers = _desc.numLayers;
        ic.numMips = _desc.numMips;

        ic.type = _desc.type;
        ic.viewType = _desc.viewType;
        ic.layout = _desc.layout;
        ic.format = _desc.format;
        ic.usage = _desc.usage;
        ic.aspect = ImageAspectFlagBits::color;

        ic.lifetime = _lifetime;
        ic.mem = _mem;

        m_cmdQueue.cmdCreateImage(handle, ic);

        return handle;
    }

    ImageHandle Context::registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        ImageHandle handle = ImageHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ImageHandle failed!");
            return ImageHandle{ kInvalidHandle };
        }

        if (kInitU32 == _desc.width || kInitU32 != _desc.height)
        {
            message(warning, "not set the width/height, will use render size");
        }

        uint32_t width = _desc.width == kInitU32 ? m_resolution.width : _desc.width;
        uint32_t height = _desc.height == kInitU32 ? m_resolution.height : _desc.height;

        ImageMetaData meta = _desc;
        meta.width = width;
        meta.height = height;
        meta.depth = _desc.depth;
        meta.imgId = idx;
        meta.format = _desc.format;
        meta.usage = ImageUsageFlagBits::color_attachment | _desc.usage;
        meta.aspectFlags = ImageAspectFlagBits::color;
        meta.lifetime = _lifetime;

        m_imageMetas[idx] = meta;

        setName(handle, _name, strlen(_name));

        setRenderGraphDataDirty();

        ImageCreate ic{};
        ic.width = width;
        ic.height = height;
        meta.depth = _desc.depth;
        ic.numLayers = _desc.numLayers;
        ic.numMips = _desc.numMips;

        ic.type = _desc.type;
        ic.viewType = _desc.viewType;
        ic.layout = _desc.layout;
        ic.format = _desc.format;
        ic.usage = ImageUsageFlagBits::color_attachment | _desc.usage;
        ic.aspect = ImageAspectFlagBits::color;

        ic.lifetime = _lifetime;

        m_cmdQueue.cmdCreateImage(handle, ic);

        return handle;
    }

    ImageHandle Context::registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        ImageHandle handle = ImageHandle{ idx };

        // check if img is valid
        if (!isValid(handle))
        {
            message(error, "create ImageHandle failed!");
            return ImageHandle{ kInvalidHandle };
        }

        if (kInitU32 == _desc.width || kInitU32 != _desc.height)
        {
            message(warning, "not set the width/height, will use render size");
        }

        // do not set the depth format
        if (_desc.format != ResourceFormat::undefined)
        {
            message(error, "do not set the resource format for depth stencil!");
            return ImageHandle{ kInvalidHandle };
        }

        ImageMetaData meta{ _desc };
        meta.width = m_resolution.width;
        meta.height = m_resolution.height;
        meta.depth = _desc.depth;
        meta.imgId = idx;
        meta.format = ResourceFormat::d32_sfloat;
        meta.usage = ImageUsageFlagBits::depth_stencil_attachment | _desc.usage;
        meta.aspectFlags = ImageAspectFlagBits::depth;
        meta.lifetime = _lifetime;

        m_imageMetas[idx] = meta;

        setName(handle, _name, strlen(_name));

        setRenderGraphDataDirty();

        ImageCreate ic{};
        ic.width = _desc.width;
        ic.height = _desc.height;
        meta.depth = _desc.depth;
        ic.numLayers = _desc.numLayers;
        ic.numMips = _desc.numMips;

        ic.type = _desc.type;
        ic.viewType = _desc.viewType;
        ic.layout = _desc.layout;
        ic.format = ResourceFormat::d32_sfloat;
        ic.usage = ImageUsageFlagBits::depth_stencil_attachment | _desc.usage;
        ic.aspect = ImageAspectFlagBits::depth;

        ic.lifetime = _lifetime;

        m_cmdQueue.cmdCreateImage(handle, ic);

        return handle;
    }

    BindlessHandle Context::registBindless(const char* _name, const BindlessDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_bindlessResHandles.alloc();
        BindlessHandle handle = BindlessHandle{ idx };

        // check if img is valid
        if (!isValid(handle))
        {
            message(error, "create BindlessResHandle failed!");
            return BindlessHandle{ kInvalidHandle };
        }

        m_bindlessResMetas[idx] = BindlessMetaData{ _desc};
        m_bindlessResMetas[idx].bindlessId = idx;

        return handle;
    }

    SamplerHandle Context::requestSampler(const SamplerDesc& _desc)
    {
        // find if the sampler already exist
        uint16_t samplerId = kInvalidHandle;
        for (uint16_t ii = 0; ii < m_samplerHandles.getNumHandles(); ++ii)
        {
            SamplerHandle handle{ m_samplerHandles.getHandleAt(ii) };

            const SamplerDesc& desc = m_samplerDescs[handle.id];

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

            m_samplerDescs[samplerId] = _desc;

            setRenderGraphDataDirty();

            m_cmdQueue.cmdCreateSampler(SamplerHandle{ samplerId }, _desc);
        }

        return SamplerHandle{ samplerId };
    }

    void Context::storeBrief()
    {
        MagicTag magic{ MagicTag::set_brief };
        bx::write(m_fgMemWriter, magic, nullptr);

        // set the brief data
        FrameGraphBrief brief;
        brief.version = 1u;
        brief.passNum = m_passHandles.getNumHandles();
        brief.bufNum = m_bufferHandles.getNumHandles();
        brief.imgNum = m_imageHandles.getNumHandles();
        brief.shaderNum = m_shaderHandles.getNumHandles();
        brief.programNum = m_programHandles.getNumHandles();
        brief.samplerNum = m_samplerHandles.getNumHandles();
        brief.bindlessNum = m_bindlessResHandles.getNumHandles();

        brief.presentImage = m_presentImage.id;
        brief.presentMipLevel = m_presentMipLevel;


        bx::write(m_fgMemWriter, brief, nullptr);

        bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
    }

    void Context::storeShaderData()
    {
        uint16_t shaderCount = m_shaderHandles.getNumHandles();
        
        for (uint16_t ii = 0; ii < shaderCount; ++ii)
        {
            ShaderHandle shader { (m_shaderHandles.getHandleAt(ii)) };
            const String& path = m_shaderPathes[shader.id];

            // magic tag
            MagicTag magic{ MagicTag::register_shader };
            bx::write(m_fgMemWriter, magic, nullptr);

            FGShaderCreateInfo info;
            info.shaderId = shader.id;
            info.strLen = (uint16_t)path.getLength();

            bx::write(m_fgMemWriter, info, nullptr);
            bx::write(m_fgMemWriter, (void*)path.getCPtr(), (int32_t)info.strLen, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeProgramData()
    {
        uint16_t programCount = m_programHandles.getNumHandles();

        for (uint16_t ii = 0; ii < programCount; ++ii)
        {
            ProgramHandle program { (m_programHandles.getHandleAt(ii)) };
            const ProgramDesc& desc = m_programDescs[program.id];

            // magic tag
            MagicTag magic{ MagicTag::register_program };
            bx::write(m_fgMemWriter, magic, nullptr);

            // info
            FGProgCreateInfo info;
            info.progId = desc.progId;
            info.sizePushConstants = desc.sizePushConstants;
            info.shaderNum = desc.shaderNum;
            info.bindlessId = desc.bindlessId;
            bx::write(m_fgMemWriter, info, nullptr);

            // actual shader indexes
            bx::write(m_fgMemWriter, desc.shaderIds, sizeof(uint16_t) * desc.shaderNum, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storePassData()
    {
        uint16_t passCount = m_passHandles.getNumHandles();

        for (uint16_t ii = 0; ii < passCount; ++ii)
        {
            auto getResInteractNum = [](
                const ContinuousMap<PassHandle, stl::vector<PassResInteract>>& __cont
                , const PassHandle __pass) -> uint32_t
            {
                if (__cont.exist(__pass)) {
                    return (uint32_t)(__cont.getIdToData(__pass).size());
                }
                
                return 0u;
            };

            PassHandle pass{ (m_passHandles.getHandleAt(ii)) };
            const PassMetaData& passMeta = m_passMetas[pass.id];

            assert(getResInteractNum(m_writeImages, pass) == passMeta.writeImageNum);
            assert(getResInteractNum(m_readImages, pass) == passMeta.readImageNum);
            assert(getResInteractNum(m_writeBuffers, pass) == passMeta.writeBufferNum);
            assert(getResInteractNum(m_readBuffers, pass) == passMeta.readBufferNum);

            // frame graph data
            MagicTag magic{ MagicTag::register_pass };
            bx::write(m_fgMemWriter, magic, nullptr);

            bx::write(m_fgMemWriter, passMeta, nullptr);

            if (0 != passMeta.vertexBindingNum)
            {
                bx::write(m_fgMemWriter, passMeta.vertexBindings, sizeof(VertexBindingDesc) * passMeta.vertexBindingNum, nullptr);
            }
            if (0 != passMeta.vertexAttributeNum)
            {
                bx::write(m_fgMemWriter, passMeta.vertexAttributes, sizeof(VertexAttributeDesc) * passMeta.vertexAttributeNum, nullptr);
            }
            if (0 != passMeta.pipelineSpecNum)
            {
                bx::write(m_fgMemWriter, passMeta.pipelineSpecData, sizeof(int) * passMeta.pipelineSpecNum, nullptr);
            }

            // write image
            if (0 != passMeta.writeImageNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_writeImages.getIdToData(pass).data()
                    , (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_writeImages, pass))
                    , nullptr
                );
            }
            // read image
            if (0 != passMeta.readImageNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_readImages.getIdToData(pass).data()
                    , (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_readImages, pass))
                    , nullptr
                );
            }
            // write buffer
            if (0 != passMeta.writeBufferNum)
            {
                bx::write(
                    m_fgMemWriter, 
                    (const void*)m_writeBuffers.getIdToData(pass).data(), 
                    (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_writeBuffers, pass))
                    , nullptr
                );
            }
            // read buffer
            if (0 != passMeta.readBufferNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_readBuffers.getIdToData(pass).data()
                    , (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_readBuffers, pass))
                    , nullptr
                );
            }

            // write res forced alias
            if (0 != passMeta.writeImgAliasNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_writeForcedImageAliases.getIdToData(pass).data()
                    , (uint32_t)(sizeof(WriteOperationAlias) * passMeta.writeImgAliasNum)
                    , nullptr
                );
            }

            if (0 != passMeta.writeBufAliasNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_writeForcedBufferAliases.getIdToData(pass).data()
                    , (uint32_t)(sizeof(WriteOperationAlias) * passMeta.writeBufAliasNum)
                    , nullptr
                );
            }

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeBufferData()
    {
        uint16_t bufCount = m_bufferHandles.getNumHandles();

        for (uint16_t ii = 0; ii < bufCount; ++ii)
        {
            BufferHandle buf { (m_bufferHandles.getHandleAt(ii)) };
            const BufferMetaData& meta = m_bufferMetas[buf.id];

            // frame graph data
            MagicTag magic{ MagicTag::register_buffer };
            bx::write(m_fgMemWriter, magic, nullptr);

            FGBufferCreateInfo info;
            info.bufId = meta.bufId;
            info.size = meta.size;
            info.pData = meta.pData;
            info.usage = meta.usage;
            info.format = meta.format;
            info.memFlags = meta.memFlags;
            info.lifetime = meta.lifetime;

            info.initialState.access = AccessFlagBits::none;
            info.initialState.layout = ImageLayout::undefined;
            info.initialState.stage = PipelineStageFlagBits::none;

            bx::write(m_fgMemWriter, info, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeImageData()
    {
        uint16_t imgCount = m_imageHandles.getNumHandles();

        for (uint16_t ii = 0; ii < imgCount; ++ii)
        {
            uint16_t imgId = m_imageHandles.getHandleAt(ii);
            const ImageMetaData& meta = m_imageMetas[imgId];

            // frame graph data
            MagicTag magic{ MagicTag::register_image };
            bx::write(m_fgMemWriter, magic, nullptr);

            FGImageCreateInfo info;
            info.imgId = meta.imgId;
            info.width = meta.width;
            info.height = meta.height;
            info.depth = meta.depth;
            info.numMips = meta.numMips;
            info.size = meta.size;
            info.pData = meta.pData;

            info.type = meta.type;
            info.viewType = meta.viewType;
            info.format = meta.format;
            info.layout = meta.layout;
            info.numLayers = meta.numLayers;
            info.usage = meta.usage;
            info.lifetime = meta.lifetime;
            info.bpp = meta.bpp;
            info.aspectFlags = meta.aspectFlags;

            info.initialState.access = AccessFlagBits::none;
            info.initialState.layout = ImageLayout::undefined;
            info.initialState.stage = PipelineStageFlagBits::none;

            bx::write(m_fgMemWriter, info, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
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
            bx::write(m_fgMemWriter, magic, nullptr);

            // info
            ResAliasInfo info;
            info.aliasNum = (uint16_t)aliasVec.size();
            info.resBase = buf.id;
            bx::write(m_fgMemWriter, info, nullptr);

            // actual alias indexes
            bx::write(m_fgMemWriter, aliasVec.data(), uint32_t(sizeof(uint16_t) * aliasVec.size()), nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
            
        }

        // images
        for (uint16_t ii = 0; ii < m_aliasImages.size(); ++ii)
        {
            ImageHandle hImg  = m_aliasImages.getIdAt(ii);
            const stl::vector<ImageHandle>& aliasVec = m_aliasImages.getIdToData(hImg);

            if (aliasVec.empty())
            {
                continue;
            }


            const ImageMetaData& meta = m_imageMetas[hImg.id];
            MagicTag magic{ MagicTag::force_alias_image };
            bx::write(m_fgMemWriter, magic, nullptr);

            // info
            ResAliasInfo info;
            info.aliasNum = (uint16_t)aliasVec.size();
            info.resBase = hImg.id;
            bx::write(m_fgMemWriter, info, nullptr);

            // actual alias indexes
            bx::write(m_fgMemWriter, aliasVec.data(), uint32_t(sizeof(uint16_t) * aliasVec.size()), nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeSamplerData()
    {
        for (uint16_t ii = 0; ii < m_samplerHandles.getNumHandles(); ++ii )
        {
            uint16_t samplerId = m_samplerHandles.getHandleAt(ii);

            const SamplerDesc& desc = m_samplerDescs[samplerId];

            // magic tag
            bx::write(m_fgMemWriter, MagicTag::register_sampler, nullptr);

            SamplerMetaData meta{ desc };
            meta.samplerId = samplerId;

            bx::write(m_fgMemWriter, meta, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeBindlessData()
    {
        for (uint16_t ii = 0; ii < m_bindlessResHandles.getNumHandles(); ++ii)
        {
            uint16_t bid = m_bindlessResHandles.getHandleAt(ii);
            const BindlessMetaData& meta = m_bindlessResMetas[bid];
            // magic tag
            bx::write(m_fgMemWriter, MagicTag::register_bindless, nullptr);
            bx::write(m_fgMemWriter, meta, nullptr);
            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    BufferHandle Context::aliasBuffer(const BufferHandle _baseBuf)
    {
        uint16_t aliasId = m_bufferHandles.alloc();
        BufferHandle aliasHandle{ aliasId };

        auto actualBaseMap = m_bufferAliasMapToBase.find(_baseBuf.id);

        BufferHandle actualBase = _baseBuf;
        // check if the base buffer is an alias
        if (m_bufferAliasMapToBase.end() != actualBaseMap)
        {
            actualBase = actualBaseMap->second;
        }

        m_bufferAliasMapToBase.insert({ aliasId, actualBase });

        if (m_aliasBuffers.exist(actualBase))
        {
            stl::vector<BufferHandle>& aliasVecRef = m_aliasBuffers.getDataRef(actualBase);
            aliasVecRef.emplace_back(aliasHandle);
        }
        else 
        {
            stl::vector<BufferHandle> aliasVec{};
            aliasVec.emplace_back(aliasHandle);

            m_aliasBuffers.addOrUpdate({ actualBase }, aliasVec);
        }

        BufferMetaData meta = m_bufferMetas[actualBase];
        meta.bufId = aliasId;
        m_bufferMetas[aliasId] = meta;

        bx::StringView baseName = getName(actualBase);
        setName(aliasHandle, baseName.getPtr(), baseName.getLength(), true);

        setRenderGraphDataDirty();

        m_cmdQueue.cmdAliasBuffer(aliasHandle, actualBase);

        return { aliasId };
    }

    ImageHandle Context::aliasImage(const ImageHandle _baseImg)
    {
        uint16_t aliasId = m_imageHandles.alloc();
        ImageHandle aliasHandle{ aliasId };

        auto actualBaseMap = m_imageAliasMapToBase.find(_baseImg.id);

        ImageHandle actualBase = _baseImg;
        // check if the base buffer is an alias
        if (m_imageAliasMapToBase.end() != actualBaseMap)
        {
            actualBase = actualBaseMap->second;
        }

        m_imageAliasMapToBase.insert({ {aliasId}, actualBase });

        if (m_aliasImages.exist(actualBase))
        {
            stl::vector<ImageHandle>& aliasVecRef = m_aliasImages.getDataRef(actualBase);
            aliasVecRef.emplace_back(aliasHandle);
        }
        else
        {
            stl::vector<ImageHandle> aliasVec{};
            aliasVec.emplace_back(aliasHandle);

            m_aliasImages.addOrUpdate({ actualBase }, aliasVec);
        }

        ImageMetaData meta = m_imageMetas[actualBase.id];
        meta.imgId = aliasId;
        m_imageMetas[aliasId] = meta;

        bx::StringView baseName = getName(actualBase);
        setName(aliasHandle, baseName.getPtr(), baseName.getLength(), true);

        setRenderGraphDataDirty();

        m_cmdQueue.cmdAliasImage(aliasHandle, actualBase);

        return { aliasId };
    }

    void Context::setName(Handle _h, const char* _name, const size_t _len, bool _alias /*= false*/)
    {
        m_nameManager->setName( _name, (uint32_t)_len, _h, _alias);

        bx::StringView name = getName(_h);

        // set to specific *meta
        Handle::Enum type = _h.getType();
        switch (type)
        {
        case kage::Handle::Shader:
            break;
        case kage::Handle::Program:
            break;
        case kage::Handle::Pass:
            m_passMetas[_h.id].name = name.getPtr();
            break;
        case kage::Handle::Buffer:
            m_bufferMetas[_h.id].name = name.getPtr();
            break;
        case kage::Handle::Image:
            m_imageMetas[_h.id].name = name.getPtr();
            break;
        case kage::Handle::Count:
            break;
        default:
            break;
        }


        int32_t len = (int32_t)name.getLength();
        m_cmdQueue.cmdSetName(_h, name.getPtr(), len);
    }

    const bx::StringView Context::getName(Handle _h)
    {
        return m_nameManager->getName(_h);
    }

    uint32_t availableBinding(ContinuousMap<PassHandle, stl::vector<uint32_t>>& _container, const PassHandle _hPass, const uint32_t _binding)
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
            _container.addOrUpdate({ _hPass }, bindingVec);
        }

        return !conflict;
    }

    uint32_t insertResInteract(ContinuousMap<PassHandle, stl::vector<PassResInteract>>& _container
        , const PassHandle _hPass, const uint16_t _resId, const ResInteractDesc& _interact
    )
    {
        uint32_t vecSize = 0u;

        PassResInteract pri;
        pri.passId = _hPass.id;
        pri.resId = _resId;
        pri.interact = _interact;

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
            _container.addOrUpdate({ _hPass }, prInteractVec);

            vecSize = (uint32_t)prInteractVec.size();
        }

        return vecSize;
    }

    uint32_t insertWriteResAlias(ContinuousMap<PassHandle, stl::vector<WriteOperationAlias>>& _container, const PassHandle _hPass, const uint16_t _resIn, const uint16_t _resOut)
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
            _container.addOrUpdate({ _hPass }, aliasVec);

            vecSize = (uint32_t)aliasVec.size();
        }

        return vecSize;
    }

    void Context::bindVertexBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _vtxCount)
    {
        PassMetaData& passMeta = m_passMetas[_hPass.id];
        passMeta.vertexBufferId = _hBuf.id;

        passMeta.vertexCount = _vtxCount;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::vertex_input;
        interact.access = AccessFlagBits::vertex_attribute_read;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, interact);

        setRenderGraphDataDirty();
    }

    void Context::bindIndexBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _idxCount)
    {
        PassMetaData& passMeta = m_passMetas[_hPass.id];
        passMeta.indexBufferId = _hBuf.id;

        passMeta.indexCount = _idxCount;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::vertex_input;
        interact.access = AccessFlagBits::index_read;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, interact);

        setRenderGraphDataDirty();
    }

    void Context::setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _defaultMaxCount)
    {
        assert(isGraphics(_hPass));

        PassMetaData& passMeta = m_passMetas[_hPass.id];
        passMeta.indirectBufferId = _hBuf.id;
        passMeta.indirectBufOffset = _offset;
        passMeta.indirectBufStride = _stride;
        passMeta.indirectMaxDrawCount = _defaultMaxCount;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::draw_indirect;
        interact.access = AccessFlagBits::indirect_command_read;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, interact);

        setRenderGraphDataDirty();
    }

    void Context::setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset)
    {
        PassMetaData& passMeta = m_passMetas[_hPass.id];
        passMeta.indirectCountBufferId = _hBuf.id;
        passMeta.indirectCountBufOffset = _offset;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::draw_indirect;
        interact.access = AccessFlagBits::indirect_command_read;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, interact);

        setRenderGraphDataDirty();
    }

    void Context::bindBuffer(PassHandle _hPass, BufferHandle _buf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias)
    {
        if (!availableBinding(m_usedBindPoints, _hPass, _binding))
        {
            message(DebugMsgType::error, "trying to binding the same point: %d", _binding);
            return;
        }

        ResInteractDesc interact{};
        interact.binding = _binding;
        interact.stage = _stage;
        interact.access = _access;

        PassMetaData& passMeta = m_passMetas[_hPass.id];
        if (isRead(_access))
        {
            passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _buf.id, interact);
            setRenderGraphDataDirty();
        }

        if (isWrite(_access, _stage))
        {
            if (!isValid(_outAlias))
            {
                message(DebugMsgType::error, "the _outAlias must be valid if trying to write to resource in pass");
                return;
            }

            passMeta.writeBufferNum = insertResInteract(m_writeBuffers, _hPass, _buf.id, interact);
            passMeta.writeBufAliasNum = insertWriteResAlias(m_writeForcedBufferAliases, _hPass, _buf.id, _outAlias.id);

            assert(passMeta.writeBufferNum == passMeta.writeBufAliasNum);
            setRenderGraphDataDirty();
        }
    }

    kage::SamplerHandle Context::sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, SamplerFilter _filter, SamplerMipmapMode _mipMode, SamplerAddressMode _addrMode, SamplerReductionMode _reductionMode)
    {
        if (!availableBinding(m_usedBindPoints, _hPass, _binding))
        {
            message(DebugMsgType::error, "trying to binding the same point: %d", _binding);
            return { kInvalidHandle };
        }

        // get the sampler id
        SamplerDesc desc;
        desc.filter = _filter;
        desc.mipmapMode = _mipMode;
        desc.addressMode = _addrMode;
        desc.reductionMode = _reductionMode;

        SamplerHandle sampler = requestSampler(desc);

        ResInteractDesc interact{};
        interact.binding = _binding;
        interact.stage = _stage;
        interact.access = AccessFlagBits::shader_read;
        interact.layout = ImageLayout::shader_read_only_optimal;

        PassMetaData& passMeta = m_passMetas[_hPass.id];
        passMeta.readImageNum = insertResInteract(m_readImages, _hPass, _hImg.id, interact);
        passMeta.sampleImageNum++; // Note: is this the only way to read texture?

        setRenderGraphDataDirty();

        return sampler;
    }

    void Context::setBindlessTextures(BindlessHandle _bindless, const Memory* _mem, uint32_t _texCount, SamplerReductionMode _reductionMode)
    {
        if (!isValid(_bindless))
        {
            message(DebugMsgType::error, "invalid bindless handle");
            return;
        }

        assert(_texCount == _mem->size / sizeof(ImageHandle));

        BindlessMetaData& meta = m_bindlessResMetas[_bindless.id];
        meta.resCount = _texCount;
        meta.reductionMode = _reductionMode;
        meta.resIdMem = _mem;

        stl::vector<ImageHandle> texIds(_texCount);
        bx::memCopy(texIds.data(), _mem->data, sizeof(ImageHandle) * _texCount);
        for (ImageHandle id : texIds)
        {
            if (!isValid(id))
            {
                message(DebugMsgType::error, "invalid image handle in bindless texture");
                continue;
            }

            // set the lifetime to not alise it in framegraph
            m_imageMetas[id].lifetime = ResourceLifetime::non_transition;
        }

        setRenderGraphDataDirty();
    }

    void Context::bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
        , const ImageHandle _outAlias)
    {
        if (!availableBinding(m_usedBindPoints, _hPass, _binding))
        {
            message(DebugMsgType::error, "trying to binding the same point: %d", _binding);
            return;
        }

        const ImageMetaData& imgMeta = m_imageMetas[_hImg.id];
        PassMetaData& passMeta = m_passMetas[_hPass.id];

        ResInteractDesc interact{};
        interact.binding = _binding; 
        interact.stage = _stage;
        interact.access = _access;
        interact.layout = _layout;


        if (isRead(_access))
        {
            if (isGraphics(_hPass))
            {
                if (isDepthStencil(_hImg))
                {
                    assert((imgMeta.aspectFlags & ImageAspectFlagBits::depth) != 0);
                    interact.access |= AccessFlagBits::depth_stencil_attachment_read;
                }
                else if (isColorAttachment(_hImg))
                {
                    assert((imgMeta.aspectFlags & ImageAspectFlagBits::color) != 0);
                    interact.access |= AccessFlagBits::color_attachment_read;
                }
            }

            passMeta.readImageNum = insertResInteract(m_readImages, _hPass, _hImg.id, interact);
            setRenderGraphDataDirty();
        }

        if (isWrite(_access, _stage))
        {
            if (!isValid(_outAlias))
            {
                message(DebugMsgType::error, "the _outAlias must be valid if trying to write to resource in pass");
            }

            if (isCompute(_hPass) || isGraphics(_hPass))
            {
                passMeta.writeImageNum = insertResInteract(m_writeImages, _hPass, _hImg.id, interact);
                passMeta.writeImgAliasNum = insertWriteResAlias(m_writeForcedImageAliases, _hPass, _hImg.id, _outAlias.id);

                assert(passMeta.writeImageNum == passMeta.writeImgAliasNum);

                setRenderGraphDataDirty();
            }
            else
            {
                message(DebugMsgType::error, "unsupported pass type!!!!");
                assert(0);
            }
        }
    }

    void Context::setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias)
    {
        if (isColorAttachment(_hImg) && !availableBinding(m_usedAttchBindPoints, _hPass, _attachmentIdx))
        {
            message(DebugMsgType::error, "trying to binding the same point: %d", _attachmentIdx);
            return;
        }

        assert(isGraphics(_hPass));
        assert(isValid(_outAlias));

        const ImageMetaData& imgMeta = m_imageMetas[_hImg.id];
        PassMetaData& passMeta = m_passMetas[_hPass.id];

        ResInteractDesc interact{};
        interact.binding = _attachmentIdx;
        

        if (isDepthStencil(_hImg))
        {
            assert((imgMeta.aspectFlags & ImageAspectFlagBits::depth) != 0);
            assert(kInvalidHandle == passMeta.writeDepthId);

            passMeta.writeDepthId = _hImg.id;
            interact.layout = ImageLayout::depth_stencil_attachment_optimal;
            interact.access = AccessFlagBits::depth_stencil_attachment_write;
            interact.stage = PipelineStageFlagBits::late_fragment_tests;
        }
        else if (isColorAttachment(_hImg))
        {
            assert((imgMeta.aspectFlags & ImageAspectFlagBits::color) != 0);
            interact.layout = ImageLayout::color_attachment_optimal;
            interact.access = AccessFlagBits::color_attachment_write;
            interact.stage = PipelineStageFlagBits::color_attachment_output;
        }
        else
        {
            assert(0);
        }

        passMeta.writeImageNum = insertResInteract(m_writeImages, _hPass, _hImg.id, interact);
        passMeta.writeImgAliasNum = insertWriteResAlias(m_writeForcedImageAliases, _hPass, _hImg.id, _outAlias.id);
        assert(passMeta.writeImageNum == passMeta.writeImgAliasNum);

        setRenderGraphDataDirty();
    }

    void Context::setMultiFrame(ImageHandle _hImg)
    {
        ImageMetaData& meta = m_imageMetas[_hImg.id];
        meta.lifetime = ResourceLifetime::non_transition;

        setRenderGraphDataDirty();
    }

    void Context::setMultiFrame(BufferHandle _hBuf)
    {
        BufferMetaData& meta = m_bufferMetas[_hBuf.id];
        meta.lifetime = ResourceLifetime::non_transition;

        setRenderGraphDataDirty();
    }

    void Context::setPresentImage(ImageHandle _rt, uint32_t _mipLv)
    {
        m_presentImage = _rt;
        m_presentMipLevel = _mipLv;

        setRenderGraphDataDirty();
    }

    void Context::updateBuffer(const BufferHandle _hBuf, const Memory* _mem, const uint32_t _offset, const uint32_t _size)
    {
        if (_mem == nullptr)
        {
            return;
        }

        m_cmdQueue.cmdUpdateBuffer(_hBuf, _mem, _offset, _size);
    }

    void Context::updateImage(
        const ImageHandle _hImg
        , uint32_t _width
        , uint32_t _height
        , const Memory* _mem
    )
    {
        m_cmdQueue.cmdUpdateImage(_hImg, _width, _height, _mem);
    }

    void Context::rendererExecCmdQ(CommandQueue& _cmdQ)
    {
        _cmdQ.reset();

        const Command* cmd;
        do
        {
            cmd = _cmdQ.poll();

            if (nullptr != cmd)
            {
                switch (cmd->m_cmd)
                {
                case Command::create_pass:
                    {
                        const CreatePassCmd* cpc = reinterpret_cast<const CreatePassCmd*>(cmd);
                    }
                    break;
                case Command::create_image:
                    {
                        const CreateImageCmd* cic = reinterpret_cast<const CreateImageCmd*>(cmd);
                    }
                    break;
                case Command::create_buffer:
                    {
                        const CreateBufferCmd* cbc = reinterpret_cast<const CreateBufferCmd*>(cmd);
                    }
                    break;
                case Command::create_program:
                    {
                        const CreateProgramCmd* cpc = reinterpret_cast<const CreateProgramCmd*>(cmd);
                    }
                    break;
                case Command::create_shader:
                    {
                        const CreateShaderCmd* csc = reinterpret_cast<const CreateShaderCmd*>(cmd);
                    }
                    break;
                case Command::create_sampler:
                    {
                        const CreateSamplerCmd* csc = reinterpret_cast<const CreateSamplerCmd*>(cmd);
                    }
                    break;
                case Command::create_bindless:
                    {
                        const CreateBindlessResCmd* bvc = reinterpret_cast<const CreateBindlessResCmd*>(cmd);
                    }
                case Command::alias_image:
                    {
                        const AliasImageCmd* aic = reinterpret_cast<const AliasImageCmd*>(cmd);
                    }
                    break;
                case Command::alias_buffer:
                    {
                        const AliasBufferCmd* abc = reinterpret_cast<const AliasBufferCmd*>(cmd);
                    }
                    break;
                case Command::update_image:
                    {
                        const UpdateImageCmd* uic = reinterpret_cast<const UpdateImageCmd*>(cmd);
                        m_rhiContext->updateImage(
                            uic->m_handle
                            , uic->m_width
                            , uic->m_height
                            , uic->m_mem
                        );
                    }
                    break;
                case Command::update_buffer:
                    {
                        const UpdateBufferCmd* ubc = reinterpret_cast<const UpdateBufferCmd*>(cmd);
                        m_rhiContext->updateBuffer(
                            ubc->m_handle
                            , ubc->m_mem
                            , ubc->m_offset
                            , ubc->m_size
                        );
                        release(ubc->m_mem);
                    }
                    break;
                case Command::set_name:
                    {
                        const SetNameCmd* snc = reinterpret_cast<const SetNameCmd*>(cmd);
                        m_rhiContext->setName(snc->m_handle, snc->m_name, snc->m_len);
                    }
                    break;
                case Command::record_start:
                    {
                        const RecordStartCmd* rsc = reinterpret_cast<const RecordStartCmd*>(cmd);
                        
                        BX_ASSERT(isValid(rsc->m_pass), "recoreded pass is not valid!");
                        const RecordingCmd rq = m_recCmds[rsc->m_pass.id];

                        m_rhiContext->setRecord(rsc->m_pass, _cmdQ, rq.startIdx, rq.count);
                    }
                    break;
                case Command::end:
                    {
                    }
                    break;
                default:
                    {
                    }
                    break;
                }
            }

        } while (cmd != nullptr);
    }

    namespace vk
    {
        extern RHIContext* rendererCreate(const Resolution& _config, void* _wnd);
        extern void rendererDestroy();
    }

    void Context::init(const Init& _init)
    {
        m_pAllocator = getAllocator();

        m_resolution = _init.resolution;
        m_nativeWnd = _init.windowHandle;

        m_rhiContext = vk::rendererCreate(m_resolution, m_nativeWnd);

        m_cmdQueue.init(_init.minCmdBufSize);
        
        m_cmdQueue.start();

        // TODO: remove one of them in the future
        m_frameGraph = BX_NEW(getAllocator(), Framegraph)(getAllocator(), m_rhiContext->memoryBlock());

        m_pFgMemBlock = m_frameGraph->getMemoryBlock();
        m_fgMemWriter = BX_NEW(getAllocator(), bx::MemoryWriter)(m_pFgMemBlock);

        m_nameManager = BX_NEW(getAllocator(), NameMgr)();

        m_transientMemories.clear();

        setRenderGraphDataDirty();
    }

    void Context::render()
    {
        KG_ZoneScopedC(Color::cyan);

        m_cmdQueue.finish();

        if (kInvalidHandle == m_presentImage.id)
        {
            message(error, "result render target is not set!");
            return;
        }

        if (isRenderGraphDataDirty())
        {
            bake();
        }

        rhi_render();

        // free transient memories
        for (const Memory* pMem : m_transientMemories)
        {
            release(pMem);
        }
        m_transientMemories.clear();

        m_cmdQueue.start();
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
        storeBindlessData();
        storePassData();
        storeAliasData();

        // write the finish tag
        MagicTag magic{ MagicTag::end };
        bx::write(m_fgMemWriter, magic, nullptr);

        m_frameGraph->setMemoryBlock(m_pFgMemBlock);
        m_frameGraph->bake();

        m_isRenderGraphDataDirty = false;

        m_rhiContext->bake();
    }

    void Context::reset(uint32_t _windth, uint32_t _height, uint32_t _reset)
    {
        message(DebugMsgType::essential, "reset!");

        m_resolution.width = _windth;
        m_resolution.height = _height;
        m_resolution.reset = _reset;

        m_rhiContext->updateResolution(m_resolution);
    }

    void Context::rhi_render()
    {
        message(DebugMsgType::essential, "start rendering");

        rendererExecCmdQ(m_cmdQueue);
        m_rhiContext->run();

        // TODO: add the post command queue to release resources
        // rendererExecCmdQ(m_cmdPostQ)

        message(DebugMsgType::essential, "finish rendering");
    }

    void Context::shutdown()
    {
        m_cmdQueue.finish();

        bx::deleteObject(m_pAllocator, m_frameGraph);
        vk::rendererDestroy();
        bx::deleteObject(m_pAllocator, m_fgMemWriter);
        bx::deleteObject(m_pAllocator, m_nameManager);

        // free memories
        for (const Memory* pMem : m_inputMemories)
        {
            release(pMem);
        }

        m_pFgMemBlock = nullptr;
        m_pAllocator = nullptr;
    }

    double Context::getPassTime(const PassHandle _hPass)
    {
        return m_rhiContext->getPassTime(_hPass);
    }

    uint64_t Context::getPassClipping(const PassHandle _hPass)
    {
        return m_rhiContext->getPassClipping(_hPass);
    }

    double Context::getGpuTime()
    {
        return m_rhiContext->getGPUTime();
    }

    void Context::startRec(const PassHandle _hPass)
    {
        if (isValid(m_recordingPass))
        {
            message(DebugMsgType::error
                , "pass [%d:%s] already started!"
                , m_recordingPass.id
                , getName(m_recordingPass).getPtr()
            );
            return;
        }

        m_recordingPass = _hPass;

        message(info, "start rec pass %s", getName(m_recordingPass).getPtr());

        m_cmdQueue.cmdRecordStart(_hPass);

        RecordingCmd& rq = m_recCmds[_hPass];
        rq.startIdx = m_cmdQueue.getIdx();
        rq.endIdx = 0;
        rq.count = 0;
    }

    void Context::setConstants(const Memory* _mem)
    {
        m_cmdQueue.cmdRecordSetConstants(_mem);

        m_transientMemories.push_back(_mem);
    }

    void Context::pushBindings(const Binding* _desc, uint16_t _count)
    {
        const uint32_t sz = sizeof(Binding) * _count;
        const Memory* mem = alloc(sz);
        bx::memCopy(mem->data, _desc, mem->size);

        m_cmdQueue.cmdRecordPushDescriptorSet(mem);

        m_transientMemories.push_back(mem);
    }

    void Context::setColorAttachments(const Attachment* _colors, uint16_t _count)
    {
        const uint32_t sz = sizeof(Attachment) * _count;
        const Memory* mem = alloc(sz);
        bx::memCopy(mem->data, _colors, mem->size);

        m_cmdQueue.cmdRecordSetColorAttachments(mem);

        m_transientMemories.push_back(mem);
    }

    void Context::setDepthAttachment(const Attachment _depth)
    {
        m_cmdQueue.cmdRecordSetDepthAttachment(_depth);
    }

    void Context::setBindless(BindlessHandle _hBindless)
    {
        m_cmdQueue.cmdRecordSetBindless(_hBindless);
    }

    void Context::setBuffer(const BufferHandle _hBuf, const uint32_t _binding, const PipelineStageFlags _stage, const AccessFlags _access, const BufferHandle _outAlias)
    {
        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
    }

    void Context::setImage(const ImageHandle _hImg, const uint32_t _binding, const PipelineStageFlags _stage, const AccessFlags _access, const ImageLayout _layout, const ImageHandle _outAlias)
    {
        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
    }

    void Context::dispatch(const uint32_t _groupCountX, const uint32_t _groupCountY, const uint32_t _groupCountZ)
    {
        m_cmdQueue.cmdRecordDispatch(_groupCountX, _groupCountY, _groupCountZ);
    }

    void Context::setVertexBuffer(BufferHandle _hBuf)
    {
        m_cmdQueue.cmdRecordSetVertexBuffer(_hBuf);
    }

    void Context::setIndexBuffer(BufferHandle _hBuf, uint32_t _offset, IndexType _type)
    {
        m_cmdQueue.cmdRecordSetIndexBuffer(_hBuf, _offset, _type);
    }

    void Context::setViewport(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height)
    {
        m_cmdQueue.cmdRecordSetViewport(_x, _y, _width, _height);
    }


    void Context::setScissor(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height)
    {
        m_cmdQueue.cmdRecordSetScissor(_x, _y, _width, _height);
    }

    void Context::fillBuffer(const BufferHandle _hBuf, const uint32_t _value)
    {
        m_cmdQueue.cmdRecordFillBuffer(_hBuf, _value);
    }

    void Context::draw(const uint32_t _vertexCount, const uint32_t _instanceCount, const uint32_t _firstVertex, const uint32_t _firstInstance)
    {
        m_cmdQueue.cmdRecordDraw(_vertexCount, _instanceCount, _firstVertex, _firstInstance);
    }

    void Context::drawIndirect( const BufferHandle _hIndirectBuf, const uint32_t _offset, const uint32_t _count, const uint32_t _stride)
    {
        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
    }

    void Context::drawIndirect(const uint32_t _offset, const BufferHandle _countBuf, const uint32_t _countOffset, const uint32_t _maxCount, const uint32_t _stride)
    {
        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
    }

    void Context::drawIndexed(const uint32_t _indexCount, const uint32_t _instanceCount, const uint32_t _firstIndex, const int32_t _vertexOffset, const uint32_t _firstInstance)
    {
        m_cmdQueue.cmdRecordDrawIndexed(_indexCount, _instanceCount, _firstIndex, _vertexOffset, _firstInstance);
    }

    void Context::drawIndexed(const BufferHandle _hIndirectBuf, const uint32_t _offset, const uint32_t _count, const uint32_t _stride)
    {
        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
    }

    void Context::drawIndexed(const BufferHandle _hIndirectBuf, const uint32_t _offset, const BufferHandle _countBuf, const uint32_t _countOffset, const uint32_t _maxCount, const uint32_t _stride)
    {
        m_cmdQueue.cmdRecordDrawIndexedIndirectCount(_hIndirectBuf, _offset, _countBuf, _countOffset, _maxCount, _stride);
    }

    void Context::drawMeshTask(const uint32_t _groupCountX, const uint32_t _groupCountY, const uint32_t _groupCountZ)
    {
        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
    }

    void Context::drawMeshTask(const BufferHandle _hIndirectBuf, const uint32_t _offset, const uint32_t _count, const uint32_t _stride)
    {
        m_cmdQueue.cmdRecordDrawMeshTaskIndirect(_hIndirectBuf, _offset, _count, _stride);
    }

    void Context::drawMeshTask(const BufferHandle _hIndirectBuf, const uint32_t _offset, const BufferHandle _countBuf, const uint32_t _countOffset, const uint32_t _maxCount, const uint32_t _stride)
    {
        BX_ASSERT(0, "NOT IMPLEMENTED YET!!!");
    }

    void Context::endRec()
    {
        if (!isValid(m_recordingPass))
        {
            message(DebugMsgType::error, "no pass is start yet, why ending?");
            return;
        }

        m_cmdQueue.cmdRecordEnd();

        RecordingCmd& rq = m_recCmds[m_recordingPass];
        rq.endIdx = m_cmdQueue.getIdx();
        rq.count = rq.endIdx - rq.startIdx;

        m_recordingPass = { kInvalidHandle };
    }

    // ================================================
    static Context* s_ctx = nullptr;

    bool checkSupports(VulkanSupportExtension _ext)
    {
        return s_ctx->checkSupports(_ext);
    }

    ShaderHandle registShader(const char* _name, const char* _path)
    {
        return s_ctx->registShader(_name, _path);
    }

    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants /*= 0*/, BindlessHandle _bindless)
    {
        const uint16_t shaderNum = static_cast<const uint16_t>(_shaders.size());
        const Memory* mem = alloc(shaderNum * sizeof(uint16_t));

        bx::StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < shaderNum; ++ii)
        {
            const ShaderHandle& handle = begin(_shaders)[ii];
            bx::write(&writer, handle.id, nullptr);
        }

        return s_ctx->registProgram(_name, mem, shaderNum, _sizePushConstants, _bindless);
    }

    BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const Memory* _mem /* = nullptr */, const ResourceLifetime _lifetime /*= ResourceLifetime::transision*/)
    {
        return s_ctx->registBuffer(_name, _desc, _mem, _lifetime);
    }

    ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const Memory* _mem /* = nullptr */, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registTexture(_name, _desc, _mem, _lifetime);
    }

    ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registRenderTarget(_name, _desc, _lifetime);
    }

    ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registDepthStencil(_name, _desc, _lifetime);
    }

    kage::BindlessHandle registBindless(const char* _name, const BindlessDesc& _desc)
    {
        return s_ctx->registBindless(_name, _desc, ResourceLifetime::non_transition);
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

    void bindVertexBuffer(PassHandle _hPass, BufferHandle _hBuf, const uint32_t _vtxCount /*= 0*/)
    {
        s_ctx->bindVertexBuffer(_hPass, _hBuf, _vtxCount);
    }

    void bindIndexBuffer(PassHandle _hPass, BufferHandle _hBuf, const uint32_t _idxCount /*= 0*/)
    {
        s_ctx->bindIndexBuffer(_hPass, _hBuf, _idxCount);
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

    kage::SamplerHandle sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, SamplerFilter _filter, SamplerMipmapMode _mipmapMode, SamplerAddressMode _addrMode, SamplerReductionMode _reductionMode)
    {
        return s_ctx->sampleImage(_hPass, _hImg, _binding, _stage, _filter, _mipmapMode, _addrMode, _reductionMode);
    }

    void setBindlessTextures(BindlessHandle _bindless, const Memory* _mem, uint32_t _texCount, SamplerReductionMode _reductionMode)
    {
        return s_ctx->setBindlessTextures(_bindless, _mem, _texCount, _reductionMode);
    }


    void bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
        , const ImageHandle _outAlias /*= {kInvalidHandle}*/)
    {
        s_ctx->bindImage(_hPass, _hImg, _binding, _stage, _access, _layout, _outAlias);
    }

    void setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias)
    {
        s_ctx->setAttachmentOutput(_hPass, _hImg, _attachmentIdx, _outAlias);
    }

    void setPresentImage(ImageHandle _rt, uint32_t _mipLv /*= 0*/)
    {
        s_ctx->setPresentImage(_rt, _mipLv);
    }
    
    void updateBuffer(
        const BufferHandle _hBuf
        , const Memory* _mem
        , uint32_t _offset /*= 0*/
        , uint32_t _size /*= 0*/
    )
    {
        const uint32_t offset =
            (_offset == 0)
            ? 0
            : _offset;

        const uint32_t size =
            (_size == 0)
            ? _mem->size
            : bx::clamp(_size, 0, _mem->size);

        s_ctx->updateBuffer(_hBuf, _mem, offset, size);
    }

    void updateImage2D(
        const ImageHandle _hImg
        , uint32_t _width
        , uint32_t _height
        , const Memory* _mem /*= nullptr */
    )
    {
        s_ctx->updateImage(_hImg, _width, _height, _mem);
    }

    void startRec(const PassHandle _hPass)
    {
        s_ctx->startRec(_hPass);
    }

    void setConstants( const Memory* _mem )
    {
        s_ctx->setConstants(_mem);
    }

    void pushBindings( const Binding* _desc , uint16_t _count )
    {
        s_ctx->pushBindings(_desc, _count);
    }

    void setColorAttachments(const Attachment* _colors, uint16_t _count)
    {
        s_ctx->setColorAttachments(_colors, _count);
    }

    void setDepthAttachment(Attachment _depth)
    {
        s_ctx->setDepthAttachment(_depth);
    }

    void setBindless(BindlessHandle _hBindless)
    {
        s_ctx->setBindless(_hBindless);
    }

    void dispatch(
        const uint32_t _groupCountX
        , const uint32_t _groupCountY
        , const uint32_t _groupCountZ
    )
    {
        s_ctx->dispatch(_groupCountX, _groupCountY, _groupCountZ);
    }


    void fillBuffer(
        BufferHandle _hBuf
        , uint32_t _value
    )
    {
        s_ctx->fillBuffer(_hBuf, _value);
    }

    void setVertexBuffer(
        BufferHandle _hBuf
    )
    {
        s_ctx->setVertexBuffer(_hBuf);
    }

    void setIndexBuffer(
        BufferHandle _hBuf
        , uint32_t _offset
        , IndexType _type
    )
    {
        s_ctx->setIndexBuffer(_hBuf, _offset, _type);
    }

    void setViewport(
        int32_t _x
        , int32_t _y
        , uint32_t _width
        , uint32_t _height
    )
    {
        s_ctx->setViewport(_x, _y, _width, _height);
    }


    void setScissor(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height)
    {
        s_ctx->setScissor(_x, _y, _width, _height);
    }

    void draw(
        const uint32_t _vertexCount
        , const uint32_t _instanceCount
        , const uint32_t _firstVertex
        , const uint32_t _firstInstance
    )
    {
        s_ctx->draw(_vertexCount, _instanceCount, _firstVertex, _firstInstance);
    }

    // draw indirect
    void drawIndirect(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const uint32_t _count
        , const uint32_t _stride
    )
    {
        s_ctx->drawIndirect(_hIndirectBuf, _offset, _count, _stride);
    }

    // draw indirect count
    void drawIndirect(
        const uint32_t _offset
        , const BufferHandle _countBuf
        , const uint32_t _countOffset
        , const uint32_t _maxCount
        , const uint32_t _stride
    )
    {
        s_ctx->drawIndirect(_offset, _countBuf, _countOffset, _maxCount, _stride);
    }

    void drawIndexed(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const uint32_t _count
        , const uint32_t _stride
    )
    {
        s_ctx->drawIndexed(_hIndirectBuf, _offset, _count, _stride);
    }

    void drawIndexed(
        const uint32_t _indexCount
        , const uint32_t _instanceCount
        , const uint32_t _firstIndex
        , const int32_t _vertexOffset
        , const uint32_t _firstInstance
    )
    {
        s_ctx->drawIndexed(_indexCount, _instanceCount, _firstIndex, _vertexOffset, _firstInstance);
    }

    void drawIndexed(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const BufferHandle _countBuf
        , const uint32_t _countOffset
        , const uint32_t _maxCount
        , const uint32_t _stride
    )
    {
        s_ctx->drawIndexed(_hIndirectBuf, _offset, _countBuf, _countOffset, _maxCount, _stride);
    }

    void drawMeshTask(
        const uint32_t _groupCountX
        , const uint32_t _groupCountY
        , const uint32_t _groupCountZ
    )
    {
        s_ctx->drawMeshTask(_groupCountX, _groupCountY, _groupCountZ);
    }

    void drawMeshTask(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const uint32_t _count
        , const uint32_t _stride
    )
    {
        s_ctx->drawMeshTask(_hIndirectBuf, _offset, _count, _stride);
    }

    void drawMeshTask(
        const BufferHandle _hIndirectBuf
        , const uint32_t _offset
        , const BufferHandle _countBuf
        , const uint32_t _countOffset
        , const uint32_t _maxCount
        , const uint32_t _stride)
    {
        s_ctx->drawMeshTask(_hIndirectBuf, _offset, _countBuf, _countOffset, _maxCount, _stride);
    }

    void endRec()
    {
        s_ctx->endRec();
    }

    const char* getName(Handle _h)
    {
        return s_ctx->getName(_h).getPtr();
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
    bool init(Init _init /*= {}*/)
    {
        s_ctx = BX_NEW(getAllocator(), Context);
        s_ctx->init(_init);

        return true;
    }

    void reset(uint32_t _width, uint32_t _height, uint32_t _reset)
    {
        s_ctx->reset(_width, _height, _reset);
    }

    void render()
    {
        s_ctx->render();
    }

    void shutdown()
    {
        bx::deleteObject(getAllocator(), s_ctx);

        shutdownAllocator();
    }

    double getPassTime(const PassHandle _hPass)
    {
        return s_ctx->getPassTime(_hPass);
    }

    double getGpuTime()
    {
        return s_ctx->getGpuTime();
    }

    uint64_t getPassClipping(const PassHandle _hPass)
    {
        return s_ctx->getPassClipping(_hPass);
    }

} // namespace kage