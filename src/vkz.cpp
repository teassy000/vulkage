#include "common.h"
#include "alloc.h"
#include "handle.h"
#include "memory_operation.h"
#include "vkz.h"

#include "config.h"
#include "name.h"

#include "framegraph_2.h"
#include <array>


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

    uint16_t getBytesPerPixel(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::Unknown:
            return 0;
        case TextureFormat::A8:
            return 1;
        case TextureFormat::R8:
            return 1;
        case TextureFormat::R8I:
            return 1;
        case TextureFormat::R8S:
            return 1;
        case TextureFormat::R8U:
            return 1;
        case TextureFormat::R16:
            return 2;
        case TextureFormat::R16I:
            return 2;
        case TextureFormat::R16U:
            return 2;
        case TextureFormat::R16F:
            return 2;
        case TextureFormat::R16S:
            return 2;
        case TextureFormat::R32:
            return 4;
        case TextureFormat::R32I:
            return 4;
        case TextureFormat::R32U:
            return 4;
        case TextureFormat::R32F:
            return 4;
        case TextureFormat::R32S:
            return 4;
        case TextureFormat::BGRA8:
            return 4;
        case TextureFormat::BGRA8I:
            return 4;
        case TextureFormat::BGRA8U:
            return 4;
        case TextureFormat::BGRA8F:
            return 4;
        case TextureFormat::BGRA8S:
            return 4;
        case TextureFormat::RGBA8:
            return 4;
        case TextureFormat::RGBA8I:
            return 4;
        case TextureFormat::RGBA8U:
            return 4;
        case TextureFormat::RGBA8F:
            return 4;
        case TextureFormat::RGBA8S:
            return 4;
        case TextureFormat::UnknownDepth:
            return 0;
        case TextureFormat::D16:
            return 2;
        case TextureFormat::D32:
            return 4;
        case TextureFormat::D16F:
            return 2;
        case TextureFormat::D32F:
            return 4;
        default:
            return 0;
        }
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
        // Context API
        ShaderHandle registShader(const char* _name, const char* _path);
        ProgramHandle registProgram(const char* _name, const Memory* _shaders, const uint16_t _shaderCount, const uint32_t _sizePushConstants = 0);

        PassHandle registPass(const char* _name, const PassDesc& _desc);

        BufferHandle registBuffer(const char* _name, BufferDesc _desc);
        TextureHandle registTexture(const char* _name, ImageDesc _desc);
        RenderTargetHandle registRenderTarget(const char* _name, ImageDesc _desc);
        DepthStencilHandle registDepthStencil(const char* _name, ImageDesc _desc);

        uint16_t aliasAlloc(MagicTag _tag);

        void aliasResrouce(const Memory* _outMem, const uint16_t _aliasCount, const uint16_t _baseRes, MagicTag _tag);
        void readwriteResource(const uint16_t _pass, const Memory* _reses, const uint16_t _resCount, MagicTag _tag);

        void setMultiFrameResource(const Memory* _mem, const uint16_t _resCount, MagicTag _tag);
        void setMutiFrameResource(const uint16_t* _reses, const uint16_t _resCount, MagicTag _tag); // do not alias atomically
        void setResultRenderTarget(RenderTargetHandle _rt);

        void init();
        void update();
        void shutdown();

        HandleArrayT<kMaxNumOfShaderHandle> m_shaderHandles;
        HandleArrayT<kMaxNumOfProgramHandle> m_programHandles;
        HandleArrayT<kMaxNumOfPipelineHandle> m_pipelineHandles;
        HandleArrayT<kMaxNumOfPassHandle> m_passHandles;
        HandleArrayT<kMaxNumOfTextureHandle> m_textureHandles;
        HandleArrayT<kMaxNumOfBufferHandle> m_bufferHandles;

        RenderTargetHandle m_resultRenderTarget{kInvalidHandle};

        // resource Info
        BufferDesc m_bufferDescs[kMaxNumOfBufferHandle];
        ImageDesc m_textureDescs[kMaxNumOfTextureHandle];
        ImageDesc m_renderTargetDescs[kMaxNumOfRenderTargetHandle];
        ImageDesc m_depthStencilDescs[kMaxNumOfDepthStencilHandle];
        PassDesc m_passDescs[kMaxNumOfPassHandle];


        // frame graph
        MemoryBlock* m_fgMemBlock;
        MemoryWriter* m_fgMemWriter;

        Framegraph2* m_frameGraph;

        AllocatorI* m_allocator = nullptr;
    };

    ShaderHandle Context::registShader(const char* _name, const char* _path)
    {
        uint16_t idx = m_shaderHandles.alloc();
        
        MagicTag magic{ MagicTag::RegisterShader };
        write(m_fgMemWriter, magic);

        int32_t len = (int32_t)strnlen_s(_path, kMaxPathLen);

        write(m_fgMemWriter, len);
        write(m_fgMemWriter, (void*)_path, len);

        return ShaderHandle{ idx };
    }

    vkz::ProgramHandle Context::registProgram(const char* _name, const Memory* _shaders, const uint16_t _shaderCount, const uint32_t _sizePushConstants /*= 0*/)
    {
        uint16_t idx = m_programHandles.alloc();

        // magic tag
        MagicTag magic{ MagicTag::RegisterProgram };
        write(m_fgMemWriter, magic);

        // push constants size
        write(m_fgMemWriter, _sizePushConstants);

        // shader indexes
        write(m_fgMemWriter, _shaderCount);
        write(m_fgMemWriter, _shaders->data, _shaders->size);

        return ProgramHandle{ idx };
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

        PassDesc& desc = m_passDescs[idx];
        desc.program = _desc.program;
        desc.queue = _desc.queue;
        desc.pipelineConfig = _desc.pipelineConfig;
        desc.vertexAttributeNum = _desc.vertexAttributeNum;
        desc.vertexBindingNum = _desc.vertexBindingNum;

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::RegisterPass);
        write(m_fgMemWriter, magic);

        PassRegisterInfo info;
        info.idx = idx;
        info.queue = _desc.queue;
        info.vtxBindingNum = _desc.vertexBindingNum;
        info.vtxAttributeNum = _desc.vertexAttributeNum;
        info.pipelineConfig = _desc.pipelineConfig;

        write(m_fgMemWriter, info);

        // TODO: iterate the vertex binding and attribute, store the index then

        

        return handle;
    }

    BufferHandle Context::registBuffer(const char* _name, BufferDesc _desc)
    {
        uint16_t idx = m_bufferHandles.alloc();

        BufferHandle handle = BufferHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create BufferHandle failed!");
            return BufferHandle{ kInvalidHandle };
        }

        BufferDesc& desc = m_bufferDescs[idx];
        desc = _desc;

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::RegisterBuffer);
        write(m_fgMemWriter, magic);

        BufRegisterInfo info;
        info.idx = idx;
        info.size = _desc.size;
        info.access = 0u; // TODO: set the barrier state
        info.layout = 0u; // TODO: set the barrier state
        info.stage = 0u;  // TODO: set the barrier state
        write(m_fgMemWriter, info);

        return handle;
    }

    TextureHandle Context::registTexture(const char* _name, ImageDesc _desc)
    {
        uint16_t idx = m_textureHandles.alloc();

        TextureHandle handle = TextureHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create BufferHandle failed!");
            return TextureHandle{ kInvalidHandle };
        }

        ImageDesc& desc = m_textureDescs[idx];
        desc = _desc;

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::RegisterTexture);
        write(m_fgMemWriter, magic);

        ImgRegisterInfo info;
        info.idx = idx;
        info.x = _desc.width;
        info.y = _desc.height;
        info.z = _desc.depth;
        info.mips = _desc.mips;
        info.format = static_cast<uint16_t>(_desc.format);
        info.usage = static_cast<uint16_t>(_desc.usage);
        info.bpp = getBytesPerPixel(_desc.format);
        
        // TODO: image type
        // TODO: image view type

        info.access = 0u; // TODO: set the barrier state
        info.layout = 0u; // TODO: set the barrier state
        info.stage = 0u;  // TODO: set the barrier state

        write(m_fgMemWriter, info);

        return handle;
    }

    RenderTargetHandle Context::registRenderTarget(const char* _name, ImageDesc _desc)
    {
        uint16_t idx = m_textureHandles.alloc();

        RenderTargetHandle handle = RenderTargetHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create RenderTargetHandle failed!");
            return RenderTargetHandle{ kInvalidHandle };
        }

        ImageDesc& desc = m_textureDescs[idx];
        desc = _desc;

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::RegisterRenderTarget);
        write(m_fgMemWriter, magic);

        ImgRegisterInfo info;
        info.idx = idx;
        info.x = _desc.width;
        info.y = _desc.height;
        info.z = _desc.depth;
        info.mips = _desc.mips;
        info.format = static_cast<uint16_t>(_desc.format);
        info.usage = static_cast<uint16_t>(_desc.usage);
        info.bpp = getBytesPerPixel(_desc.format);

        // TODO: image type
        // TODO: image view type

        info.access = 0u; // TODO: set the barrier state
        info.layout = 0u; // TODO: set the barrier state
        info.stage = 0u;  // TODO: set the barrier state

        write(m_fgMemWriter, info);

        return handle;
    }

    DepthStencilHandle Context::registDepthStencil(const char* _name, ImageDesc _desc)
    {
        uint16_t idx = m_textureHandles.alloc();

        DepthStencilHandle handle = DepthStencilHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create DepthStencilHandle failed!");
            return DepthStencilHandle{ kInvalidHandle };
        }

        ImageDesc& desc = m_textureDescs[idx];
        desc = _desc;

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::RegisterDepthStencil);
        write(m_fgMemWriter, magic);

        ImgRegisterInfo info;
        info.idx = idx;
        info.x = _desc.width;
        info.y = _desc.height;
        info.z = _desc.depth;
        info.mips = _desc.mips;
        info.format = static_cast<uint16_t>(_desc.format);
        info.usage = static_cast<uint16_t>(_desc.usage);
        info.bpp = getBytesPerPixel(_desc.format);

        // TODO: image type
        // TODO: image view type

        info.access = 0u; // TODO: set the barrier state
        info.layout = 0u; // TODO: set the barrier state
        info.stage = 0u;  // TODO: set the barrier state

        write(m_fgMemWriter, info);

        return handle;
    }

    uint16_t Context::aliasAlloc(MagicTag _tag)
    {
        if (MagicTag::AliasBuffer == _tag) {
            return m_bufferHandles.alloc();
        }
        if (MagicTag::AliasTexture == _tag) {
            return m_textureHandles.alloc();
        }
        if (MagicTag::AliasRenderTarget == _tag) {
            return m_textureHandles.alloc();
        }
        if (MagicTag::AliasDepthStencil == _tag) {
            return m_textureHandles.alloc();
        }

        message(error, "invalid alias tag!");
        return kInvalidHandle;
    }

    void Context::aliasResrouce(const Memory* _outMem, const uint16_t _aliasCount, const uint16_t _baseRes, MagicTag _tag)
    {
        assert(_aliasCount > 0 && _aliasCount < kMaxNumOfBufferHandle);
        assert(_outMem != nullptr);

        StaticMemoryBlockWriter outWriter(_outMem->data, _outMem->size);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            uint16_t idx = aliasAlloc(_tag);

            assert(idx != kInvalidHandle);
            write(&outWriter, idx);
        }

        uint32_t magic = static_cast<uint32_t>(_tag);
        write(m_fgMemWriter, magic);

        ResAliasInfo info;
        info.resBase = _baseRes;
        info.aliasNum = _aliasCount;
        write(m_fgMemWriter, info);

        write(m_fgMemWriter, (void*)_outMem->data, sizeof(uint16_t) * _aliasCount);
    }

    void Context::readwriteResource(const uint16_t _pass, const Memory* _reses, const uint16_t _resCount, MagicTag _tag)
    {
        uint32_t magic = static_cast<uint32_t>(_tag);
        write(m_fgMemWriter, magic);

        PassRWInfo info;
        info.pass = _pass;
        info.resNum = _resCount;
        write(m_fgMemWriter, info);
        write(m_fgMemWriter, _reses->data, _reses->size);
    }

    void Context::setMultiFrameResource(const Memory* _mem, const uint16_t _resCount, MagicTag _tag)
    {
        assert(_mem != nullptr);
        assert(_mem->size == sizeof(uint16_t) * _resCount);

        uint32_t magic = static_cast<uint32_t>(_tag);
        write(m_fgMemWriter, magic);
        write(m_fgMemWriter, _resCount);
        write(m_fgMemWriter, (void*)_mem->data, _mem->size);
    }

    void Context::setMutiFrameResource(const uint16_t* _reses, const uint16_t _resCount, MagicTag _tag)
    {
        uint32_t magic = static_cast<uint32_t>(_tag);
        write(m_fgMemWriter, magic);

        write(m_fgMemWriter, _resCount);

        write(m_fgMemWriter, _reses, sizeof(uint16_t) * _resCount);
    }

    void Context::setResultRenderTarget(RenderTargetHandle _rt)
    {
        uint32_t magic = static_cast<uint32_t>(MagicTag::SetResultRenderTarget);
        write(m_fgMemWriter, magic);
        
        write(m_fgMemWriter, _rt.idx);
        
        m_resultRenderTarget = _rt;
    }

    void Context::init()
    {
        m_allocator = getAllocator();
        m_fgMemBlock = VKZ_NEW(getAllocator(), MemoryBlock(m_allocator));
        m_fgMemBlock->expand(kInitialFrameGraphMemSize);
        m_fgMemWriter = VKZ_NEW(getAllocator(), MemoryWriter(m_fgMemBlock));

        m_frameGraph = VKZ_NEW(getAllocator(), Framegraph2(getAllocator()));

        uint32_t magic = static_cast<uint32_t>(MagicTag::SetBrief);
        write(m_fgMemWriter, magic);

        // reserve the brief
        write(m_fgMemWriter, FrameGraphBrief());
    }

    void Context::update()
    {
        if (kInvalidHandle == m_resultRenderTarget.idx)
        {
            message(error, "result render target is not set!");
            return;
        }

        // write the finish tag
        uint32_t magic = static_cast<uint32_t>(MagicTag::End);
        write(m_fgMemWriter, magic);

        // move mem pos to the beginning of the memory block
        int64_t size = m_fgMemWriter->seek(0, Whence::Current);
        m_fgMemWriter->seek(sizeof(MagicTag), Whence::Begin);
        
        // replace the brief data
        FrameGraphBrief brief;
        brief.version = 1u;
        brief.passNum = m_passHandles.getNumHandles();
        brief.bufNum = m_bufferHandles.getNumHandles();
        brief.imgNum = m_textureHandles.getNumHandles();
        write(m_fgMemWriter, brief);

        // set back the mem pos
        size -= sizeof(FrameGraphBrief);
        m_fgMemWriter->seek(size, Whence::Current);

        m_frameGraph->setMemoryBlock(m_fgMemBlock);
        m_frameGraph->bake();
    }

    void Context::shutdown()
    {
        VKZ_DELETE(m_frameGraph);
        VKZ_DELETE(m_fgMemWriter);
        VKZ_DELETE(m_fgMemBlock);
    }

    // ================================================
    static Context* s_ctx = nullptr;


    vkz::ShaderHandle registShader(const char* _name, const char* _path)
    {
        return s_ctx->registShader(_name, _path);
    }

    vkz::ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants /*= 0*/)
    {
        const uint16_t size = static_cast<const uint16_t>(_shaders.size());
        const Memory* mem = alloc(size * sizeof(uint16_t));

        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const ShaderHandle& handle = begin(_shaders)[ii];
            write(&writer, handle.idx);
        }

        return s_ctx->registProgram(_name, mem, size);
    }

    vkz::BufferHandle registBuffer(const char* _name, const BufferDesc& _desc)
    {
        return s_ctx->registBuffer(_name, _desc);
    }

    vkz::TextureHandle registTexture(const char* _name, ImageDesc& _desc, const Memory* _mem /* = nullptr */)
    {
        return s_ctx->registTexture(_name, _desc);
    }

    vkz::RenderTargetHandle registRenderTarget(const char* _name, ImageDesc& _desc)
    {
        return s_ctx->registRenderTarget(_name, _desc);
    }

    vkz::DepthStencilHandle registDepthStencil(const char* _name, ImageDesc& _desc)
    {
        return s_ctx->registDepthStencil(_name, _desc);
    }

    PassHandle registPass(const char* _name, PassDesc _desc)
    {
        return s_ctx->registPass(_name, _desc);
    }

    void aliasBuffer(BufferHandle** _aliases, const uint16_t _aliasCount, const BufferHandle _base)
    {
        const Memory* mem = alloc(_aliasCount * sizeof(uint16_t));
        
        s_ctx->aliasResrouce(mem, _aliasCount, _base.idx, MagicTag::AliasBuffer);
        
        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            (*_aliases[ii]).idx = mem->data[ii];
        }

        release(mem);
    }

    void aliasTexture(TextureHandle** _aliases, const uint16_t _aliasCount, const TextureHandle _base)
    {
        const Memory* mem = alloc(_aliasCount * sizeof(uint16_t));

        s_ctx->aliasResrouce(mem, _aliasCount, _base.idx, MagicTag::AliasTexture);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            (*_aliases[ii]).idx = mem->data[ii];
        }

        release(mem);
    }

    void aliasRenderTarget(RenderTargetHandle** _aliases, const uint16_t _aliasCount, const RenderTargetHandle _base)
    {
        const Memory* mem = alloc(_aliasCount * sizeof(uint16_t));

        s_ctx->aliasResrouce(mem, _aliasCount, _base.idx, MagicTag::AliasRenderTarget);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            (*_aliases[ii]).idx = mem->data[ii];
        }

        release(mem);
    }

    void aliasDepthStencil(DepthStencilHandle** _aliases, const uint16_t _aliasCount, const DepthStencilHandle _base)
    {
        const Memory* mem = alloc(_aliasCount * sizeof(uint16_t));
        s_ctx->aliasResrouce(mem, _aliasCount, _base.idx, MagicTag::AliasDepthStencil);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            (*_aliases[ii]).idx = mem->data[ii];
        }

        release(mem);
    }

    BufferHandle aliasBuffer(const BufferHandle _handle)
    {
        BufferHandle alias[1];
        BufferHandle* ptr = alias;

        aliasBuffer(&ptr, (uint16_t)1, _handle);

        BufferHandle ret = alias[0];

        return ret;
    }

    TextureHandle aliasTexture(const TextureHandle _handle)
    {
        TextureHandle alias[1];
        TextureHandle* ptr = alias;
        aliasTexture(&ptr, (uint16_t)1, _handle);

        TextureHandle ret = alias[0];

        return ret;
    }

    RenderTargetHandle aliasRenderTarget(const RenderTargetHandle _handle)
    {
        RenderTargetHandle alias[1];
        RenderTargetHandle* ptr = alias;

        aliasRenderTarget(&ptr, (uint16_t)1, _handle);

        RenderTargetHandle ret = alias[0];

        return ret;
    }

    DepthStencilHandle aliasDepthStencil(const DepthStencilHandle _handle)
    {
        DepthStencilHandle alias[1];
        DepthStencilHandle* ptr = alias;
        
        aliasDepthStencil(&ptr, (uint16_t)1, _handle);

        DepthStencilHandle ret = alias[0];

        return ret;
    }

    void passWriteBuffers(PassHandle _pass, BufferHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        const Memory* mem = alloc(size * sizeof(uint16_t));

        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const BufferHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->readwriteResource(_pass.idx, mem, size, MagicTag::PassWriteBuffer);

        release(mem);
    }

    void passReadBuffers(PassHandle _pass, BufferHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        const Memory* mem = alloc(size * sizeof(uint16_t));

        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const BufferHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->readwriteResource(_pass.idx, mem, size, MagicTag::PassReadBuffer);

        release(mem);
    }

    void passWriteTextures(PassHandle _pass, TextureHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        const Memory* mem = alloc(size * sizeof(uint16_t));

        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const TextureHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->readwriteResource(_pass.idx, mem, size, MagicTag::PassWriteTexture);

        release(mem);
    }

    void passReadTextures(PassHandle _pass, TextureHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        const Memory* mem = alloc(size * sizeof(uint16_t));

        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const TextureHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }
        
        s_ctx->readwriteResource(_pass.idx, mem, size, MagicTag::PassReadTexture);

        release(mem);
    }

    void passWriteRTs(PassHandle _pass, RenderTargetHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        const Memory* mem = alloc(size * sizeof(uint16_t));

        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const RenderTargetHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->readwriteResource(_pass.idx, mem, size, MagicTag::PassWriteRenderTarget);

        release(mem);
    }

    void passReadRTs(PassHandle _pass, RenderTargetHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        const Memory* mem = alloc(size * sizeof(uint16_t));

        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const RenderTargetHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->readwriteResource(_pass.idx, mem, size, MagicTag::PassReadRenderTarget);

        release(mem);
    }

    void passWriteDSs(PassHandle _pass, DepthStencilHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        const Memory* mem = alloc(size * sizeof(uint16_t));
        
        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const DepthStencilHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->readwriteResource(_pass.idx, mem, size, MagicTag::PassWriteDepthStencil);

        release(mem);
    }

    void passReadDSs(PassHandle _pass, DepthStencilHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        const Memory* mem = alloc(size * sizeof(uint16_t));

        StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const DepthStencilHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->readwriteResource(_pass.idx, mem, size, MagicTag::PassReadDepthStencil);

        release(mem);
    }

    void setMultiFrameBuffer(BufferHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());

        const Memory* mem = alloc((uint32_t)(_resList.size() * sizeof(uint16_t)));
        StaticMemoryBlockWriter writer(mem->data, mem->size);

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const BufferHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->setMultiFrameResource(mem, size, MagicTag::SetMuitiFrameBuffer);

        release(mem);
    }

    void setMultiFrameTexture(TextureHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());

        const Memory* mem = alloc((uint32_t)(size * sizeof(uint16_t)));
        StaticMemoryBlockWriter writer(mem->data, mem->size);

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const TextureHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->setMultiFrameResource(mem, size, MagicTag::SetMuitiFrameTexture);

        release(mem);
    }

    void setMultiFrameRenderTarget(RenderTargetHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());

        const Memory* mem = alloc((uint32_t)(size * sizeof(uint16_t)));
        StaticMemoryBlockWriter writer(mem->data, mem->size);

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const RenderTargetHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->setMultiFrameResource(mem, size, MagicTag::SetMultiFrameRenderTarget);

        release(mem);
    }

    void setMultiFrameDepthStencil(DepthStencilHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());

        const Memory* mem = alloc((uint32_t)(size * sizeof(uint16_t)));
        StaticMemoryBlockWriter writer(mem->data, mem->size);

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const DepthStencilHandle& handle = begin(_resList)[ii];
            write(&writer, handle.idx);
        }

        s_ctx->setMultiFrameResource(mem, size, MagicTag::SetMultiFrameDepthStencil);

        release(mem);
    }

    void setResultRenderTarget(RenderTargetHandle _rt)
    {
        s_ctx->setResultRenderTarget(_rt);
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

    void update()
    {
        s_ctx->update();
    }

    void shutdown()
    {
        shutdownAllocator();

        deleteObject(getAllocator(), s_ctx);
    }
}