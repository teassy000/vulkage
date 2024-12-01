
#include "terrain.h"
#include "kage.h"
#include "file_helper.h"
#include "kage_math.h"

// read data to image 
// build terrain mesh via cs

const uint32_t kTerrainTileSize = 256;

static uint32_t nextPow2(uint32_t v)
{
    uint32_t r = 1;

    while (r < v)
        r <<= 1;
    return r;
}

struct TerrainConstants
{
    float bx, by, bz; // base pos

    float sh, sv; // size horizon, vertical
    
    uint32_t w, h; // image size

    uint32_t tcw, tch; // tile count
};

struct TerrainVertex
{
    float vx, vy, vz;
    float tu, tv;
};

struct TerrainTile
{
    uint32_t x, y;
    uint32_t w, h;
};

void buildTerrainMesh(float _x_step, float _y_step, float _h_step)
{
    // Load heightmap from file
    const char* path = "./data/heightmap/heightmap.ktx";
    const char* name = "heightmap";
    textureResolution res{};
    kage::ImageHandle hImg = loadImageFromFile(name, path, res);

    uint16_t width = res.width;
    uint16_t height = res.height;

    // Image size must be power of 2 - 1
    uint16_t pow2_width = nextPow2(width) - 1;
    uint16_t pow2_height = nextPow2(height) - 1;
    if ( width + 1 != pow2_width
        || height + 1 != pow2_height)
    {
        kage::message(kage::error, "Image[%d]: %s res: [%d * %d] not 2^n -1", hImg.id, name, width, height);
        return;
    }

    // create vertex buffer
    kage::BufferDesc vtxDesc;
    vtxDesc.size = pow2_width * pow2_height * sizeof(TerrainVertex);
    vtxDesc.usage = kage::BufferUsageFlagBits::vertex;
    kage::BufferHandle vtxBuf = kage::registBuffer("terrain_vtx", vtxDesc);

    // create index buffer
    kage::BufferDesc idxDesc;
    idxDesc.size = pow2_width * pow2_height * 6 * sizeof(uint32_t);

    // setup cs process
    kage::ShaderHandle hShader = kage::registShader("terrain_shader", "./data/shaders/terrain_genvtx.comp.spv");
    kage::ProgramHandle hProgram = kage::registProgram("terrain_prog", { hShader }, sizeof(TerrainConstants));

    kage::PassHandle hPass = kage::registPass("terrain_pass", { hProgram.id, kage::PassExeQueue::compute });

    // add dependencies
    kage::sampleImage(hPass, hImg, 0, kage::PipelineStageFlagBits::compute_shader, kage::SamplerFilter::nearest, kage::SamplerMipmapMode::nearest, kage::SamplerAddressMode::clamp_to_edge, kage::SamplerReductionMode::min);
    
    // create vertex buffer
}

