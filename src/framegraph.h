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


struct RenderPass
{
    std::vector<uint32_t> parentPassID;
    std::vector<uint32_t> childPassID;
    std::vector<ResourceID> inputResID;
    std::vector<ResourceID> outputResID;
};

struct FrameGraph
{
    std::vector<RenderPass> nodes;

    std::vector<ResourceID> readResources;
    std::vector<ResourceID> writenResources;
};


void buildGraph(Graph& graph);
void buildGraphWithRes(Graph& graph);
void addNode(Graph& graph, Node& node);
void TopologicalSort(Graph& graph, std::vector<uint32_t>& sortedNodeIdxes);
