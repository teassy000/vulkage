# version 450

# extension GL_EXT_shader_16bit_storage: require
# extension GL_EXT_shader_8bit_storage: require

# extension GL_GOOGLE_include_directive: require

# include "mesh_gpu.h"
# include "rc_common.h"


layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(push_constant) uniform block
{
    VoxelizationConsts consts;
};

layout(location = 0) in flat uint in_drawId[];
layout(location = 1) in vec2 in_uv[];
layout(location = 2) in vec3 in_norm[];

layout(location = 0) out flat uint out_drawId;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec4 out_tangent;
layout(location = 4) out vec3 out_pos;

layout(location = 5) out flat vec3 out_minAABB;
layout(location = 6) out flat vec3 out_maxAABB;


const float RIDICAL_3 = 1.732050808;
const float RIDICAL_2 = 1.414213562;

void main()
{
    vec4 verts[3];
    for (uint ii = 0; ii < 3; ii++)
    {
        verts[ii] = gl_in[ii].gl_Position;
    }

    vec3 tri_norm = cross(verts[1].xyz - verts[0].xyz, verts[2].xyz - verts[0].xyz);
    tri_norm = abs(tri_norm);

    // calculate the destination plane: the greatest normal component would be the plane normal
    // x = 0
    // y = 1
    // z = 2
    uint maxIdx =
        (tri_norm.y > tri_norm.x)
        ? ((tri_norm.z > tri_norm.y) ? 2 : 1)
        : ((tri_norm.z > tri_norm.x) ? 2 : 0);

    vec3 voxRange = vec3(consts.voxGridCount, consts.voxGridCount, consts.voxGridCount);
    vec3 hvoxelSz = 1.f / voxRange;
    float voxelDiag = hvoxelSz.x * RIDICAL_3;

    vec3 edges[3];
    vec3 edgeNorms[3]; // edge normal: to the outside of the triangle
    vec3 minAABB = vec3(2.0);
    vec3 maxAABB = vec3(-2.0);
    for (uint ii = 0; ii < 3; ii++)
    {
        edges[ii] = normalize(verts[(ii + 1) % 3].xyz / verts[(ii + 1) % 3].w - verts[ii].xyz / verts[ii].w);
        edgeNorms[ii] = normalize(cross(edges[ii], tri_norm));
        minAABB = min(minAABB, verts[ii].xyz);
        maxAABB = max(maxAABB, verts[ii].xyz);
    }

    // conservative fit the voxel
    minAABB -= hvoxelSz;
    maxAABB += hvoxelSz;

    // clip space range is:
    // x: [-1.0, 1.0]
    // y: [-1.0, 1.0]
    // z: [0.0, 1.0]
    // mapping z to the clip space
    minAABB.z = (minAABB.z * .5f) + .5f;
    maxAABB.z = (maxAABB.z * .5f) + .5f;

    // project the triangle on the plane
    // conservative enlarge the triangle to fit the voxel, using the 2nd method of:
    // https://developer.nvidia.com/gpugems/gpugems2/part-v-image-oriented-computing/chapter-42-conservative-rasterization
    for (uint ii = 0; ii < 3; ii++)
    {
        vec3 ee0 = edges[(ii + 2) % 3] / dot(edges[(ii + 2) % 3], edgeNorms[ii]);
        vec3 ee1 = edges[ii] / dot(edges[ii], edgeNorms[(ii + 2) % 3]);
        // for simplification
        // enlarge the triangle by the same factor(the longest diagnal line)
        vec3 bisct = voxelDiag * (ee0 + ee1); 

        vec3 pos = verts[ii].xyz / verts[ii].w + bisct;
        pos.z = (pos.z * .5f) + .5f; // mapping z to the clip space, or this would cut by the rasterzation stage

        out_pos = pos;
        out_minAABB = minAABB;
        out_maxAABB = maxAABB;
        out_uv = in_uv[ii];
        out_drawId = in_drawId[ii];

        switch (maxIdx)
        {
            case 0:
                gl_Position = vec4(verts[ii].yz + bisct.yz, 0.f, verts[ii].w);
                break;
            case 1:
                gl_Position = vec4(verts[ii].xz + bisct.xz, 0.f, verts[ii].w);
                break;
            case 2:
                gl_Position = vec4(verts[ii].xy + bisct.xy, 0.f, verts[ii].w);
                break;
        }
        EmitVertex();
    }

    EndPrimitive();
}