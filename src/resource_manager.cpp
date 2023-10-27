#include "common.h"
#include "resources.h"
#include "framegraph_2.h"
#include "resource_manager.h"


namespace vkz
{
    // just create a manager level resource, which contains meta data

    ResourceID ResMgr::registerRenderTarget(const std::string& name, const FGImgInitInfo& props)
    {
        return invalidResourceID;
    }

    ResourceID ResMgr::registerDepthStencil(const std::string& name, const FGImgInitInfo& props)
    {
        return invalidResourceID;
    }

    ResourceID ResMgr::registerTexture(const std::string& name, const FGImgInitInfo& props)
    {
        return invalidResourceID;
    }

    ResourceID ResMgr::registerBuffer(const std::string& name, const FGBufInitInfo& props)
    {
        return invalidResourceID;
    }

    ResourceID ResMgr::aliasWriteRenderTarget(const std::string& src, std::string& alias)
    {
        static uint32_t id = 0u;
        alias = "alias_" + src + std::to_string(id++);

        return invalidResourceID;
    }

    ResourceID ResMgr::aliasWriteDepthStencil(const std::string& src, std::string& alias)
    {
        static uint32_t id = 0u;
        alias = "alias_" + src + std::to_string(id++);

        return invalidResourceID;
    }

    ResourceID ResMgr::aliasWriteTexture(const std::string& src, std::string& alias)
    {
        static uint32_t id = 0u;
        alias = "alias_" + src + std::to_string(id++);

        return invalidResourceID;
    }

    ResourceID ResMgr::aliasWriteBuffer(const std::string& src, std::string& alias)
    {
        static uint32_t id = 0u;
        alias = "alias_" + src + std::to_string(id++);

        return invalidResourceID;
    }

    // create actual physical resource
    Buffer ResMgr::createBuffer(const FGBufInitInfo& props)
    {
        return Buffer{};
    }

    Image ResMgr::createImage(const FGImgInitInfo& props)
    {
        return Image{};
    }

} // namespace vkz