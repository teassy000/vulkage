#include "common.h"
#include "resources.h"

#include "framegraph_2.h"

namespace vkz
{
    PassID IPass::pass(const std::string& name, const PassInitInfo& pii)
    {
        return invalidPassID;
    }

    void IPass::aliasWriteRenderTarget(const std::string& name)
    {
        _fg.aliasRenderTarget(name); 
    }

    void IFramegraph::aliasRenderTarget(const std::string& name)
    {

    }

}
