#include "common.h"
#include "resources.h"

#include "vkz.h"
#include "framegraph_2.h"

namespace vkz
{
    uint32_t Pass2::pass(const std::string& name, const PassInitInfo& pii)
    {
        return invalidID;
    }

    void Pass2::aliasWriteRenderTarget(const std::string& name)
    {
        _fg.aliasRenderTarget(name); 
    }

    void Framegraph2::aliasRenderTarget(const std::string& name)
    {

    }

}
