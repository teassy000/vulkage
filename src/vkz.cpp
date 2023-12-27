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
        // Context API
        ShaderHandle registShader(const char* _name, const char* _path);
        ProgramHandle registProgram(const char* _name, const Memory* _shaders, const uint16_t _shaderCount, const uint32_t _sizePushConstants = 0);

        PassHandle registPass(const char* _name, const PassDesc& _desc);

        BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const ResourceLifetime _lifetime);
        TextureHandle registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);
        RenderTargetHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);
        DepthStencilHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);

        uint16_t aliasAlloc(MagicTag _tag);

        void aliasResrouce(const Memory* _outMem, const uint16_t _aliasCount, const uint16_t _baseRes, MagicTag _tag);

        void readwriteBuffer(const uint16_t _pass, MagicTag _tag, const uint16_t _resCount, const ResInteractDesc& _interact);
        void readwriteImage(const uint16_t _pass, MagicTag _tag, const uint16_t _resCount, const ResInteractDesc& _interact);

        void readwriteResource(const uint16_t _pass, const Memory* _reses, const uint16_t _resCount, MagicTag _tag);

        void setMultiFrameResource(const Memory* _mem, const uint16_t _resCount, MagicTag _tag);
        void setMutiFrameResource(const uint16_t* _reses, const uint16_t _resCount, MagicTag _tag); // do not alias atomically
        void setResultRenderTarget(RenderTargetHandle _rt);

        void init();
        void loop();

        void update();
        void render();

        void shutdown();

        HandleArrayT<kMaxNumOfShaderHandle> m_shaderHandles;
        HandleArrayT<kMaxNumOfProgramHandle> m_programHandles;
        HandleArrayT<kMaxNumOfPipelineHandle> m_pipelineHandles;
        HandleArrayT<kMaxNumOfPassHandle> m_passHandles;
        HandleArrayT<kMaxNumOfImageHandle> m_imageHandles;
        HandleArrayT<kMaxNumOfBufferHandle> m_bufferHandles;

        RenderTargetHandle m_resultRenderTarget{kInvalidHandle};

        // resource Info
        UniDataContainer<uint16_t, BufferDesc> m_bufferDescs;
        UniDataContainer<uint16_t, ImageDesc> m_imageDescs;
        UniDataContainer<uint16_t, PassDesc> m_passDescs;

        // frame graph
        MemoryBlockI* m_pFgMemBlock{ nullptr };
        MemoryWriter* m_fgMemWriter{ nullptr };

        RHIContext* m_rhiContext{ nullptr };

        Framegraph2* m_frameGraph{ nullptr };

        AllocatorI* m_pAllocator{ nullptr };
    };

    ShaderHandle Context::registShader(const char* _name, const char* _path)
    {
        uint16_t idx = m_shaderHandles.alloc();

        ShaderHandle handle = ShaderHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ShaderHandle failed!");
            return ShaderHandle{ kInvalidHandle };
        }
        
        MagicTag magic{ MagicTag::register_shader };
        write(m_fgMemWriter, magic);

        ShaderRegisterInfo info;
        info.shaderId = idx;
        info.strLen = (uint16_t)strnlen_s(_path, kMaxPathLen);

        write(m_fgMemWriter, info);
        write(m_fgMemWriter, (void*)_path, (int32_t)info.strLen);

        write(m_fgMemWriter, MagicTag::magic_body_end);

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

        // magic tag
        MagicTag magic{ MagicTag::register_program };
        write(m_fgMemWriter, magic);

        // info
        ProgramRegisterInfo info;
        info.progId = idx;
        info.sizePushConstants = _sizePushConstants;
        info.shaderNum = _shaderNum;
        write(m_fgMemWriter, info);
        
        // actual shader indexes
        write(m_fgMemWriter, _mem->data, _mem->size);

        write(m_fgMemWriter, MagicTag::magic_body_end);

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

        m_passDescs.addData(idx, _desc);

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::register_pass);
        write(m_fgMemWriter, magic);

        PassRegisterInfo info;
        info.passId = idx;
        info.programId = _desc.programId;
        info.queue = _desc.queue;
        info.vertexBindingNum = _desc.vertexBindingNum; // binding struct
        info.vertexAttributeNum = _desc.vertexAttributeNum; // attribute struct 
        info.vertexAttributeInfos = nullptr;
        info.vertexBindingInfos = nullptr;
        info.pushConstantNum = _desc.pushConstantNum; // int
        info.pipelineConfig = _desc.pipelineConfig;

        write(m_fgMemWriter, info);
        
        if (0 != _desc.vertexBindingNum)
        {
            write(m_fgMemWriter, _desc.vertexBindingInfos, sizeof(VertexBindingDesc) * _desc.vertexBindingNum);
        }
        if (0 != _desc.vertexAttributeNum)
        {
            write(m_fgMemWriter, _desc.vertexAttributeInfos, sizeof(VertexAttributeDesc) * _desc.vertexAttributeNum);
        }
        if (0 != _desc.pushConstantNum)
        {
            write(m_fgMemWriter, _desc.pushConstants, sizeof(int) * _desc.pushConstantNum);
        }

        write(m_fgMemWriter, MagicTag::magic_body_end);
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

        m_bufferDescs.addData(idx, _desc);

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::register_buffer);
        write(m_fgMemWriter, magic);

        BufRegisterInfo info;
        info.bufId = idx;
        info.size = _desc.size;
        info.usage = _desc.usage;
        info.memFlags = _desc.memFlags;
        info.lifetime = _lifetime;

        info.initialState.access = 0u; // TODO: set the barrier state
        info.initialState.layout = ImageLayout::undefined; // TODO: set the barrier state
        info.initialState.stage = 0u;  // TODO: set the barrier state

        write(m_fgMemWriter, info);

        write(m_fgMemWriter, MagicTag::magic_body_end);

        return handle;
    }

    vkz::TextureHandle Context::registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        TextureHandle handle = TextureHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create BufferHandle failed!");
            return TextureHandle{ kInvalidHandle };
        }

        m_imageDescs.addData(idx, _desc);

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::register_texture);
        write(m_fgMemWriter, magic);

        ImgRegisterInfo info;
        info.imgId = idx;
        info.width = _desc.width;
        info.height = _desc.height;
        info.depth = _desc.depth;
        info.mips = _desc.mips;
        info.format = _desc.format;
        info.arrayLayers = _desc.arrayLayers;
        info.usage = _desc.usage;
        info.lifetime = _lifetime;
        info.bpp = getBytesPerPixel(_desc.format);
        
        info.aspectFlags = ImageAspectFlagBits::color;
        // TODO: image type
        // TODO: image view type

        info.initialState.access = AccessFlagBits::none;
        info.initialState.layout = ImageLayout::undefined; 
        info.initialState.stage = PipelineStageFlagBits::host;

        write(m_fgMemWriter, info);

        write(m_fgMemWriter, MagicTag::magic_body_end);

        return handle;
    }

    vkz::RenderTargetHandle Context::registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        RenderTargetHandle handle = RenderTargetHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create RenderTargetHandle failed!");
            return RenderTargetHandle{ kInvalidHandle };
        }

        // do not set the texture format
        if (_desc.format != ResourceFormat::undefined)
        {
            message(error, "do not set the resource format for render target");
            return RenderTargetHandle{ kInvalidHandle };
        }

        m_imageDescs.addData(idx, _desc);

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::register_render_target);
        write(m_fgMemWriter, magic);

        ImgRegisterInfo info;
        info.imgId = idx;
        info.width = _desc.width;
        info.height = _desc.height;
        info.depth = _desc.depth;
        info.mips = _desc.mips;
        info.format = ResourceFormat::undefined;
        info.arrayLayers = _desc.arrayLayers;
        info.usage = ImageUsageFlagBits::color_attachment | _desc.usage;
        info.aspectFlags = ImageAspectFlagBits::color;
        info.lifetime = _lifetime;
        info.bpp = getBytesPerPixel(_desc.format);

        // TODO: image type
        // TODO: image view type

        info.initialState.access = AccessFlagBits::shader_write;
        info.initialState.layout = ImageLayout::undefined;
        info.initialState.stage = PipelineStageFlagBits::host;

        write(m_fgMemWriter, info);

        write(m_fgMemWriter, MagicTag::magic_body_end);

        return handle;
    }

    vkz::DepthStencilHandle Context::registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        DepthStencilHandle handle = DepthStencilHandle{ idx };

        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create DepthStencilHandle failed!");
            return DepthStencilHandle{ kInvalidHandle };
        }

        // do not set the texture format
        if (_desc.format != ResourceFormat::undefined)
        {
            message(error, "do not set the resource format for depth stencil!");
            return DepthStencilHandle{ kInvalidHandle };
        }

        m_imageDescs.addData(idx, _desc);

        // frame graph data
        uint32_t magic = static_cast<uint32_t>(MagicTag::register_depth_stencil);
        write(m_fgMemWriter, magic);

        ImgRegisterInfo info;
        info.imgId = idx;
        info.width = _desc.width;
        info.height = _desc.height;
        info.depth = _desc.depth;
        info.mips = _desc.mips;
        info.format = ResourceFormat::undefined;
        info.arrayLayers = _desc.arrayLayers;
        info.usage = ImageUsageFlagBits::depth_stencil_attachment | _desc.usage;
        info.aspectFlags = ImageAspectFlagBits::depth;
        info.lifetime = _lifetime;
        info.bpp = getBytesPerPixel(_desc.format);

        // TODO: image type
        // TODO: image view type
        info.initialState.access = AccessFlagBits::shader_write;
        info.initialState.layout = ImageLayout::undefined;
        info.initialState.stage = PipelineStageFlagBits::host;

        write(m_fgMemWriter, info);

        write(m_fgMemWriter, MagicTag::magic_body_end);

        return handle;
    }

    uint16_t Context::aliasAlloc(MagicTag _tag)
    {
        if (MagicTag::force_alias_buffer == _tag) {
            return m_bufferHandles.alloc();
        }
        if (MagicTag::force_alias_texture == _tag) {
            return m_imageHandles.alloc();
        }
        if (MagicTag::force_alias_render_target == _tag) {
            return m_imageHandles.alloc();
        }
        if (MagicTag::force_alias_depth_stencil == _tag) {
            return m_imageHandles.alloc();
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

        write(m_fgMemWriter, MagicTag::magic_body_end);
    }

    void Context::readwriteBuffer(const uint16_t _pass, MagicTag _tag, const uint16_t _resId, const ResInteractDesc& _interact)
    {
        // magic: 4 bytes
        // pass id: 2 bytes
        // res id: 2 bytes
        // interact: sizeof(BufferInteractDesc)

        uint32_t magic = static_cast<uint32_t>(_tag);

        write(m_fgMemWriter, magic);

        RWResInfo info;
        info.passId = _pass;
        info.resId = _resId;
        write(m_fgMemWriter, info);

        write(m_fgMemWriter, _interact);

        write(m_fgMemWriter, MagicTag::magic_body_end);
    }

    void Context::readwriteImage(const uint16_t _pass, MagicTag _tag, const uint16_t _resId, const ResInteractDesc& _interact)
    {
        uint32_t magic = static_cast<uint32_t>(_tag);

        write(m_fgMemWriter, magic);

        RWResInfo info;
        info.passId = _pass;
        info.resId = _resId;
        write(m_fgMemWriter, info);

        write(m_fgMemWriter, _interact);

        write(m_fgMemWriter, MagicTag::magic_body_end);
    }

    void Context::readwriteResource(const uint16_t _pass, const Memory* _reses, const uint16_t _resCount, MagicTag _tag)
    {
        // magic: 4 bytes
        // info: 4 bytes
        // reses: 2 bytes * _resCount + sizeof(ResourceInteractDesc)
        return;

        /*
        uint32_t magic = static_cast<uint32_t>(_tag);
        write(m_fgMemWriter, magic);

        PassRWInfo info;
        info.pass = _pass;
        info.resNum = _resCount;
        write(m_fgMemWriter, info);
        write(m_fgMemWriter, _reses->data, _reses->size);

        write(m_fgMemWriter, MagicTag::magic_body_end);
        */
    }


    void Context::setMultiFrameResource(const Memory* _mem, const uint16_t _resCount, MagicTag _tag)
    {
        assert(_mem != nullptr);
        assert(_mem->size == sizeof(uint16_t) * _resCount);

        uint32_t magic = static_cast<uint32_t>(_tag);
        write(m_fgMemWriter, magic);
        write(m_fgMemWriter, _resCount);
        write(m_fgMemWriter, (void*)_mem->data, _mem->size);
        write(m_fgMemWriter, MagicTag::magic_body_end);
    }

    void Context::setMutiFrameResource(const uint16_t* _reses, const uint16_t _resCount, MagicTag _tag)
    {
        uint32_t magic = static_cast<uint32_t>(_tag);
        write(m_fgMemWriter, magic);

        write(m_fgMemWriter, _resCount);

        write(m_fgMemWriter, _reses, sizeof(uint16_t) * _resCount);
        write(m_fgMemWriter, MagicTag::magic_body_end);
    }

    void Context::setResultRenderTarget(RenderTargetHandle _rt)
    {
        uint32_t magic = static_cast<uint32_t>(MagicTag::set_present);
        write(m_fgMemWriter, magic);
        
        write(m_fgMemWriter, _rt.id);
        write(m_fgMemWriter, MagicTag::magic_body_end);
        
        m_resultRenderTarget = _rt;
    }

    void Context::init()
    {
        m_pAllocator = getAllocator();
        
        m_rhiContext = VKZ_NEW(m_pAllocator, RHIContext_vk(m_pAllocator));

        m_frameGraph = VKZ_NEW(m_pAllocator, Framegraph2(m_pAllocator, m_rhiContext->getMemoryBlock()));

        m_pFgMemBlock = m_frameGraph->getMemoryBlock();
        m_fgMemWriter = VKZ_NEW(m_pAllocator, MemoryWriter(m_pFgMemBlock));

        uint32_t magic = static_cast<uint32_t>(MagicTag::set_brief);
        write(m_fgMemWriter, magic);

        // reserve the brief
        write(m_fgMemWriter, FrameGraphBrief());
        write(m_fgMemWriter, MagicTag::magic_body_end);
    }

    void Context::loop()
    {
        update();
        render();
    }

    void Context::update()
    {
        if (kInvalidHandle == m_resultRenderTarget.id)
        {
            message(error, "result render target is not set!");
            return;
        }

        // write the finish tag
        uint32_t magic = static_cast<uint32_t>(MagicTag::end);
        write(m_fgMemWriter, magic);

        // move mem pos to the beginning of the memory block
        int64_t size = m_fgMemWriter->seek(0, Whence::Current);
        m_fgMemWriter->seek(sizeof(MagicTag), Whence::Begin);

        // replace the brief data
        FrameGraphBrief brief;
        brief.version = 1u;
        brief.passNum = m_passHandles.getNumHandles();
        brief.bufNum = m_bufferHandles.getNumHandles();
        brief.imgNum = m_imageHandles.getNumHandles();
        brief.shaderNum = m_shaderHandles.getNumHandles();
        brief.programNum = m_programHandles.getNumHandles();

        write(m_fgMemWriter, brief);

        // set back the mem pos
        size -= sizeof(FrameGraphBrief);
        m_fgMemWriter->seek(size, Whence::Current);

        m_frameGraph->setMemoryBlock(m_pFgMemBlock);
        m_frameGraph->bake();

        m_rhiContext->update();
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

    vkz::TextureHandle registTexture(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/, const Memory* _mem /*= nullptr*/)
    {
        return s_ctx->registTexture(_name, _desc, _lifetime);
    }

    vkz::RenderTargetHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registRenderTarget(_name, _desc, _lifetime);
    }

    vkz::DepthStencilHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registDepthStencil(_name, _desc, _lifetime);
    }

    PassHandle registPass(const char* _name, PassDesc _desc)
    {
        return s_ctx->registPass(_name, _desc);
    }

    void aliasBuffer(BufferHandle** _aliases, const uint16_t _aliasCount, const BufferHandle _base)
    {
        const Memory* mem = alloc(_aliasCount * sizeof(uint16_t));
        
        s_ctx->aliasResrouce(mem, _aliasCount, _base.id, MagicTag::force_alias_buffer);
        
        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            (*_aliases[ii]).id = mem->data[ii];
        }

        release(mem);
    }

    void aliasTexture(TextureHandle** _aliases, const uint16_t _aliasCount, const TextureHandle _base)
    {
        const Memory* mem = alloc(_aliasCount * sizeof(uint16_t));

        s_ctx->aliasResrouce(mem, _aliasCount, _base.id, MagicTag::force_alias_texture);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            (*_aliases[ii]).id = mem->data[ii];
        }

        release(mem);
    }

    void aliasRenderTarget(RenderTargetHandle** _aliases, const uint16_t _aliasCount, const RenderTargetHandle _base)
    {
        const Memory* mem = alloc(_aliasCount * sizeof(uint16_t));

        s_ctx->aliasResrouce(mem, _aliasCount, _base.id, MagicTag::force_alias_render_target);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            (*_aliases[ii]).id = mem->data[ii];
        }

        release(mem);
    }

    void aliasDepthStencil(DepthStencilHandle** _aliases, const uint16_t _aliasCount, const DepthStencilHandle _base)
    {
        const Memory* mem = alloc(_aliasCount * sizeof(uint16_t));
        s_ctx->aliasResrouce(mem, _aliasCount, _base.id, MagicTag::force_alias_depth_stencil);

        for (uint16_t ii = 0; ii < _aliasCount; ++ii)
        {
            (*_aliases[ii]).id = mem->data[ii];
        }

        release(mem);
    }

    void passReadBuffer(PassHandle _pass, BufferHandle _buf, ResInteractDesc _interact)
    {
        s_ctx->readwriteBuffer(_pass.id, MagicTag::pass_read_buffer, _buf.id, _interact);
    }

    void passWriteBuffer(PassHandle _pass, BufferHandle _buf, ResInteractDesc _interact)
    {
        s_ctx->readwriteBuffer(_pass.id, MagicTag::pass_write_buffer, _buf.id, _interact);
    }

    void passReadTexture(PassHandle _pass, TextureHandle _img, ResInteractDesc _interact)
    {
        s_ctx->readwriteImage(_pass.id, MagicTag::pass_read_texture, _img.id, _interact);
    }

    void passWriteTexture(PassHandle _pass, TextureHandle _img, ResInteractDesc _interact)
    {
        s_ctx->readwriteImage(_pass.id, MagicTag::pass_write_texture, _img.id, _interact);
    }

    void passReadRT(PassHandle _pass, RenderTargetHandle _rt, ResInteractDesc _interact)
    {
        _interact.access |= AccessFlagBits::color_attachment_read;
        s_ctx->readwriteImage(_pass.id, MagicTag::pass_read_render_target, _rt.id, _interact);
    }

    void passWriteRT(PassHandle _pass, RenderTargetHandle _rt, ResInteractDesc _interact)
    {
        _interact.access |= AccessFlagBits::color_attachment_write;
        s_ctx->readwriteImage(_pass.id, MagicTag::pass_write_render_target, _rt.id, _interact);
    }

    void passReadDS(PassHandle _pass, DepthStencilHandle _ds, ResInteractDesc _interact)
    {
        _interact.access |= AccessFlagBits::depth_stencil_attachment_read;
        s_ctx->readwriteImage(_pass.id, MagicTag::pass_read_depth_stencil, _ds.id, _interact);
    }

    void passWriteDS(PassHandle _pass, DepthStencilHandle _ds, ResInteractDesc _interact)
    {
        _interact.access |= AccessFlagBits::depth_stencil_attachment_write;
        s_ctx->readwriteImage(_pass.id, MagicTag::pass_write_depth_stencil, _ds.id, _interact);
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
            write(&writer, handle.id);
        }

        s_ctx->readwriteResource(_pass.id, mem, size, MagicTag::pass_write_buffer);

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
            write(&writer, handle.id);
        }

        s_ctx->readwriteResource(_pass.id, mem, size, MagicTag::pass_read_buffer);

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
            write(&writer, handle.id);
        }

        s_ctx->readwriteResource(_pass.id, mem, size, MagicTag::pass_write_texture);

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
            write(&writer, handle.id);
        }
        
        s_ctx->readwriteResource(_pass.id, mem, size, MagicTag::pass_read_texture);

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
            write(&writer, handle.id);
        }

        s_ctx->readwriteResource(_pass.id, mem, size, MagicTag::pass_write_render_target);

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
            write(&writer, handle.id);
        }

        s_ctx->readwriteResource(_pass.id, mem, size, MagicTag::pass_read_render_target);

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
            write(&writer, handle.id);
        }

        s_ctx->readwriteResource(_pass.id, mem, size, MagicTag::pass_write_depth_stencil);

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
            write(&writer, handle.id);
        }

        s_ctx->readwriteResource(_pass.id, mem, size, MagicTag::pass_read_depth_stencil);

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
            write(&writer, handle.id);
        }

        s_ctx->setMultiFrameResource(mem, size, MagicTag::set_non_transition_buffer);

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
            write(&writer, handle.id);
        }

        s_ctx->setMultiFrameResource(mem, size, MagicTag::set_non_transition_texture);

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
            write(&writer, handle.id);
        }

        s_ctx->setMultiFrameResource(mem, size, MagicTag::set_non_transition_render_target);

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
            write(&writer, handle.id);
        }

        s_ctx->setMultiFrameResource(mem, size, MagicTag::set_non_transition_depth_stencil);

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