#pragma once

#include <stdint.h>

#include "kage_inner.h"

#include "config.h"

#include "util.h"
#include "bx/allocator.h"
#include "bx/readerwriter.h"

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
        create_program,
        create_shader,
        create_sampler,
        create_image_view,

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

    class UniformMemoryBlock
    {
    public:
        UniformMemoryBlock(bx::AllocatorI* _allocator)
            : m_pbxAllocator{ _allocator }
            , m_pUniformData{ nullptr }
        {
            m_pUniformData = (Memory*)bx::alloc(m_pbxAllocator, kInitialUniformTotalMemSize);// BX_NEW(m_pbxAllocator, bx::MemoryBlock)(m_pbxAllocator);
            //m_pUniformData->expand(kInitialUniformTotalMemSize);
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

    private:
        bx::AllocatorI* m_pbxAllocator;
        Memory* m_pUniformData;

        stl::vector<PassHandle> m_passes;
        stl::vector<UniformBuffer> m_uniforms;
    };

    struct Constants
    {
        uint16_t    passId;
        uint32_t    size;
        int64_t     offset;
    };

    class ConstantsMemoryBlock
    {
    public:
        ConstantsMemoryBlock(bx::AllocatorI* _allocator)
            : m_pbxAllocator{ _allocator }
            , m_pConstantData{ nullptr }
            , m_pWriter{nullptr}
            , m_usedSize{0}
        {
            m_pConstantData = BX_NEW(m_pbxAllocator, bx::MemoryBlock)(m_pbxAllocator);
            m_pConstantData->more(kInitialConstantsTotalMemSize);

            m_pWriter = BX_NEW(m_pbxAllocator, bx::MemoryWriter)(m_pConstantData);
        }

        ~ConstantsMemoryBlock()
        {
            bx::deleteObject(m_pbxAllocator, m_pWriter);
            bx::deleteObject(m_pbxAllocator, m_pConstantData);
        }

        void addConstant(const PassHandle _hPass, uint32_t _size);

        const uint32_t getConstantSize(const PassHandle _hPass) const;
        const void* getConstantData(const PassHandle _hPass) const;
        void updateConstantData(const PassHandle _hPass, const void* _data, uint32_t _size);

    private:
        bx::AllocatorI* m_pbxAllocator;
        bx::MemoryBlockI* m_pConstantData;
        bx::MemoryWriter* m_pWriter;

        stl::vector<PassHandle> m_passes;
        stl::vector<Constants> m_constants;

        uint32_t m_usedSize;
    };


    struct RHIContextI
    {
        virtual void init(const Resolution& _resolution, void* _wnd) = 0;
        virtual void bake() = 0;
        virtual bool render() = 0;
        // check supports
        virtual bool checkSupports(VulkanSupportExtension _ext) = 0;

        // swapchain 
        virtual void updateResolution(const Resolution& _resolution) = 0;

        // update resources
        virtual void updatePushConstants(PassHandle _hPass, const void* _data, uint32_t _size) = 0;
        virtual void updateUniform(PassHandle _hPass, const void* _data, uint32_t _size) = 0;
        virtual void updateBuffer(BufferHandle _hBuf, const void* _data, uint32_t _size) = 0;

        // update settings
        virtual void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ) = 0;

        virtual void updateCustomFuncData(const PassHandle _hPass, const void* _data, uint32_t _size) = 0;

        // naive profiling
        virtual double getPassTime(const PassHandle _hPass) = 0;

        virtual void createShader(bx::MemoryReader& reader) = 0;
        virtual void createProgram(bx::MemoryReader& reader) = 0;
        virtual void createPass(bx::MemoryReader& reader) = 0;
        virtual void createImage(bx::MemoryReader& reader) = 0;
        virtual void createBuffer(bx::MemoryReader& reader) = 0;
        virtual void createSampler(bx::MemoryReader& _reader) = 0;
        virtual void createImageView(bx::MemoryReader& _reader) = 0;
        virtual void setBrief(bx::MemoryReader& reader) = 0;
    };

    struct RHIContext : public RHIContextI
    {
        RHIContext(bx::AllocatorI* _allocator)
            : m_pAllocator{ _allocator }
            , m_pMemBlockBaked{ nullptr }
            , m_constantsMemBlock{ _allocator }
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

        void init(const Resolution& _resolution, void* _wnd) override {};

        void bake() override;
        bool render() override { return false; }


        bool checkSupports(VulkanSupportExtension _ext) override { return false; }
        void updateResolution(const Resolution& _resolution) override {};

        // update 
        void updatePushConstants(PassHandle _hPass, const void* _data, uint32_t _size) override;
        void updateUniform(PassHandle _hPass, const void* _data, uint32_t _size) override {};
        void updateBuffer(BufferHandle _hBuf, const void* _data, uint32_t _size) override {};
        void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ) override {};
        
        void updateCustomFuncData(const PassHandle _hPass, const void* _data, uint32_t _size) override {};

        double getPassTime(const PassHandle _hPass) override { return 0.0; }

        void parseOp();

        void createShader(bx::MemoryReader& reader) override {};
        void createProgram(bx::MemoryReader& reader) override {};
        void createPass(bx::MemoryReader& reader) override {};
        void createImage(bx::MemoryReader& reader) override {};
        void createBuffer(bx::MemoryReader& reader) override {};
        void createSampler(bx::MemoryReader& _reader) override {};
        void createImageView(bx::MemoryReader& _reader) override {};
        void setBrief(bx::MemoryReader& reader) override {};

        ConstantsMemoryBlock m_constantsMemBlock;
        bx::AllocatorI* m_pAllocator;

        bx::MemoryBlockI*   m_pMemBlockBaked;
    };

} // namespace kage