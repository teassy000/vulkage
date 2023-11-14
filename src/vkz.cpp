#include "common.h"
#include "alloc.h"
#include "handle.h"
#include "memory_operation.h"
#include "vkz.h"

#include "config.h"
#include "name.h"

#include "framegraph_2.h"
#include "resource_manager.h"
#include <array>



namespace vkz
{
    struct Context
    {
        // Context API
        ShaderHandle registShader(const char* _name, const Memory* code);
        ProgramHandle registProgram(const char* _name, ShaderHandleList shaders);
        PipelineHandle registPipeline(const char* _name, ProgramHandle program);
        PassHandle registPass(const char* _name, PassDesc _desc);

        BufferHandle registBuffer(const char* _name, BufferDesc _desc);
        TextureHandle registTexture(const char* _name, ImageDesc _desc);
        RenderTargetHandle registRenderTarget(const char* _name, ImageDesc _desc);
        DepthStencilHandle registDepthStencil(const char* _name, ImageDesc _desc);

        void aliasResrouce(uint16_t* _aliases, const uint16_t _aliasCount, const uint16_t _baseRes, MagicTag _tag);

        void readwriteResource(const uint16_t _pass, const uint16_t* _res, const uint16_t _resCount, MagicTag _tag);

        void setResultRenderTarget(RenderTargetHandle _rt);

        void update();

        HandleArrayT<kMaxNumOfShaderHandle> m_shaderHandles;
        HandleArrayT<kMaxNumOfProgramHandle> m_programHandles;
        HandleArrayT<kMaxNumOfPipelineHandle> m_pipelineHandles;
        HandleArrayT<kMaxNumOfPassHandle> m_passHandles;
        HandleArrayT<kMaxNumOfRenderTargetHandle> m_renderTargetHandles;
        HandleArrayT<kMaxNumOfDepthStencilHandle> m_depthStencilHandles;
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

        // store actual resource data, after frame graph processed
        // so all resources are late allocated
        ResMgr m_resMgr;

        AllocatorI* m_allocator = nullptr;
    };

    vkz::ShaderHandle Context::registShader(const char* _name, const Memory* _mem)
    {
        uint16_t idx = m_shaderHandles.alloc();

        ShaderHandle handle = ShaderHandle{ idx };
        
        // TODO: backup resources info
        // maybe in resource manager, setup those data first, and try to allocate resources after frame graph processed

        return handle;
    }

    ProgramHandle Context::registProgram(const char* _name, ShaderHandleList _shaders)
    {
        uint16_t idx = m_programHandles.alloc();

        ProgramHandle handle = ProgramHandle{ idx };

        return handle;
    }

    PipelineHandle Context::registPipeline(const char* _name, ProgramHandle _prog)
    {
        uint16_t idx = m_passHandles.alloc();

        PipelineHandle handle = PipelineHandle{ idx };
        return handle;
    }

    PassHandle Context::registPass(const char* _name, PassDesc _desc)
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
        desc = _desc;

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::RegisterPass);
        write(m_fgMemWriter, magic);

        PassRegisterInfo info;
        info.idx = idx;
        info.queue = _desc.queue;
        write(m_fgMemWriter, info);

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
        info.x = _desc.x;
        info.y = _desc.y;
        info.z = _desc.z;
        info.mips = _desc.mips;
        info.format = static_cast<uint16_t>(_desc.format);
        info.usage = static_cast<uint16_t>(_desc.usage);
        
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
        uint16_t idx = m_renderTargetHandles.alloc();

        RenderTargetHandle handle = RenderTargetHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create BufferHandle failed!");
            return RenderTargetHandle{ kInvalidHandle };
        }

        ImageDesc& desc = m_textureDescs[idx];
        desc = _desc;

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::RegisterRenderTarget);
        write(m_fgMemWriter, magic);

        ImgRegisterInfo info;
        info.idx = idx;
        info.x = _desc.x;
        info.y = _desc.y;
        info.z = _desc.z;
        info.mips = _desc.mips;
        info.format = static_cast<uint16_t>(_desc.format);
        info.usage = static_cast<uint16_t>(_desc.usage);

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
        uint16_t idx = m_renderTargetHandles.alloc();

        DepthStencilHandle handle = DepthStencilHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create BufferHandle failed!");
            return DepthStencilHandle{ kInvalidHandle };
        }

        ImageDesc& desc = m_textureDescs[idx];
        desc = _desc;

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::RegisterDepthStencil);
        write(m_fgMemWriter, magic);

        ImgRegisterInfo info;
        info.idx = idx;
        info.x = _desc.x;
        info.y = _desc.y;
        info.z = _desc.z;
        info.mips = _desc.mips;
        info.format = static_cast<uint16_t>(_desc.format);
        info.usage = static_cast<uint16_t>(_desc.usage);

        // TODO: image type
        // TODO: image view type

        info.access = 0u; // TODO: set the barrier state
        info.layout = 0u; // TODO: set the barrier state
        info.stage = 0u;  // TODO: set the barrier state

        write(m_fgMemWriter, info);

        return handle;
    }

    void Context::aliasResrouce(uint16_t* _aliases, const uint16_t _aliasCount, const uint16_t _baseRes, MagicTag _tag)
    {
        assert(_aliasCount > 0 && _aliasCount < kMaxNumOfBufferHandle);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            uint16_t idx = m_bufferHandles.alloc();

            assert(idx != kInvalidHandle);
            _aliases[ii] = idx;
        }

        uint32_t magic = static_cast<uint32_t>(_tag);
        write(m_fgMemWriter, magic);

        ResAliasInfo info;
        info.resBase = _baseRes;
        info.aliasNum = _aliasCount;
        write(m_fgMemWriter, info);

        write(m_fgMemWriter, _aliases, sizeof(uint16_t) * _aliasCount);
    }

    void Context::readwriteResource(const uint16_t _pass, const uint16_t* _reses, const uint16_t _resCount, MagicTag _tag)
    {
        uint32_t magic = static_cast<uint32_t>(_tag);
        write(m_fgMemWriter, magic);

        PassRWInfo info;
        info.pass = _pass;
        info.resNum = _resCount;
        write(m_fgMemWriter, info);

        write(m_fgMemWriter, _reses, sizeof(uint16_t) * _resCount);
    }

    void Context::setResultRenderTarget(RenderTargetHandle _rt)
    {
        uint32_t magic = static_cast<uint32_t>(MagicTag::SetResultRenderTarget);
        write(m_fgMemWriter, magic);
        
        write(m_fgMemWriter, _rt.idx);
        
        m_resultRenderTarget = _rt;
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

        m_frameGraph->setMemoryBlock(m_fgMemBlock);
        m_frameGraph->build();
    }

    static Context* s_ctx = nullptr;
    static AllocatorI* s_allocator = nullptr;

    static AllocatorI* getAllocator()
    {
        if (s_allocator == nullptr)
        {
            s_allocator = new DefaultAllocator();
        }
        return s_allocator;
    }

    static void shutdownAllocator()
    {
        delete s_allocator;
        s_allocator = nullptr;
    }

    vkz::ShaderHandle registShader(const char* _name, const Memory* _mem)
    {
        return s_ctx->registShader(_name, _mem);
    }

    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders)
    {
        return s_ctx->registProgram(_name, _shaders);
    }

    PipelineHandle registPipeline(const char* _name, ProgramHandle _program)
    {
        return s_ctx->registPipeline(_name, _program);
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

    void aliasBuffer(BufferHandle* _aliases, const uint16_t _aliasCount, const BufferHandle _buf)
    {
        uint16_t* aliases = new uint16_t[_aliasCount];

        s_ctx->aliasResrouce(aliases, _aliasCount, _buf.idx, MagicTag::AliasBuffer);
        
        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            _aliases[ii].idx = aliases[ii];
        }

        delete[] aliases;
    }

    void aliasTexture(TextureHandle* _aliases, const uint16_t _aliasCount, const TextureHandle _tex)
    {
        uint16_t* aliases = new uint16_t[_aliasCount];

        s_ctx->aliasResrouce(aliases, _aliasCount, _tex.idx, MagicTag::AliasTexture);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            _aliases[ii].idx = aliases[ii];
        }

        delete[] aliases;
    }

    void aliasRenderTarget(RenderTargetHandle* _aliases, const uint16_t _aliasCount, const RenderTargetHandle _rt)
    {
        uint16_t* aliases = new uint16_t[_aliasCount];

        s_ctx->aliasResrouce(aliases, _aliasCount, _rt.idx, MagicTag::AliasRenderTarget);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            _aliases[ii].idx = aliases[ii];
        }

        delete[] aliases;
    }

    void aliasDepthStencil(DepthStencilHandle* _aliases, const uint16_t _aliasCount, const DepthStencilHandle _ds)
    {
        uint16_t* aliases = new uint16_t[_aliasCount];

        s_ctx->aliasResrouce(aliases, _aliasCount, _ds.idx, MagicTag::AliasDepthStencil);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            _aliases[ii].idx = aliases[ii];
        }

        delete[] aliases;
    }

    BufferHandle aliasBuffer(const BufferHandle _handle)
    {
        BufferHandle alias[1];

        aliasBuffer(alias, (uint16_t)1, _handle);

        BufferHandle ret = alias[0];

        return ret;
    }

    TextureHandle aliasTexture(const TextureHandle _handle)
    {
        TextureHandle alias[1];
        aliasTexture(alias, (uint16_t)1, _handle);

        TextureHandle ret = alias[0];

        return ret;
    }

    RenderTargetHandle aliasRenderTarget(const RenderTargetHandle _handle)
    {
        RenderTargetHandle alias[1];
        aliasRenderTarget(alias, (uint16_t)1, _handle);

        RenderTargetHandle ret = alias[0];

        return ret;
    }

    DepthStencilHandle aliasDepthStencil(const DepthStencilHandle _handle)
    {
        DepthStencilHandle alias[1];
        aliasDepthStencil(alias, (uint16_t)1, _handle);

        DepthStencilHandle ret = alias[0];

        return ret;
    }

    void writeBuffers(PassHandle _pass, BufferHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        uint16_t* reses = new uint16_t[size];

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const BufferHandle& handle = begin(_resList)[ii];
            reses[ii] = handle.idx;
        }

        s_ctx->readwriteResource(_pass.idx, reses, size, MagicTag::PassWriteBuffer);

        delete[] reses;
        reses = nullptr;
    }

    void readBuffers(PassHandle _pass, BufferHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        uint16_t* reses = new uint16_t[size];

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const BufferHandle& handle = begin(_resList)[ii];
            reses[ii] = handle.idx;
        }

        s_ctx->readwriteResource(_pass.idx, reses, size, MagicTag::PassReadBuffer);

        delete[] reses;
        reses = nullptr;
    }

    void writeTextures(PassHandle _pass, TextureHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        uint16_t* reses = new uint16_t[size];

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const TextureHandle& handle = begin(_resList)[ii];
            reses[ii] = handle.idx;
        }

        s_ctx->readwriteResource(_pass.idx, reses, size, MagicTag::PassWriteTexture);

        delete[] reses;
        reses = nullptr;
    }

    void readTextures(PassHandle _pass, TextureHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        uint16_t* reses = new uint16_t[size];

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const TextureHandle& handle = begin(_resList)[ii];
            reses[ii] = handle.idx;
        }

        s_ctx->readwriteResource(_pass.idx, reses, size, MagicTag::PassReadTexture);

        delete[] reses;
        reses = nullptr;
    }

    void writeRenderTargets(PassHandle _pass, RenderTargetHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        uint16_t* reses = new uint16_t[size];

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const RenderTargetHandle& handle = begin(_resList)[ii];
            reses[ii] = handle.idx;
        }

        s_ctx->readwriteResource(_pass.idx, reses, size, MagicTag::PassWriteRenderTarget);

        delete[] reses;
        reses = nullptr;
    }

    void readRenderTargets(PassHandle _pass, RenderTargetHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        uint16_t* reses = new uint16_t[size];

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const RenderTargetHandle& handle = begin(_resList)[ii];
            reses[ii] = handle.idx;
        }

        s_ctx->readwriteResource(_pass.idx, reses, size, MagicTag::PassReadRenderTarget);

        delete[] reses;
        reses = nullptr;
    }

    void writeDepthStencils(PassHandle _pass, DepthStencilHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        uint16_t* reses = new uint16_t[size];

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const DepthStencilHandle& handle = begin(_resList)[ii];
            reses[ii] = handle.idx;
        }

        s_ctx->readwriteResource(_pass.idx, reses, size, MagicTag::PassWriteDepthStencil);

        delete[] reses;
        reses = nullptr;
    }

    void readDepthStencils(PassHandle _pass, DepthStencilHandleList _resList)
    {
        const uint16_t size = static_cast<const uint16_t>(_resList.size());
        uint16_t* reses = new uint16_t[size];

        for (uint16_t ii = 0; ii < size; ++ii)
        {
            const DepthStencilHandle& handle = begin(_resList)[ii];
            reses[ii] = handle.idx;
        }

        s_ctx->readwriteResource(_pass.idx, reses, size, MagicTag::PassReadDepthStencil);

        delete[] reses;
        reses = nullptr;
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

    struct MemoryRef
    {
        Memory mem;
        ReleaseFn releaseFn;
        void* userData;
    };

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
        s_ctx = new Context();
        s_ctx->m_allocator = getAllocator();

        s_ctx->m_fgMemBlock = new MemoryBlock(getAllocator());
        s_ctx->m_fgMemBlock->expand(kInitialFrameGraphMemSize);

        s_ctx->m_fgMemWriter = new MemoryWriter((void*)(s_ctx->m_fgMemBlock->expand(0)), s_ctx->m_fgMemBlock->size());

        s_ctx->m_frameGraph = new Framegraph2();

        return true;
    }

    void update()
    {
        s_ctx->update();
    }

    void shutdown()
    {
        s_ctx->m_frameGraph = nullptr;
        delete s_ctx->m_frameGraph;

        s_ctx->m_fgMemWriter = nullptr;
        delete s_ctx->m_fgMemWriter;

        s_ctx->m_fgMemBlock = nullptr;
        delete s_ctx->m_fgMemBlock;

        shutdownAllocator();

        s_ctx = nullptr;
        delete s_ctx;
    }
}