#pragma once

#include <stdint.h>

#include "vkz_structs_inner.h"
#include "vkz.h"
#include "config.h"
#include "name.h"
#include "memory_operation.h"
#include "util.h"

namespace vkz
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
        create_specific_image_view,

        set_back_buffers,

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
        UniformMemoryBlock(AllocatorI* _allocator)
            : m_pAllocator{ _allocator }
            , m_pUniformData{ nullptr }
        {
            m_pUniformData = VKZ_NEW(m_pAllocator, MemoryBlock(m_pAllocator));
            m_pUniformData->expand(kInitialUniformTotalMemSize);
        }

        ~UniformMemoryBlock()
        {
            deleteObject(m_pAllocator, m_pUniformData);
        }

        inline MemoryBlockI* getMemoryBlock() const { return m_pUniformData; }

        inline void addUniform(const UniformBuffer& _uniform, const void* _data, uint32_t _size)
        {
            m_passes.push_back({ _uniform.passId });
            m_uniforms.push_back(_uniform);
        }

        const void* getUniformData(const PassHandle _hPass);
        void updateUniformData(const PassHandle _hPass, const void* _data, uint32_t _size);

    private:
        AllocatorI* m_pAllocator;
        MemoryBlockI* m_pUniformData;

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
        ConstantsMemoryBlock(AllocatorI* _allocator)
            : m_pAllocator{ _allocator }
            , m_pConstantData{ nullptr }
            , m_pWriter{nullptr}
            , m_usedSize{0}
        {
            m_pConstantData = VKZ_NEW(m_pAllocator, MemoryBlock(m_pAllocator));
            m_pConstantData->expand(kInitialUniformTotalMemSize);

            m_pWriter = VKZ_NEW(m_pAllocator, MemoryWriter(m_pConstantData));
        }

        ~ConstantsMemoryBlock()
        {
            deleteObject(m_pAllocator, m_pWriter);
            deleteObject(m_pAllocator, m_pConstantData);
        }

        inline MemoryBlockI* getMemoryBlock() const { return m_pConstantData; }

        void addConstant(const PassHandle _hPass, uint32_t _size);

        const uint32_t getConstantSize(const PassHandle _hPass) const;
        const void* getConstantData(const PassHandle _hPass) const;
        void updateConstantData(const PassHandle _hPass, const void* _data, uint32_t _size);

    private:
        AllocatorI* m_pAllocator;
        MemoryBlockI* m_pConstantData;
        MemoryWriter* m_pWriter;

        stl::vector<PassHandle> m_passes;
        stl::vector<Constants> m_constants;

        uint32_t m_usedSize;
    };


    class RHIContextI
    {
    public:
        virtual void init(RHI_Config _config, void* _wnd) = 0;
        virtual void bake() = 0;
        virtual bool render() = 0;

        // swapchain 
        virtual void resizeBackbuffers(uint32_t _width, uint32_t _height) = 0;

        // update resources
        virtual void updatePushConstants(PassHandle _hPass, const void* _data, uint32_t _size) = 0;
        virtual void updateUniform(PassHandle _hPass, const void* _data, uint32_t _size) = 0;
        virtual void updateBuffer(BufferHandle _hBuf, const void* _data, uint32_t _size) = 0;

        // update settings
        virtual void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ) = 0;

        virtual void updateCustomFuncData(const PassHandle _hPass, const void* _data, uint32_t _size) = 0;

    private:
        virtual void createShader(MemoryReader& reader) = 0;
        virtual void createProgram(MemoryReader& reader) = 0;
        virtual void createPass(MemoryReader& reader) = 0;
        virtual void createImage(MemoryReader& reader) = 0;
        virtual void createBuffer(MemoryReader& reader) = 0;
        virtual void createSampler(MemoryReader& _reader) = 0;
        virtual void createImageView(MemoryReader& _reader) = 0;
        virtual void createSpecificImageView(MemoryReader& _reader) = 0;
        virtual void setBackBuffers(MemoryReader& _reader) = 0;
        virtual void setBrief(MemoryReader& reader) = 0;
    };

    class RHIContext : public RHIContextI
    {
    public:
        RHIContext(AllocatorI* _allocator, NameManager* _nameManager)
            : m_pAllocator{ _allocator }
            , m_pNameManager{ _nameManager }
            , m_pMemBlockBaked{ nullptr }
            , m_constantsMemBlock{ _allocator }
        {
            m_pMemBlockBaked = VKZ_NEW(m_pAllocator, MemoryBlock(m_pAllocator));
            m_pMemBlockBaked->expand(kInitialFrameGraphMemSize);
        }

        virtual ~RHIContext()
        {
            deleteObject(m_pAllocator, m_pMemBlockBaked);
        }

        inline MemoryBlockI* memoryBlock() const {return m_pMemBlockBaked;}
        inline AllocatorI* allocator() const { return m_pAllocator; }

        void init(RHI_Config _config, void* _wnd) override {};

        void bake() override;
        bool render() override { return false; };

        void resizeBackbuffers(uint32_t _width, uint32_t _height) override {};

        // update 
        void updatePushConstants(PassHandle _hPass, const void* _data, uint32_t _size) override;
        void updateUniform(PassHandle _hPass, const void* _data, uint32_t _size) override {};
        void updateBuffer(BufferHandle _hBuf, const void* _data, uint32_t _size) override {};
        void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ) override {};
        
        void updateCustomFuncData(const PassHandle _hPass, const void* _data, uint32_t _size) override {};

    private:
        void parseOp();

        void createShader(MemoryReader& reader) override {};
        void createProgram(MemoryReader& reader) override {};
        void createPass(MemoryReader& reader) override {};
        void createImage(MemoryReader& reader) override {};
        void createBuffer(MemoryReader& reader) override {};
        void createSampler(MemoryReader& _reader) override {};
        void createSpecificImageView(MemoryReader& _reader) override {};
        void setBackBuffers(MemoryReader& _reader) override {};
        void setBrief(MemoryReader& reader) override {};

    protected:
        ConstantsMemoryBlock m_constantsMemBlock;
        AllocatorI* m_pAllocator;
        NameManager* m_pNameManager;
    private:

        MemoryBlockI*   m_pMemBlockBaked;
    };

} // namespace vkz