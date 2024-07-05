
#include "common.h"

#include "rhi_context.h"
#include "util.h"

namespace kage
{

    void RHIContext::bake()
    {
        parseOp();
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
}