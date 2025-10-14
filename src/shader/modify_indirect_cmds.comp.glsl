# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

// for using uint8_t in general code
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_EXT_shader_explicit_arithmetic_types_int8: require

// for default initialization of struct
# extension GL_EXT_null_initializer: require

# extension GL_GOOGLE_include_directive: require


#include "mesh_gpu.h"

layout(constant_id = 0) const int DISPATCH_MODE = 0; 

const uint _CLEAR               = 0; // clear as 0, 1, 1
const uint _MESHLET_CULLING     = 1; // modify as (count / MR_MESHLETGP_SIZE, 1, 1)
const uint _TRIANGLE_CULLING    = 2; // modify as (count / MR_TRIANGLEGP_SIZE, 1, 1)
const uint _SOFT_RASTERIZATION  = 3; // modify as (count, rt_width, rt_height)

// local_size_x must be greater than:
// group size like MR_MESHLETGP_SIZE, MR_TRIANGLEGP_SIZE, MR_SOFT_RASTGP_SIZE
// so that we can modify all dispatch commands in one dispatch
// all size above must be less than or equal to local_size_x
layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

const uint c_group_size[] = {
    1,                  // _CLEAR
    MR_MESHLETGP_SIZE, // _MESHLET_CULLING
    MR_TRIANGLEGP_SIZE,// _TRIANGLE_CULLING
    MR_SOFT_RASTGP_SIZE// _SOFT_RASTERIZATION
};

layout(push_constant) uniform block
{
    uvec2 res;
};

// read/write
layout(binding = 0) buffer IndirectCmd
{
    IndirectDispatchCommand cmd;
};

layout(binding = 1) buffer MeshDrawCommands
{
    MeshDrawCommand meshCmds[];
};

layout(binding = 1) buffer MeshletCommands
{
    MeshletPayload mltCmds[]; 
};

layout(binding = 1) buffer TriangleCommands
{
    TrianglePayload triCmds [];
};

void main()
{
    const uint id = gl_LocalInvocationID.x;

    uint count = cmd.count;

    uint gp_size = c_group_size[DISPATCH_MODE];

    if (id == 0)
    {
        // x dimension is number of groups to dispatch
        cmd.local_x = min((count + gp_size - 1) / gp_size, 65535);

        if (DISPATCH_MODE == _CLEAR)
        {
            cmd.local_x = 0;
            cmd.local_y = 1;
            cmd.local_z = 1;
        }
        else if (DISPATCH_MODE == _MESHLET_CULLING)
        {
            cmd.local_y = gp_size;
            cmd.local_z = 1;
        }
        else if (DISPATCH_MODE == _TRIANGLE_CULLING)
        {
            cmd.local_y = gp_size;
            cmd.local_z = 1;
        }
        else if (DISPATCH_MODE == _SOFT_RASTERIZATION)
        {
            cmd.local_y = (res.x + MR_SOFT_RAST_TILE_SIZE  - 1 )/ MR_SOFT_RAST_TILE_SIZE;
            cmd.local_z = (res.y + MR_SOFT_RAST_TILE_SIZE  - 1 )/ MR_SOFT_RAST_TILE_SIZE;
        }
    }

    // fill the rest dispatch commands as dummy
    if (id < gp_size)
    {
        uint boundary = (count + gp_size - 1) & ~(gp_size - 1);

        if (count + id < boundary)
        {
            if (DISPATCH_MODE == _MESHLET_CULLING)
            {
                MeshDrawCommand dummy = { };
                dummy.drawId = 1000 + id;
                dummy.taskOffset = 1000 + id;
                dummy.local_x = 2000 + id;
                dummy.local_y = 2000 + id;
                dummy.local_z = 2000 + id;

                meshCmds[count + id] = dummy;
            }
            else if (DISPATCH_MODE == _TRIANGLE_CULLING)
            {
                MeshletPayload dummy = { };
                dummy.meshletIdx = 3000 + id;
                dummy.drawId = 4000 + id;

                mltCmds[count + id] = dummy;
            }
            else if (DISPATCH_MODE == _SOFT_RASTERIZATION)
            {
                TrianglePayload dummy = {};
                dummy.drawId = 5000 + id;
                dummy.meshletIdx = 5000 + id;

                triCmds[count + id] = dummy;
            }
        }
    }
}