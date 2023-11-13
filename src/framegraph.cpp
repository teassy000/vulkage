#include "common.h"
#include "resources.h"

#include "framegraph.h"

#include <string>
#include <map>
#include <set>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <iterator>

namespace vkz
{

uint32_t registerBuffer(FrameGraphData& fd, const std::string& name, const size_t size)
{
    static uint32_t id = 0x0001;

    fd._resmap._managedBuffers.insert({ id, {id, name, size, 0u, 0u} });
    
    return id++;
}

uint32_t registerBuffer(FrameGraphData& fd, const BufferProps& props)
{
    static uint32_t id = 0x0001;

    fd._resmap._managedBuffers.insert({ id, {id, props.name, props.size, 0u, 0u} });

    return id++;
}

uint32_t registerImage(FrameGraphData& fd, const ImageProps& props)
{
    static uint32_t id = 0x1001;

    ManagedImage img = { id, props.name, props.x, props.y, props.z, props.mipLevels, 0u };

    fd._resmap._managedImages.insert({ id, img });

    return id++;
}

vkz::uint32_t registerPass(FrameGraphData& fd, FrameGraph& fg, const std::string name, ResourceIDs read, ResourceIDs write, RenderPassExeQueue queue /*= RenderPassExeQueue::graphics*/)
{
    static uint32_t id = 0x0001;

    RenderPass pass{};
    pass._ID = id;
    pass._name = name;
    pass._queue = queue;
    pass.inputResIDs.assign(begin(read), end(read));
    pass.outputResIDs.assign(begin(write), end(write));

    fd._passes.insert({ id, pass });
    fg._passes.push_back(id);

    return id++;
}

bool isBuffer(uint32_t res)
{
    uint32_t mask = res >> 12;

    return mask == 0x0;
}

bool isImage(uint32_t res)
{
    uint32_t mask = res >> 12;

    return mask == 0x1;
}

void setupResources(FrameGraphData& fd, FrameGraph& fg)
{
    uint32_t buf_vtx = 0x0001;
    uint32_t buf_idx = 0x0002;
    uint32_t buf_mesh = 0x0003;
    uint32_t buf_mlt = 0x0004;
    uint32_t buf_mltData = 0x0005;
    uint32_t buf_transData = 0x0006;
    uint32_t buf_mDr = 0x0007;
    uint32_t buf_mDrCmd = 0x0008;
    uint32_t buf_mDrCmd_a1 = 0x0018;
    uint32_t buf_mDrCmd_a2 = 0x0028;
    uint32_t buf_mDrCC = 0x0009;
    uint32_t buf_mDrCC_a1 = 0x0019;
    uint32_t buf_mDrCC_a2 = 0x0029;
    uint32_t buf_mDrVis = 0x000a;
    uint32_t buf_mDrVis_a1 = 0x001a;
    uint32_t buf_mDrVis_a2 = 0x002a;
    uint32_t buf_mltVis = 0x000b;
    uint32_t buf_mltVis_a1 = 0x001b;
    uint32_t buf_mltVis_a2 = 0x002b;

    uint32_t img_ct = 0x1001;
    uint32_t img_ct_a1 = 0x1011;
    uint32_t img_ct_a2 = 0x1021;
    uint32_t img_dt = 0x1002;
    uint32_t img_dt_a1 = 0x1012;
    uint32_t img_dt_a2 = 0x1022;
    uint32_t img_rt = 0x1003;
    uint32_t img_rt_a1 = 0x1013;
    uint32_t img_rt_a2 = 0x1023;
    uint32_t img_dpy = 0x1004;
    uint32_t img_dpy_a1 = 0x1014;

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

typedef std::initializer_list<uint32_t> Resources;

void addForceAliasBuffers(FrameGraph& fg, FrameGraphData& fd, const Resources reses)
{
    std::vector<uint32_t> resourceList{reses};

    assert(!resourceList.empty());

    // check if all resources have the same size
    size_t baseSize = fd._resmap._managedBuffers[resourceList[0]]._size;
    for (const uint32_t curr : reses)
    {
        assert(isBuffer(curr));

        size_t currSize = fd._resmap._managedBuffers[curr]._size;
        assert(currSize == baseSize);
    }

    fg._forceAliasBuffers.push_back(resourceList);

    for (const uint32_t curr : reses)
    {
        std::vector<uint32_t> result{};
        std::copy_if(begin(resourceList), end(resourceList), std::back_inserter(result),
            [curr](uint32_t id) { return id != curr; }
        );

        std::vector<uint32_t>& currForceAliased = fd._resmap._managedBuffers[curr]._forceAlias;
        currForceAliased.insert(end(currForceAliased), begin(result), end(result));
    }
}

void addForceAliasImages(FrameGraph& fg, FrameGraphData& fd, const Resources reses)
{
    std::vector<uint32_t> resourceList{reses};

    assert(!resourceList.empty());
    // check if all resources have the same size
    size_t baseX = fd._resmap._managedImages[resourceList[0]]._x;
    size_t baseY = fd._resmap._managedImages[resourceList[0]]._y;
    size_t baseZ = fd._resmap._managedImages[resourceList[0]]._z;
    uint32_t mipLevels = fd._resmap._managedImages[resourceList[0]]._mipLevels;

    for (const uint32_t curr : reses)
    {
        assert(isImage(curr));

        size_t currX = fd._resmap._managedImages[curr]._x;
        size_t currY = fd._resmap._managedImages[curr]._y;
        size_t currZ = fd._resmap._managedImages[curr]._z;
        uint32_t currMipLevels = fd._resmap._managedImages[curr]._mipLevels;
        
        assert(currX == baseX);
        assert(currY == baseY);
        assert(currZ == baseZ);
        assert(currMipLevels == mipLevels);
    }

    fg._forceAliasImages.push_back(resourceList);

    for (const uint32_t curr : reses)
    {
        std::vector<uint32_t> result{};
        std::copy_if(begin(resourceList), end(resourceList), std::back_inserter(result),
            [curr](uint32_t id) { return id != curr; }
        );

        std::vector<uint32_t>& currForceAliased = fd._resmap._managedImages[curr]._forceAlias;
        currForceAliased.insert(end(currForceAliased), begin(result), end(result));
    }
}

void setupResources_managed(FrameGraphData& fd, FrameGraph& fg)
{
    uint32_t buf_vtx = registerBuffer(fd, "vertex buffer", 192 * 1024 * 1024);
    uint32_t buf_idx = registerBuffer(fd, "index buffer", 20 * 1024 * 1024);
    uint32_t buf_mesh = registerBuffer(fd, "mesh buffer",  4 * 1024 * 1024); 
    uint32_t buf_mlt = registerBuffer(fd, "meshlet buffer", 32 * 1024 * 1024);
    uint32_t buf_mltData = registerBuffer(fd, "meshlet Data buffer", 4 * 1024 * 1024);
    uint32_t buf_transData = registerBuffer(fd, "transform buffer", 8 * 1024);
    uint32_t buf_mDr = registerBuffer(fd, "mesh draw buffer", 48 * 1024 * 1024);
    uint32_t buf_mDrCmd = registerBuffer(fd, "mesh draw command buffer", 128 * 1024 * 1024);
    uint32_t buf_mDrCmd_a1 = registerBuffer(fd, "A1: mesh draw command buffer",128 * 1024 * 1024);
    uint32_t buf_mDrCmd_a2 = registerBuffer(fd, "A2: mesh draw command buffer", 128 * 1024 * 1024);
    uint32_t buf_mDrCC = registerBuffer(fd, "mesh draw command count buffer", 64);
    uint32_t buf_mDrCC_a1 = registerBuffer(fd, "A1: mesh draw command count buffer", 64);
    uint32_t buf_mDrCC_a2 = registerBuffer(fd, "A2: mesh draw command count buffer", 64);
    uint32_t buf_mDrVis = registerBuffer(fd, "mesh draw visibility buffer", 32 * 1024 * 1024);
    uint32_t buf_mDrVis_a1 = registerBuffer(fd, "A1: mesh draw visibility buffer", 32 * 1024 * 1024);
    uint32_t buf_mDrVis_a2 = registerBuffer(fd, "A2: mesh draw visibility buffer", 32 * 1024 * 1024);
    uint32_t buf_mltVis = registerBuffer(fd, "meshlet visibility buffer", 256 * 1024 * 1024);
    uint32_t buf_mltVis_a1 = registerBuffer(fd, "A1: meshlet visibility buffer", 256 * 1024 * 1024);
    uint32_t buf_mltVis_a2 = registerBuffer(fd, "A2: meshlet visibility buffer", 256 * 1024 * 1024);

    uint32_t sc_x = 1920u;
    uint32_t sc_y = 1080u;

    ImageProps img_ct_props = { "color target" };
    img_ct_props.x = sc_x;
    img_ct_props.y = sc_y;
    uint32_t img_ct = registerImage(fd, img_ct_props);

    ImageProps img_ct_a1_props = { "A1: color target" };
    img_ct_a1_props.x = sc_x;
    img_ct_a1_props.y = sc_y;
    uint32_t img_ct_a1 = registerImage(fd, img_ct_a1_props);

    ImageProps img_dt_props = { "depth target" };
    img_dt_props.x = sc_x;
    img_dt_props.y = sc_y;
    uint32_t img_dt = registerImage(fd, img_dt_props);

    ImageProps img_dt_a1_props = { "A1: depth target" };
    img_dt_a1_props.x = sc_x;
    img_dt_a1_props.y = sc_y;
    uint32_t img_dt_a1 = registerImage(fd, img_dt_a1_props);

    ImageProps img_rt_props = { "render target" };
    img_rt_props.x = sc_x;
    img_rt_props.y = sc_y;
    uint32_t img_rt = registerImage(fd, img_rt_props);

    ImageProps img_rt_a1_props = { "A1: render target" };
    img_rt_a1_props.x = sc_x;
    img_rt_a1_props.y = sc_y;
    uint32_t img_rt_a1 = registerImage(fd, img_rt_a1_props);

    ImageProps img_dpy_props = { "depth pyramid" };
    img_dpy_props.x = sc_x;
    img_dpy_props.y = sc_y;
    img_dpy_props.z = 16u;
    uint32_t img_dpy = registerImage(fd, img_dpy_props);

    ImageProps img_dpy_a1_props = { "A1: depth pyramid" };
    img_dpy_a1_props.x = sc_x;
    img_dpy_a1_props.y = sc_y;
    img_dpy_a1_props.z = 16u;
    uint32_t img_dpy_a1 = registerImage(fd, img_dpy_a1_props);

    uint32_t p1 = registerPass(fd, fg, "early cull pass", { buf_mesh, buf_transData, buf_mDr, buf_mDrCC, buf_mDrVis , img_dpy }, { buf_mDrCmd, buf_mDrCC_a1, buf_mDrVis_a1 }, RenderPassExeQueue::asyncCompute0);
    uint32_t p2 = registerPass(fd, fg, "early draw pass", { buf_vtx, buf_idx, buf_mesh, buf_mltData, buf_transData, buf_mDr, buf_mDrCmd, buf_mltVis , img_dpy }, { buf_mltVis_a1, img_ct, img_dt });
    uint32_t p3 = registerPass(fd, fg, "pyramid pass", { img_dt }, { img_dpy_a1 }, RenderPassExeQueue::asyncCompute0);
    uint32_t p4 = registerPass(fd, fg, "late cull pass", { buf_mesh, buf_transData, buf_mDr, buf_mDrCC_a1, buf_mDrVis_a1, img_dpy_a1 }, { buf_mDrCmd_a1, buf_mDrCC_a2, buf_mDrVis_a2 }, RenderPassExeQueue::asyncCompute0);
    uint32_t p5 = registerPass(fd, fg, "late draw pass", { buf_vtx, buf_idx, buf_mesh, buf_mltData, buf_transData, buf_mDr, buf_mDrCmd_a1, buf_mltVis_a1, img_dpy_a1 }, { buf_mltVis_a2, img_ct_a1, img_dt_a1 });
    uint32_t p6 = registerPass(fd, fg, "copy pass", { img_ct_a1 }, { img_rt });
    uint32_t p7 = registerPass(fd, fg, "UI pass", { img_rt }, { img_rt_a1 });

    fg._outputPass = p7;

    fg._multiFrameReses.insert(fg._multiFrameReses.end(), { img_dpy_a1, });

    addForceAliasBuffers(fg, fd, { buf_mDrCmd , buf_mDrCmd_a1, buf_mDrCmd_a2 });
    addForceAliasBuffers(fg, fd, { buf_mDrCC , buf_mDrCC_a1, buf_mDrCC_a2 });
    addForceAliasBuffers(fg, fd, { buf_mDrVis , buf_mDrVis_a1, buf_mDrVis_a2 });
    addForceAliasBuffers(fg, fd, { buf_mltVis , buf_mltVis_a1, buf_mltVis_a2 });

    addForceAliasImages(fg, fd, { img_ct , img_ct_a1 });
    addForceAliasImages(fg, fd, { img_dt , img_dt_a1 });
    addForceAliasImages(fg, fd, { img_rt , img_rt_a1 });
    addForceAliasImages(fg, fd, { img_dpy, img_dpy_a1 });
}


// p1 <-- p2 <-- p3 <-- p7
//   ____________|  ____|
//  /       /      /
// p4 <-- p5 <-- p6
//
void setupResources_test_a(FrameGraphData& fd, FrameGraph& fg)
{
    uint32_t r1 = 0x0001;
    uint32_t r2 = 0x0002;
    uint32_t r3 = 0x0003;
    uint32_t r4 = 0x0004;
    uint32_t r5 = 0x0005;
    uint32_t r6 = 0x0006;
    uint32_t r7 = 0x0007;

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
    uint32_t r1 = registerBuffer(fd, "r1", 1024 * 1024);
    uint32_t r2 = registerBuffer(fd, "r2", 1024 * 1024);
    uint32_t r3 = registerBuffer(fd, "r3", 1024 * 1024);
    uint32_t r4 = registerBuffer(fd, "r4", 1024 * 1024);
    uint32_t r5 = registerBuffer(fd, "r5", 1024 * 1024);
    uint32_t r6 = registerBuffer(fd, "r6", 1024 * 1024);
    uint32_t r7 = registerBuffer(fd, "r7", 1024 * 1024);

    uint32_t r11 = registerBuffer(fd, "r11", 1024 * 1024);
    uint32_t r22 = registerBuffer(fd, "r22", 1024 * 1024);
    uint32_t r33 = registerBuffer(fd, "r33", 1024 * 1024);
    uint32_t r44 = registerBuffer(fd, "r44", 1024 * 1024);

    uint32_t p1 = registerPass(fd, fg, "p1", { r11 }, { r1 });
    uint32_t p2 = registerPass(fd, fg, "p2", { r1 }, { r2 });
    uint32_t p3 = registerPass(fd, fg, "p3", { r2, r4, r5 }, { r3 });
    uint32_t p4 = registerPass(fd, fg, "p4", { r22 }, { r4 }, RenderPassExeQueue::asyncCompute0);
    uint32_t p5 = registerPass(fd, fg, "p5", { r4 }, { r5 }, RenderPassExeQueue::asyncCompute0);
    uint32_t p6 = registerPass(fd, fg, "p6", { r5 }, { r6, r44 }, RenderPassExeQueue::asyncCompute0);
    uint32_t p7 = registerPass(fd, fg, "p7", { r3, r6 }, { r7 });

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
    

    uint32_t r1 = 0x0001;
    uint32_t r2 = 0x0002;
    uint32_t r3 = 0x0003;
    uint32_t r4 = 0x0004;
    uint32_t r5 = 0x0005;
    uint32_t r6 = 0x0006;
    uint32_t r7 = 0x0007;

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
    uint32_t r1 = 0x0001;
    uint32_t r2 = 0x0002;
    uint32_t r3 = 0x0003;
    uint32_t r4 = 0x0004;
    uint32_t r5 = 0x0005;
    uint32_t r6 = 0x0006;
    uint32_t r7 = 0x0007;

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
    uint32_t r1 = 0x0001;
    uint32_t r2 = 0x0002;
    uint32_t r3 = 0x0003;
    uint32_t r4 = 0x0004;
    uint32_t r5 = 0x0005;
    uint32_t r6 = 0x0006;
    uint32_t r7 = 0x0007;

    uint32_t r11 = 0x0011;
    uint32_t r22 = 0x0022;
    uint32_t r33 = 0x0033;
    uint32_t r44 = 0x0044;


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


void DFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<uint32_t, bool>& visited, std::unordered_map<uint32_t, bool>& onStack, std::vector<uint32_t>& sortedPasses)
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

void DFS(FrameGraph& graph, FrameGraphData& fd, std::vector<uint32_t>& sortedPasses)
{
    uint32_t passCount = (uint32_t)graph._passes.size();
    std::unordered_map<uint32_t, bool> visited{};
    std::unordered_map<uint32_t, bool> onStack{};

    for (uint32_t i =0; i < passCount; ++i)
    {
        uint32_t id = graph._passes[i];
        visited.insert({ id, false });
        onStack.insert({ id, false });
    }

    for (uint32_t id : graph._passes)
    {
        if (!visited[id])
        {
            DFSVisit(fd, id, visited, onStack, sortedPasses);
        }
    }
}

void reverseDFSVisit(FrameGraphData& fd, uint32_t currentPassID, std::unordered_map<uint32_t, bool >& visited, std::unordered_map<uint32_t, bool>& onStack, std::vector<uint32_t>& sortedPassIDs)
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
void reverseTraversalDFS(FrameGraph& graph, FrameGraphData& fd, const uint32_t startPass, std::vector<uint32_t>& sortedPassIDs)
{
    // pass not exist
    assert(std::find(graph._passes.begin(), graph._passes.end(), startPass) != graph._passes.end());

    uint32_t passCount = (uint32_t)graph._passes.size();
    std::unordered_map<uint32_t, bool> visited{};
    std::unordered_map<uint32_t, bool> onStack{};

    for (uint32_t i = 0; i < passCount; ++i)
    {
        uint32_t id = graph._passes[i];
        visited.insert({ id, false });
        onStack.insert({ id, false });
    }

    uint32_t currentPass = startPass;


    if (!visited[currentPass])
    {
        reverseDFSVisit(fd, currentPass, visited, onStack, sortedPassIDs);
    }
}

void TopologicalSort(FrameGraph& graph, FrameGraphData& fd, std::vector<uint32_t>& sortedPasses)
{
    DFS(graph, fd, sortedPasses);

    std::reverse(sortedPasses.begin(), sortedPasses.end());
}


void formatDependencyLevels(FrameGraph& fg, FrameGraphData& fd, std::unordered_map<uint32_t, uint32_t> longestDists
    , std::unordered_map<uint32_t, std::vector<uint32_t>>& levelPasses, const uint32_t currentPass, const uint32_t baseLevel)
{
    
    const uint32_t level = longestDists[currentPass];
    const RenderPass& rp = fd._passes[currentPass];

    for (uint32_t id : rp.parentPassIDs)
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

    for (uint32_t id : rp.parentPassIDs)
    {
        formatDependencyLevels(fg, fd, longestDists, levelPasses, id, baseLevel);
    }

}


void buildDenpendencyLevel(FrameGraph& fg, FrameGraphData& fd)
{
    const std::vector<uint32_t>& visitedPasses = fg._linearVisitedPasses;

    message(info, "dependency level: \n");

    std::unordered_map<uint32_t, uint32_t> longestDists{};
    for (uint32_t i = 0; i < visitedPasses.size(); ++i)
    {
        uint32_t pass = visitedPasses[i];
        longestDists.insert({ pass, 0 });
    }

    uint32_t detectedQueueCount = 1u;

    for (uint32_t pass : visitedPasses)
    {
        const RenderPass& rp = fd._passes[pass];
        uint32_t longestDist = 0u;
        for (uint32_t parentPass : rp.parentPassIDs)
        {
            uint32_t dist = longestDists[parentPass] + 1u;
            longestDist = std::max(longestDist, dist);
        }
        longestDists[pass] = longestDist;
        fg._passesInQueue[rp._queue].push_back(pass);
        detectedQueueCount = std::max(detectedQueueCount, std::underlying_type_t<RenderPassExeQueue>(rp._queue) + 1u);
    }

    fg._detectedQueueCount = detectedQueueCount;

    for (uint32_t pass : visitedPasses)
    {
        std::unordered_map<DependLevel, std::vector<uint32_t>>& levelPasses = fg._dependedLevelsPerPass[pass]._dependPasses;
        formatDependencyLevels(fg, fd, longestDists, levelPasses, pass, longestDists[pass]);
    }

    for (uint32_t pass : visitedPasses)
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
    for (uint32_t pass : visitedPasses)
    {
        const std::unordered_map<DependLevel, std::vector<uint32_t>>& dependPasses = fg._dependedLevelsPerPass[pass]._dependPasses;
        RenderPass& rp = fd._passes[pass];
        
        std::unordered_map<RenderPassExeQueue, bool> foundSyncPass{};

        // if current pass depends on specific pass
        for (const auto& it : dependPasses)
        {
            uint32_t level = it.first;
            const std::vector<uint32_t>& currLeveldependPasses = it.second;

            for (uint32_t dp : currLeveldependPasses)
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
        uint32_t pass = *vpIt;
        const std::unordered_map<DependLevel, std::vector<uint32_t>>& dependPasses = fg._dependedLevelsPerPass[pass]._dependPasses;
        RenderPass& rp = fd._passes[pass];

        uint32_t syncQueueCount = 1u;
        std::vector<uint32_t> plainNearstSyncPasses{};
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
            const std::vector<uint32_t>& currLeveldependPasses = it.second;

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
        uint32_t pass = *vpIt;
        const std::unordered_map<DependLevel, std::vector<uint32_t>>& dependPasses = fg._dependedLevelsPerPass[pass]._dependPasses;
        RenderPass& rp = fd._passes[pass];

        uint32_t syncQueueCount = 1u;
        std::vector<uint32_t> plainNearstSyncPasses{};
        
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
    for (uint32_t pass : visitedPasses)
    {
        const std::unordered_map<DependLevel, std::vector<uint32_t>>& levelPasses = fg._dependedLevelsPerPass[pass]._dependPasses;

        if(levelPasses.empty())
            continue;

        message(info, "\tpass: %s\n", fd._passes[pass]._name.c_str());
        for (auto& it : levelPasses)
        {
            uint32_t level = it.first;
            const std::vector<uint32_t>& passes = it.second;

            for (uint32_t passID : passes)
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


bool isBufferAliasable(FrameGraphData& fd, const uint32_t res, const BufferBucket& bucket, const std::vector<uint32_t>& currLevelReses)
{
    assert(isBuffer(res));

    const ManagedBuffer& mb = fd._resmap._managedBuffers[res];

    // size checking
    size_t remaingSize = bucket._size;
    for (uint32_t clres : currLevelReses)
    {
        remaingSize -= fd._resmap._managedBuffers[clres]._size;
    }

    bool sizeMatch = mb._size <= remaingSize;


    // lifetime checking, should not overlap with any resource in current bucket
    // TODO: maybe use a stack only check pass overlap with current resource:
    // e.g. the memory aliasing as follow
    //     [r0---------------------]
    //     [r1--------][r2---------]
    //     [r3---][r4-]
    // current method: 
    //      every resource is not overlap with each other in lifetime range
    // maybe a better method: 
    //      only need to check if current resource is overlap with the top resource in stack, like r3 vs r2, r4 vs r2
    bool stackMatch = true;
    const std::pair<uint32_t, uint32_t> currResLifetime = fd._resmap.bufferLifetimeIdx[res];
    for (uint32_t renInBucket : bucket._reses)
    {
        std::pair<uint32_t, uint32_t> stackLifetime = fd._resmap.bufferLifetimeIdx[renInBucket];

        // if lifetime overlap with any resource in current bucket, then it's not aliasable
        stackMatch &= (stackLifetime.first > currResLifetime.second || stackLifetime.second < currResLifetime.first);
    }

    return sizeMatch && stackMatch;
}


void fillBufferBucket(FrameGraph& fg, FrameGraphData& fd, const std::vector<uint32_t>& sortedResbySize
    , std::vector<std::vector<uint32_t>>& resInLevel, BufferBucket& bucket)
{
    std::vector<uint32_t> restRes = sortedResbySize;
    resInLevel.push_back({});

    // find force aliasable resource
    for (uint32_t res : restRes)
    {
        const std::vector<uint32_t>& currLevelReses = resInLevel.back();
    }

    // find normal aliasable resource
    for (uint32_t res : restRes)
    {
        const std::vector<uint32_t>& currLevelReses = resInLevel.back();
        // if current resource is aliasable
        if (isBufferAliasable(fd, res, bucket, currLevelReses))
        {
            // add current resource into current level
            resInLevel.back().push_back(res);

            // add current resource into current bucket
            bucket._reses.push_back(res);
        }
    }

    if (!resInLevel.back().empty())
    {
        // remove resources that already in current level from rest resource
        for (uint32_t res : resInLevel.back())
        {
            restRes.erase(std::find(restRes.begin(), restRes.end(), res));
        }

        fillBufferBucket(fg, fd, restRes, resInLevel, bucket);
    }
}

void createBufferBucket(std::vector<BufferBucket>& buckets
    , const size_t sz, const std::vector<uint32_t>& reses, const std::vector<uint32_t>& forceAlias)
{
    BufferBucket bkt = {};
    bkt._size = sz;
    bkt._reses = reses;
    bkt._forceAliasedReses = forceAlias;
    bkt._idx = (uint32_t)buckets.size() + 1u;

    buckets.push_back(bkt);
}

void aliseBuffers(FrameGraph& fg, FrameGraphData& fd, const std::vector<uint32_t>& sortedResbySize
    , std::vector<BufferBucket>& buckets)
{
    std::vector<uint32_t> restRes = sortedResbySize;

    std::vector<std::vector<uint32_t>> resInLevel{{}};

    if (restRes.empty()) {
        return;
    }

    // process force alias resources first
    for (const std::vector<uint32_t> aliasBufs : fg._forceAliasBuffers)
    {
        assert(!aliasBufs.empty());
        
        std::vector<uint32_t> bktReses {};
        for (uint32_t albuf : aliasBufs)
        {
            auto it = std::find(begin(restRes), restRes.end(), albuf);

            if (it != restRes.end())
            {
                bktReses.push_back(albuf);
                restRes.erase(it);
            }
        }

        if (bktReses.empty())
            continue;

        // new bucket
        uint32_t baseRes = *begin(bktReses);
        createBufferBucket(buckets
            , fd._resmap._managedBuffers[baseRes]._size
            , bktReses, bktReses);
    }

    // now process normal alias resources
    while (!restRes.empty())
    {
        uint32_t baseRes = *restRes.begin();

        // check if current resource can alias into any existing bucket
        bool aliased = false;
        for (BufferBucket& bkt : buckets)
        {
            if (!isBufferAliasable(fd, baseRes, bkt, resInLevel.back()))
                continue;
            
            // aliasable, add current resource into current bucket
            bkt._reses.push_back(baseRes);

            resInLevel.back().push_back(baseRes);

            for (uint32_t res : bkt._reses)
            {
                auto it = std::find(restRes.begin(), restRes.end(), res);
                if (it != restRes.end())
                    restRes.erase(it);
            }
            aliased = true;
            break;
        }


        if (aliased)
        {
            continue;
        }

        // no suitable bucket found, create a new one
        createBufferBucket(buckets
            , fd._resmap._managedBuffers[baseRes]._size
            , { baseRes }, { baseRes });

        restRes.erase(restRes.begin());

        resInLevel.clear();
        resInLevel.push_back({});
    }

}


bool isImageAliasable(FrameGraphData& fd, const uint32_t res, const ImageBucket& bucket)
{
    assert(isImage(res));

    const ManagedImage& mi = fd._resmap._managedImages[res];

    bool aliasable = true;
    const std::pair<uint32_t, uint32_t> currResLifetime = fd._resmap.imageLifetimeIdx[res];
    for (uint32_t renInBucket : bucket._reses)
    {
        // if lifetime overlap with any resource in current bucket, then it's not aliasable
        std::pair<uint32_t, uint32_t> stackLifetime = fd._resmap.imageLifetimeIdx[renInBucket];
        aliasable &= (stackLifetime.first > currResLifetime.second || stackLifetime.second < currResLifetime.first);

        // check size
        aliasable &= (fd._resmap._managedImages[renInBucket]._x = mi._x
            && fd._resmap._managedImages[renInBucket]._y == mi._y
            && fd._resmap._managedImages[renInBucket]._z == mi._z);

        // check mips
        aliasable &= (fd._resmap._managedImages[renInBucket]._mipLevels == mi._mipLevels);

        // TODO: check usage
        // TODO
        // TODO!!!!!
    }

    return aliasable;
}


void fillImageBucket(FrameGraph& fg, FrameGraphData& fd, const std::vector<uint32_t>& sortedResbySize, ImageBucket& bucket)
{
    std::vector<uint32_t> restRes = sortedResbySize;

    if (restRes.empty())
        return;

    // find the first aliasable resource
    for (uint32_t res : restRes)
    {
        // if current resource is aliasable
        if (isImageAliasable(fd, res, bucket))
        {
            // add current resource into current bucket
            bucket._reses.push_back(res);
        }
    }
}

void createImageBucket(std::vector<ImageBucket>& buckets
    , const uint32_t x, const uint32_t y, const uint32_t z, const std::vector<uint32_t>& reses, const std::vector<uint32_t>& forceAlias)
{
    ImageBucket bkt = {};
    bkt._x = x;
    bkt._y = y;
    bkt._z = z;
    bkt._reses = reses;
    bkt._forceAliasedReses = forceAlias;
    bkt._idx = (uint32_t)buckets.size() + 1u;

    buckets.push_back(bkt);
}

void aliaseImages(FrameGraph& fg, FrameGraphData& fd, const std::vector<uint32_t>& sortedResbySize
    , std::vector<ImageBucket>& buckets)
{
    std::vector<uint32_t> restRes = sortedResbySize;

    // process force alias resources first
    for (const std::vector<uint32_t> aliasBufs : fg._forceAliasImages)
    {
        assert(!aliasBufs.empty());

        std::vector<uint32_t> bktReses {};
        for (uint32_t albuf : aliasBufs)
        {
            auto it = std::find(restRes.begin(), restRes.end(), albuf);

            if (it != restRes.end())
            {
                bktReses.push_back(albuf);
                restRes.erase(it);
            }
        }

        if (bktReses.empty())
            continue;

        uint32_t baseRes = *bktReses.begin();
        // new bucket
        createImageBucket(buckets
            , fd._resmap._managedImages[baseRes]._x
            , fd._resmap._managedImages[baseRes]._y
            , fd._resmap._managedImages[baseRes]._z
            , bktReses, bktReses);
    }

    while (!restRes.empty())
    {
        uint32_t baseRes = *restRes.begin();

        // check if current resource can alias into any existing bucket
        bool aliased = false;
        for (ImageBucket& bkt : buckets)
        {
            if (!isImageAliasable(fd, baseRes, bkt))
                continue;

            // aliasable, add current resource into current bucket
            bkt._reses.push_back(baseRes);

            for (uint32_t res : bkt._reses)
            {
                auto it = std::find(restRes.begin(), restRes.end(), res);
                if (it != restRes.end())
                    restRes.erase(it);
            }
            aliased = true;
            break;
        }


        if (aliased)
        {
            continue;
        }

        // if not aliasiable, create a new bucket
        createImageBucket(buckets
            , fd._resmap._managedImages[baseRes]._x
            , fd._resmap._managedImages[baseRes]._y
            , fd._resmap._managedImages[baseRes]._z
            , { baseRes }, { baseRes });

        // remove current one from rest resource
        restRes.erase(restRes.begin());
    }
}

void aliaseImages2(FrameGraph& fg, FrameGraphData& fd, const std::vector<uint32_t>& sortedResbySize
    , std::vector<ImageBucket>& buckets)
{
    std::vector<uint32_t> restRes = sortedResbySize;

    while (!restRes.empty())
    {
        uint32_t baseRes = *restRes.begin();

        // new bucket
        ImageBucket bucket{};
        bucket._x = fd._resmap._managedImages[baseRes]._x;
        bucket._y = fd._resmap._managedImages[baseRes]._y;
        bucket._z = fd._resmap._managedImages[baseRes]._z;
        bucket._reses.push_back(baseRes);
        bucket._idx = (uint32_t)buckets.size();

        buckets.push_back(bucket);

        // remove current one from rest resource
        restRes.erase(restRes.begin());


        fillImageBucket(fg, fd, restRes, buckets.back());

        // once fillImageBucket returns, which means one bucket finished
        // remove resources in current bucket from restRes list
        for (uint32_t res : buckets.back()._reses)
        {
            auto it = std::find(restRes.begin(), restRes.end(), res);
            if (it != restRes.end())
                restRes.erase(it);
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
    setupResources_managed(fd, fg);
    //setupResources_test_a_managed(fd, fg);

    // make a plain table that can find producer of a resource easily.
    std::vector<uint32_t> all_outResID;
    std::vector<uint32_t> all_parentPassID;
    for (uint32_t passID : fg._passes)
    {
        RenderPass& rp = fd._passes[passID];

        all_outResID.insert(all_outResID.end(), rp.outputResIDs.begin(), rp.outputResIDs.end());
        std::vector<uint32_t> tmp(rp.outputResIDs.size(), passID);
        all_parentPassID.insert(all_parentPassID.end(), tmp.begin(), tmp.end());

        assert(all_outResID.size() == all_parentPassID.size());
    }

    // build graph based on resource dependency
    for (uint32_t passID : fg._passes)
    {
        RenderPass& rp = fd._passes[passID];

        for (uint32_t j = 0; j < rp.inputResIDs.size(); ++j)
        {
            uint32_t currentResID = rp.inputResIDs[j];
            std::vector<uint32_t>::iterator it = std::find(all_outResID.begin(), all_outResID.end(), currentResID);
            if (it == all_outResID.end())
                continue; // not found producer for current resource, might be a read only resource

            uint32_t parentPassID = all_parentPassID[it - all_outResID.begin()];
            rp.parentPassIDs.push_back(parentPassID); // set parent pass for current pass
            fd._passes[parentPassID].childPassIDs.push_back(passID); // set child pass back
        }
    }

    // reverse traversal DFS to cut unnecessary passes
    std::vector<uint32_t>& visitedPasses = fg._linearVisitedPasses;
    reverseTraversalDFS(fg, fd, fg._outputPass, visitedPasses);

    // https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
    // build dependency levels
    buildDenpendencyLevel(fg, fd);

    // print reverse traversal DFS
    {
        message(info, "visited pass: \n");
        for (uint32_t passID : visitedPasses)
        {
            message(info, "\t pass: %s\n", fd._passes[passID]._name.c_str());
        }
    }

    // build lifetime table
    {
        std::map<uint32_t, std::vector<uint32_t>> lifetimeTable;
        std::set<uint32_t> readReses;
        std::set<uint32_t> wroteReses;
        for (uint32_t passID : fg._passes)
        {
             RenderPass& rp = fd._passes[passID];

             // add all input resources
             for (uint32_t resID : rp.inputResIDs)
             {
                std::vector<uint32_t>& passIDs = lifetimeTable[resID];
                passIDs.push_back(passID);
                readReses.insert(resID);
             }

             // add all output resources
             for (uint32_t resID : rp.outputResIDs)
             {
                std::vector<uint32_t>& passIDs = lifetimeTable[resID];
                passIDs.push_back(passID);
                wroteReses.insert(resID);
             }
        }

        std::vector<uint32_t> readonlyReses;
        {
            for (uint32_t res : readReses)
            {
                if (std::find(wroteReses.begin(), wroteReses.end(), res) == wroteReses.end())
                {
                    readonlyReses.push_back(res);
                }
            }
        }

        // now add two pass into the visitedPass, they are fake pass to represent frame start and frame end
        uint32_t frameStart = 0x0000;
        uint32_t frameEnd = 0x1111;
        std::vector<uint32_t> visitedPassesWithFrameStartEnd = visitedPasses;
        visitedPassesWithFrameStartEnd.insert(visitedPassesWithFrameStartEnd.begin(), { frameStart });
        visitedPassesWithFrameStartEnd.push_back(frameEnd);

        for (auto& resPair : lifetimeTable)
        {
            std::vector<uint32_t>& passIDs = resPair.second;
            if (std::find(readonlyReses.begin(), readonlyReses.end(), resPair.first) != readonlyReses.end())
            {
                passIDs.insert(passIDs.begin(), frameStart);
            }
            if (std::find(fg._multiFrameReses.begin(), fg._multiFrameReses.end(), resPair.first) != fg._multiFrameReses.end())
            {
                passIDs.push_back(frameEnd);
            }
        }

        std::unordered_map<uint32_t, int32_t> activedResourceCount{};

        // initialize activedPassCount
        for (uint32_t passID : visitedPassesWithFrameStartEnd)
        {
            activedResourceCount.insert({ passID, 0 });
        }

        // process lifetime:
        for (auto& resPair : lifetimeTable)
        {
            std::vector<uint32_t>& passIDs = resPair.second;
            std::vector<uint32_t>::iterator right = visitedPassesWithFrameStartEnd.begin(), left = visitedPassesWithFrameStartEnd.end();

            for (uint32_t id : passIDs)
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
                fd._resmap.imageLifetimeIdx.insert({ resPair.first, {uint32_t(left - visitedPassesWithFrameStartEnd.begin()), uint32_t(right - visitedPassesWithFrameStartEnd.begin())} });
            }
            
            for (uint32_t passID : visitedPassesWithFrameStartEnd)
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
        for (uint32_t passID : visitedPassesWithFrameStartEnd)
        {
            currActResourceCount += activedResourceCount[passID];
            activedResourceCount[passID] = currActResourceCount;
        }


        // print lifetime table
        {
            char msg[4096];
            snprintf(msg, COUNTOF(msg), "resource lifetime: \n");
            snprintf(msg, COUNTOF(msg), "%s|      |", msg);
            for (uint32_t id : visitedPassesWithFrameStartEnd)
            {
                snprintf(msg, COUNTOF(msg), "%s %4x |", msg, id);
            }
            snprintf(msg, COUNTOF(msg), "%s\n", msg);
            for (auto& it : lifetimeTable)
            {
                snprintf(msg, COUNTOF(msg), "%s| %4x |", msg, it.first);
                for (uint32_t passID : visitedPassesWithFrameStartEnd)
                {
                    std::vector<uint32_t>& passIDs = it.second;
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
        }

        for (auto& resPair : activedResourceCount)
        {
            message(info, "[0x%4x]:[%d]\n", resPair.first, resPair.second);
        }

        // TODO: sort resources by size
        std::vector<uint32_t> sortedBufs_size{};
        {
            std::vector<uint32_t> sortedReses{};
            for (auto& resPair : lifetimeTable)
            {
                if (!isBuffer(resPair.first))
                    continue;

                sortedReses.push_back(resPair.first);
            }

            std::sort(sortedReses.begin(), sortedReses.end(), [&](uint32_t a, uint32_t b) {
                return fd._resmap._managedBuffers[a]._size > fd._resmap._managedBuffers[b]._size;
            });

            sortedBufs_size = sortedReses;
        }

        std::vector<uint32_t> sortedImgs_size{};
        {
            std::vector<uint32_t> sortedReses{};
            for (auto& resPair : lifetimeTable)
            {
                if (!isImage(resPair.first))
                    continue;

                sortedReses.push_back(resPair.first);
            }

            std::sort(sortedReses.begin(), sortedReses.end(), [&](uint32_t a, uint32_t b) {
                size_t sz_a = fd._resmap._managedImages[a]._x * fd._resmap._managedImages[a]._y * fd._resmap._managedImages[a]._z * fd._resmap._managedImages[a]._mipLevels;
                size_t sz_b = fd._resmap._managedImages[b]._x * fd._resmap._managedImages[b]._y * fd._resmap._managedImages[b]._z * fd._resmap._managedImages[b]._mipLevels;

                return sz_a > sz_b;
                });

            sortedImgs_size = sortedReses;
        }

        std::vector<BufferBucket> bufferBuckets{};
        std::vector<uint32_t> aliasedReses{};
        aliseBuffers(fg, fd, sortedBufs_size, bufferBuckets);


        {
            char msg[4096];
            size_t totalSize = 0;
            size_t none_aliased_size = 0;
            snprintf(msg, COUNTOF(msg), "\taliased resource: \n");
            for (const BufferBucket& bkt : bufferBuckets)
            {
                snprintf(msg, COUNTOF(msg), "%s\n", msg);
                for (uint32_t res : bkt._reses)
                {
                    snprintf(msg, COUNTOF(msg), "%s\t\t0x%4x\t%s\n", msg, res, fd._resmap._managedBuffers[res]._name.c_str());
                    none_aliased_size += fd._resmap._managedBuffers[res]._size;
                }
                totalSize += bkt._size;
            }
            message(info, "%s", msg);
            message(info, "total size: %dMB/%dMB\n", totalSize / 1'000'000u, none_aliased_size/ 1'000'000u);
        }

        std::vector<ImageBucket> imageBuckets{};
        aliaseImages(fg, fd, sortedImgs_size, imageBuckets);

        {
            char msg[4096];
            size_t totalSize = 0;
            snprintf(msg, COUNTOF(msg), "\taliased image: \n");
            for (const ImageBucket& bkt : imageBuckets)
            {
                snprintf(msg, COUNTOF(msg), "%s\n", msg);
                for (uint32_t res : bkt._reses)
                {
                    snprintf(msg, COUNTOF(msg), "%s\t\t0x%4x\t%s\n", msg, res, fd._resmap._managedImages[res]._name.c_str());
                }
                totalSize += (bkt._x * bkt._y * bkt._z);
            }
            message(info, "%s", msg);

            message(info, "total size: %d\n", totalSize);
        }
    }
}

}; // namespace vkz