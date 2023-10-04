#pragma once

typedef uint32_t PassID;

struct RenderPass
{
    PassID ID;
    std::string name;
    
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

struct FrameGraph
{
    std::vector<PassID> passes;

    std::vector<ResourceID> readResources;
    std::vector<ResourceID> writenResources;
};

void buildGraph(FrameGraph& graph);
void getResourceAliases(ResourceAlias& output, ResourceID id, const ResourceTable& table);
void getResource(Image& image, Buffer& buffer, ResourceMap resMap, const ResourceID id);
void resetActiveAlias(ResourceTable& table); // call at the end of every frame
ResourceID getActiveAlias(const ResourceTable& table, const ResourceID baseResourceID);
ResourceID nextAlias(ResourceTable& table, const ResourceID id); // call after a pass wrote to a resource