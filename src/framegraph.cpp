#include "common.h"
#include "resources.h"

#include "framegraph.h"

#include <string>
#include <map>

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

void buildGraph(Graph& graph)
{
    buildGraphWithRes(graph);
}

void getResourceAliases(ResourceAlias& output, ResourceID id, const ResTable& table)
{
    ResourceAlias result = {};
    uint32_t resIdx = -1;
    const std::vector<ResourceID>& reses = table.baseReses;
    for (uint32_t idx = 0; idx != reses.size(); ++idx)
    {
        if (reses[idx] == id)
        {
            resIdx = idx;
            break;
        }
    }

    if (resIdx == -1)
    {
        assert("!no resource found!");
        return;
    }

    const std::vector<ResourceAlias>& aliases = table.aliases;
    const ResourceAlias& alias = aliases[resIdx];


    result.aliasesID.assign(alias.aliasesID.begin(), alias.aliasesID.end());
}

void getResource(Image& image, Buffer& buffer, ResourceMap resMap, const ResourceID id)
{
    if (id & 0x80000000) {
        buffer = resMap.buffers[id];
    }
    else {
        image = resMap.images[id];
    }
}

void resetActiveAlias(ResTable& table)
{
    const std::vector<ResourceID>& reses = table.activeAlias;
    for (uint32_t idx = 0; idx != reses.size(); ++idx)
    {
        table.activeAlias[idx] = table.baseReses[idx];
        table.activeAliasesIdx[idx] = -1;
    }
}

ResourceID getActiveAlias(const ResTable& table, const ResourceID baseResourceID)
{
    ResourceID result = baseResourceID;

    uint32_t resIdx = -1;

    const std::vector<ResourceID>& bases = table.baseReses;
    for (uint32_t idx = 0; idx != bases.size(); ++idx)
    {
        if (bases[idx] == baseResourceID)
        {
            resIdx = idx;
            result = table.activeAlias[idx];
            break;
        }
    }

    if (resIdx == -1)
    {
        assert("!no resource found!");
    }

    return result;

}

ResourceID nextAlias(ResTable& table, const ResourceID baseResourceID)
{
    ResourceID result = baseResourceID;

    uint32_t resIdx = -1;
    const std::vector<ResourceID>& bases = table.baseReses;
    for (uint32_t idx = 0; idx != bases.size(); ++idx)
    {
        if (bases[idx] == baseResourceID)
        {
            resIdx = idx;
            break;
        }
    }

    if (resIdx == -1)
    {
        assert("!no base resource found!");
        return result;
    }

    // update active alias and idx
    const std::vector<ResourceID>& aliases = table.aliases[resIdx].aliasesID;
    if ((table.activeAliasesIdx[resIdx] + 1) < aliases.size())
    {
        table.activeAliasesIdx[resIdx]++;
        ResourceID nextAliasID = aliases[table.activeAliasesIdx[resIdx]];
        table.activeAlias[resIdx] = aliases[nextAliasID];
    }
    else
    {
        assert("!no more alias!");
        table.activeAliasesIdx[resIdx] = -1;
        table.activeAlias[resIdx] = baseResourceID;
    }

    result = table.activeAlias[resIdx];


    return result;
}

void setupResources(FrameData& fd, FrameGraph& fg)
{
    ResourceID buf_vtx = 0x0001;
    ResourceID buf_idx = 0x0002;
    ResourceID buf_mesh = 0x0003;
    ResourceID buf_mlt = 0x0004;
    ResourceID buf_mltData = 0x0005;
    ResourceID buf_transData = 0x0006;
    ResourceID buf_mDr = 0x0007;
    ResourceID buf_mDrCmd = 0x0008;
    ResourceID buf_mDrCmd_a1 = 0x0018;
    ResourceID buf_mDrCmd_a2 = 0x0028;
    ResourceID buf_mDrCC = 0x0009;
    ResourceID buf_mDrCC_a1 = 0x0019;
    ResourceID buf_mDrCC_a2 = 0x0029;
    ResourceID buf_mDrVis = 0x000a;
    ResourceID buf_mDrVis_a1 = 0x001a;
    ResourceID buf_mDrVis_a2 = 0x002a;
    ResourceID buf_mltVis = 0x000b;
    ResourceID buf_mltVis_a1 = 0x001b;
    ResourceID buf_mltVis_a2 = 0x002b;

    ResourceID img_ct = 0x1001;
    ResourceID img_ct_a1 = 0x1011;
    ResourceID img_ct_a2 = 0x1021;
    ResourceID img_dt = 0x1002;
    ResourceID img_dt_a1 = 0x1012;
    ResourceID img_dt_a2 = 0x1022;
    ResourceID img_rt = 0x1003;
    ResourceID img_rt_a1 = 0x1013;
    ResourceID img_rt_a2 = 0x1023;
    ResourceID img_dpy = 0x1004;
    ResourceID img_dpy_a1 = 0x1014;

    ResourceMap& resMap = fd.resmap;
    resMap.bufferNames.insert({ buf_vtx, "vertex buffer" });
    resMap.bufferNames.insert({ buf_idx, "index buffer" });
    resMap.bufferNames.insert({ buf_mesh, "mesh buffer" });
    resMap.bufferNames.insert({ buf_mlt, "meshlet buffer" });
    resMap.bufferNames.insert({ buf_mltData, "meshlet Data buffer" });
    resMap.bufferNames.insert({ buf_transData, "transform buffer" });
    resMap.bufferNames.insert({ buf_mDr, "mesh draw buffer" });
    resMap.bufferNames.insert({ buf_mDrCmd, "mesh draw command buffer" });
    resMap.bufferNames.insert({ buf_mDrCmd_a1, "A1: mesh draw command buffer" });
    resMap.bufferNames.insert({ buf_mDrCmd_a2, "A2: mesh draw command buffer" });
    resMap.bufferNames.insert({ buf_mDrCC, "mesh draw command count buffer" });
    resMap.bufferNames.insert({ buf_mDrCC_a1, "A1: mesh draw command count buffer" });
    resMap.bufferNames.insert({ buf_mDrCC_a2, "A2: mesh draw command count buffer" });
    resMap.bufferNames.insert({ buf_mDrVis, "mesh draw visibility buffer" });
    resMap.bufferNames.insert({ buf_mDrVis_a1, "A1: mesh draw visibility buffer" });
    resMap.bufferNames.insert({ buf_mDrVis_a2, "A2: mesh draw visibility buffer" });
    resMap.bufferNames.insert({ buf_mltVis, "meshlet visibility buffer" });
    resMap.bufferNames.insert({ buf_mltVis_a1, "A1: meshlet visibility buffer" });
    resMap.bufferNames.insert({ buf_mltVis_a2, "A2: meshlet visibility buffer" });

    resMap.imageNames.insert({ img_ct, "color target" });
    resMap.imageNames.insert({ img_ct_a1, "A1: color target" });
    resMap.imageNames.insert({ img_ct_a2, "A2: color target" });
    resMap.imageNames.insert({ img_dt, "depth target" });
    resMap.imageNames.insert({ img_dt_a1, "A1: depth target" });
    resMap.imageNames.insert({ img_dt_a2, "A2: depth target" });
    resMap.imageNames.insert({ img_rt, "render target" });
    resMap.imageNames.insert({ img_rt_a1, "A1: render target" });
    resMap.imageNames.insert({ img_rt_a2, "A2: render target" });
    resMap.imageNames.insert({ img_dpy, "depth pyramid" });
    resMap.imageNames.insert({ img_dpy_a1, "A1: depth pyramid" });

    RenderPass earlycullPass{};
    earlycullPass.ID = 0x2001;
    earlycullPass.name = "early cull pass";
    earlycullPass.inputResIDs = { buf_mesh, buf_transData, buf_mDr, buf_mDrCC, buf_mDrVis , img_dpy};
    earlycullPass.outputResIDs = { buf_mDrCmd, buf_mDrCC_a1, buf_mDrVis_a1 };

    RenderPass earlydrawPass{};
    earlydrawPass.ID =  0x2002;
    earlydrawPass.name = "early draw pass";
    earlydrawPass.inputResIDs = { buf_vtx, buf_idx, buf_mesh, buf_mltData, buf_transData, buf_mDr, buf_mDrCmd, buf_mltVis , img_dpy};
    earlydrawPass.outputResIDs = { buf_mltVis_a1, img_ct, img_dt };

    RenderPass pyramidPass{};
    pyramidPass.ID = 0x2003;
    pyramidPass.name = "pyramid pass";
    pyramidPass.inputResIDs = { img_dt };
    pyramidPass.outputResIDs = { img_dpy_a1 };

    RenderPass latecullPass{};
    latecullPass.ID = 0x2004;
    latecullPass.name = "late cull pass";
    latecullPass.inputResIDs = { buf_mesh, buf_transData, buf_mDr, buf_mDrCC_a1, buf_mDrVis_a1, img_dpy_a1 };
    latecullPass.outputResIDs = { buf_mDrCmd_a1, buf_mDrCC_a2, buf_mDrVis_a2 };

    RenderPass latedrawPass{};
    latedrawPass.ID = 0x2005;
    latedrawPass.name = "late draw pass";
    latedrawPass.inputResIDs = { buf_vtx, buf_idx, buf_mesh, buf_mltData, buf_transData, buf_mDr, buf_mDrCmd_a1, buf_mltVis_a1, img_dpy_a1 };
    latedrawPass.outputResIDs = { buf_mltVis_a2, img_ct_a1, img_dt_a1 };

    RenderPass copyPass{};
    copyPass.ID = 0x2006;
    copyPass.name = "copy pass";
    copyPass.inputResIDs = { img_ct_a1 };
    copyPass.outputResIDs = { img_rt };

    RenderPass UIPass{};
    UIPass.ID = 0x2007;
    UIPass.name = "UI pass";
    UIPass.inputResIDs = { img_rt };
    UIPass.outputResIDs = { img_rt_a1 };


    fg.passes.push_back(earlycullPass.ID);
    fg.passes.push_back(earlydrawPass.ID);
    fg.passes.push_back(pyramidPass.ID);
    fg.passes.push_back(latecullPass.ID);
    fg.passes.push_back(latedrawPass.ID);
    fg.passes.push_back(copyPass.ID);
    fg.passes.push_back(UIPass.ID);

    fd.passes.insert({ earlycullPass.ID, earlycullPass });
    fd.passes.insert({ earlydrawPass.ID, earlydrawPass });
    fd.passes.insert({ pyramidPass.ID, pyramidPass });
    fd.passes.insert({ latecullPass.ID, latecullPass });
    fd.passes.insert({ latedrawPass.ID, latedrawPass });
    fd.passes.insert({ copyPass.ID, copyPass });
    fd.passes.insert({ UIPass.ID, UIPass });
}

void DFSVisit(FrameData& fd, uint32_t currentPassID, std::unordered_map<PassID, bool>& visited, std::unordered_map<PassID, bool>& onStack, std::vector<PassID>& sortedPasses)
{
    visited[currentPassID] = true;
    onStack[currentPassID] = true;

    RenderPass& rp = fd.passes[currentPassID];

    for (uint32_t adjPassID : rp.childPassIDs)
    {
        if (!visited[adjPassID])
        {
            DFSVisit(fd, adjPassID, visited, onStack, sortedPasses);
        }
        else if (onStack[adjPassID])
        {
            // cycle detected
            assert(false);
        }
    }
    sortedPasses.push_back(currentPassID);
    onStack[currentPassID] = false;
}

void DFS(FrameGraph& graph, FrameData& fd, std::vector<PassID>& sortedPasses)
{
    uint32_t passCount = (uint32_t)graph.passes.size();
    std::unordered_map<PassID, bool> visited{};
    std::unordered_map<PassID, bool> onStack{};

    for (uint32_t i =0; i < passCount; ++i)
    {
        PassID id = graph.passes[i];
        visited.insert({ id, false });
        onStack.insert({ id, false });
    }

    for (PassID id : graph.passes)
    {
        if (!visited[id])
        {
            DFSVisit(fd, id, visited, onStack, sortedPasses);
        }
    }
}

void reverseDFSVisit(FrameData& fd, uint32_t currentPassID, std::unordered_map<PassID, bool >& visited, std::unordered_map<PassID, bool>& onStack, std::vector<uint32_t>& sortedPassIDs)
{
    visited[currentPassID] = true;
    onStack[currentPassID] = true;

    RenderPass& rp = fd.passes[currentPassID];

    for (uint32_t parentPassID : rp.parentPassIDs)
    {
        if (!visited[parentPassID])
        {
            reverseDFSVisit(fd, parentPassID, visited, onStack, sortedPassIDs);
        }
        else if (onStack[parentPassID])
        {
            // cycle detected
            assert(false);
        }
    }
    sortedPassIDs.push_back(currentPassID);
    onStack[currentPassID] = false;
}

// reverse traversal deep first search
void reverseTraversalDFS(FrameGraph& graph, FrameData& fd, uint32_t startPass, std::vector<uint32_t>& sortedPassIDs)
{
    // node not exist
    assert(std::find(graph.passes.begin(), graph.passes.end(), startPass) != graph.passes.end());

    uint32_t passCount = (uint32_t)graph.passes.size();
    std::unordered_map<PassID, bool> visited{};
    std::unordered_map<PassID, bool> onStack{};

    for (uint32_t i = 0; i < passCount; ++i)
    {
        PassID id = graph.passes[i];
        visited.insert({ id, false });
        onStack.insert({ id, false });
    }

    uint32_t currentPass = startPass;


    if (!visited[currentPass])
    {
        reverseDFSVisit(fd, currentPass, visited, onStack, sortedPassIDs);
    }

}

void TopologicalSort(FrameGraph& graph, FrameData& fd, std::vector<PassID>& sortedPasses)
{
    DFS(graph, fd, sortedPasses);

    std::reverse(sortedPasses.begin(), sortedPasses.end());
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
    FrameGraph fg{};
    FrameData fd{};
    setupResources(fd, fg);

    // make a plain table that can find producer of a resource easily.
    std::vector<ResourceID> all_outResID;
    std::vector<PassID> all_parentPassID;
    for (PassID passID : fg.passes)
    {
        RenderPass& rp = fd.passes[passID];

        all_outResID.insert(all_outResID.end(), rp.outputResIDs.begin(), rp.outputResIDs.end());
        std::vector<uint32_t> tmp(rp.outputResIDs.size(), passID);
        all_parentPassID.insert(all_parentPassID.end(), tmp.begin(), tmp.end());

        assert(all_outResID.size() == all_parentPassID.size());
    }

    // build graph based on resource dependency
    for (PassID passID : fg.passes)
    {
        RenderPass& rp = fd.passes[passID];

        for (uint32_t j = 0; j < rp.inputResIDs.size(); ++j)
        {
            ResourceID currentResID = rp.inputResIDs[j];
            std::vector<ResourceID>::iterator it = std::find(all_outResID.begin(), all_outResID.end(), currentResID);
            if (it == all_outResID.end())
                continue; // not found producer for current resource, might be a read only resource

            PassID parentPassID = all_parentPassID[it - all_outResID.begin()];
            rp.parentPassIDs.push_back(parentPassID); // set parent pass for current pass
            fd.passes[parentPassID].childPassIDs.push_back(passID); // set child pass back
        }
    }

    // topology sort
    std::vector<PassID> sortedPasses;
    TopologicalSort(fg, fd, sortedPasses);

    {
        printf("topology sorted: \n");
        for (PassID passID : sortedPasses)
        {
            printf("\t pass: %s\n", fd.passes[passID].name.c_str());
        }
    }


    // reverse traversal DFS
    std::vector<PassID> visitedPasses;
    reverseTraversalDFS(fg, fd, 0x2007, visitedPasses);

    {
        printf("visited pass: \n");
        for (PassID passID : visitedPasses)
        {
            printf("\t pass: %s\n", fd.passes[passID].name.c_str());
        }
    }

    {
        // build lifecycle table
        std::map<ResourceID, std::vector<PassID>> lifecycleTable;
        for (PassID passID : fg.passes)
        {
             RenderPass& rp = fd.passes[passID];
             for (ResourceID resID : rp.inputResIDs)
             {
                std::vector<PassID>& passIDs = lifecycleTable[resID];
                passIDs.push_back(passID);
             }
             /*
             for (ResourceID resID : rp.outputResIDs)
             {
                std::vector<PassID>& passIDs = lifecycleTable[resID];
                passIDs.push_back(passID);
             }
             */
        }

        // process lifecycle:
        for (auto& it : lifecycleTable)
        {
            std::vector<PassID>& passIDs = it.second;
            std::vector<PassID>::iterator right = sortedPasses.begin(), left = sortedPasses.end();
            for (PassID id : passIDs)
            {
                auto it = std::find(sortedPasses.begin(), sortedPasses.end(), id);
                right = std::max(right, it);
                left = std::min(left, it);
            }

            for (auto it = left; it < right; ++it)
            {
                passIDs.push_back(*it);
            }
        }

        // print lifecycle table
        printf("resource lifecycle: \n");
        printf("|      |");
        for (PassID id : sortedPasses)
        {
            printf(" %4x |", id);
        }
        printf("\n");
        for (auto& it : lifecycleTable)
        {
            printf("| %4x |", it.first);
            for (PassID passID : sortedPasses)
            {
                std::vector<PassID>& passIDs = it.second;
                if (std::find(passIDs.begin(), passIDs.end(), passID) != passIDs.end())
                {
                    printf(" ==== |");
                }
                else
                {
                    printf("      |");
                }
            }
            printf("\n");
        }

    }
}

