
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
            RHIContextOpMagic magic = RHIContextOpMagic::InvalidMagic;
            read(&reader, magic);

            bool finished = false;

            switch (magic)
            {
            case RHIContextOpMagic::CreateShader:
                createShader(reader);
                break;
            case RHIContextOpMagic::CreateProgram:
                createProgram(reader);
                break;
            case RHIContextOpMagic::CreatePass:
                createPass(reader);
                break;
            case RHIContextOpMagic::CreateBuffer:
                createBuffer(reader);
                break;
            case RHIContextOpMagic::CreateImage:
                createImage(reader);
                break;
                // End
            case RHIContextOpMagic::InvalidMagic:
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
        }

        reader.seek(0, Whence::Begin); // reset reader
    }
}