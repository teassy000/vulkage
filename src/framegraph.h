#pragma once

namespace vkz
{
    typedef uint32_t uint32_t;
    typedef uint32_t DependLevel;
    const uint32_t invalidPassID = ~0u;
    const uint32_t invalidID = ~0u;


    enum class RenderPassExeQueue : uint32_t
    {
        graphics = 0u,
        asyncCompute0 = 1u,
        asyncCompute1 = 2u,
        asyncCompute2 = 3u,
        NUM_OF_QUEUES = 4u,
    };

    struct ResourceState
    {
        VkAccessFlags2 _vkaccess{ 0u };
        VkPipelineStageFlags2 _vkstage{ 0u };
    };

    struct BufferState : public ResourceState
    {
    };

    struct ImageState : public ResourceState
    {
        // image layout 
        VkImageLayout _vklayout{ VK_IMAGE_LAYOUT_UNDEFINED };
    };

    struct BufferProps
    {
        std::string name{"invalid"};
        size_t size{ 0 };

        BufferState initState;
        VkBufferUsageFlags usage;
    };

    struct ManagedBuffer
    {
        uint32_t _id{ invalidID };
        std::string _name;
        size_t _size{ 0u };

        uint32_t _bucketIdx{ 0u };

        uint64_t _memory{ 0u };
        size_t _offset{ 0u };

        std::vector<uint32_t> _forceAlias;
    };


    // only same size image can be aliased
    struct ImageProps
    {
        std::string name;

        uint32_t x{ 0u }, y{ 0u }, z{ 1u };
        uint16_t mipLevels{ 1u };

        ImageState          state;
        VkImageUsageFlags   usage;
        VkImageType         type{ VK_IMAGE_TYPE_2D };
        VkImageViewType     viewType{ VK_IMAGE_VIEW_TYPE_2D };
    };


    struct ManagedImage
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


    struct BufferBucket
    {
        uint32_t _idx;
        size_t _size;
        std::vector<uint32_t> _reses;
        std::vector<uint32_t> _forceAliasedReses;
    };

    struct ImageBucket
    {
        uint32_t _idx;

        uint32_t _x{ 0u };
        uint32_t _y{ 0u };
        uint32_t _z{ 1u };
        
        std::vector<uint32_t> _reses;
        std::vector<uint32_t> _forceAliasedReses;
    };

    struct RenderPass
    {
        uint32_t _ID{ invalidPassID };
        RenderPassExeQueue _queue{ RenderPassExeQueue::graphics };
        std::string _name;

        std::unordered_map<RenderPassExeQueue, uint32_t> _nearestSyncPasses;
        std::vector<uint32_t> _passToSync; // optimal pass to sync with, no redundant sync

        std::vector<uint32_t> parentPassIDs;
        std::vector<uint32_t> childPassIDs;

        std::vector<uint32_t> inputResIDs;
        std::vector<uint32_t> outputResIDs;
    };

    struct ResourceMap
    {
        // store all resources, no matter it is a alias or not
        std::unordered_map<uint32_t, ManagedBuffer> _managedBuffers;
        std::unordered_map<uint32_t, ManagedImage> _managedImages;

        // store all resources, no matter it is a alias or not
        std::unordered_map<uint32_t, std::string> bufferNames;
        std::unordered_map<uint32_t, std::string> imageNames;

        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> bufferLifetime;
        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> bufferLifetimeIdx;
        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> imageLifetime;
        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> imageLifetimeIdx;
    };

    struct FrameGraphData
    {
        std::unordered_map<uint32_t, RenderPass> _passes;
        ResourceMap _resmap;
    };

    struct DependedLevels
    {
        uint32_t _basedPass{ invalidPassID };
        DependLevel _currLevel = ~0u;

        std::unordered_map<DependLevel, std::vector<uint32_t>> _dependPasses;
    };

    struct FrameGraph
    {
        uint32_t _outputPass{ invalidPassID };
        uint64_t _detectedQueueCount{ 1 };
        std::unordered_map<uint32_t, DependedLevels> _dependedLevelsPerPass;

        // used for generate dependency level
        std::vector<uint32_t> _linearVisitedPasses;
        std::unordered_map<RenderPassExeQueue, std::vector<uint32_t>> _passesInQueue; // transition passID to queue
        
        // all passes in the graph
        std::vector<uint32_t> _passes;

        std::vector<uint32_t> _multiFrameReses;

        std::vector<std::vector<uint32_t>> _forceAliasBuffers;
        std::vector<std::vector<uint32_t>> _forceAliasImages;
    };

    
    void DFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<uint32_t, bool>& visited, std::unordered_map<uint32_t, bool>& onStack, std::vector<uint32_t>& sortedPasses);
    void DFS(FrameGraph& graph, FrameGraphData& fd, std::vector<uint32_t>& sortedPasses);
    void reverseDFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<uint32_t, bool >& visited, std::unordered_map<uint32_t, bool>& onStack, std::vector<uint32_t>& sortedPassIDs);
    void reverseTraversalDFS(FrameGraph& graph, FrameGraphData& fd, const uint32_t startPass, std::vector<uint32_t>& sortedPassIDs);
    void TopologicalSort(FrameGraph& graph, FrameGraphData& fd, std::vector<uint32_t>& sortedPasses);

    void buildGraph(FrameGraph& graph);

    uint32_t registerBuffer(FrameGraphData& fd, const BufferProps& props);
    uint32_t registerBuffer(FrameGraphData& fd, const std::string& name, const size_t size);
    uint32_t registerImage(FrameGraphData& fd, const ImageProps& props);
    
    uint32_t registerPass(FrameGraphData& fd, FrameGraph& fg, const std::string name, ResourceIDs read, ResourceIDs write, RenderPassExeQueue queue = RenderPassExeQueue::graphics);
}
