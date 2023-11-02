#include "common.h"
#include "resources.h"
#include "framegraph_2.h"
#include "vkz.h"
#include "resource_manager.h"



namespace vkz
{
    uint32_t hash32(const std::string& name, ResourceType type)
    {
        const uint32_t type_i = std::underlying_type_t<ResourceType>(type);
        const uint32_t TYPE_MASK = (type_i << 28) & 0x0fffffff;
        uint32_t hash = std::hash<std::string>{}(name + std::to_string(type_i)) & TYPE_MASK;

        return hash;
    }

    // just create a manager level resource, which contains meta data
    uint32_t ResMgr::registerRenderTarget(const std::string& name, const FGImgInitInfo& props)
    {
        return invalidID;
    }

    uint32_t ResMgr::registerDepthStencil(const std::string& name, const FGImgInitInfo& props)
    {
        return invalidID;
    }

    uint32_t ResMgr::registerTexture(const std::string& name, const FGImgInitInfo& props)
    {
        return invalidID;
    }

    uint32_t ResMgr::registerBuffer(const std::string& name, const FGBufInitInfo& props)
    {
        return invalidID;
    }

    uint32_t ResMgr::registerAliasRenderTarget(const std::string& srcName, std::string& alias)
    {
        return registerAliasResrouce(alias, _data._rt_forceAliased_base, _data._rt_forceAliased, srcName, ResourceType::renderTarget);
    }

    uint32_t ResMgr::registerAliasDepthStencil(const std::string& src, std::string& alias)
    {
        return registerAliasResrouce(alias, _data._dp_forceAliased_base, _data._dp_forceAliased, src, ResourceType::depthStencil);
    }

    uint32_t ResMgr::registerAliasTexture(const std::string& src, std::string& alias)
    {
        return registerAliasResrouce(alias, _data._tex_forceAliased_base, _data._tex_forceAliased, src, ResourceType::texture);
    }

    uint32_t ResMgr::registerAliasBuffer(const std::string& src, std::string& alias)
    {
        return registerAliasResrouce(alias, _data._buf_forceAliased_base, _data._buf_forceAliased, src, ResourceType::buffer);
    }

    uint32_t ResMgr::registerAliasResrouce(std::string& alias, std::vector<uint32_t>& aliasBases, std::vector<std::vector<uint32_t>>& aliases, const std::string& srcName, const ResourceType type)
    {
        std::string alias_name{};
        generateAliasName(srcName, alias_name);



        return kInvalidHandle;
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

    void ResMgr::generateAliasName(const std::string& src, std::string& alias)
    {
        // TODO
    }

    vkz::ResourceType ResMgr::getResourceType(const uint32_t id)
    {
        const uint32_t type_i = (id >> 28);
        return static_cast<ResourceType>(type_i);
    }

    bool ResMgr::getAliasedResourceIdx(const uint32_t baseResID, uint32_t& idx)
    {
        bool baseExist = false;
        ResourceType type = getResourceType(baseResID);

        std::vector<uint32_t> aliased_base;

        switch (type)
        {
        case vkz::ResourceType::renderTarget:
            aliased_base = _data._rt_forceAliased_base;
            break;
        case vkz::ResourceType::depthStencil:
            aliased_base = _data._dp_forceAliased_base;
            break;
        case vkz::ResourceType::texture:
            aliased_base = _data._tex_forceAliased_base;
            break;
        case vkz::ResourceType::buffer:
            aliased_base = _data._buf_forceAliased_base;
            break;
        case vkz::ResourceType::NUM_OF_RESOURCE_TYPES:
            break;
        default:
            break;
        }

        auto it = std::find(begin(aliased_base), end(aliased_base), baseResID);

        baseExist = (it != end(aliased_base));
        
        // set even if it not exist, would be the last one
        idx = (uint32_t)std::distance(begin(aliased_base), it);

        return baseExist;
    }

    uint32_t ResMgr::getAliasedCount(const uint32_t id)
    {
        return _data._aliasedCount[id];
    }

} // namespace vkz