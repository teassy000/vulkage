#pragma once


namespace vkz
{
    typedef uint32_t DependLevel;


    enum RenderPassExeQueue
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
        uint32_t            id;
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
        VkPipeline pipeline;
    };

    class Framegraph2;

    class IPass
    {
    public:
        uint32_t pass(const std::string& name, const PassInitInfo& pii);
        
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
        Framegraph2& _fg;
    };

    struct PassRenderData
    {
        std::string     _name;
        uint32_t        _id;
        VkPipeline      _pipeline;
        RenderPassExeQueue _queue{ RenderPassExeQueue::graphics };
        std::vector<uint32_t> _resources;
        
        // constants

        // resource state in current pass
        std::unordered_map<uint32_t, FGResourceState>     _resourceState;

        std::unordered_map<RenderPassExeQueue, uint32_t>      _nearestSyncPasses;
        // optimal pass to sync with, no redundant sync
        std::vector<uint32_t>                                 _passToSync; 

        std::vector<uint32_t>         _parentPassIDs;
        std::vector<uint32_t>         _childPassIDs;

        std::vector<uint32_t>     _inputResIDs;
        std::vector<uint32_t>     _outputResIDs;
    };

    struct BufBucket
    {
        uint32_t        idx;
        
        size_t          size;

        std::vector<uint32_t> _reses;
        std::vector<uint32_t> _forceAliasedReses;
    };

    struct ImgBucket
    {
        uint32_t        idx;

        uint32_t        x{ 0u };
        uint32_t        y{ 0u };
        uint32_t        z{ 1u };
        uint16_t        mips{ 1u };

        std::vector<uint32_t> _reses;
        std::vector<uint32_t> _forceAliasedReses;
    };

    struct FramegraphData
    {
        // all passes
        //std::unordered_map<const std::string, uint32_t>       _nameToPass;
        //std::unordered_map<const uint32_t, uint32_t>          _passIdx;

        std::vector<PassRenderData>                         _passData;

        // sorted and cut passes
        std::vector<uint32_t> _sortedPass;

        // each resource has a initial state, for barrier usage
        std::unordered_map<uint32_t, FGResourceState>     _initialState;
    };

    class Framegraph2
    {
    public:
        uint32_t addPass(uint32_t pass);
        uint32_t findPass(const std::string& name);

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

        void reverseDFSVisit(const uint32_t currPass, std::vector<uint32_t>& sortedPasses);
        void reverseTraversalDFS();
        void buildDenpendencyLevel();
        void buildResourceLifetime();
        void buildResourceBuckets();

        void optimize_sync();
        void optimize_alias();
        void allocate_resources();

    public:
        void inline setFinalPass(uint32_t p)
        {
            _finalPass = p;
        }

        PassRenderData& getPassData(uint32_t p)
        {
            return _data._passData[p];
        }

    private:
        FramegraphData  _data;
        
        // graph properties
        uint32_t          _finalPass;
    };


}; // namespace vkz