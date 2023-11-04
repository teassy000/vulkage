#include "common.h"
#include "resources.h"

#include "vkz.h"
#include "framegraph_2.h"

namespace vkz
{
    uint32_t IPass::pass(const std::string& name, const PassInitInfo& pii)
    {
        return invalidID;
    }

    void IPass::aliasWriteRenderTarget(const std::string& name)
    {
        _fg.aliasRenderTarget(name); 
    }

    void Framegraph2::aliasRenderTarget(const std::string& name)
    {

    }

}
