
#include "common.h"
#include "memory_operation.h"

#include "rhi_context.h"
#include "util.h"

namespace vkz
{

    void RHIContext::bake()
    {
        parseOp();
    }

    void RHIContext::updatePushConstants(PassHandle _hPass, const void* _data, uint32_t _size)
    {
         m_constantsMemBlock.updateConstantData(_hPass, _data, _size);
    }

    void RHIContext::parseOp()
    {
        assert(m_pMemBlockBaked != nullptr);
        MemoryReader reader(m_pMemBlockBaked->expand(0), m_pMemBlockBaked->size());

        while (true)
        {
            RHIContextOpMagic magic = RHIContextOpMagic::invalid_magic;
            read(&reader, magic);

            bool finished = false;

            switch (magic)
            {
            case RHIContextOpMagic::create_shader:
                createShader(reader);
                break;
            case RHIContextOpMagic::create_program:
                createProgram(reader);
                break;
            case RHIContextOpMagic::create_pass:
                createPass(reader);
                break;
            case RHIContextOpMagic::create_buffer:
                createBuffer(reader);
                break;
            case RHIContextOpMagic::create_image:
                createImage(reader);
                break;
            case RHIContextOpMagic::create_sampler:
                createSampler(reader);
                break;
            case RHIContextOpMagic::create_specific_image_view:
                createSpecificImageView(reader);
                break;
            case RHIContextOpMagic::set_brief:
                setBrief(reader);
                break;
                // End
            case RHIContextOpMagic::invalid_magic:
                message(DebugMessageType::warning, "invalid magic tag, data incorrect!");
            case RHIContextOpMagic::end:
            default:
                finished = true;
                break;
            }

            if (finished)
            {
                break;
            }

            RHIContextOpMagic bodyEnd;
            read(&reader, bodyEnd);
            assert(RHIContextOpMagic::magic_body_end == bodyEnd);

        }

        reader.seek(0, Whence::Begin); // reset reader
    }

    const void* UniformMemoryBlock::getUniformData(const PassHandle _hPass)
    {
        return nullptr;
    }

    void UniformMemoryBlock::updateUniformData(const PassHandle _hPass, const void* _data, uint32_t _size)
    {
        // TODO
    }

    void ConstantsMemoryBlock::addConstant(const PassHandle _hPass, uint32_t _size)
    {
        if (0 == _size )
            return;

        int64_t offset = m_usedSize;
        m_usedSize += _size;

        Constants constants{ };
        constants.passId = _hPass.id;
        constants.size = _size;
        constants.offset = offset;

        if (m_pConstantData->size() <= offset + _size)
        {
            m_pConstantData->expand(m_pConstantData->size()); // double current size
        }

        m_passes.push_back(_hPass);
        m_constants.push_back(constants);
    }

    const uint32_t ConstantsMemoryBlock::getConstantSize(const PassHandle _hPass) const
    {
        const uint16_t idx = (uint16_t)getElemIndex(m_passes, _hPass);
        if (kInvalidIndex == idx) {
            return 0;
        }

        return m_constants[idx].size;
    }

    const void* ConstantsMemoryBlock::getConstantData(const PassHandle _hPass) const
    {
        const uint16_t idx = (uint16_t)getElemIndex(m_passes, _hPass);
        if (kInvalidIndex == idx) {
            return nullptr;
        }

        uint8_t* data = (uint8_t*)m_pConstantData->expand(0);
        const void* pC = data + m_constants[idx].offset;

        return pC;
    }

    void ConstantsMemoryBlock::updateConstantData(const PassHandle _hPass, const void* _data, uint32_t _size)
    {
        const uint16_t idx = (uint16_t)getElemIndex(m_passes, _hPass);
        if (kInvalidIndex == idx) {
            return;
        }
        assert(m_constants[idx].size == _size);

        m_pWriter->seek(m_constants[idx].offset, Whence::Begin);
        write(m_pWriter, _data, _size);
    }

}