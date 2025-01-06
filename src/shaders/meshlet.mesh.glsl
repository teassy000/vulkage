#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_mesh_shader: require

#extension GL_GOOGLE_include_directive: require


#include "mesh_gpu.h"
#include "math.h"

layout(constant_id = 1) const bool ALPHA_PASS = false;
layout(constant_id = 2) const bool SEAMLESS_LOD = false;

#define LIGHT 0
#define CULL 1

layout(local_size_x = MESHGP_SIZE, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices= 64, max_primitives = 64) out;

layout(push_constant) uniform block 
{
    Globals globals;
};
// readonly
layout(binding = 2) readonly buffer MeshDraws
{
    MeshDraw meshDraws [];
};

layout(binding = 3) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 4) readonly buffer Vertices
{
    Vertex vertices [];
};

layout(binding = 5) readonly buffer Clusters
{
    Cluster clusters [];
};

layout(binding = 5) readonly buffer Meshlets
{
    Meshlet meshlets[];
};

layout(binding = 6) readonly buffer MeshletData
{
    uint meshletData[];
};

layout(binding = 6) readonly buffer MeshletData8
{
    uint8_t meshletData8[];
};

layout(location = 0) out flat uint out_drawId[];
layout(location = 1) out vec3 out_wPos[];
layout(location = 2) out vec3 out_norm[];
layout(location = 3) out vec4 out_tan[];
layout(location = 4) out vec2 out_uv[];

taskPayloadSharedEXT TaskPayload payload;

#if CULL
shared vec3 vertexClip[64];
#endif

void main()
{
    uint ti = gl_LocalInvocationID.x;
    uint ci = payload.meshletIndices[gl_WorkGroupID.x];

    uint mi = payload.offset+ (ci >> 24);
    uint drawId = payload.drawId;

    MeshDraw meshDraw = meshDraws[drawId];
    uint vertexCount = 0;
    uint triangleCount = 0;
    uint dataOffset = 0;
    uint lod = 0;


    if (SEAMLESS_LOD) 
    {
        vertexCount = uint(clusters[mi].vertexCount);
        triangleCount = uint(clusters[mi].triangleCount);
        dataOffset = clusters[mi].dataOffset;
        lod = clusters[mi].self.lod;
    }
    else // normal lod
    {
        vertexCount = uint(meshlets[mi].vertexCount);
        triangleCount = uint(meshlets[mi].triangleCount);
        dataOffset = meshlets[mi].dataOffset;
    }

    SetMeshOutputsEXT(vertexCount, triangleCount);
    uint vertexOffset = dataOffset;
    uint indexOffset = dataOffset + vertexCount;

    if(ti < vertexCount)
    {
        uint i = ti;
        uint vi = meshletData[vertexOffset + i] + meshDraw.vertexOffset;
    
        vec3 pos = vec3(vertices[vi].vx, vertices[vi].vy, vertices[vi].vz);
        vec3 norm = vec3(int(vertices[vi].nx), int(vertices[vi].ny), int(vertices[vi].nz)) / 127.0 - 1.0;
        vec4 tan = vec4(int(vertices[vi].tx), int(vertices[vi].ty), int(vertices[vi].tz), int(vertices[vi].tw)) / 127.0 - 1.0;
        vec2 uv = vec2(vertices[vi].tu, vertices[vi].tv);

        vec3 wPos = rotateQuat(pos, meshDraw.orit) * meshDraw.scale + meshDraw.pos;

        vec4 result =  trans.proj * trans.view * vec4(wPos, 1.0);

        norm = rotateQuat(norm, meshDraw.orit);

        gl_MeshVerticesEXT[i].gl_Position = result;
        out_drawId[i] = drawId;
        out_norm[i] = norm;
        out_wPos[i] = wPos;
        out_tan[i] = tan;
        out_uv[i] = uv;

#if DEBUG_MESHLET
         out_drawId[i] = mi;
#endif

#if CULL
        vertexClip[i] = vec3(result.xy/result.w, result.w);
#endif
    }

#if CULL
	barrier();
#endif

    vec2 screen = vec2(globals.screenWidth, globals.screenHeight);

    for(uint i = ti; i < triangleCount; i += MESHGP_SIZE )
    {
        uint offset = indexOffset * 4 + i * 3; // x4 for uint8_t

        uint idx0 = uint(meshletData8[ offset + 0]);
        uint idx1 = uint(meshletData8[ offset + 1]);
        uint idx2 = uint(meshletData8[ offset + 2]);
        
        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(idx0, idx1, idx2);
#if CULL
        bool culled = false;

        vec2 pa = vertexClip[idx0].xy;
        vec2 pb = vertexClip[idx1].xy;
        vec2 pc = vertexClip[idx2].xy;

        vec2 eb = pb - pa;
        vec2 ec = pc - pa;

        culled = culled || (eb.x * ec.y >= eb.y * ec.x);

        vec2 bmin = (min(pa, min(pb, pc)) * 0.5 + vec2(0.5)) * screen;
        vec2 bmax = (max(pa, max(pb, pc)) * 0.5 + vec2(0.5)) * screen;
        float sbprec = 1.0/ 256.0;

        culled = culled || (round(bmin.x - sbprec) == round(bmax.x - sbprec) || round(bmin.y - sbprec) == round(bmax.y - sbprec));
        
        culled = culled && (vertexClip[idx0].z > 0.0 && vertexClip[idx1].z > 0.0 && vertexClip[idx2].z > 0.0);

        gl_MeshPrimitivesEXT[i].gl_CullPrimitiveEXT = culled;
#endif
    }
}