#pragma once


// the resources that actual used read, write, target, etc
enum ResTag
{
    SingleFrame = 0x0,
    MultiFrameIn = 0x1 << 0, // represent the input point in current frame
    MultiFrameOut = 0x1 << 1, // represent the out point in current frame
    MultiFrame = MultiFrameIn | MultiFrameOut, // which means the resource is used in multi frames not followed the 
};
struct Res
{
    uint32_t resVar;
};

struct Node
{
    std::vector<uint32_t> imcomeNodeIdxes;
    std::vector<uint32_t> childNodeIdxes;

    std::vector<uint32_t> readResHandles;
    std::vector<uint32_t> writenResHandles;
};

struct Graph
{
    std::vector<Node> nodes;
    std::vector<Res>  reses;

    std::vector<uint32_t> inputResources;
    std::vector<uint32_t> outputResources;
};

void buildGraph(Graph& graph);
void buildGraphWithRes(Graph& graph);
void addNode(Graph& graph, Node& node);
void TopologicalSort(Graph& graph, std::vector<uint32_t>& sortedNodeIdxes);

typedef uint32_t PassID;

struct RenderPass
{
    uint32_t ID;
    std::string name;
    
    std::vector<PassID> parentPassIDs;
    std::vector<PassID> childPassIDs;

    std::vector<ResourceID> inputResIDs;
    std::vector<ResourceID> outputResIDs;
};

struct FrameGraph
{
    std::vector<PassID> passes;

    std::vector<ResourceID> readResources;
    std::vector<ResourceID> writenResources;
};


struct ResourceAlias
{
    std::vector<ResourceID> aliasesID;
};

struct ResTable
{
    std::vector<ResourceID> baseReses;
    std::vector<ResourceAlias> aliases;
    std::vector<ResourceID> activeAlias;
    std::vector<uint32_t> activeAliasesIdx;
};

struct ResourceMap
{
    // store all resources, no matter it is a alias or not
    std::unordered_map<ResourceID, Buffer> buffers;
    std::unordered_map<ResourceID, Image> images;

    // store all resources, no matter it is a alias or not
    std::unordered_map<ResourceID, std::string> bufferNames;
    std::unordered_map<ResourceID, std::string> imageNames;
};

struct FrameData
{
    std::unordered_map<PassID, RenderPass> passes;
    ResourceMap resmap;
};

void buildGraph2(FrameGraph& graph);
void getResourceAliases(ResourceAlias& output, ResourceID id, const ResTable& table);
void getResource(Image& image, Buffer& buffer, ResourceMap resMap, const ResourceID id);
void resetActiveAlias(ResTable& table); // call at the end of every frame
ResourceID getActiveAlias(const ResTable& table, const ResourceID baseResourceID);
ResourceID nextAlias(ResTable& table, const ResourceID id); // call after a pass wrote to a resource