#pragma once

namespace vkz
{
    struct ManagedBuffer
    {
        ResourceID _id{ invalidResourceID };
        std::string _name;
        size_t _size{ 0u };

        uint32_t _bucketIdx{ 0u };

        uint64_t _memory{ 0u };
        size_t _offset{ 0u };

        std::vector<ResourceID> _forceAlias;
    };

    struct ManagedImage
    {
        ResourceID _id{ invalidResourceID };
        std::string _name;
        uint32_t _x{ 0u };
        uint32_t _y{ 0u };
        uint32_t _z{ 1u };

        uint16_t _mipLevels{ 1u };
        uint16_t _padding{ 0u };
        uint32_t _bucketIdx{ 0u };

        std::vector<ResourceID> _forceAlias;
    };

    struct ResData
    {
        std::unordered_map<const std::string, ResourceID> _nameToRes;
        std::unordered_map<const ResourceID, uint32_t>    _resIdx;

        std::vector<ResourceID> _rt_registed;
        std::vector<ResourceID> _dp_registed;
        std::vector<ResourceID> _tex_registed;
        std::vector<ResourceID> _buf_registed;

        std::vector<ResourceID> _rt_inUse;
        std::vector<ResourceID> _dp_inUse;
        std::vector<ResourceID> _tex_inUse;
        std::vector<ResourceID> _buf_inUse;

        std::vector<ResourceID> _rt_base_forceAliased;
        std::vector<ResourceID> _dp_base_forceAliased;
        std::vector<ResourceID> _tex_base_forceAliased;
        std::vector<ResourceID> _buf_base_forceAliased;

        std::vector<std::vector<ResourceID>> _rt_forceAliased;
        std::vector<std::vector<ResourceID>> _dp_forceAliased;
        std::vector<std::vector<ResourceID>> _tex_forceAliased;
        std::vector<std::vector<ResourceID>> _buf_forceAliased;

        std::unordered_map<ResourceID, std::pair<uint32_t, uint32_t>> _rt_lifetimeIdx;
        std::unordered_map<ResourceID, std::pair<uint32_t, uint32_t>> _dp_lifetimeIdx;
        std::unordered_map<ResourceID, std::pair<uint32_t, uint32_t>> _tex_lifetimeIdx;
        std::unordered_map<ResourceID, std::pair<uint32_t, uint32_t>> _buf_lifetimeIdx;

        std::vector<ManagedImage> _rt_phys;
        std::vector<ManagedImage> _dp_phys;
        std::vector<ManagedImage> _tex_phys;
        std::vector<ManagedBuffer> _buf_phys;
    };

    class ResMgr
    {
    public:
        ResourceID registerRenderTarget(const std::string& name, const FGImgInitInfo& props);
        ResourceID registerDepthStencil(const std::string& name, const FGImgInitInfo& props);
        ResourceID registerTexture(const std::string& name, const FGImgInitInfo& props);
        ResourceID registerBuffer(const std::string& name, const FGBufInitInfo& props);

        // use when a resource needs to write multiple times in several passes in a same graph 
        // returns the aliased resource id
        ResourceID aliasWriteRenderTarget(const std::string& src, std::string& alias);
        ResourceID aliasWriteDepthStencil(const std::string& src, std::string& alias);
        ResourceID aliasWriteTexture(const std::string& src, std::string& alias);
        ResourceID aliasWriteBuffer(const std::string& src, std::string& alias);    
    private:

        // phys resources
        Buffer createBuffer(const FGBufInitInfo& props);
        Image createImage(const FGImgInitInfo& props);
    };

    
} // namespace vkz