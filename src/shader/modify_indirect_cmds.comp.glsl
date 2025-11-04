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
const uint _MESHLET_CULLING     = 1; // modify as (count, 1, 1)
const uint _TRIANGLE_CULLING    = 2; // modify as (count, 1, 1)
const uint _SOFT_RASTER         = 3; // modify as (count / MR_SOFT_RASTGP_SIZE, rt_width, rt_height)
const uint _HARD_RASTER         = 4; // modify as (count / MESHGP_SIZE, MESHGP_SIZE, 1)
const uint _TASK_MODIFY         = 5; // modify as (count / MESHGP_SIZE, MESHGP_SIZE, 1)

// local_size_x must be greater than:
// group size like MR_MESHLETGP_SIZE, MR_TRIANGLEGP_SIZE, MR_SOFT_RASTGP_SIZE
// so that we can modify all dispatch commands in one dispatch
// all size above must be less than or equal to local_size_x
layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

const uint c_group_size[] = {
    1,                  // _CLEAR
    MR_MESHLETGP_SIZE,  // _MESHLET_CULLING
    MR_TRIANGLEGP_SIZE, // _TRIANGLE_CULLING
    MR_SOFT_RASTGP_SIZE,// _SOFT_RASTER
    MESHGP_SIZE,        // _HARD_RASTER
    MESHGP_SIZE         // _TASK_MODIFY
};

const uint c_cmd_idx[] =
{
    0, // _CLEAR
    0, // _MESHLET_CULLING
    0, // _TRIANGLE_CULLING
    0, // _SOFT_RASTER
    1, // _HARD_RASTER
    0  // _TASK_MODIFY
};

layout(push_constant) uniform block
{
    uvec2 res;
};

// read/write

// special case:
// _SOFT_RASTER & _HARD_RASTER share the same indirect dispatch command structure
// 0: _SOFT_RASTER
// 1: _HARD_RASTER
layout(binding = 0) buffer IndirectCmd
{
    IndirectDispatchCommand cmd[];
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

layout(binding = 1) buffer TaskCommands
{
    MeshTaskCommand taskCmds [];
};

void main()
{
    const uint id = gl_LocalInvocationID.x;

    uint cmd_idx = c_cmd_idx[DISPATCH_MODE];

    uint count = cmd[cmd_idx].count;

    uint gp_size = c_group_size[DISPATCH_MODE];

    if (id == 0)
    {
        // x dimension is number of groups to dispatch
        if (DISPATCH_MODE == _CLEAR)
        {
            cmd[cmd_idx].local_x = 1;
            cmd[cmd_idx].local_y = 1;
            cmd[cmd_idx].local_z = 1;
        }
        else if (DISPATCH_MODE == _MESHLET_CULLING)
        {
            cmd[cmd_idx].local_x = count;
            cmd[cmd_idx].local_y = 1;
            cmd[cmd_idx].local_z = 1;
        }
        else if (DISPATCH_MODE == _TRIANGLE_CULLING)
        {
            cmd[cmd_idx].local_x = count;
            cmd[cmd_idx].local_y = 1;
            cmd[cmd_idx].local_z = 1;
        }
        else if (DISPATCH_MODE == _SOFT_RASTER)
        {
            cmd[cmd_idx].local_x = min((count + gp_size - 1) / gp_size, 65535);
            cmd[cmd_idx].local_y = (res.x + MR_SOFT_RAST_TILE_SIZE  - 1 )/ MR_SOFT_RAST_TILE_SIZE;
            cmd[cmd_idx].local_z = (res.y + MR_SOFT_RAST_TILE_SIZE  - 1 )/ MR_SOFT_RAST_TILE_SIZE;
        }
        else if (DISPATCH_MODE == _HARD_RASTER)
        {
            cmd[cmd_idx].local_x = min((count + gp_size - 1) / gp_size, 65535);
            cmd[cmd_idx].local_y = gp_size;
            cmd[cmd_idx].local_z = 1;
        }
        else if (DISPATCH_MODE == _TASK_MODIFY)
        {
            cmd[cmd_idx].local_x = min((count + gp_size - 1) / gp_size, 65535);
            cmd[cmd_idx].local_y = gp_size;
            cmd[cmd_idx].local_z = 1;
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
            else if (DISPATCH_MODE == _SOFT_RASTER)
            {
                TrianglePayload dummy = {};
                dummy.drawId = 5000 + id;
                dummy.meshletIdx = 5000 + id;

                triCmds[count + id] = dummy;
            }
            else if (DISPATCH_MODE == _HARD_RASTER)
            {
                TrianglePayload dummy = {};
                dummy.drawId = 6000 + id;
                dummy.meshletIdx = 6000 + id;

                triCmds[count + id] = dummy;
            }
            else if (DISPATCH_MODE == _TASK_MODIFY)
            {
                MeshTaskCommand dummy = {};
                taskCmds[count + id] = dummy;
            }
        }
    }
}