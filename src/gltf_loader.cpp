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

bool processNode(Scene& _scene, std::vector<std::pair<uint32_t, uint32_t>>& _prims, const cgltf_data* _gltfData, const cgltf_node* _gltfNode, bool _seamlessLod)
{
    assert(_gltfData);
    assert(_gltfNode);

    if (_gltfNode->mesh)
    {

        size_t meshIdx = cgltf_mesh_index(_gltfData, _gltfNode->mesh);
        std::pair<uint32_t, uint32_t> prim = _prims[meshIdx];

        uint32_t meshletVisibilityOffset = _scene.meshletVisibilityCount;
        
        for (uint32_t ii = 0; ii < prim.second; ++ii)
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

            draw.meshIdx = prim.first + ii;

            auto getTextureIdx = [&_gltfData](const cgltf_material* _mat, const cgltf_texture* _tex, const uint32_t _default) -> uint32_t
                {
                    return (_mat && _tex) 
                        ? (uint32_t)cgltf_texture_index(_gltfData, _tex) 
                        : _default;
                };

            const cgltf_primitive* gltfPrim = &_gltfData->meshes[meshIdx].primitives[ii];
            const cgltf_material* mat = gltfPrim->material;
            draw.albedoTex = getTextureIdx(
                mat
                , mat->pbr_metallic_roughness.base_color_texture.texture, 
                getTextureIdx(mat, mat->pbr_specular_glossiness.diffuse_texture.texture, 0)
                );
            draw.normalTex = getTextureIdx(mat, mat->normal_texture.texture, 0);
            draw.specularTex = getTextureIdx(mat, mat->pbr_metallic_roughness.metallic_roughness_texture.texture, 0);
            draw.emissiveTex = getTextureIdx(mat, mat->emissive_texture.texture, 0);

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

static kage::ResourceFormat bimgToKageFromat(bimg::TextureFormat::Enum _btf)
{
    using KageFormat = kage::ResourceFormat;

    KageFormat result = KageFormat::undefined;
    // translate bimg format to kage format
    switch (_btf)
    {
    case bimg::TextureFormat::R1:       /*unsupport*/                               break;
    case bimg::TextureFormat::A8:       /*unsupport*/                               break;
    case bimg::TextureFormat::R8:       result = KageFormat::r8_unorm;              break;
    case bimg::TextureFormat::R8I:      result = KageFormat::r8_sint;               break;
    case bimg::TextureFormat::R8U:      result = KageFormat::r8_uint;               break;
    case bimg::TextureFormat::R8S:      result = KageFormat::r8_snorm;              break;
    case bimg::TextureFormat::R16:      result = KageFormat::r16_unorm;             break;
    case bimg::TextureFormat::R16I:     result = KageFormat::r16_sint;              break;
    case bimg::TextureFormat::R16U:     result = KageFormat::r16_uint;              break;
    case bimg::TextureFormat::R16F:     result = KageFormat::r16_sfloat;            break;
    case bimg::TextureFormat::R16S:     result = KageFormat::r16_snorm;             break;
    case bimg::TextureFormat::R32I:     result = KageFormat::r32_sint;              break;
    case bimg::TextureFormat::R32U:     result = KageFormat::r32_uint;              break;
    case bimg::TextureFormat::R32F:     result = KageFormat::r32_sfloat;            break;
    case bimg::TextureFormat::RG8:      result = KageFormat::r8g8_unorm;            break;
    case bimg::TextureFormat::RG8I:     result = KageFormat::r8g8_sint;             break;
    case bimg::TextureFormat::RG8U:     result = KageFormat::r8g8_uint;             break;
    case bimg::TextureFormat::RG8S:     result = KageFormat::r8g8_snorm;            break;
    case bimg::TextureFormat::RG16:     result = KageFormat::r16g16_unorm;          break;
    case bimg::TextureFormat::RG16I:    result = KageFormat::r16g16_sint;           break;
    case bimg::TextureFormat::RG16U:    result = KageFormat::r16g16_uint;           break;
    case bimg::TextureFormat::RG16F:    result = KageFormat::r16g16_sfloat;         break;
    case bimg::TextureFormat::RG16S:    result = KageFormat::r16g16_snorm;          break;
    case bimg::TextureFormat::RG32I:    result = KageFormat::r32g32_sint;           break;
    case bimg::TextureFormat::RG32U:    result = KageFormat::r32g32_uint;           break;
    case bimg::TextureFormat::RG32F:    result = KageFormat::r32g32_sfloat;         break;
    case bimg::TextureFormat::RGB8:     result = KageFormat::r8g8b8_unorm;          break;
    case bimg::TextureFormat::RGB8I:    result = KageFormat::r8g8b8_sint;           break;
    case bimg::TextureFormat::RGB8U:    result = KageFormat::r8g8b8_uint;           break;
    case bimg::TextureFormat::RGB8S:    result = KageFormat::r8g8b8_snorm;          break;
    case bimg::TextureFormat::RGB9E5F:  /*unsupport*/                               break;
    case bimg::TextureFormat::BGRA8:    result = KageFormat::b8g8r8a8_unorm;        break;
    case bimg::TextureFormat::RGBA8:    result = KageFormat::r8g8b8a8_unorm;        break;
    case bimg::TextureFormat::RGBA8I:   result = KageFormat::r8g8b8a8_sint;         break;
    case bimg::TextureFormat::RGBA8U:   result = KageFormat::r8g8b8a8_uint;         break;
    case bimg::TextureFormat::RGBA8S:   result = KageFormat::r8g8b8a8_snorm;        break;
    case bimg::TextureFormat::RGBA16:   result = KageFormat::r16g16b16a16_unorm;    break;
    case bimg::TextureFormat::RGBA16I:  result = KageFormat::r16g16b16a16_sint;     break;
    case bimg::TextureFormat::RGBA16U:  result = KageFormat::r16g16b16a16_uint;     break;
    case bimg::TextureFormat::RGBA16F:  result = KageFormat::r16g16b16a16_sfloat;   break;
    case bimg::TextureFormat::RGBA16S:  result = KageFormat::r16g16b16a16_snorm;    break;
    case bimg::TextureFormat::RGBA32I:  result = KageFormat::r32g32b32a32_sint;     break;
    case bimg::TextureFormat::RGBA32U:  result = KageFormat::r32g32b32a32_uint;     break;
    case bimg::TextureFormat::RGBA32F:  result = KageFormat::r32g32b32a32_sfloat;   break;
    case bimg::TextureFormat::B5G6R5:   result = KageFormat::b5g6r5_unorm;          break;
    case bimg::TextureFormat::R5G6B5:   result = KageFormat::r5g6b5_unorm;          break;
    case bimg::TextureFormat::BGRA4:    result = KageFormat::b4g4r4a4_unorm;        break;
    case bimg::TextureFormat::RGBA4:    result = KageFormat::r4g4b4a4_unorm;        break;
    case bimg::TextureFormat::BGR5A1:   result = KageFormat::b5g5r5a1_unorm;        break;
    case bimg::TextureFormat::RGB5A1:   result = KageFormat::r5g5b5a1_unorm;        break;
    case bimg::TextureFormat::RGB10A2:  /*unsupport*/                               break;
    case bimg::TextureFormat::RG11B10F: /*unsupport*/                               break;
    case bimg::TextureFormat::D16:      result = KageFormat::d16;                   break;
    case bimg::TextureFormat::D24:      /*unsupport*/                               break;
    case bimg::TextureFormat::D24S8:    result = KageFormat::d24_sfloat_s8_uint;    break;
    case bimg::TextureFormat::D32:      /*unsupport*/                               break;
    case bimg::TextureFormat::D16F:     /*unsupport*/                               break;
    case bimg::TextureFormat::D24F:     /*unsupport*/                               break;
    case bimg::TextureFormat::D32F:     result = KageFormat::d32_sfloat;            break;
    case bimg::TextureFormat::D0S8:     /*unsupport*/                               break;
    default:
        kage::message(kage::error, "Unsupported bimg format: %d", _btf);
        result = KageFormat::undefined;
    }

    return result;
}


bool processImage(Scene& _scene, const char* _name, const uint8_t* _data, size_t _size)
{
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

    return true;
}

bool loadGltfScene(Scene& _scene, const char* _path, bool _buildMeshlet, bool _seamlessLod)
{
    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result res = cgltf_parse_file(&options, _path, &data);

    if (res != cgltf_result_success)
    {
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
    for (uint32_t ii = 0; ii < data->textures_count; ++ii)
    {
        const cgltf_texture& tex = data->textures[ii];
        assert(tex.image);
        const cgltf_image* img = tex.image;
        const cgltf_buffer_view* view = img->buffer_view;

        if (void* data = (view && view->buffer) ? view->buffer->data : nullptr)
        {
            const size_t offset = view->offset;
            const size_t size = view->size;
            const uint8_t* ptr = (const uint8_t*)data + offset;

            processImage(_scene, img->name, ptr, (uint32_t)size);
        }
    }

    // free gltf stuff
    cgltf_free(data);


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
