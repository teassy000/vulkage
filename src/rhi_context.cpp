
#include "common.h"
#include "memory_operation.h"

#include "rhi_context.h"

namespace vkz
{

    void RHIContext::update()
    {
        parseOp();
    }

    void RHIContext::parseOp()
    {
        assert(m_pMemBlock != nullptr);
        MemoryReader reader(m_pMemBlock->expand(0), m_pMemBlock->size());

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
                // End
            case RHIContextOpMagic::invalid_magic:
                message(DebugMessageType::warning, "invalid magic tag, data incorrect!");
            case RHIContextOpMagic::End:
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
}