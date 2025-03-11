#pragma once

#include <stdint.h>

#include "kage_inner.h"

#include "config.h"

#include "util.h"
#include "bx/allocator.h"
#include "bx/readerwriter.h"
#include "command_buffer.h"

// ffx
#include "FidelityFX/host/ffx_types.h"
#include "FidelityFX/host/ffx_interface.h"

namespace kage
{
    enum class RHIContextOpMagic : uint32_t
    {
        invalid_magic = 0,

        magic_body_end,

        create_pass,
        create_image,
        create_buffer,
        create_descriptor_set,
        create_descriptor_set_array_layout,
        create_program,
        create_shader,
        create_bindless,
        create_sampler,

        set_brief,

        end,
    };

    struct UniformBuffer
    {
        uint16_t    passId;
        uint16_t    binding;
        uint16_t    size;
        uint16_t    offset;
    };

    struct UniformMemoryBlock
    {
        UniformMemoryBlock(bx::AllocatorI* _allocator)
            : m_pbxAllocator{ _allocator }
            , m_pUniformData{ nullptr }
        {
            m_pUniformData = (Memory*)bx::alloc(m_pbxAllocator, kInitialUniformMemSize);
        }

        ~UniformMemoryBlock()
        {

            free(m_pbxAllocator, m_pUniformData);
        }

        inline Memory* getMemory() const { return m_pUniformData; }

        inline void addUniform(const UniformBuffer& _uniform, const void* _data, uint32_t _size)
        {
            m_passes.push_back({ _uniform.passId });
            m_uniforms.push_back(_uniform);
        }

        const void* getUniformData(const PassHandle _hPass);
        void updateUniformData(const PassHandle _hPass, const void* _data, uint32_t _size);

        bx::AllocatorI* m_pbxAllocator;
        Memory* m_pUniformData;

        stl::vector<PassHandle> m_passes;
        stl::vector<UniformBuffer> m_uniforms;
    };

    struct BufferInfoToUpdate
    {
        BufferHandle m_hBuf;
        uint32_t m_offset;
        uint32_t m_size;

        uint32_t m_lastSize;
    };

    struct BufferMemoryPerFrame
    {
        struct BufferInfo
        {
            BufferHandle m_hBuf;
            uint32_t m_offset;
            uint32_t m_size;
        };

        BufferMemoryPerFrame(bx::AllocatorI* _allocator)
            : m_pbxAllocator{ _allocator }
            , m_pBuffer{ nullptr }
            , m_pWriter{ nullptr }
            , m_usedSize{ 0 }
        {
            m_pBuffer = BX_NEW(m_pbxAllocator, bx::MemoryBlock)(m_pbxAllocator);
            m_pBuffer->more(kInitialStorageMemSize);

            m_pWriter = BX_NEW(m_pbxAllocator, bx::MemoryWriter)(m_pBuffer);
        }

        ~BufferMemoryPerFrame()
        {
            bx::deleteObject(m_pbxAllocator, m_pWriter);
            bx::deleteObject(m_pbxAllocator, m_pBuffer);
        }

        void addBuffer(const BufferHandle _hBuf, uint32_t _size);

        const uint32_t getBufferSize(const BufferHandle _hBuf) const;
        const void* getBufferData(const BufferHandle _hBuf) const;
        void updateBufferData(const BufferHandle _hBuf, const void* _data, uint32_t _size);

        bx::AllocatorI* m_pbxAllocator;
        bx::MemoryBlockI* m_pBuffer;
        bx::MemoryWriter* m_pWriter;

        stl::vector<BufferHandle> m_buffers;
        stl::vector<BufferInfo> m_bufferInfos;

        uint32_t m_usedSize;
    };

    struct RHIContext
    {
        RHIContext(bx::AllocatorI* _allocator)
            : m_pAllocator{ _allocator }
            , m_pMemBlockBaked{ nullptr }
        {
            m_pMemBlockBaked = BX_NEW(m_pAllocator, bx::MemoryBlock)(m_pAllocator);
            m_pMemBlockBaked->more(kInitialFrameGraphMemSize);
        }

        virtual ~RHIContext()
        {
            bx::deleteObject(m_pAllocator, m_pMemBlockBaked);
        }

        inline bx::MemoryBlockI* memoryBlock() const {return m_pMemBlockBaked;}
        inline bx::AllocatorI* allocator() const { return m_pAllocator; }

        virtual void init(const Resolution& _resolution, void* _wnd) {};

        virtual void bake();
        virtual bool run() { return false; };
        virtual bool isBaked() { return m_baked; }

        virtual bool checkSupports(VulkanSupportExtension _ext) { return false; }
        virtual void updateResolution(const Resolution& _resolution) {};

        // update 
        virtual void updateUniform(PassHandle _hPass, const void* _data, uint32_t _size) {};
        virtual void updateBuffer(
            const BufferHandle _hBuf
            , const Memory* _mem
            , const uint32_t _offset
            , const uint32_t _size
        ) {};

        virtual void updateImage(
            const ImageHandle _hImg
            , const uint16_t _width
            , const uint16_t _height
            , const Memory* _mem
        ) {};

        virtual double getPassTime(const PassHandle _hPass) { return 0.0; }
        virtual double getGPUTime() { return 0.0; }
        virtual uint64_t getPassClipping(const PassHandle _hPass) { return 0; }

        void parseOp();

        virtual void createShader(bx::MemoryReader& reader) {};
        virtual void createProgram(bx::MemoryReader& reader) {};
        virtual void createPass(bx::MemoryReader& reader) {};
        virtual void createImage(bx::MemoryReader& reader) {};
        virtual void createBuffer(bx::MemoryReader& reader) {};
        virtual void createSampler(bx::MemoryReader& _reader) {};
        virtual void createBindless(bx::MemoryReader& _reader) {};
        virtual void setBrief(bx::MemoryReader& reader) {};

        virtual void setName(Handle _h, const char* _name, uint32_t _len) {};

        // rendering commands
        virtual void setRecord(PassHandle _hPass, const CommandQueue& _cq, const uint32_t _offset, const uint32_t _size) {};
        // -- rendering commands ends


        bx::AllocatorI* m_pAllocator;

        bx::MemoryBlockI*   m_pMemBlockBaked;


        // ffx
        virtual void initFFX() {};
        virtual void* getRhiResource(ImageHandle _img) {return nullptr; }
        virtual void* getRhiResource(BufferHandle _buf) { return nullptr; }

        void getFFXInterface(void* _ffxInterface) { *(FfxInterface*)_ffxInterface = m_ffxInterface; };
        
        FfxDevice m_ffxDevice;
        FfxInterface m_ffxInterface;
        stl::vector<FfxResource> m_ffxRhiResources;
        bool m_baked{ false };
    };

} // namespace kage