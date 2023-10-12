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

    struct FGBuffer
    {
        ResourceID _id{ invalidResourceID };
        std::string _name;

        size_t _size{ 0u };
    };

    

    struct ResourceMap
    {
        // store all resources, no matter it is a alias or not
        std::unordered_map<ResourceID, Buffer> buffers;
        std::unordered_map<ResourceID, Image> images;

        // store all resources, no matter it is a alias or not
        std::unordered_map<ResourceID, std::string> bufferNames;
        std::unordered_map<ResourceID, std::string> imageNames;

        std::unordered_map<ResourceID, size_t> bufferSize;
        std::unordered_map<ResourceID, size_t> imageSize;

        std::unordered_map<ResourceID, std::pair<PassID, PassID>> bufferLifetime;
        std::unordered_map<ResourceID, std::pair<PassID, PassID>> imageLifetime;
    };

    struct FrameGraphData
    {
        std::unordered_map<PassID, RenderPass> _passes;
        ResourceMap resmap;
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
};
