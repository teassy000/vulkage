#pragma once

namespace vkz
{
    enum class ResourceType : uint32_t
    {
        renderTarget = 0u,
        depthStencil = 1u,
        texture = 3u,
        buffer = 4u,
        NUM_OF_RESOURCE_TYPES,
    };

    struct ManagedBuffer2
    {
        uint32_t _id{ invalidID };
        std::string _name;
        size_t _size{ 0u };

        uint32_t _bucketIdx{ 0u };

        uint64_t _memory{ 0u };
        size_t _offset{ 0u };

        std::vector<uint32_t> _forceAlias;
    };

    struct ManagedImage2
    {
        uint32_t _id{ invalidID };
        std::string _name;
        uint32_t _x{ 0u };
        uint32_t _y{ 0u };
        uint32_t _z{ 1u };

        uint16_t _mipLevels{ 1u };
        uint16_t _padding{ 0u };
        uint32_t _bucketIdx{ 0u };

        std::vector<uint32_t> _forceAlias;
    };

    struct ResData
    {
        // registered:
        //    those resources are registered via register{resourceType}() function
        std::vector<uint32_t> _rt_registed;
        std::vector<uint32_t> _dp_registed;
        std::vector<uint32_t> _tex_registed;
        std::vector<uint32_t> _buf_registed;

        // in use:
        //    those resources are actually would contribute the final result.
        std::vector<uint32_t> _rt_inUse;
        std::vector<uint32_t> _dp_inUse;
        std::vector<uint32_t> _tex_inUse;
        std::vector<uint32_t> _buf_inUse;

        // aliased:
        //    those resources are aliased from other resources
        //    _rt_*_base: set from register{resource type}
        //    _rt_* : set form registerAlias{resource type}
        std::vector<uint32_t> _rt_forceAliased_base;
        std::vector<uint32_t> _dp_forceAliased_base;
        std::vector<uint32_t> _tex_forceAliased_base;
        std::vector<uint32_t> _buf_forceAliased_base;
        std::vector<std::vector<uint32_t>> _rt_forceAliased;
        std::vector<std::vector<uint32_t>> _dp_forceAliased;
        std::vector<std::vector<uint32_t>> _tex_forceAliased;
        std::vector<std::vector<uint32_t>> _buf_forceAliased;
        std::unordered_map< uint32_t, uint32_t> _aliasedCount;

        // lifetime
        std::vector<std::pair<const uint32_t, const uint32_t>> _rt_lifetimeIdx;
        std::vector<std::pair<const uint32_t, const uint32_t>> _dp_lifetimeIdx;
        std::vector<std::pair<const uint32_t, const uint32_t>> _tex_lifetimeIdx;
        std::vector<std::pair<const uint32_t, const uint32_t>> _buf_lifetimeIdx;

        // physical resources
        std::vector<ManagedImage2> _rt_phys;
        std::vector<ManagedImage2> _dp_phys;
        std::vector<ManagedImage2> _tex_phys;
        std::vector<ManagedBuffer2> _buf_phys;
    };

    class ResMgr
    {
    public:
        uint32_t registerRenderTarget(const std::string& name, const FGImgInitInfo& props);
        uint32_t registerDepthStencil(const std::string& name, const FGImgInitInfo& props);
        uint32_t registerTexture(const std::string& name, const FGImgInitInfo& props);
        uint32_t registerBuffer(const std::string& name, const FGBufInitInfo& props);

        // use when a resource needs to write multiple times in several passes in a same graph 
        // returns the aliased resource id
        uint32_t registerAliasRenderTarget(const std::string& src, std::string& alias);
        uint32_t registerAliasDepthStencil(const std::string& src, std::string& alias);
        uint32_t registerAliasTexture(const std::string& src, std::string& alias);
        uint32_t registerAliasBuffer(const std::string& src, std::string& alias);
    private:
        uint32_t registerAliasResrouce(std::string& alias, std::vector<uint32_t>& aliasBases, std::vector<std::vector<uint32_t>>& aliases, const std::string& srcName, const ResourceType type);

        void generateAliasName(const std::string& src, std::string& alias);
        ResourceType getResourceType(const uint32_t id);
        uint32_t getResourceID(const std::string& res);
        bool getAliasedResourceIdx(const uint32_t src, uint32_t& idx);
        uint32_t getAliasedCount(const uint32_t id);

    private:
        ResData _data;
    };

    
} // namespace vkz