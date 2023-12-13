
#include "common.h"
#include "memory_operation.h"

#include "res_creator.h"

namespace vkz
{
    void ResCreator::parseOp()
    {
        assert(m_pMemBlock != nullptr);
        MemoryReader reader(m_pMemBlock->expand(0), m_pMemBlock->size());

        while (true)
        {
            ResCreatorOpMagic magic = ResCreatorOpMagic::InvalidMagic;
            read(&reader, magic);

            bool finished = false;

            switch (magic)
            {
            case ResCreatorOpMagic::Init:
                init(reader);
                break;
            case ResCreatorOpMagic::CreateShader:
                createShader(reader);
                break;
            case ResCreatorOpMagic::CreateProgram:
                createProgram(reader);
                break;
            case ResCreatorOpMagic::CreatePass:
                createPass(reader);
                break;
            case ResCreatorOpMagic::CreateBuffer:
                createBuffer(reader);
                break;
            case ResCreatorOpMagic::CreateImage:
                createImage(reader);
                break;
                // End
            case ResCreatorOpMagic::InvalidMagic:
                message(DebugMessageType::warning, "invalid magic tag, data incorrect!");
            case ResCreatorOpMagic::End:
            default:
                finished = true;
                break;
            }
        }
    }
}