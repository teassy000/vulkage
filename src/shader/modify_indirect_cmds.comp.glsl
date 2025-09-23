# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

# extension GL_EXT_null_initializer: require

#include "mesh_gpu.h"

layout(constant_id = 0) const int DISPATCH_MODE = 0; 

const uint _CLEAR                 = 0; // clear as 0, 1, 1
const uint _MESHLET_CULLING     = 1; // modify as (count / MR_MESHLETGP_SIZE, 1, 1)
const uint _TRIANGLE_CULLING    = 2; // modify as (count / MR_TRIANGLEGP_SIZE, 1, 1)
const uint _SOFT_RASTERIZATION  = 3; // modify as (count, rt_width, rt_height)

// local_size_x must be greater than:
// group size like MR_MESHLETGP_SIZE, MR_TRIANGLEGP_SIZE, MR_SOFT_RASTGP_SIZE
// so that we can modify all dispatch commands in one dispatch
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block
{
    uvec2 res;
};

// read/write
layout(binding = 0) buffer IndirectCmd
{
    IndirectDispatchCommand cmds[];
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
    uvec3 triCmds [];
};

void main()
{
    const uint id = gl_LocalInvocationID.x;

    uint count = cmds[id].local_x;
    uint gp_size = 1;
    if (id == 0)
    {
        if (DISPATCH_MODE == _CLEAR)
        {
            cmds[id].local_x = 0;
            cmds[id].local_y = 1;
            cmds[id].local_z = 1;
        }
        else if (DISPATCH_MODE == _MESHLET_CULLING)
        {
            gp_size = MR_MESHLETGP_SIZE;

            cmds[id].local_y = gp_size;
            cmds[id].local_z = 1;
        }
        else if (DISPATCH_MODE == _TRIANGLE_CULLING)
        {
            gp_size = MR_TRIANGLEGP_SIZE;

            cmds[id].local_y = gp_size;
            cmds[id].local_z = 1;
        }
        else if (DISPATCH_MODE == _SOFT_RASTERIZATION)
        {
            gp_size = MR_SOFT_RASTGP_SIZE;

            cmds[id].local_y = res.x;
            cmds[id].local_z = res.y;
        }

        cmds[id].local_x = min((count + gp_size - 1) / gp_size, 65535);
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
                meshCmds[count + id] = dummy;
            }
            else if (DISPATCH_MODE == _TRIANGLE_CULLING)
            {
                MeshletPayload dummy = { };
                mltCmds[count + id] = dummy;
            }
            else if (DISPATCH_MODE == _SOFT_RASTERIZATION)
            {
                uvec3 dummy = uvec3(0, 0, 0);
                triCmds[count + id] = dummy;
            }
        }
    }
}