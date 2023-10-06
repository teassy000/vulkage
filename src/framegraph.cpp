#include "common.h"
#include "resources.h"

#include "framegraph.h"

#include <string>
#include <map>
#include <set>
#include <type_traits>

void vkz::getResourceAliases(ResourceAlias& output, ResourceID id, const ResourceTable& table)
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

void vkz::getResource(Image& image, Buffer& buffer, ResourceMap resMap, const ResourceID id)
{
    if (id & 0x80000000) {
        buffer = resMap.buffers[id];
    }
    else {
        image = resMap.images[id];
    }
}

void vkz::resetActiveAlias(ResourceTable& table)
{
    const std::vector<ResourceID>& reses = table.activeAlias;
    for (uint32_t idx = 0; idx != reses.size(); ++idx)
    {
        table.activeAlias[idx] = table.baseReses[idx];
        table.activeAliasesIdx[idx] = -1;
    }
}

ResourceID vkz::getActiveAlias(const ResourceTable& table, const ResourceID baseResourceID)
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

ResourceID vkz::nextAlias(ResourceTable& table, const ResourceID baseResourceID)
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

void setupResources(vkz::FrameGraphData& fd, vkz::FrameGraph& fg)
{
    using namespace vkz;

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
    earlycullPass._ID = 0x2001;
    earlycullPass._name = "early cull pass";
    earlycullPass.inputResIDs = { buf_mesh, buf_transData, buf_mDr, buf_mDrCC, buf_mDrVis , img_dpy};
    earlycullPass.outputResIDs = { buf_mDrCmd, buf_mDrCC_a1, buf_mDrVis_a1 };

    RenderPass earlydrawPass{};
    earlydrawPass._ID =  0x2002;
    earlydrawPass._name = "early draw pass";
    earlydrawPass.inputResIDs = { buf_vtx, buf_idx, buf_mesh, buf_mltData, buf_transData, buf_mDr, buf_mDrCmd, buf_mltVis , img_dpy};
    earlydrawPass.outputResIDs = { buf_mltVis_a1, img_ct, img_dt };

    RenderPass pyramidPass{};
    pyramidPass._ID = 0x2003;
    pyramidPass._name = "pyramid pass";
    pyramidPass.inputResIDs = { img_dt };
    pyramidPass.outputResIDs = { img_dpy_a1 };

    RenderPass latecullPass{};
    latecullPass._ID = 0x2004;
    latecullPass._name = "late cull pass";
    latecullPass.inputResIDs = { buf_mesh, buf_transData, buf_mDr, buf_mDrCC_a1, buf_mDrVis_a1, img_dpy_a1 };
    latecullPass.outputResIDs = { buf_mDrCmd_a1, buf_mDrCC_a2, buf_mDrVis_a2 };

    RenderPass latedrawPass{};
    latedrawPass._ID = 0x2005;
    latedrawPass._name = "late draw pass";
    latedrawPass.inputResIDs = { buf_vtx, buf_idx, buf_mesh, buf_mltData, buf_transData, buf_mDr, buf_mDrCmd_a1, buf_mltVis_a1, img_dpy_a1 };
    latedrawPass.outputResIDs = { buf_mltVis_a2, img_ct_a1, img_dt_a1 };

    RenderPass copyPass{};
    copyPass._ID = 0x2006;
    copyPass._name = "copy pass";
    copyPass.inputResIDs = { img_ct_a1 };
    copyPass.outputResIDs = { img_rt };

    RenderPass UIPass{};
    UIPass._ID = 0x2007;
    UIPass._name = "UI pass";
    UIPass.inputResIDs = { img_rt };
    UIPass.outputResIDs = { img_rt_a1 };


    fg.passes.push_back(earlycullPass._ID);
    fg.passes.push_back(earlydrawPass._ID);
    fg.passes.push_back(pyramidPass._ID);
    fg.passes.push_back(latecullPass._ID);
    fg.passes.push_back(latedrawPass._ID);
    fg.passes.push_back(copyPass._ID);
    fg.passes.push_back(UIPass._ID);

    fd.passes.insert({ earlycullPass._ID, earlycullPass });
    fd.passes.insert({ earlydrawPass._ID, earlydrawPass });
    fd.passes.insert({ pyramidPass._ID, pyramidPass });
    fd.passes.insert({ latecullPass._ID, latecullPass });
    fd.passes.insert({ latedrawPass._ID, latedrawPass });
    fd.passes.insert({ copyPass._ID, copyPass });
    fd.passes.insert({ UIPass._ID, UIPass });
}

void setupResources_test_a(vkz::FrameGraphData& fd, vkz::FrameGraph& fg)
{
    using namespace vkz;

    ResourceID r1 = 0x0001;
    ResourceID r2 = 0x0002;
    ResourceID r3 = 0x0003;
    ResourceID r4 = 0x0004;
    ResourceID r5 = 0x0005;
    ResourceID r6 = 0x0006;
    ResourceID r7 = 0x0007;

    ResourceMap& resMap = fd.resmap;
    resMap.bufferNames.insert({ r1, "r1"});
    resMap.bufferNames.insert({ r2, "r2"});
    resMap.bufferNames.insert({ r3, "r3"});
    resMap.bufferNames.insert({ r4, "r4"});
    resMap.bufferNames.insert({ r5, "r5"});
    resMap.bufferNames.insert({ r6, "r6"});
    resMap.bufferNames.insert({ r7, "r7"});

    RenderPass p1{};
    p1._ID = 0x2001;
    p1._name = "p1";
    p1.inputResIDs = { };
    p1.outputResIDs = {r1};

    RenderPass p2{};
    p2._ID = 0x2002;
    p2._name = "p2";
    p2.inputResIDs = { r1};
    p2.outputResIDs = {r2};

    RenderPass p3{};
    p3._ID = 0x2003;
    p3._name = "p3";
    p3.inputResIDs = { r2, r4, r5};
    p3.outputResIDs = {r3 };

    RenderPass p4{};
    p4._ID = 0x2004;
    p4._name = "p4";
    p4._queue = RenderPassExeQueue::asyncCompute0;
    p4.inputResIDs = {};
    p4.outputResIDs = {r4};

    RenderPass p5{};
    p5._ID = 0x2005;
    p5._name = "p5";
    p5._queue = RenderPassExeQueue::asyncCompute0;
    p5.inputResIDs = {r4};
    p5.outputResIDs = {r5};

    RenderPass p6{};
    p6._ID = 0x2006;
    p6._name = "p6";
    p6._queue = RenderPassExeQueue::asyncCompute0;
    p6.inputResIDs = { r5};
    p6.outputResIDs = { r6 };

    RenderPass p7{};
    p7._ID = 0x2007;
    p7._name = "p7";
    p7.inputResIDs = { r3, r6 };
    p7.outputResIDs = { r7 };

    fg.passes.push_back(p1._ID);
    fg.passes.push_back(p2._ID);
    fg.passes.push_back(p3._ID);
    fg.passes.push_back(p4._ID);
    fg.passes.push_back(p5._ID);
    fg.passes.push_back(p6._ID);
    fg.passes.push_back(p7._ID);

    fd.passes.insert({ p1._ID, p1 });
    fd.passes.insert({ p2._ID, p2 });
    fd.passes.insert({ p3._ID, p3 });
    fd.passes.insert({ p4._ID, p4 });
    fd.passes.insert({ p5._ID, p5 });
    fd.passes.insert({ p6._ID, p6 });
    fd.passes.insert({ p7._ID, p7 });

    fg._outputPass = p7._ID;
}

void setupResources_test_b(vkz::FrameGraphData& fd, vkz::FrameGraph& fg)
{
    using namespace vkz;

    ResourceID r1 = 0x0001;
    ResourceID r2 = 0x0002;
    ResourceID r3 = 0x0003;
    ResourceID r4 = 0x0004;
    ResourceID r5 = 0x0005;
    ResourceID r6 = 0x0006;
    ResourceID r7 = 0x0007;

    ResourceMap& resMap = fd.resmap;
    resMap.bufferNames.insert({ r1, "r1" });
    resMap.bufferNames.insert({ r2, "r2" });
    resMap.bufferNames.insert({ r3, "r3" });
    resMap.bufferNames.insert({ r4, "r4" });
    resMap.bufferNames.insert({ r5, "r5" });
    resMap.bufferNames.insert({ r6, "r6" });
    resMap.bufferNames.insert({ r7, "r7" });

    RenderPass p1{};
    p1._ID = 0x2001;
    p1._name = "p1";
    p1._queue = RenderPassExeQueue::graphics;
    p1.inputResIDs = { };
    p1.outputResIDs = { r1 };

    RenderPass p2{};
    p2._ID = 0x2002;
    p2._name = "p2";
    p2._queue = RenderPassExeQueue::graphics;
    p2.inputResIDs = { r1 };
    p2.outputResIDs = { r2 };

    RenderPass p3{};
    p3._ID = 0x2003;
    p3._name = "p3";
    p3._queue = RenderPassExeQueue::graphics;
    p3.inputResIDs = { r2 };
    p3.outputResIDs = { r3 };

    RenderPass p4{};
    p4._ID = 0x2004;
    p4._name = "p4";
    p4._queue = RenderPassExeQueue::asyncCompute0;
    p4.inputResIDs = {};
    p4.outputResIDs = { r4 };

    RenderPass p5{};
    p5._ID = 0x2005;
    p5._name = "p5";
    p5._queue = RenderPassExeQueue::asyncCompute0;
    p5.inputResIDs = { r1, r4 };
    p5.outputResIDs = { r5 };

    RenderPass p6{};
    p6._ID = 0x2006;
    p6._name = "p6";
    p6._queue = RenderPassExeQueue::asyncCompute0;
    p6.inputResIDs = { r1, r5 };
    p6.outputResIDs = { r6 };

    RenderPass p7{};
    p7._ID = 0x2007;
    p7._name = "p7";
    p7.inputResIDs = { r3, r6 };
    p7.outputResIDs = { r7 };

    fg.passes.push_back(p1._ID);
    fg.passes.push_back(p2._ID);
    fg.passes.push_back(p3._ID);
    fg.passes.push_back(p4._ID);
    fg.passes.push_back(p5._ID);
    fg.passes.push_back(p6._ID);
    fg.passes.push_back(p7._ID);

    fd.passes.insert({ p1._ID, p1 });
    fd.passes.insert({ p2._ID, p2 });
    fd.passes.insert({ p3._ID, p3 });
    fd.passes.insert({ p4._ID, p4 });
    fd.passes.insert({ p5._ID, p5 });
    fd.passes.insert({ p6._ID, p6 });
    fd.passes.insert({ p7._ID, p7 });

    fg._outputPass = p7._ID;
}

void setupResources_test_c(vkz::FrameGraphData& fd, vkz::FrameGraph& fg)
{
    using namespace vkz;

    ResourceID r1 = 0x0001;
    ResourceID r2 = 0x0002;
    ResourceID r3 = 0x0003;
    ResourceID r4 = 0x0004;
    ResourceID r5 = 0x0005;
    ResourceID r6 = 0x0006;
    ResourceID r7 = 0x0007;

    ResourceMap& resMap = fd.resmap;
    resMap.bufferNames.insert({ r1, "r1" });
    resMap.bufferNames.insert({ r2, "r2" });
    resMap.bufferNames.insert({ r3, "r3" });
    resMap.bufferNames.insert({ r4, "r4" });
    resMap.bufferNames.insert({ r5, "r5" });
    resMap.bufferNames.insert({ r6, "r6" });
    resMap.bufferNames.insert({ r7, "r7" });

    RenderPass p1{};
    p1._ID = 0x2001;
    p1._name = "p1";
    p1.inputResIDs = { };
    p1.outputResIDs = { r1 };

    RenderPass p2{};
    p2._ID = 0x2002;
    p2._name = "p2";
    p2._queue = RenderPassExeQueue::asyncCompute0;
    p2.inputResIDs = { };
    p2.outputResIDs = { r2 };

    RenderPass p3{};
    p3._ID = 0x2003;
    p3._name = "p3";
    p3._queue = RenderPassExeQueue::asyncCompute0;
    p3.inputResIDs = { r1, r2 };
    p3.outputResIDs = { r3 };

    RenderPass p4{};
    p4._ID = 0x2004;
    p4._name = "p4";
    p4._queue = RenderPassExeQueue::asyncCompute1;
    p4.inputResIDs = {};
    p4.outputResIDs = { r4 };

    RenderPass p5{};
    p5._ID = 0x2005;
    p5._name = "p5";
    p5._queue = RenderPassExeQueue::asyncCompute1;
    p5.inputResIDs = { r4 };
    p5.outputResIDs = { r5 };

    RenderPass p6{};
    p6._ID = 0x2006;
    p6._name = "p6";
    p6._queue = RenderPassExeQueue::asyncCompute2;
    p6.inputResIDs = { r1, r3, r5 };
    p6.outputResIDs = { r6 };

    RenderPass p7{};
    p7._ID = 0x2007;
    p7._name = "p7";
    p7.inputResIDs = { r6 };
    p7.outputResIDs = { r7 };

    fg.passes.push_back(p1._ID);
    fg.passes.push_back(p2._ID);
    fg.passes.push_back(p3._ID);
    fg.passes.push_back(p4._ID);
    fg.passes.push_back(p5._ID);
    fg.passes.push_back(p6._ID);
    fg.passes.push_back(p7._ID);

    fd.passes.insert({ p1._ID, p1 });
    fd.passes.insert({ p2._ID, p2 });
    fd.passes.insert({ p3._ID, p3 });
    fd.passes.insert({ p4._ID, p4 });
    fd.passes.insert({ p5._ID, p5 });
    fd.passes.insert({ p6._ID, p6 });
    fd.passes.insert({ p7._ID, p7 });

    fg._outputPass = p7._ID;
}

void vkz::DFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<PassID, bool>& visited, std::unordered_map<PassID, bool>& onStack, std::vector<PassID>& sortedPasses)
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

void vkz::DFS(FrameGraph& graph, FrameGraphData& fd, std::vector<PassID>& sortedPasses)
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

void vkz::reverseDFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<PassID, bool >& visited, std::unordered_map<PassID, bool>& onStack, std::vector<uint32_t>& sortedPassIDs)
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
void vkz::reverseTraversalDFS(FrameGraph& graph, FrameGraphData& fd, const PassID startPass, std::vector<uint32_t>& sortedPassIDs)
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

    PassID currentPass = startPass;


    if (!visited[currentPass])
    {
        reverseDFSVisit(fd, currentPass, visited, onStack, sortedPassIDs);
    }

}

void vkz::TopologicalSort(FrameGraph& graph, FrameGraphData& fd, std::vector<PassID>& sortedPasses)
{
    DFS(graph, fd, sortedPasses);

    std::reverse(sortedPasses.begin(), sortedPasses.end());
}


void formatDependencyLevels(vkz::FrameGraph& fg, vkz::FrameGraphData& fd, std::unordered_map<vkz::PassID, uint32_t> longestDists
    , std::unordered_map<uint32_t, std::vector<vkz::PassID>>& levelPasses, const vkz::PassID currentPass, const uint32_t baseLevel)
{
    using namespace vkz;
    const uint32_t level = longestDists[currentPass];
    const RenderPass& rp = fd.passes[currentPass];

    for (PassID id : rp.parentPassIDs)
    {
        if (level - 1 == longestDists[id])
        {
            RenderPass& rpi = fd.passes[id];

            levelPasses[baseLevel - level].push_back(id);
        }
    }

    for (PassID id : rp.parentPassIDs)
    {
        formatDependencyLevels(fg, fd, longestDists, levelPasses, id, baseLevel);
    }

};

void buildDenpendencyLevel(vkz::FrameGraph& fg, vkz::FrameGraphData& fd)
{
    using namespace vkz;

    const std::vector<PassID>& visitedPasses = fg._linearVisitedPasses;

    vkz::message(vkz::info, "dependency level: \n");

    std::unordered_map<PassID, uint32_t> longestDists{};
    for (uint32_t i = 0; i < visitedPasses.size(); ++i)
    {
        PassID pass = visitedPasses[i];
        longestDists.insert({ pass, 0 });
    }

    uint32_t detectedQueueCount = 1u;

    for (PassID pass : visitedPasses)
    {
        const RenderPass& rp = fd.passes[pass];
        uint32_t longestDist = 0u;
        for (PassID parentPass : rp.parentPassIDs)
        {
            uint32_t dist = longestDists[parentPass] + 1u;
            longestDist = std::max(longestDist, dist);
        }
        longestDists[pass] = longestDist;
        fg._passesInQueue[rp._queue].push_back(pass);
        detectedQueueCount = std::max(detectedQueueCount, std::underlying_type_t<RenderPassExeQueue>(rp._queue) + 1u);
    }

    fg._detectedQueueCount = detectedQueueCount;

    for (PassID pass : visitedPasses)
    {
        std::unordered_map<DependLevel, std::vector<PassID>>& levelPasses = fg._dependedLevelsPerPass[pass]._dependPasses;
        formatDependencyLevels(fg, fd, longestDists, levelPasses, pass, longestDists[pass]);
    }

    for (PassID pass : visitedPasses)
    {
        RenderPass& rp = fd.passes[pass];
        // initialize nearest sync pass
        uint32_t nQueues = std::underlying_type_t<RenderPassExeQueue>(RenderPassExeQueue::NUM_OF_QUEUES);
        for (uint32_t i = 0; i < nQueues; ++i)
        {
            rp._nearestSyncPasses[RenderPassExeQueue(i)] = invalidPassID;
        }
    }

    // find nearest sync pass in each queue for each pass
    for (PassID pass : visitedPasses)
    {
        const std::unordered_map<DependLevel, std::vector<PassID>>& levelPasses = fg._dependedLevelsPerPass[pass]._dependPasses;
        RenderPass& rp = fd.passes[pass];
        
        std::unordered_map<RenderPassExeQueue, bool> foundSyncPass{};

        // if current pass depends on specific pass
        for (auto& it : levelPasses)
        {
            uint32_t level = it.first;
            const std::vector<PassID>& dependPasses = it.second;

            for (PassID dp : dependPasses)
            {
                for (auto& piq : fg._passesInQueue)
                {
                    // find depend passes in each queue, if exist, set it as nearest sync pass
                    if ( rp._nearestSyncPasses[piq.first] == invalidPassID &&
                        std::find(piq.second.begin(), piq.second.end(), dp) != piq.second.end())
                    {
                        rp._nearestSyncPasses[piq.first] = dp;
                        break;
                    }
                }
            }
        }
        
    }

    // print level pass data
    for (PassID pass : visitedPasses)
    {

        const std::unordered_map<DependLevel, std::vector<PassID>>& levelPasses = fg._dependedLevelsPerPass[pass]._dependPasses;

        if(levelPasses.empty())
            continue;

        vkz::message(vkz::info, "\tpass: %s\n", fd.passes[pass]._name.c_str());
        for (auto& it : levelPasses)
        {
            uint32_t level = it.first;
            const std::vector<PassID>& passes = it.second;

            for (PassID passID : passes)
            {
                vkz::message(vkz::info, "\t\t%d: %s\n", level, fd.passes[passID]._name.c_str());
            }
        }

        const RenderPass& rp = fd.passes[pass];

        for (const auto& nsp: rp._nearestSyncPasses)
        {
            vkz::message(vkz::info, "\t\t\tnearest sync pass in queue %d: %s\n", nsp.first, fd.passes[nsp.second]._name.c_str());
        }
    }

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
void vkz::buildGraph(FrameGraph& frameGraph)
{
    FrameGraph& fg = frameGraph;
    FrameGraphData fd{};
    //setupResources(fd, fg);
    setupResources_test_a(fd, fg);

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
    std::vector<PassID> sortedPasses{};
    TopologicalSort(fg, fd, sortedPasses);

    // reverse traversal DFS to cut unnecessary passes
    std::vector<PassID>& visitedPasses = fg._linearVisitedPasses;
    reverseTraversalDFS(fg, fd, fg._outputPass, visitedPasses);

    // https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
    // build dependency levels
    buildDenpendencyLevel(fg, fd);

    // print topology sort
    {
        vkz::message(vkz::info, "topology sorted: \n");
        for (PassID passID : sortedPasses)
        {
            vkz::message(vkz::info, "\t pass: %s\n", fd.passes[passID]._name.c_str());
        }
    }

    // print reverse traversal DFS
    {
        vkz::message(vkz::info, "visited pass: \n");
        for (PassID passID : visitedPasses)
        {
            vkz::message(vkz::info, "\t pass: %s\n", fd.passes[passID]._name.c_str());
        }
    }

    // build lifecycle table
    {
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

