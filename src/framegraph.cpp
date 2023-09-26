#include "common.h"
#include "resources.h"

#include "framegraph.h"

void addNode(Graph& graph, Node& node)
{
    graph.nodes.push_back(node);
    graph.inputResources.insert(graph.inputResources.end(), node.readResHandles.begin(), node.readResHandles.end());
    graph.outputResources.insert(graph.outputResources.end(), node.writenResHandles.begin(), node.writenResHandles.end());
}

void DFSVisit(Graph& graph, uint32_t currentNodeIdx, std::vector<bool>& visited, std::vector<bool>& onStack, std::vector<uint32_t>& sortedNodeIdxes)
{
    visited[currentNodeIdx] = true;
    onStack[currentNodeIdx] = true;
    for (uint32_t adjNodeIdx : graph.nodes[currentNodeIdx].childNodeIdxes)
    {
        if (!visited[adjNodeIdx])
        {
            DFSVisit(graph, adjNodeIdx, visited, onStack, sortedNodeIdxes);
        }
        else if (onStack[adjNodeIdx])
        {
            // cycle detected
            assert(false);
        }
    }
    sortedNodeIdxes.push_back(currentNodeIdx);
    onStack[currentNodeIdx] = false;
}

void DFS(Graph& graph, std::vector<uint32_t>& sortedNodeIdxes)
{
    uint32_t nodeCount = (uint32_t)graph.nodes.size();
    std::vector<bool> visited(nodeCount, false);
    std::vector<bool> onStack(nodeCount, false);
    uint32_t currentNodeIdx = 0;

    while (currentNodeIdx < nodeCount)
    {
        if (!visited[currentNodeIdx])
        {
            DFSVisit(graph, currentNodeIdx, visited, onStack, sortedNodeIdxes);
        }
        currentNodeIdx++;
    }
}

void reverseDFSVisit(Graph& graph, uint32_t currentNodeIdx, std::vector<bool>& visited, std::vector<bool>& onStack, std::vector<uint32_t>& sortedNodeIdxes)
{
    visited[currentNodeIdx] = true;
    onStack[currentNodeIdx] = true;
    for (uint32_t incomeNodeIdx : graph.nodes[currentNodeIdx].imcomeNodeIdxes)
    {
        if (!visited[incomeNodeIdx])
        {
            reverseDFSVisit(graph, incomeNodeIdx, visited, onStack, sortedNodeIdxes);
        }
        else if (onStack[incomeNodeIdx])
        {
            // cycle detected
            assert(false);
        }
    }
    sortedNodeIdxes.push_back(currentNodeIdx);
    onStack[currentNodeIdx] = false;
}

// reverse traversal deep first search
void reverseTraversalDFS(Graph& graph, uint32_t startNodeIdx, std::vector<uint32_t>& visitNodeIdxes)
{
    // node not exist
    assert(startNodeIdx <= graph.nodes.size() );
    

    uint32_t nodeCount = (uint32_t)graph.nodes.size();
    std::vector<bool> visited(nodeCount, false);
    std::vector<bool> onStack(nodeCount, false);

    uint32_t currentNodeIdx = startNodeIdx;


    if (!visited[currentNodeIdx])
    {
        reverseDFSVisit(graph, currentNodeIdx, visited, onStack, visitNodeIdxes);
    }

}

void TopologicalSort(Graph& graph, std::vector<uint32_t>& sortedNodeIdxes)
{
    DFS(graph, sortedNodeIdxes);

    std::reverse(sortedNodeIdxes.begin(), sortedNodeIdxes.end());
}

void buildGraphWithRes(Graph& graph)
{
    uint32_t resCount = 16;
    for (uint32_t i = 0; i < resCount; ++i)
    {
        Res res;
        res.resVar = i;
        graph.reses.push_back(res);
    }

    // ping / pong buffers
    // 0 / 7 next frame would be 7 / 0  pyramid
    // 4 / 6 render target
    // (1 / 8) / 9  : 9 is from last frame, 1 / 8 is from current frame

    Node node0;
    node0.readResHandles = { 0, 9, 2, 3 };
    node0.writenResHandles = { 1 };

    Node node1;
    node1.readResHandles = { 0, 1, 2, 3 };
    node1.writenResHandles = { 4, 5 };

    Node node2;
    node2.readResHandles = { 7, 1, 2, 3 };
    node2.writenResHandles = { 8 };

    Node node3;
    node3.readResHandles = { 7, 8, 2, 3 };
    node3.writenResHandles = { 6 };

    Node node4;
    node4.readResHandles = { 6 };
    node4.writenResHandles = { 10 };

    Node node5;
    node5.readResHandles = { 5 };
    node5.writenResHandles = { 7 };

    Node node6;
    node6.readResHandles = { 7 };
    node6.writenResHandles = { 13 };


    std::vector<Node> nodes;
    nodes.push_back(node0);
    nodes.push_back(node1);
    nodes.push_back(node2);
    nodes.push_back(node3);
    nodes.push_back(node4);
    nodes.push_back(node5);
    nodes.push_back(node6);

    // collect all resources that 
    std::vector<uint32_t> readResIdx;
    std::vector<uint32_t> outResIdx;
    std::vector<uint32_t> outResNodeIdx;
    for (uint32_t i = 0; i < nodes.size(); ++i)
    {
        readResIdx.insert(readResIdx.end(), nodes[i].readResHandles.begin(), nodes[i].readResHandles.end());
        outResIdx.insert(outResIdx.end(), nodes[i].writenResHandles.begin(), nodes[i].writenResHandles.end());
        std::vector<uint32_t> tmp(nodes[i].writenResHandles.size(), i);
        outResNodeIdx.insert(outResNodeIdx.end(), tmp.begin(), tmp.end());

        assert(outResIdx.size() == outResNodeIdx.size());
    }

    // build graph based on resource dependency
    for (uint32_t i = 0; i < nodes.size(); ++i)
    {
        // for each node, traverse all readResHandles then match the parentNode
        for (uint32_t j = 0; j < nodes[i].readResHandles.size(); ++j)
        {
            uint32_t resIdx = nodes[i].readResHandles[j];
            std::vector<uint32_t>::iterator it = std::find(outResIdx.begin(), outResIdx.end(), resIdx);
            if (it == outResIdx.end())
                continue;

            uint32_t parentNodeIdx = (uint32_t)(it - outResIdx.begin());
            uint32_t outNodeIdx = outResNodeIdx[parentNodeIdx];
            Node& outNode = nodes[outNodeIdx];
            outNode.childNodeIdxes.push_back(i); // current node is the child node of outNode
            nodes[i].imcomeNodeIdxes.push_back(outNodeIdx);
        }
    }

    addNode(graph, nodes[0]);
    addNode(graph, nodes[1]);
    addNode(graph, nodes[2]);
    addNode(graph, nodes[3]);
    addNode(graph, nodes[4]);
    addNode(graph, nodes[5]);
    addNode(graph, nodes[6]);


    std::vector<uint32_t> sortedNodeIdxes;
    TopologicalSort(graph, sortedNodeIdxes);

    // a reverse traversal DFS can be used all visited nodes
    std::vector<uint32_t> visitedNodeIdxes;
    reverseTraversalDFS(graph, 4, visitedNodeIdxes);
}

// Pass =======================================
//    |- Resources
//    |         |- in resource
//    |         |- out resource
//    |- Connections
//    |         |- parent passes
//    |         |- child passes
//=============================================
// a. in/out resources are required before build the frame graph, should be set manually
// b. connections are computed by the frame graph builder, depends on the resource dependency
// c. a topology sort is used for get the execute order of passes
// d. a reverse traversal DFS is used for get passes that contribute the final output resource

void buildGraph2(FrameGraph& frameGraph)
{

}


void buildGraph(Graph& graph)
{
    buildGraphWithRes(graph);
}
