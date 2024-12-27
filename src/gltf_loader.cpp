#include "cgltf_impl.h"
#include "scene.h"

#include "meshoptimizer.h"
#include "debug.h"

#include "kage_structs.h"
#include "file_helper.h"

#include "entry/entry.h" // for allocator
#include "bimg/decode.h"

#include <map>
#include <string>
#include "bx/math.h"


// only inner 3x3 will be used
float detMatrix(const float _mtx[4][4])
{
    return _mtx[0][0] * (_mtx[1][1] * _mtx[2][2] - _mtx[1][2] * _mtx[2][1]) -
        _mtx[0][1] * (_mtx[1][0] * _mtx[2][2] - _mtx[1][2] * _mtx[2][0]) +
        _mtx[0][2] * (_mtx[1][0] * _mtx[2][1] - _mtx[1][1] * _mtx[2][0]);
}

void decRotation(float _rot[4], const float _mtx[4][4], const float _scale[3])
{
    // normalize 
    float inv_sx = (_scale[0] == 0.f) ? 0.f : 1.f / _scale[0];
    float inv_sy = (_scale[1] == 0.f) ? 0.f : 1.f / _scale[1];
    float inv_sz = (_scale[2] == 0.f) ? 0.f : 1.f / _scale[2];

    float r00 = _mtx[0][0] * inv_sx;
    float r01 = _mtx[0][1] * inv_sy;
    float r02 = _mtx[0][2] * inv_sz;
    float r10 = _mtx[1][0] * inv_sx;
    float r11 = _mtx[1][1] * inv_sy;
    float r12 = _mtx[1][2] * inv_sz;
    float r20 = _mtx[2][0] * inv_sx;
    float r21 = _mtx[2][1] * inv_sy;
    float r22 = _mtx[2][2] * inv_sz;

    float t{ 0.f };
    quat q{};
    if (r22 < 0) {
        if (r00 > r11) {
            t = 1 + r00 - r11 - r22;
            q = quat(t, r01 + r10, r20 + r02, r12 - r21);
        } else {
            t = 1 - r00 + r11 - r22;
            q = quat(r01 + r10, t, r12 + r21, r20 - r02);
        }
    } else {
        if (r00 < -r11) {
            t = 1 - r00 - r11 + r22;
            q = quat(r20 + r02, r12 + r21, t, r01 - r10);
        } else {
            t = 1 + r00 + r11 + r22;
            q = quat(r12 - r21, r20 - r02, r01 - r10, t);
        }
    }
    q *= 0.5f / bx::sqrt(t);

    _rot[0] = q.x;
    _rot[1] = q.y;
    _rot[2] = q.z;
    _rot[3] = q.w;
}

void decomposeTransform(float _trans[3], float _rot[4], float _scale[3], const float* _transform)
{
    float m[4][4] = {};
    bx::memCopy(m, _transform, sizeof(m));

    // translation from last row
    _trans[0] = m[3][0];
    _trans[1] = m[3][1];
    _trans[2] = m[3][2];

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#instantiation
    // det < 0, then cw, else ccw
    // here we want the ccw which is the default for glTF
    float det = detMatrix(m);
    float sign = (det < 0.f) ? -1.f : 1.f;

    // scale
    _scale[0] = sign * bx::sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);
    _scale[1] = sign * bx::sqrt(m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]);
    _scale[2] = sign * bx::sqrt(m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]);

    // rotation
    decRotation(_rot, m, _scale);
}

bool processMesh(Geometry& _geo, std::vector<std::pair<uint32_t, uint32_t>>& _prims, const cgltf_mesh* _gltfMesh, bool _buildMeshlet)
{
    assert(_gltfMesh);

    uint32_t meshOffset = (uint32_t)_geo.meshes.size();

    for (size_t ii = 0; ii < _gltfMesh->primitives_count; ++ii)
    {
        const cgltf_primitive& prim = _gltfMesh->primitives[ii];

        size_t vtxCount = prim.attributes[0].data->count;
        std::vector<float> scratch(vtxCount * 4);
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
            assert(sz == vtxCount * 4);
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
    }

    _prims.emplace_back(std::make_pair(meshOffset, (uint32_t)_geo.meshes.size() - meshOffset));

    return true;
}

bool processNode(Scene& _scene, std::vector<std::pair<uint32_t, uint32_t>>& _prims, const cgltf_data* _data, const cgltf_node* _node, bool _seamlessLod)
{
    assert(_data);
    assert(_node);

    if (_node->mesh)
    {

        size_t meshIdx = cgltf_mesh_index(_data, _node->mesh);
        std::pair<uint32_t, uint32_t> prim = _prims[meshIdx];

        uint32_t meshletVisibilityOffset = _scene.meshletVisibilityCount;
        
        for (uint32_t ii = 0; ii < prim.second; ++ii)
        {
            MeshDraw draw = {};

            float mtx[16];
            cgltf_node_transform_world(_node, mtx);

            float trans[3];
            float rot[4];
            float scale[3];
            decomposeTransform(trans, rot, scale, mtx);

            draw.pos = vec3(trans[0], trans[1], trans[2]);
            draw.orit = quat(rot[0], rot[1], rot[2], rot[3]);
            draw.scale = bx::max(bx::abs(scale[0]), bx::max(bx::abs(scale[1]), bx::abs(scale[2])));

            draw.meshIdx = prim.first + ii;

            auto getTextureIdx = [&_data](const cgltf_material* _mat, const cgltf_texture* _tex, const uint32_t _default) -> uint32_t
                {
                    return (_mat && _tex) 
                        ? 1 + (uint32_t)cgltf_texture_index(_data, _tex) 
                        : _default;
                };

            const cgltf_primitive* gltfPrim = &_data->meshes[meshIdx].primitives[ii];
            const cgltf_material* mat = gltfPrim->material;
            draw.albedoTex = getTextureIdx(
                mat
                , mat->pbr_metallic_roughness.base_color_texture.texture, 
                getTextureIdx(mat, mat->pbr_specular_glossiness.diffuse_texture.texture, 0)
                );
            
            draw.normalTex = getTextureIdx(mat, mat->normal_texture.texture, 0);
            
            draw.specularTex = getTextureIdx(
                mat
                , mat->pbr_metallic_roughness.metallic_roughness_texture.texture
                , getTextureIdx(mat, mat->specular.specular_texture.texture, 0)
            );
            
            draw.emissiveTex = getTextureIdx(mat, mat->emissive_texture.texture, 0);

            const Mesh& mesh = _scene.geometry.meshes[draw.meshIdx];
            draw.vertexOffset = mesh.vertexOffset;
            draw.meshletVisibilityOffset = meshletVisibilityOffset;

            if (mat && (mat->alpha_mode != cgltf_alpha_mode_opaque)) {
                draw.withAlpha = 1;
            }
            
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
    if (_node->camera)
    {
        if (_node->camera->type == cgltf_camera_type_perspective)
        {
            float mtx[16];
            cgltf_node_transform_world(_node, mtx);

            float trans[3];
            float rot[4];
            float scale[3];
            decomposeTransform(trans, rot, scale, mtx);

            Camera camera{};
            camera.pos = vec3{ trans[0], trans[1], trans[2] };
            camera.orit = quat{ rot[0], rot[1], rot[2], rot[3] };
            camera.fov = _node->camera->data.perspective.yfov;

            _scene.cameras.emplace_back(camera);
        }
        else
        {
            kage::message(kage::error, "Only perspective camera is supported");
        }
    }

    return false;
}

bool processImage(Scene& _scene, const char* _name, const uint8_t* _data, size_t _size)
{
    // there's lot of copied memory here, due to using the bimg::ImageContainer
    bimg::ImageContainer* imageContainer = bimg::imageParse(entry::getAllocator(), _data, (uint32_t)_size);
    if (imageContainer == nullptr)
    {
        kage::message(kage::error, "Failed to parse image from memory");
        return false;
    }

    // convert rgb8 to rgba8
    if (imageContainer->m_format == bimg::TextureFormat::RGB8)
    {
        bimg::ImageContainer* rgba8Container = bimg::imageConvert(entry::getAllocator(), bimg::TextureFormat::RGBA8, *imageContainer);
        bimg::imageFree(imageContainer);

        imageContainer = rgba8Container;
    }

    uint32_t offset = (uint32_t)_scene.imageDatas.size();
    _scene.imageDatas.resize(offset + imageContainer->m_size);
    memcpy(_scene.imageDatas.data() + offset, imageContainer->m_data, imageContainer->m_size);

    ImageInfo info{};
    strcpy(info.name, _name);
    info.dataOffset = offset;
    info.dataSize = imageContainer->m_size;

    info.w = imageContainer->m_width;
    info.h = imageContainer->m_height;
    info.mipCount = imageContainer->m_numMips;
    info.layerCount = imageContainer->m_numLayers;
    info.format = bimgToKageFromat(imageContainer->m_format);
    info.isCubeMap = imageContainer->m_cubeMap;

    _scene.images.emplace_back(info);

    bimg::imageFree(imageContainer);

    return true;
}

bool loadGltfScene(Scene& _scene, const char* _path, bool _buildMeshlet, bool _seamlessLod)
{
    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result res = cgltf_parse_file(&options, _path, &data);

    if (res != cgltf_result_success)
    {
        kage::message(kage::error, "Failed to parse gltf file: %s", _path);
        return false;
    }

    res = cgltf_load_buffers(&options, data, _path);
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
        cgltf_mesh* gltfMesh = &data->meshes[ii];

        processMesh(_scene.geometry, primitives, gltfMesh, _buildMeshlet);
    }

    // -- nodes
    for (uint32_t ii = 0; ii < data->nodes_count; ++ii)
    {
        cgltf_node* gltfNode = &data->nodes[ii];

        processNode(_scene, primitives, data, gltfNode, _seamlessLod);
    }

    // -- images
    char root_path[256];
    getCurrFolder(root_path, 256, _path);
    for (uint32_t ii = 0; ii < data->textures_count; ++ii)
    {
        const cgltf_texture& tex = data->textures[ii];
        assert(tex.image);
        const cgltf_image* img = tex.image;
        const cgltf_buffer_view* view = img->buffer_view;

        uint8_t* ptr = nullptr;
        uint32_t sz = 0;
        if (void* pBuf = (view && view->buffer) ? view->buffer->data : nullptr)
        {
            const size_t offset = view->offset;
            sz = (uint32_t)view->size;
            ptr = (uint8_t*)pBuf + offset;
        }
        else
        {
            std::string path =  std::string(root_path) + img->uri;
            ptr = (uint8_t*)load(path.c_str(), &sz);
        }

        if (ptr)
        {
            processImage(_scene, img->name, ptr, sz);
        }
    }

    // free gltf stuff
    cgltf_free(data);

    // dump the scene anyway
    std::string sceneName = _path;
    sceneName.append(".scene");
    FILE* file = fopen(sceneName.c_str(), "wb");
    if (file)
    {
        // delete the file
        fclose(file);
        remove(sceneName.c_str());
    }

    dumpScene(_scene, sceneName.c_str());
    return true;
}
