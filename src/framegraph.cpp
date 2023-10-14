#include "common.h"
#include "resources.h"

#include "framegraph.h"

#include <string>
#include <map>
#include <set>
#include <type_traits>
#include <functional>
#include <algorithm>

namespace vkz
{

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
        message(error, "!no resource found!");
        return;
    }

    const std::vector<ResourceAlias>& aliases = table.aliases;
    const ResourceAlias& alias = aliases[resIdx];


    result.aliasesID.assign(alias.aliasesID.begin(), alias.aliasesID.end());
}

void getResource(Image& image, Buffer& buffer, ResourceMap resMap, const ResourceID id)
{
    if (id & 0x80000000) {
        buffer = resMap._buffers[id];
    }
    else {
        image = resMap._images[id];
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
        message(error, "!no resource found!");
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
        message(error, "!no base resource found!");
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
        message(error, "!no more alias!");
        table.activeAliasesIdx[resIdx] = -1;
        table.activeAlias[resIdx] = baseResourceID;
    }

    result = table.activeAlias[resIdx];


    return result;
}

ResourceID registerBuffer(FrameGraphData& fd, const std::string& name, const size_t size)
{
    
    static ResourceID id = 0x0001;

    fd._resmap._managedBuffers.insert({ id, {id, name, size, 0u, 0u} });
    return id++;
}

vkz::PassID registerPass(FrameGraphData& fd, FrameGraph& fg, const std::string name, InputResources read, OutputResources write, RenderPassExeQueue queue /*= RenderPassExeQueue::graphics*/)
{
    static PassID id = 0x0001;

    RenderPass pass{};
    pass._ID = id;
    pass._name = name;
    pass._queue = queue;
    pass.inputResIDs.assign(read.begin(), read.end());
    pass.outputResIDs.assign(write.begin(), write.end());

    fd._passes.insert({ id, pass });
    fg._passes.push_back(id);

    return id++;
}

bool isBuffer(ResourceID res)
{
    uint32_t mask = res >> 12;

    return mask == 0x0;
}

bool isImage(ResourceID res)
{
    uint32_t mask = res >> 12;

    return mask == 0x1;
}

void setupResources(FrameGraphData& fd, FrameGraph& fg)
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

    ResourceMap& resMap = fd._resmap;
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
    earlycullPass._queue = RenderPassExeQueue::asyncCompute0;
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
    pyramidPass._queue = RenderPassExeQueue::asyncCompute0;
    pyramidPass.inputResIDs = { img_dt };
    pyramidPass.outputResIDs = { img_dpy_a1 };

    RenderPass latecullPass{};
    latecullPass._ID = 0x2004;
    latecullPass._name = "late cull pass";
    latecullPass._queue = RenderPassExeQueue::asyncCompute0;
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


    fg._passes.push_back(earlycullPass._ID);
    fg._passes.push_back(earlydrawPass._ID);
    fg._passes.push_back(pyramidPass._ID);
    fg._passes.push_back(latecullPass._ID);
    fg._passes.push_back(latedrawPass._ID);
    fg._passes.push_back(copyPass._ID);
    fg._passes.push_back(UIPass._ID);

    fd._passes.insert({ earlycullPass._ID, earlycullPass });
    fd._passes.insert({ earlydrawPass._ID, earlydrawPass });
    fd._passes.insert({ pyramidPass._ID, pyramidPass });
    fd._passes.insert({ latecullPass._ID, latecullPass });
    fd._passes.insert({ latedrawPass._ID, latedrawPass });
    fd._passes.insert({ copyPass._ID, copyPass });
    fd._passes.insert({ UIPass._ID, UIPass });

    fg._outputPass = UIPass._ID;
}

// p1 <-- p2 <-- p3 <-- p7
//   ____________|  ____|
//  /       /      /
// p4 <-- p5 <-- p6
//
void setupResources_test_a(FrameGraphData& fd, FrameGraph& fg)
{
    

    ResourceID r1 = 0x0001;
    ResourceID r2 = 0x0002;
    ResourceID r3 = 0x0003;
    ResourceID r4 = 0x0004;
    ResourceID r5 = 0x0005;
    ResourceID r6 = 0x0006;
    ResourceID r7 = 0x0007;

    ResourceMap& resMap = fd._resmap;
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

    fg._passes.push_back(p1._ID);
    fg._passes.push_back(p2._ID);
    fg._passes.push_back(p3._ID);
    fg._passes.push_back(p4._ID);
    fg._passes.push_back(p5._ID);
    fg._passes.push_back(p6._ID);
    fg._passes.push_back(p7._ID);

    fd._passes.insert({ p1._ID, p1 });
    fd._passes.insert({ p2._ID, p2 });
    fd._passes.insert({ p3._ID, p3 });
    fd._passes.insert({ p4._ID, p4 });
    fd._passes.insert({ p5._ID, p5 });
    fd._passes.insert({ p6._ID, p6 });
    fd._passes.insert({ p7._ID, p7 });

    fg._outputPass = p7._ID;
}

void setupResources_test_a_managed(FrameGraphData& fd, FrameGraph& fg)
{
    ResourceID r1 = registerBuffer(fd, "r1", 1024 * 1024);
    ResourceID r2 = registerBuffer(fd, "r2", 1024 * 1024);
    ResourceID r3 = registerBuffer(fd, "r3", 1024 * 1024);
    ResourceID r4 = registerBuffer(fd, "r4", 1024 * 1024);
    ResourceID r5 = registerBuffer(fd, "r5", 1024 * 1024);
    ResourceID r6 = registerBuffer(fd, "r6", 1024 * 1024);
    ResourceID r7 = registerBuffer(fd, "r7", 1024 * 1024);

    ResourceID r11 = registerBuffer(fd, "r11", 1024 * 1024);
    ResourceID r22 = registerBuffer(fd, "r22", 1024 * 1024);
    ResourceID r33 = registerBuffer(fd, "r33", 1024 * 1024);
    ResourceID r44 = registerBuffer(fd, "r44", 1024 * 1024);

    PassID p1 = registerPass(fd, fg, "p1", { r11 }, { r1 });
    PassID p2 = registerPass(fd, fg, "p2", { r1 }, { r2 });
    PassID p3 = registerPass(fd, fg, "p3", { r2, r4, r5 }, { r3 });
    PassID p4 = registerPass(fd, fg, "p4", { r22 }, { r4 }, RenderPassExeQueue::asyncCompute0);
    PassID p5 = registerPass(fd, fg, "p5", { r4 }, { r5 }, RenderPassExeQueue::asyncCompute0);
    PassID p6 = registerPass(fd, fg, "p6", { r5 }, { r6, r44 }, RenderPassExeQueue::asyncCompute0);
    PassID p7 = registerPass(fd, fg, "p7", { r3, r6 }, { r7 });

    fg._outputPass = p7;

    fg._multiFrameReses.push_back(r44);
}


// p1 <-- p2 <-- p3 <-- p7
//  \____________  _____|
//         |     |/
// p4 <-- p5 <-- p6
//
void setupResources_test_b(FrameGraphData& fd, FrameGraph& fg)
{
    

    ResourceID r1 = 0x0001;
    ResourceID r2 = 0x0002;
    ResourceID r3 = 0x0003;
    ResourceID r4 = 0x0004;
    ResourceID r5 = 0x0005;
    ResourceID r6 = 0x0006;
    ResourceID r7 = 0x0007;

    ResourceMap& resMap = fd._resmap;
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

    fg._passes.push_back(p1._ID);
    fg._passes.push_back(p2._ID);
    fg._passes.push_back(p3._ID);
    fg._passes.push_back(p4._ID);
    fg._passes.push_back(p5._ID);
    fg._passes.push_back(p6._ID);
    fg._passes.push_back(p7._ID);

    fd._passes.insert({ p1._ID, p1 });
    fd._passes.insert({ p2._ID, p2 });
    fd._passes.insert({ p3._ID, p3 });
    fd._passes.insert({ p4._ID, p4 });
    fd._passes.insert({ p5._ID, p5 });
    fd._passes.insert({ p6._ID, p6 });
    fd._passes.insert({ p7._ID, p7 });

    fg._outputPass = p7._ID;
}


// Q1 p1
//     \______________             
//            |       | 
//            |       |
// Q2 p2 <-- p3       |
//            \______ |
//                   ||
// Q3 p4 <-- p5      ||
//            \_____ ||
//                  |||
// Q4                p6 <--- p7
//
void setupResources_test_c(FrameGraphData& fd, FrameGraph& fg)
{
    

    ResourceID r1 = 0x0001;
    ResourceID r2 = 0x0002;
    ResourceID r3 = 0x0003;
    ResourceID r4 = 0x0004;
    ResourceID r5 = 0x0005;
    ResourceID r6 = 0x0006;
    ResourceID r7 = 0x0007;

    ResourceMap& resMap = fd._resmap;
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

    fg._passes.push_back(p1._ID);
    fg._passes.push_back(p2._ID);
    fg._passes.push_back(p3._ID);
    fg._passes.push_back(p4._ID);
    fg._passes.push_back(p5._ID);
    fg._passes.push_back(p6._ID);
    fg._passes.push_back(p7._ID);

    fd._passes.insert({ p1._ID, p1 });
    fd._passes.insert({ p2._ID, p2 });
    fd._passes.insert({ p3._ID, p3 });
    fd._passes.insert({ p4._ID, p4 });
    fd._passes.insert({ p5._ID, p5 });
    fd._passes.insert({ p6._ID, p6 });
    fd._passes.insert({ p7._ID, p7 });

    fg._outputPass = p7._ID;

}

// same dependency as test_c, 
// but some resources are exist from the beginning
// and some resources are marked as permanently(multi-frame data)

void setupResources_test_d(FrameGraphData& fd, FrameGraph& fg)
{
    ResourceID r1 = 0x0001;
    ResourceID r2 = 0x0002;
    ResourceID r3 = 0x0003;
    ResourceID r4 = 0x0004;
    ResourceID r5 = 0x0005;
    ResourceID r6 = 0x0006;
    ResourceID r7 = 0x0007;

    ResourceID r11 = 0x0011;
    ResourceID r22 = 0x0022;
    ResourceID r33 = 0x0033;
    ResourceID r44 = 0x0044;


    ResourceMap& resMap = fd._resmap;
    resMap.bufferNames.insert({ r1, "r1" });
    resMap.bufferNames.insert({ r2, "r2" });
    resMap.bufferNames.insert({ r3, "r3" });
    resMap.bufferNames.insert({ r4, "r4" });
    resMap.bufferNames.insert({ r5, "r5" });
    resMap.bufferNames.insert({ r6, "r6" });
    resMap.bufferNames.insert({ r7, "r7" });

    resMap.bufferNames.insert({ r11, "r11" });
    resMap.bufferNames.insert({ r22, "r22" });
    resMap.bufferNames.insert({ r33, "r33" });
    resMap.bufferNames.insert({ r44, "r44" });

    RenderPass p1{};
    p1._ID = 0x2001;
    p1._name = "p1";
    p1.inputResIDs = { r11 };
    p1.outputResIDs = { r1 };

    RenderPass p2{};
    p2._ID = 0x2002;
    p2._name = "p2";
    p2._queue = RenderPassExeQueue::asyncCompute0;
    p2.inputResIDs = { r22, r33};
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
    p6.outputResIDs = { r6 , r44};

    RenderPass p7{};
    p7._ID = 0x2007;
    p7._name = "p7";
    p7.inputResIDs = { r6 };
    p7.outputResIDs = { r7 };

    fg._passes.push_back(p1._ID);
    fg._passes.push_back(p2._ID);
    fg._passes.push_back(p3._ID);
    fg._passes.push_back(p4._ID);
    fg._passes.push_back(p5._ID);
    fg._passes.push_back(p6._ID);
    fg._passes.push_back(p7._ID);

    fd._passes.insert({ p1._ID, p1 });
    fd._passes.insert({ p2._ID, p2 });
    fd._passes.insert({ p3._ID, p3 });
    fd._passes.insert({ p4._ID, p4 });
    fd._passes.insert({ p5._ID, p5 });
    fd._passes.insert({ p6._ID, p6 });
    fd._passes.insert({ p7._ID, p7 });

    fg._outputPass = p7._ID;

    fg._multiFrameReses.push_back(r44);
}


void DFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<PassID, bool>& visited, std::unordered_map<PassID, bool>& onStack, std::vector<PassID>& sortedPasses)
{
    visited[currentPassID] = true;
    onStack[currentPassID] = true;

    RenderPass& rp = fd._passes[currentPassID];

    for (uint32_t adjPassID : rp.childPassIDs)
    {
        if (!visited[adjPassID])
        {
            DFSVisit(fd, adjPassID, visited, onStack, sortedPasses);
        }
        else if (onStack[adjPassID])
        {
            message(error, "cycle detected!");
        }
    }
    sortedPasses.push_back(currentPassID);
    onStack[currentPassID] = false;
}

void DFS(FrameGraph& graph, FrameGraphData& fd, std::vector<PassID>& sortedPasses)
{
    uint32_t passCount = (uint32_t)graph._passes.size();
    std::unordered_map<PassID, bool> visited{};
    std::unordered_map<PassID, bool> onStack{};

    for (uint32_t i =0; i < passCount; ++i)
    {
        PassID id = graph._passes[i];
        visited.insert({ id, false });
        onStack.insert({ id, false });
    }

    for (PassID id : graph._passes)
    {
        if (!visited[id])
        {
            DFSVisit(fd, id, visited, onStack, sortedPasses);
        }
    }
}

void reverseDFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<PassID, bool >& visited, std::unordered_map<PassID, bool>& onStack, std::vector<uint32_t>& sortedPassIDs)
{
    visited[currentPassID] = true;
    onStack[currentPassID] = true;

    RenderPass& rp = fd._passes[currentPassID];

    for (uint32_t parentPassID : rp.parentPassIDs)
    {
        if (!visited[parentPassID])
        {
            reverseDFSVisit(fd, parentPassID, visited, onStack, sortedPassIDs);
        }
        else if (onStack[parentPassID])
        {
            message(error, "cycle detected!");
        }
    }
    sortedPassIDs.push_back(currentPassID);
    onStack[currentPassID] = false;
}

// reverse traversal deep first search
void reverseTraversalDFS(FrameGraph& graph, FrameGraphData& fd, const PassID startPass, std::vector<uint32_t>& sortedPassIDs)
{
    // pass not exist
    assert(std::find(graph._passes.begin(), graph._passes.end(), startPass) != graph._passes.end());

    uint32_t passCount = (uint32_t)graph._passes.size();
    std::unordered_map<PassID, bool> visited{};
    std::unordered_map<PassID, bool> onStack{};

    for (uint32_t i = 0; i < passCount; ++i)
    {
        PassID id = graph._passes[i];
        visited.insert({ id, false });
        onStack.insert({ id, false });
    }

    PassID currentPass = startPass;


    if (!visited[currentPass])
    {
        reverseDFSVisit(fd, currentPass, visited, onStack, sortedPassIDs);
    }
}

void TopologicalSort(FrameGraph& graph, FrameGraphData& fd, std::vector<PassID>& sortedPasses)
{
    DFS(graph, fd, sortedPasses);

    std::reverse(sortedPasses.begin(), sortedPasses.end());
}


void formatDependencyLevels(FrameGraph& fg, FrameGraphData& fd, std::unordered_map<PassID, uint32_t> longestDists
    , std::unordered_map<uint32_t, std::vector<PassID>>& levelPasses, const PassID currentPass, const uint32_t baseLevel)
{
    
    const uint32_t level = longestDists[currentPass];
    const RenderPass& rp = fd._passes[currentPass];

    for (PassID id : rp.parentPassIDs)
    {
        if (level - 1 == longestDists[id])
        {
            RenderPass& rpi = fd._passes[id];

            // if not exist in current level
            if (std::find(levelPasses[baseLevel - level].begin(), levelPasses[baseLevel - level].end(), id) == levelPasses[baseLevel - level].end())
            {
                levelPasses[baseLevel - level].push_back(id);
            }
        }
    }

    for (PassID id : rp.parentPassIDs)
    {
        formatDependencyLevels(fg, fd, longestDists, levelPasses, id, baseLevel);
    }

};

void buildDenpendencyLevel(FrameGraph& fg, FrameGraphData& fd)
{
    

    const std::vector<PassID>& visitedPasses = fg._linearVisitedPasses;

    message(info, "dependency level: \n");

    std::unordered_map<PassID, uint32_t> longestDists{};
    for (uint32_t i = 0; i < visitedPasses.size(); ++i)
    {
        PassID pass = visitedPasses[i];
        longestDists.insert({ pass, 0 });
    }

    uint32_t detectedQueueCount = 1u;

    for (PassID pass : visitedPasses)
    {
        const RenderPass& rp = fd._passes[pass];
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
        RenderPass& rp = fd._passes[pass];
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
        const std::unordered_map<DependLevel, std::vector<PassID>>& dependPasses = fg._dependedLevelsPerPass[pass]._dependPasses;
        RenderPass& rp = fd._passes[pass];
        
        std::unordered_map<RenderPassExeQueue, bool> foundSyncPass{};

        // if current pass depends on specific pass
        for (const auto& it : dependPasses)
        {
            uint32_t level = it.first;
            const std::vector<PassID>& currLeveldependPasses = it.second;

            for (PassID dp : currLeveldependPasses)
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

    // fill optimal sync passes
    for (auto vpIt = visitedPasses.rbegin(); vpIt != visitedPasses.rend(); ++vpIt)
    {
        PassID pass = *vpIt;
        const std::unordered_map<DependLevel, std::vector<PassID>>& dependPasses = fg._dependedLevelsPerPass[pass]._dependPasses;
        RenderPass& rp = fd._passes[pass];

        uint32_t syncQueueCount = 1u;
        std::vector<PassID> plainNearstSyncPasses{};
        for (const auto& p : rp._nearestSyncPasses)
        {
            if (p.second != invalidPassID)
            {
                plainNearstSyncPasses.push_back(p.second);

                syncQueueCount += fd._passes[p.second]._queue == rp._queue ? 0u : 1u;
            }
        }

        // if current pass depends on specific pass
        for (const auto& it : dependPasses)
        {
            uint32_t level = it.first;
            const std::vector<PassID>& currLeveldependPasses = it.second;

            if(level == 0)
                rp._passToSync.insert(rp._passToSync.end(), currLeveldependPasses.begin(), currLeveldependPasses.end());

            for (const auto np : plainNearstSyncPasses)
            {
                auto ptsIt =  std::find(rp._passToSync.begin(), rp._passToSync.end(), np);
                   
                if (ptsIt == rp._passToSync.end())
                    message(info, "cut sync [from:] %s [to:] %s\n", rp._name.c_str(), fd._passes[np]._name.c_str());
            }
            
        }
    }

    // calc fence node
    for (auto vpIt = visitedPasses.rbegin(); vpIt != visitedPasses.rend(); ++vpIt)
    {
        PassID pass = *vpIt;
        const std::unordered_map<DependLevel, std::vector<PassID>>& dependPasses = fg._dependedLevelsPerPass[pass]._dependPasses;
        RenderPass& rp = fd._passes[pass];

        uint32_t syncQueueCount = 1u;
        std::vector<PassID> plainNearstSyncPasses{};
        
        char msg[256];
        snprintf(msg, COUNTOF(msg), " ");
        
        for (const auto& p : rp._passToSync)
        {
            bool sameQueue = fd._passes[p]._queue == rp._queue;

            syncQueueCount += sameQueue ? 0u : 1u;

            if (!sameQueue)
                snprintf(msg, COUNTOF(msg), "%sQ%d ", msg, std::underlying_type_t<RenderPassExeQueue>(fd._passes[p]._queue));
        }
        
        if(syncQueueCount > 1u) // only different queue need fence
            message(info, "pass %s make fence: Q%d  ---> (%s)\n", rp._name.c_str(),
                std::underlying_type_t<RenderPassExeQueue>(rp._queue), msg);
    }

    // print level pass data
    for (PassID pass : visitedPasses)
    {
        const std::unordered_map<DependLevel, std::vector<PassID>>& levelPasses = fg._dependedLevelsPerPass[pass]._dependPasses;

        if(levelPasses.empty())
            continue;

        message(info, "\tpass: %s\n", fd._passes[pass]._name.c_str());
        for (auto& it : levelPasses)
        {
            uint32_t level = it.first;
            const std::vector<PassID>& passes = it.second;

            for (PassID passID : passes)
            {
                message(info, "\t\t%d: %s\n", level, fd._passes[passID]._name.c_str());
            }
        }

        const RenderPass& rp = fd._passes[pass];
        
        if(false)
            for (const auto& nsp : rp._nearestSyncPasses)
            {
                message(info, "\t\t\tnearest sync pass in queue %d: %s\n", nsp.first, fd._passes[nsp.second]._name.c_str());
            }

        for (const auto& pts : rp._passToSync)
        {
            message(info, "\t\t\toptimal sync with: %s\n", fd._passes[pts]._name.c_str());
        }
    }
}


void fillInBuckets(FrameGraph& fg, FrameGraphData& fd, std::vector<ResourceID>& sortedResbySize
    , std::vector<ResourceID> aliasedReses, std::vector<std::vector<ResourceID>>& resInLevel, const std::vector<ResourceID>& resStack
    , std::vector<Bucket>& buckets)
{
    std::vector<ResourceID>& restRes = sortedResbySize;
    std::vector<ResourceID> stack = resStack;

    if (restRes.empty())
        return;
    
    // if level is 0, then it's a new bucket
    // pop the biggest one as the base resource
    if (resInLevel.empty())
    {
        assert(stack.empty());

        ResourceID baseRes = *restRes.begin();

        std::vector<ResourceID> currlevelReses{};
        currlevelReses.push_back(baseRes);

        // store the level resource
        resInLevel.push_back({ baseRes });

        // new bucket
        Bucket bucket{};
        bucket._reses.push_back(baseRes);
        bucket._size = fd._resmap._managedBuffers[baseRes]._size;
        bucket._idx = (uint32_t)buckets.size();

        buckets.push_back(bucket);

        stack.push_back(baseRes);

        aliasedReses.push_back(baseRes);


        // remove current one from rest resource
        restRes.erase(restRes.begin());
    }

    // a new level
    resInLevel.push_back({});

    Bucket& currBucket = buckets.back();

    // find the first aliasable resource
    for (ResourceID res: restRes)
    {
        // lamda that check if a resource is aliasable or not
        auto aliasiable = [&](ResourceID resource)
        {
            const ManagedBuffer mb = fd._resmap._managedBuffers[resource];
            const std::vector<ResourceID>& currLevelReses = resInLevel.back();

            // size checking
            size_t remaingSize = currBucket._size;
            for (ResourceID clres : currLevelReses)
            {
                remaingSize -= fd._resmap._managedBuffers[clres]._size;
            }

            bool sizeMatch = mb._size <= remaingSize;

            bool stackMatch = true;
            const std::pair<uint32_t, uint32_t> currResLifetime = fd._resmap.bufferLifetimeIdx[resource];
            // lifetime checking, should not overlap with any resource in current stack
            for (ResourceID csres : stack)
            {
                std::pair<uint32_t, uint32_t> stackLifetime = fd._resmap.bufferLifetimeIdx[csres];

                // if lifetime overlap with any resource in current stack, then it's not aliasable
                stackMatch &= (stackLifetime.first > currResLifetime.second || stackLifetime.second < currResLifetime.first);
            }

            return sizeMatch && stackMatch;
        };

        // if current resource is aliasable
        if (aliasiable(res))
        {
            // add current resource into current level
            resInLevel.back().push_back(res);

            // add current resource into current bucket
            currBucket._reses.push_back(res);

            aliasedReses.push_back(res);
        }
    }

    // if current level is empty, then it's done for current bucket
    if (resInLevel.back().empty())
    {
        resInLevel.clear();
        fillInBuckets(fg, fd, restRes, aliasedReses, resInLevel, {}, buckets);
    }
    else
    {
        // remove resources that already in current level from rest resource
        for (ResourceID res : resInLevel.back())
        {
            restRes.erase(std::find(restRes.begin(), restRes.end(), res));
        }

        for (ResourceID res : resInLevel.back())
        {
            stack.push_back(res);
            fillInBuckets(fg, fd, restRes, aliasedReses, resInLevel, stack, buckets);
            stack.pop_back();
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
// d. a reverse traversal DFS is used for get passes that contribute the final output resource, the execution order is guaranteed too
void buildGraph(FrameGraph& frameGraph)
{
    FrameGraph& fg = frameGraph;
    FrameGraphData fd{};
    //setupResources(fd, fg);
    setupResources_test_a_managed(fd, fg);

    // make a plain table that can find producer of a resource easily.
    std::vector<ResourceID> all_outResID;
    std::vector<PassID> all_parentPassID;
    for (PassID passID : fg._passes)
    {
        RenderPass& rp = fd._passes[passID];

        all_outResID.insert(all_outResID.end(), rp.outputResIDs.begin(), rp.outputResIDs.end());
        std::vector<uint32_t> tmp(rp.outputResIDs.size(), passID);
        all_parentPassID.insert(all_parentPassID.end(), tmp.begin(), tmp.end());

        assert(all_outResID.size() == all_parentPassID.size());
    }

    // build graph based on resource dependency
    for (PassID passID : fg._passes)
    {
        RenderPass& rp = fd._passes[passID];

        for (uint32_t j = 0; j < rp.inputResIDs.size(); ++j)
        {
            ResourceID currentResID = rp.inputResIDs[j];
            std::vector<ResourceID>::iterator it = std::find(all_outResID.begin(), all_outResID.end(), currentResID);
            if (it == all_outResID.end())
                continue; // not found producer for current resource, might be a read only resource

            PassID parentPassID = all_parentPassID[it - all_outResID.begin()];
            rp.parentPassIDs.push_back(parentPassID); // set parent pass for current pass
            fd._passes[parentPassID].childPassIDs.push_back(passID); // set child pass back
        }
    }

    // reverse traversal DFS to cut unnecessary passes
    std::vector<PassID>& visitedPasses = fg._linearVisitedPasses;
    reverseTraversalDFS(fg, fd, fg._outputPass, visitedPasses);

    // https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
    // build dependency levels
    buildDenpendencyLevel(fg, fd);

    // print reverse traversal DFS
    {
        message(info, "visited pass: \n");
        for (PassID passID : visitedPasses)
        {
            message(info, "\t pass: %s\n", fd._passes[passID]._name.c_str());
        }
    }

    // build lifetime table
    {
        std::map<ResourceID, std::vector<PassID>> lifetimeTable;
        std::set<ResourceID> readReses;
        std::set<ResourceID> wroteReses;
        for (PassID passID : fg._passes)
        {
             RenderPass& rp = fd._passes[passID];

             // add all input resources
             for (ResourceID resID : rp.inputResIDs)
             {
                std::vector<PassID>& passIDs = lifetimeTable[resID];
                passIDs.push_back(passID);
                readReses.insert(resID);
             }

             // add all output resources
             for (ResourceID resID : rp.outputResIDs)
             {
                std::vector<PassID>& passIDs = lifetimeTable[resID];
                passIDs.push_back(passID);
                wroteReses.insert(resID);
             }
        }

        std::vector<ResourceID> readonlyReses;
        {
            for (ResourceID res : readReses)
            {
                if (std::find(wroteReses.begin(), wroteReses.end(), res) == wroteReses.end())
                {
                    readonlyReses.push_back(res);
                }
            }
            // set readonly resources
            fg._readonlyReses = readonlyReses;
        }

        // now add two pass into the visitedPass, they are fake pass to represent frame start and frame end
        PassID frameStart = 0x0000;
        PassID frameEnd = 0x1111;
        std::vector<PassID> visitedPassesWithFrameStartEnd = visitedPasses;
        visitedPassesWithFrameStartEnd.insert(visitedPassesWithFrameStartEnd.begin(), { frameStart });
        visitedPassesWithFrameStartEnd.push_back(frameEnd);

        for (auto& resPair : lifetimeTable)
        {
            std::vector<PassID>& passIDs = resPair.second;
            if (std::find(readonlyReses.begin(), readonlyReses.end(), resPair.first) != readonlyReses.end())
            {
                passIDs.insert(passIDs.begin(), frameStart);
            }
            if (std::find(fg._multiFrameReses.begin(), fg._multiFrameReses.end(), resPair.first) != fg._multiFrameReses.end())
            {
                passIDs.push_back(frameEnd);
            }
        }

        std::unordered_map<PassID, int32_t> activedResourceCount{};

        // initialize activedPassCount
        for (PassID passID : visitedPassesWithFrameStartEnd)
        {
            activedResourceCount.insert({ passID, 0 });
        }

        // process lifetime:
        for (auto& resPair : lifetimeTable)
        {
            std::vector<PassID>& passIDs = resPair.second;
            std::vector<PassID>::iterator right = visitedPassesWithFrameStartEnd.begin(), left = visitedPassesWithFrameStartEnd.end();

            for (PassID id : passIDs)
            {
                auto it = std::find(visitedPassesWithFrameStartEnd.begin(), visitedPassesWithFrameStartEnd.end(), id);
                right = std::max(right, it);
                left = std::min(left, it);
            }
            passIDs.clear();

            if (isBuffer(resPair.first)) {
                fd._resmap.bufferLifetime.insert({ resPair.first, {*left, *right} });
                fd._resmap.bufferLifetimeIdx.insert({ resPair.first, {uint32_t(left - visitedPassesWithFrameStartEnd.begin()), uint32_t(right - visitedPassesWithFrameStartEnd.begin())} });
            }

            if (isImage(resPair.first)) {
                fd._resmap.imageLifetime.insert({ resPair.first, {*left, *right} });
            }
            
            for (PassID passID : visitedPassesWithFrameStartEnd)
            {
                if(passID == *left)
                    activedResourceCount[passID]++;
                if (passID == *right)
                    activedResourceCount[passID]--;
            }

            for (auto it = left; it < (right + 1); ++it)
            {
                passIDs.push_back(*it);
            }
        }

        uint32_t currActResourceCount = 0;
        for (PassID passID : visitedPassesWithFrameStartEnd)
        {
            currActResourceCount += activedResourceCount[passID];
            activedResourceCount[passID] = currActResourceCount;
        }


        // print lifetime table
        char msg[4096];
        snprintf(msg, COUNTOF(msg), "resource lifetime: \n");
        snprintf(msg, COUNTOF(msg), "%s|      |", msg);
        for (PassID id : visitedPassesWithFrameStartEnd)
        {
            snprintf(msg, COUNTOF(msg), "%s %4x |", msg, id);
        }
        snprintf(msg, COUNTOF(msg), "%s\n", msg);
        for (auto& it : lifetimeTable)
        {
            snprintf(msg, COUNTOF(msg), "%s| %4x |", msg, it.first);
            for (PassID passID : visitedPassesWithFrameStartEnd)
            {
                std::vector<PassID>& passIDs = it.second;
                if (std::find(passIDs.begin(), passIDs.end(), passID) != passIDs.end())
                {
                    snprintf(msg, COUNTOF(msg), "%s ==== |", msg);
                }
                else
                {
                    snprintf(msg, COUNTOF(msg), "%s      |", msg);
                }
            }
            snprintf(msg, COUNTOF(msg), "%s\n", msg);
        }

        message(info, msg);

        for (auto& resPair : activedResourceCount)
        {
            message(info, "[0x%4x]:[%d]\n", resPair.first, resPair.second);
        }

        // TODO: sort resources by size
        std::vector<ResourceID> sortedReses_size{};
        {
            std::vector<ResourceID> sortedReses{};
            for (auto& resPair : lifetimeTable)
            {
                sortedReses.push_back(resPair.first);
            }

            std::sort(sortedReses.begin(), sortedReses.end(), [&](ResourceID a, ResourceID b) {
                return fd._resmap._managedBuffers[a]._size > fd._resmap._managedBuffers[b]._size;
            });

            sortedReses_size = sortedReses;
        }

        std::vector<Bucket> buckets{};
        std::vector<std::vector<ResourceID>> resInLevel{};
        uint32_t levelIdx = 0;
        std::vector<ResourceID> aliasedReses{};
        fillInBuckets(fg, fd, sortedReses_size, aliasedReses, resInLevel, {}, buckets);
    }
}

}; // namespace vkz