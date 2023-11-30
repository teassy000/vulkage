#include "common.h"
#include "macro.h"
#include "math.h"

#include "mesh.h"
#include "meshoptimizer.h"

#include "fast_obj.h"
#include <vector>



size_t appendMeshlets(Geometry& result, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
    const size_t max_vertices = 64;
    const size_t max_triangles = 64;
    const float cone_weight = 1.f;

    std::vector<meshopt_Meshlet> meshlets(meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles));
    std::vector<unsigned int> meshlet_vertices(meshlets.size() * max_vertices);
    std::vector<unsigned char> meshlet_triangles(meshlets.size() * max_triangles * 3);

    meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), indices.data(), indices.size()
        , &vertices[0].vx, vertices.size(), sizeof(Vertex), max_vertices, max_triangles, cone_weight);

    for (const meshopt_Meshlet meshlet : meshlets)
    {
        Meshlet m = {};
        size_t dataOffset = result.meshletdata.size();

        for (uint32_t i = 0; i < meshlet.vertex_count; ++i)
        {
            result.meshletdata.push_back(meshlet_vertices[meshlet.vertex_offset + i]);
        }

        const uint32_t* idxGroup = reinterpret_cast<const unsigned int*>(&meshlet_triangles[0] + meshlet.triangle_offset);
        uint32_t idxGroupCount = (meshlet.triangle_count * 3 + 3) / 4;

        for (uint32_t i = 0; i < idxGroupCount; ++i)
        {
            result.meshletdata.push_back(idxGroup[i]);
        }

        meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset]
            , meshlet.triangle_count, &vertices[0].vx, vertices.size(), sizeof(Vertex));

        m.dataOffset = uint32_t(dataOffset);
        m.triangleCount = meshlet.triangle_count;
        m.vertexCount = meshlet.vertex_count;

        m.center[0] = bounds.center[0];
        m.center[1] = bounds.center[1];
        m.center[2] = bounds.center[2];
        m.radius = bounds.radius;

        m.cone_axis[0] = bounds.cone_axis_s8[0];
        m.cone_axis[1] = bounds.cone_axis_s8[1];
        m.cone_axis[2] = bounds.cone_axis_s8[2];
        m.cone_cutoff = bounds.cone_cutoff_s8;

        result.meshlets.push_back(m);
    }

    return meshlets.size();
}

bool loadMesh(Geometry& result, const char* path, bool buildMeshlets)
{
    fastObjMesh* obj = fast_obj_read(path);
    if (!obj)
        return false;

    size_t index_count = obj->index_count;

    std::vector<Vertex> triangle_vertices;
    triangle_vertices.resize(index_count);

    size_t vertex_offset = 0, index_offset = 0;

    size_t texcoord_count = obj->texcoord_count;

    for (uint32_t i = 0; i < obj->face_count; ++i)
    {
        for (uint32_t j = 0; j < obj->face_vertices[i]; ++j)
        {
            fastObjIndex gi = obj->indices[index_offset + j];

            assert(j < 3);

            Vertex& v = triangle_vertices[vertex_offset++];

            v.vx = (obj->positions[gi.p * 3 + 0]);
            v.vy = (obj->positions[gi.p * 3 + 1]);
            v.vz = (obj->positions[gi.p * 3 + 2]);

            v.nx = uint8_t(obj->normals[gi.n * 3 + 0] * 127.f + 127.5f);
            v.ny = uint8_t(obj->normals[gi.n * 3 + 1] * 127.f + 127.5f);
            v.nz = uint8_t(obj->normals[gi.n * 3 + 2] * 127.f + 127.5f);
            v.nw = uint8_t(0);

            v.tu = meshopt_quantizeHalf(obj->texcoords[gi.t * 2 + 0]);
            v.tv = meshopt_quantizeHalf(obj->texcoords[gi.t * 2 + 1]);
        }

        index_offset += obj->face_vertices[i];
    }

    assert(vertex_offset == index_count);

    fast_obj_destroy(obj);

    std::vector<uint32_t> remap(index_count);
    size_t vertex_count = meshopt_generateVertexRemap(remap.data(), 0, index_count, triangle_vertices.data(), index_count, sizeof(Vertex));

    std::vector<Vertex> vertices(vertex_count);
    std::vector<uint32_t> indices(index_count);

    meshopt_remapVertexBuffer(vertices.data(), triangle_vertices.data(), index_count, sizeof(Vertex), remap.data());
    meshopt_remapIndexBuffer(indices.data(), 0, index_count, remap.data());

    meshopt_optimizeVertexCache(indices.data(), indices.data(), index_count, vertex_count);
    meshopt_optimizeVertexFetch(vertices.data(), indices.data(), index_count, vertices.data(), vertex_count, sizeof(Vertex));


    Mesh mesh = {};

    mesh.vertexOffset = uint32_t(result.vertices.size());
    result.vertices.insert(result.vertices.end(), vertices.begin(), vertices.end());

    vec3 meshCenter = vec3(0.f);

    for (Vertex& v : vertices) {
        meshCenter += vec3(v.vx, v.vy, v.vz);
    }

    meshCenter /= float(vertices.size());

    float radius = 0.0;
    for (Vertex& v : vertices) {
        radius = glm::max(radius, glm::distance(meshCenter, vec3(v.vx, v.vy, v.vz)));
    }

    mesh.center = meshCenter;
    mesh.radius = radius;

    std::vector<uint32_t> lodIndices = indices;
    while (mesh.lodCount < COUNTOF(mesh.lods))
    {
        MeshLod& lod = mesh.lods[mesh.lodCount++];

        lod.indexOffset = uint32_t(result.indices.size());
        result.indices.insert(result.indices.end(), lodIndices.begin(), lodIndices.end());

        lod.indexCount = uint32_t(lodIndices.size());

        lod.meshletOffset = (uint32_t)result.meshlets.size();
        lod.meshletCount = buildMeshlets ? (uint32_t)appendMeshlets(result, vertices, lodIndices) : 0u;

        if (mesh.lodCount < COUNTOF(mesh.lods))
        {
            size_t nextIndicesTarget = size_t(double(lodIndices.size()) * 0.75);
            size_t nextIndices = meshopt_simplify(lodIndices.data(), lodIndices.data(), lodIndices.size(), &vertices[0].vx, vertices.size(), sizeof(Vertex), nextIndicesTarget, 1e-1f);
            assert(nextIndices <= lodIndices.size());

            if (nextIndices == lodIndices.size())
                break;

            lodIndices.resize(nextIndices);
            meshopt_optimizeVertexCache(lodIndices.data(), lodIndices.data(), lodIndices.size(), vertex_count);
        }

    }

    while (result.meshlets.size() % 64)
    {
        result.meshlets.push_back(Meshlet());
    }

    result.meshes.push_back(mesh);

    return true;
}