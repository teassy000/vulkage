#pragma once


namespace vkz
{
    typedef uint32_t DependLevel;


    enum class RenderPassExeQueue : uint32_t
    {
        graphics = 0u,
        asyncCompute0 = 1u,
        asyncCompute1 = 2u,
        asyncCompute2 = 3u,
        NUM_OF_QUEUES = 4u,
    };

    // for barrier 
    struct FGResourceState
    {
        VkImageLayout           layout;
        VkAccessFlags2          access;
        VkPipelineStageFlags2   stage;
    };

    struct FGResInitInfo
    {
        std::string         name;
        ResourceID          id;
        FGResourceState     state;
    };

    struct FGBufInitInfo : FGResInitInfo
    {
        size_t             size;
    };

    struct FGImgInitInfo : FGResInitInfo
    {
        uint32_t        x{ 0u };
        uint32_t        y{ 0u };
        uint32_t        z{ 1u };
        uint16_t        mips{ 1u };

        VkFormat            format;
        VkImageUsageFlags   usage;
        VkImageType         type{ VK_IMAGE_TYPE_2D };
        VkImageViewType     viewType{ VK_IMAGE_VIEW_TYPE_2D };
    };

    struct PassInitInfo
    {
        RenderPassExeQueue queue;
        VkPipeline program;
    };

    class IFramegraph;

    class IPass
    {
    public:
        PassID pass(const std::string& name, const PassInitInfo& pii);
        
        void readRenderTarget(const std::string& name);
        void readDepthStencil(const std::string& name);
        void readTexture(const std::string& name);
        void readBuffer(const std::string& name);

        void writeRenderTarget(const std::string& name);
        void writeDepthStencil(const std::string& name);
        void writeTexture(const std::string& name);
        void writeBuffer(const std::string& name);

        void aliasWriteRenderTarget(const std::string& name);
        void aliasWriteDepthStencil(const std::string& name);
        void aliasWriteTexture(const std::string& name);
        void aliasWriteBuffer(const std::string& name);
    
    private:
        IFramegraph& _fg;
    };

    struct PassRenderData
    {
        std::string     _name;
        PassID          _id;
        VkPipeline      _pipeline;
        RenderPassExeQueue _queue{ RenderPassExeQueue::graphics };
        std::vector<ResourceID> _resources;
        
        // constants

        // resource state in current pass
        std::unordered_map<ResourceID, FGResourceState>     _resourceState;

        std::unordered_map<RenderPassExeQueue, PassID>      _nearestSyncPasses;
        // optimal pass to sync with, no redundant sync
        std::vector<PassID>                                 _passToSync; 

        std::vector<PassID>         _parentPassIDs;
        std::vector<PassID>         _childPassIDs;

        std::vector<ResourceID>     _inputResIDs;
        std::vector<ResourceID>     _outputResIDs;
    };

    struct BufBucket
    {
        uint32_t        idx;
        
        size_t          size;

        std::vector<ResourceID> _reses;
        std::vector<ResourceID> _forceAliasedReses;
    };

    struct ImgBucket
    {
        uint32_t        idx;

        uint32_t        x{ 0u };
        uint32_t        y{ 0u };
        uint32_t        z{ 1u };
        uint16_t        mips{ 1u };

        std::vector<ResourceID> _reses;
        std::vector<ResourceID> _forceAliasedReses;
    };

    struct FramegraphData
    {
        // all passes
        std::unordered_map<const std::string, PassID>       _nameToPass;
        std::unordered_map<const PassID, uint32_t>          _passIdx;

        std::vector<PassRenderData>                         _passData;

        // sorted and cut passes
        std::vector<PassID> _sortedPass;

        // resources
        std::unordered_map<const std::string, ResourceID>   _nameToResource;
        
        // actual physical resources, can be alias
        std::unordered_map<ResourceID, Image>               _imageData;
        std::unordered_map<ResourceID, Buffer>              _bufferData;

        // each resource has a initial state, for barrier usage
        std::unordered_map<ResourceID, FGResourceState>     _initialState;

        // for resource dependency
        std::unordered_map<ResourceID, std::pair<uint32_t, uint32_t>>   _bufferLifetimeIdx;
        std::unordered_map<ResourceID, std::pair<uint32_t, uint32_t>>   _imageLifetimeIdx;
    };

    class IFramegraph
    {
    public:
        PassID addPass(PassID pass);
        PassID findPass(const std::string& name);

        void aliasBuffer(const std::string& name);
        void aliasRenderTarget(const std::string& name);
        void aliasDepthStencil(const std::string& name);
        void aliasTexture(const std::string& name);

        void forceAlias(ResourceIDs); // affect after 


    public:
        void build();
        void execute(); // execute passes


    private:
        void sort(); // reverse dfs
        void optimize(); // alias and sync 
        void pre_execute(); // allocate resource and prepare passes, maybe merge passes, detect transist resources

        void reverseDFSVisit(const PassID currPass, std::vector<PassID>& sortedPasses);
        void reverseTraversalDFS();
        void buildDenpendencyLevel();
        void buildResourceLifetime();
        void buildResourceBuckets();

        void optimize_sync();
        void optimize_alias();
        void allocate_resources();

    public:
        void inline setFinalPass(PassID p)
        {
            _finalPass = p;
        }

        PassRenderData& getPassData(PassID p)
        {
            return _data._passData[p];
        }

    private:
        FramegraphData  _data;
        
        // graph properties
        PassID          _finalPass;
    };


}; // namespace vkz