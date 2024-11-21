#include "cgltf_impl.h"
#include "scene.h"

#include "meshoptimizer.h"
#include "debug.h"

bool processMesh(Geometry& _geo, const cgltf_mesh* _gltfMesh, bool _buildMeshlet)
{
    assert(_gltfMesh);

    for (size_t ii = 0; ii < _gltfMesh->primitives_count; ++ii)
    {
        const cgltf_primitive& prim = _gltfMesh->primitives[ii];

        size_t vtxCount = prim.attributes[0].data->count;
        std::vector<float> scratch(vtxCount * 3);
        std::vector<Vertex> vertices(vtxCount);

        // -- position
        const cgltf_accessor* pos = cgltf_find_accessor(&prim, cgltf_attribute_type_position, 0);
        if (pos)
        {
            assert(pos->type == cgltf_type_vec3);
            assert(pos->component_type == cgltf_component_type_r_32f);

            cgltf_size sz = cgltf_accessor_unpack_floats(pos, scratch.data(), vtxCount * 3);

            assert(sz == vtxCount * 3);
            for (size_t jj = 0; jj < vtxCount; ++jj)
            {
                vertices[jj].vx = scratch[jj * 3 + 0];
                vertices[jj].vy = scratch[jj * 3 + 1];
                vertices[jj].vz = scratch[jj * 3 + 2];
            }
        }

        // -- normal
        const cgltf_accessor* norm = cgltf_find_accessor(&prim, cgltf_attribute_type_normal, 0);
        if (norm)
        {
            assert(norm->type == cgltf_type_vec3);
            assert(norm->component_type == cgltf_component_type_r_32f);
            cgltf_size sz = cgltf_accessor_unpack_floats(norm, scratch.data(), vtxCount * 3);
            assert(sz == vtxCount * 3);
            for (size_t jj = 0; jj < vtxCount; ++jj)
            {
                vertices[jj].nx = uint8_t(scratch[jj * 3 + 0] * 127.f + 127.5f);
                vertices[jj].ny = uint8_t(scratch[jj * 3 + 1] * 127.f + 127.5f);
                vertices[jj].nz = uint8_t(scratch[jj * 3 + 2] * 127.f + 127.5f);
                vertices[jj].nw = uint8_t(0);
            }
        }

        // -- tangent
        const cgltf_accessor* tang = cgltf_find_accessor(&prim, cgltf_attribute_type_tangent, 0);
        if (tang)
        {
            assert(tang->type == cgltf_type_vec4);
            assert(tang->component_type == cgltf_component_type_r_32f);
            cgltf_size sz = cgltf_accessor_unpack_floats(tang, scratch.data(), vtxCount * 4);
            assert(sz == scratch.size());
            for (size_t jj = 0; jj < vtxCount; ++jj)
            {
                vertices[jj].tx = uint8_t(scratch[jj * 4 + 0] * 127.f + 127.5f);
                vertices[jj].ty = uint8_t(scratch[jj * 4 + 1] * 127.f + 127.5f);
                vertices[jj].tz = uint8_t(scratch[jj * 4 + 2] * 127.f + 127.5f);
                vertices[jj].tw = uint8_t(scratch[jj * 4 + 3] * 127.f + 127.5f);
            }
        }

        // -- uv
        const cgltf_accessor* uv = cgltf_find_accessor(&prim, cgltf_attribute_type_texcoord, 0);
        if (uv)
        {
            assert(uv->type == cgltf_type_vec2);
            assert(uv->component_type == cgltf_component_type_r_32f);
            cgltf_size sz = cgltf_accessor_unpack_floats(uv, scratch.data(), vtxCount * 2);
            assert(sz == vtxCount * 2);
            for (size_t jj = 0; jj < vtxCount; ++jj)
            {
                vertices[jj].tu = meshopt_quantizeHalf(scratch[jj * 2 + 0]);
                vertices[jj].tv = meshopt_quantizeHalf(scratch[jj * 2 + 1]);
            }
        }

        // -- indices
        std::vector<uint32_t> indices(prim.indices->count);
        cgltf_accessor_unpack_indices(prim.indices, indices.data(), sizeof(uint32_t), indices.size());

        appendMesh(_geo, vertices, indices, _buildMeshlet);

        // -- material
        // TODO
        prim.material;
    }

    return true;
}

bool processNode(Scene& _scene, const cgltf_data* _gltfData, const cgltf_node* _gltfNode, uint32_t _meshOffset, uint32_t meshCount, bool _seamlessLod)
{
    assert(_gltfData);
    assert(_gltfNode);

    if (_gltfNode->mesh)
    {
        uint32_t meshletVisibilityOffset = _scene.meshletVisibilityCount;
        
        for (uint32_t ii = 0; ii < meshCount; ++ii)
        {
            MeshDraw draw = {};
            draw.pos[0] = _gltfNode->translation[0];
            draw.pos[1] = _gltfNode->translation[1];
            draw.pos[2] = _gltfNode->translation[2];

            // TODO: use the x scale temporary, fix it
            draw.scale = _gltfNode->scale[0];
            
            draw.orit[0] = _gltfNode->rotation[0];
            draw.orit[1] = _gltfNode->rotation[1];
            draw.orit[2] = _gltfNode->rotation[2];
            draw.orit[3] = _gltfNode->rotation[3];

            draw.meshIdx = _meshOffset + ii;


            const Mesh& mesh = _scene.geometry.meshes[draw.meshIdx];
            draw.vertexOffset = mesh.vertexOffset;
            draw.meshletVisibilityOffset = meshletVisibilityOffset;
            
            _scene.meshDraws.push_back(draw);

            uint32_t meshletCount = 0;
            if (_seamlessLod) {
                meshletCount = std::max(meshletCount, mesh.seamlessLod.meshletCount);
            }
            else {
                for (uint32_t lod = 0; lod < mesh.lodCount; lod++)
                    meshletCount = std::max(meshletCount, mesh.lods[lod].meshletCount); // use the maximum one for current mesh
            }

            meshletVisibilityOffset += meshletCount;
        }

        _scene.meshletVisibilityCount = meshletVisibilityOffset;
        _scene.drawCount = (uint32_t)_scene.meshDraws.size();
    }
    // light?

    // camera?

    return false;
}

bool loadGltfScene(Scene& _scene, const char** _pathes, uint32_t _pathCount, bool _buildMeshlet, bool _seamlessLod)
{
    assert(_pathCount == 1);

    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result res = cgltf_parse_file(&options, _pathes[0], &data);

    if (res != cgltf_result_success)
    {
        return false;
    }

    res = cgltf_load_buffers(&options, data, _pathes[0]);
    if (res != cgltf_result_success)
    {
        cgltf_free(data);
        return false;
    }

    res = cgltf_validate(data);
    if (res != cgltf_result_success)
    {
        cgltf_free(data);
        return false;
    }

    // key: start offset, value: count
    std::vector<std::pair<uint32_t, uint32_t>> primitives;
    // -- meshes 
    for (uint32_t ii = 0; ii < data->meshes_count; ++ii)
    {
        uint32_t meshOffset = (uint32_t)_scene.geometry.meshes.size();

        cgltf_mesh* gltfMesh = &data->meshes[ii];

        processMesh(_scene.geometry, gltfMesh, _buildMeshlet);

        primitives.emplace_back(std::make_pair(meshOffset, (uint32_t)_scene.geometry.meshes.size() - meshOffset));
    }

    // -- nodes
    for (uint32_t ii = 0; ii < data->nodes_count; ++ii)
    {
        cgltf_node* gltfNode = &data->nodes[ii];
        size_t meshIdx = cgltf_mesh_index(data, gltfNode->mesh);
        std::pair<uint32_t, uint32_t> prim = primitives[meshIdx];
        processNode(_scene, data, gltfNode, prim.first, prim.second, _seamlessLod);
    }

    return true;
}
