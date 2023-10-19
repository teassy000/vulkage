#pragma once



namespace vkz
{
    typedef uint32_t PassID;
    typedef uint32_t DependLevel;
    const PassID invalidPassID = ~0u;
    const ResourceID invalidResourceID = ~0u;


    enum class RenderPassExeQueue : uint32_t
    {
        graphics = 0u,
        asyncCompute0 = 1u,
        asyncCompute1 = 2u,
        asyncCompute2 = 3u,
        NUM_OF_QUEUES = 4u,
    };

    struct DependedLevels
    {
        PassID _basedPass{invalidPassID};
        DependLevel _currLevel = ~0u;

        std::unordered_map<DependLevel, std::vector<PassID>> _dependPasses;
    };

    struct RenderPass
    {
        PassID _ID{ invalidPassID };
        RenderPassExeQueue _queue{ RenderPassExeQueue::graphics };
        std::string _name;

        std::unordered_map<RenderPassExeQueue, PassID> _nearestSyncPasses;

        std::vector<PassID> _plainNearestSyncPasses; // all nearest sync passes, including redundant sync

        std::vector<PassID> _passToSync; // optimal pass to sync with, no redundant sync

        std::vector<PassID> parentPassIDs;
        std::vector<PassID> childPassIDs;

        std::vector<ResourceID> inputResIDs;
        std::vector<ResourceID> outputResIDs;
    };

    struct ResourceAlias
    {
        std::vector<ResourceID> aliasesID;
    };

    struct ResourceTable
    {
        std::vector<ResourceID> baseReses;
        std::vector<ResourceAlias> aliases;
        std::vector<ResourceID> activeAlias;
        std::vector<uint32_t> activeAliasesIdx;
    };

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


    // since this is not for virtual texture, so only same size image can be aliased
    struct ImageProps
    {
        std::string name;

        uint32_t x{ 0u }, y{ 0u }, z{ 1u };
        uint16_t mipLevels{ 1u };

        VkImageUsageFlags usage;
        VkImageType       type{ VK_IMAGE_TYPE_2D };
        VkImageLayout     layout{ VK_IMAGE_LAYOUT_GENERAL };
        VkImageViewType   viewType{ VK_IMAGE_VIEW_TYPE_2D };
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


    struct BufferBucket
    {
        uint32_t _idx;
        size_t _size;
        std::vector<ResourceID> _reses;
        std::vector<ResourceID> _forceAliasedReses;
    };

    struct ImageBucket
    {
        uint32_t _idx;

        uint32_t _x{ 0u };
        uint32_t _y{ 0u };
        uint32_t _z{ 1u };
        
        std::vector<ResourceID> _reses;
        std::vector<ResourceID> _forceAliasedReses;
    };

    struct ResourceMap
    {
        // store all resources, no matter it is a alias or not
        std::unordered_map<ResourceID, Buffer> _buffers;
        std::unordered_map<ResourceID, Image> _images;

        std::unordered_map<ResourceID, ManagedBuffer> _managedBuffers;
        std::unordered_map<ResourceID, ManagedImage> _managedImages;

        // store all resources, no matter it is a alias or not
        std::unordered_map<ResourceID, std::string> bufferNames;
        std::unordered_map<ResourceID, std::string> imageNames;

        std::unordered_map<ResourceID, std::pair<PassID, PassID>> bufferLifetime;
        std::unordered_map<ResourceID, std::pair<uint32_t, uint32_t>> bufferLifetimeIdx;
        std::unordered_map<ResourceID, std::pair<PassID, PassID>> imageLifetime;
        std::unordered_map<ResourceID, std::pair<uint32_t, uint32_t>> imageLifetimeIdx;
    };

    struct FrameGraphData
    {
        std::unordered_map<PassID, RenderPass> _passes;
        ResourceMap _resmap;
    };

    struct FrameGraph
    {
        PassID _outputPass{ invalidPassID };
        uint64_t _detectedQueueCount{ 1 };
        std::unordered_map<PassID, DependedLevels> _dependedLevelsPerPass;

        std::vector<PassID> _linearVisitedPasses;

        std::unordered_map<RenderPassExeQueue, std::vector<PassID>> _passesInQueue;
        std::vector<PassID> _passes;

        std::vector<ResourceID> _readonlyReses;
        std::vector<ResourceID> _multiFrameReses;

        std::vector<std::vector<ResourceID>> _forceAliasBuffers;
        std::vector<std::vector<ResourceID>> _forceAliasImages;
    };

    
    void DFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<PassID, bool>& visited, std::unordered_map<PassID, bool>& onStack, std::vector<PassID>& sortedPasses);
    void DFS(FrameGraph& graph, FrameGraphData& fd, std::vector<PassID>& sortedPasses);
    void reverseDFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<PassID, bool >& visited, std::unordered_map<PassID, bool>& onStack, std::vector<uint32_t>& sortedPassIDs);
    void reverseTraversalDFS(FrameGraph& graph, FrameGraphData& fd, const PassID startPass, std::vector<uint32_t>& sortedPassIDs);
    void TopologicalSort(FrameGraph& graph, FrameGraphData& fd, std::vector<PassID>& sortedPasses);

    void buildGraph(FrameGraph& graph);
    void getResourceAliases(ResourceAlias& output, ResourceID id, const ResourceTable& table);
    void getResource(Image& image, Buffer& buffer, ResourceMap resMap, const ResourceID id);
    void resetActiveAlias(ResourceTable& table); // call at the end of every frame
    ResourceID getActiveAlias(const ResourceTable& table, const ResourceID baseResourceID);
    ResourceID nextAlias(ResourceTable& table, const ResourceID id); // call after a pass wrote to a resource

    typedef std::initializer_list<ResourceID> InputResources;
    typedef std::initializer_list<ResourceID> OutputResources;
    ResourceID registerBuffer(FrameGraphData& fd, const std::string& name, const size_t size);

    ResourceID registerImage(FrameGraphData& fd, const ImageProps& props);
    PassID registerPass(FrameGraphData& fd, FrameGraph& fg, const std::string name, InputResources read, OutputResources write, RenderPassExeQueue queue = RenderPassExeQueue::graphics);
};
