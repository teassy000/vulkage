#include "common.h"
#include "resources.h"

#include "framegraph.h"

#include <string>
#include <map>

void getResourceAliases(ResourceAlias& output, ResourceID id, const ResourceTable& table)
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
        vkz::message(vkz::error, "!no resource found!");
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

void resetActiveAlias(ResourceTable& table)
{
    const std::vector<ResourceID>& reses = table.activeAlias;
    for (uint32_t idx = 0; idx != reses.size(); ++idx)
    {
        table.activeAlias[idx] = table.baseReses[idx];
        table.activeAliasesIdx[idx] = -1;
    }
}

ResourceID getActiveAlias(const ResourceTable& table, const ResourceID baseResourceID)
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
        vkz::message(vkz::error, "!no resource found!");
    }

    return result;

}

ResourceID nextAlias(ResourceTable& table, const ResourceID baseResourceID)
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
        vkz::message(vkz::error, "!no base resource found!");
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
        vkz::message(vkz::error, "!no more alias!");
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
            vkz::message(vkz::error, "cycle detected!");
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
            vkz::message(vkz::error, "cycle detected!");
        }
    }
    sortedPassIDs.push_back(currentPassID);
    onStack[currentPassID] = false;
}

// reverse traversal deep first search
void reverseTraversalDFS(FrameGraph& graph, FrameData& fd, uint32_t startPass, std::vector<uint32_t>& sortedPassIDs)
{
    // pass not exist
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

void buildGraph(FrameGraph& frameGraph)
{
    FrameGraph& fg = frameGraph;
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
        vkz::message(vkz::info, "topology sorted: \n");
        for (PassID passID : sortedPasses)
        {
            vkz::message(vkz::info, "\t pass: %s\n", fd.passes[passID].name.c_str());
        }
    }


    // reverse traversal DFS
    std::vector<PassID> visitedPasses;
    reverseTraversalDFS(fg, fd, 0x2007, visitedPasses);

    {
        vkz::message(vkz::info, "visited pass: \n");
        for (PassID passID : visitedPasses)
        {
            vkz::message(vkz::info, "\t pass: %s\n", fd.passes[passID].name.c_str());
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
             // TODO:
             // aliasing happens right after the write pass finished
             // consider it starts from the next pass start?
             
             for (ResourceID resID : rp.outputResIDs)
             {
                std::vector<PassID>& passIDs = lifecycleTable[resID];
                passIDs.push_back(passID);
             }
             

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
            passIDs.clear();



            for (auto it = left; it < (right + 1); ++it)
            {
                passIDs.push_back(*it);
            }
            
        }

        // print lifecycle table
        printf("resource lifetime: \n");
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

