#pragma once

extern struct Graph;

struct RenderPass
{
    // only require in/out resource to deduct the 
    virtual void SetupResources() = 0;
    virtual void JoinGraph(Graph& graph) = 0;
    virtual void BuildCommand() = 0;
};
