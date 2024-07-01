
#include "common.h"

#include "rhi_context.h"
#include "util.h"

namespace kage
{

    void RHIContext::bake()
    {
        parseOp();
    }

    void RHIContext::updateConstants(const PassHandle _hPass, const Memory* _mem)
    {
        m_constantsMemBlock.update(_hPass, _mem->data, _mem->size);
    }

    void RHIContext::parseOp()
    {
        assert(m_pMemBlockBaked != nullptr);
        bx::MemoryReader reader(m_pMemBlockBaked->more(), m_pMemBlockBaked->getSize());

        while (true)
        {
            RHIContextOpMagic magic = RHIContextOpMagic::invalid_magic;
            bx::read(&reader, magic, nullptr);

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
            case RHIContextOpMagic::set_brief:
                setBrief(reader);
                break;
                // End
            case RHIContextOpMagic::invalid_magic:
                message(DebugMsgType::warning, "invalid magic tag, data incorrect!");
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
            bx::read(&reader, bodyEnd, nullptr);
            assert(RHIContextOpMagic::magic_body_end == bodyEnd);

        }

        reader.seek(0, bx::Whence::Begin); // reset reader
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

        if (m_pConstantData->getSize() <= offset + _size)
        {
            m_pConstantData->more(m_pConstantData->getSize()); // double current size
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

        uint8_t* data = (uint8_t*)m_pConstantData->more(0);
        const void* pC = data + m_constants[idx].offset;

        return pC;
    }

    void ConstantsMemoryBlock::update(const PassHandle _hPass, const void* _data, uint32_t _size)
    {
        const uint16_t idx = (uint16_t)getElemIndex(m_passes, _hPass);
        if (kInvalidIndex == idx) {
            return;
        }
        assert(m_constants[idx].size == _size);

        m_pWriter->seek(m_constants[idx].offset, bx::Whence::Begin);
        bx::write(m_pWriter, _data, _size, nullptr);
    }

}