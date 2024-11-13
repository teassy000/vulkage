#include "common.h"
#include "macro.h"
#include "kage_math.h"

#include "mesh.h"
#include "meshoptimizer.h"
#include "metis.h"

#include "fast_obj.h"
#include <vector>
#include <map>



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

// nanite-like rendering

const size_t kClusterSize = 64; // triangle count
const size_t kMaxVtxInCluster = 64;
const size_t kGroupSize = 8;
const bool kUseLocks = true;
const bool kUseNormals = true;

const bool kUseMetisClusterize = false;
const bool kUseMetisPartition = true;


bool parseObj(const char* _path, std::vector<NaniteVertex>& _vertices, std::vector<uint32_t>& _indices)
{
    fastObjMesh* obj = fast_obj_read(_path);
    if (!obj)
        return false;

    size_t index_count = obj->index_count;

    std::vector<NaniteVertex> triangle_vertices;
    triangle_vertices.resize(index_count);

    size_t vertex_offset = 0, index_offset = 0;

    size_t texcoord_count = obj->texcoord_count;

    for (uint32_t i = 0; i < obj->face_count; ++i)
    {
        for (uint32_t j = 0; j < obj->face_vertices[i]; ++j)
        {
            fastObjIndex gi = obj->indices[index_offset + j];

            assert(j < 3);

            NaniteVertex& v = triangle_vertices[vertex_offset++];

            v.px = (obj->positions[gi.p * 3 + 0]);
            v.py = (obj->positions[gi.p * 3 + 1]);
            v.pz = (obj->positions[gi.p * 3 + 2]);

            v.nx = obj->normals[gi.n * 3 + 0];
            v.ny = obj->normals[gi.n * 3 + 1];
            v.nz = obj->normals[gi.n * 3 + 2];

            v.tu = obj->texcoords[gi.t * 2 + 0];
            v.tv = obj->texcoords[gi.t * 2 + 1];
        }

        index_offset += obj->face_vertices[i];
    }

    assert(vertex_offset == index_count);

    fast_obj_destroy(obj);

    std::vector<uint32_t> remap(index_count);
    size_t vertex_count = meshopt_generateVertexRemap(remap.data(), 0, index_count, triangle_vertices.data(), index_count, sizeof(NaniteVertex));

    std::vector<NaniteVertex> vertices(vertex_count);
    std::vector<uint32_t> indices(index_count);

    meshopt_remapVertexBuffer(vertices.data(), triangle_vertices.data(), index_count, sizeof(NaniteVertex), remap.data());
    meshopt_remapIndexBuffer(indices.data(), 0, index_count, remap.data());

    meshopt_optimizeVertexCache(indices.data(), indices.data(), index_count, vertex_count);
    meshopt_optimizeVertexFetch(vertices.data(), indices.data(), index_count, vertices.data(), vertex_count, sizeof(NaniteVertex));

    _vertices.insert(_vertices.end(), vertices.begin(), vertices.end());
    _indices.insert(_indices.end(), indices.begin(), indices.end());

    return true;
}

static void clusterizeMetisRec(std::vector<NaniteCluster>& _result, const std::vector<unsigned int>& _indices, const std::vector<int>& _triidx, const std::vector<int>& _triadj)
{

    assert(_triadj.size() == _triidx.size() * 3);

    // if triangle count is small enough, just add it to the result
    if (_triidx.size() <= kClusterSize)
    {
        NaniteCluster cluster;
        for (size_t i = 0; i < _triidx.size(); ++i)
        {
            cluster.indices.push_back(_indices[_triidx[i] * 3 + 0]);
            cluster.indices.push_back(_indices[_triidx[i] * 3 + 1]);
            cluster.indices.push_back(_indices[_triidx[i] * 3 + 2]);
            cluster.triangleCount++;

        }

        cluster.parent.error = FLT_MAX;
        _result.push_back(cluster);
        return;
    }

    std::vector<int32_t> xadj(_triidx.size() + 1);
    std::vector<int32_t> adjncy;
    std::vector<int32_t> part(_triidx.size());

    for (size_t i = 0; i < _triidx.size(); ++i)
    {
        for (int32_t j = 0; j < 3; ++j) {
            if (_triadj[i * 3 + j] != -1) {
                adjncy.push_back(_triadj[i * 3 + j]);
            }
        }

        xadj[i + 1] = (int32_t)adjncy.size();
    }

    int32_t options[METIS_NOPTIONS];
    METIS_SetDefaultOptions(options);
    options[METIS_OPTION_SEED] = 42;
    options[METIS_OPTION_UFACTOR] = _triidx.size() > 8 * kClusterSize ? 100 : 1;

    int32_t nvtxs = int32_t(_triidx.size());
    int32_t ncon = 1;
    int32_t nparts = 2;
    int32_t edgecut = 0;

    int32_t nvtxspad = (nvtxs + kClusterSize - 1) / kClusterSize * kClusterSize;
    int32_t idealcut = ((nvtxspad / kClusterSize) / 2) * kClusterSize;
    float partw[2] = { idealcut / float(nvtxspad), 1.f - idealcut / float(nvtxspad) };

    int32_t err = METIS_PartGraphRecursive(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL, NULL, &nparts, partw, NULL, options, &edgecut, &part[0]);
    assert(err == METIS_OK);
    (void)err;

    int32_t partsize[2] = {};
    std::vector<int32_t> partoff(part.size());
    for (size_t i = 0; i < part.size(); ++i) {
        partoff[i] = partsize[part[i]]++;
    }

    for (int32_t p = 0; p < 2; ++p) {
        std::vector<int32_t> partidx, partadj;
        partidx.reserve(partsize[p]);
        partadj.reserve(partsize[p] * 3);

        for (size_t ii = 0; ii < _triidx.size(); ++ii) {
            if (part[ii] != p) {
                continue;
            }

            partidx.push_back(_triidx[ii]);

            for (int32_t jj = 0; jj < 3; ++jj) {
                if (_triadj[ii * 3 + jj] >= 0 && part[_triadj[ii * 3 + jj]] == p) {
                    partadj.push_back(partoff[_triadj[ii * 3 + jj]]);
                } 
                else {
                    partadj.push_back(-1);
                }
            }
        }

        clusterizeMetisRec(_result, _indices, partidx, partadj);
    }
}

static std::vector<NaniteCluster> clusterizeMetis(const std::vector<NaniteVertex>& _vertices, const std::vector<uint32_t>& _indices)
{
    std::vector<unsigned int> shadowib(_indices.size());
    meshopt_generateShadowIndexBuffer(&shadowib[0], &_indices[0], _indices.size(), &_vertices[0].px, _vertices.size(), sizeof(float) * 3, sizeof(Vertex));

    std::map<std::pair<unsigned int, unsigned int>, unsigned int> edges;

    for (size_t i = 0; i < _indices.size(); ++i)
    {
        unsigned int v0 = shadowib[i + 0];
        unsigned int v1 = shadowib[i + (i % 3 == 2 ? -2 : 1)];

        // we don't track adjacency fully on non-manifold edges for now
        edges[std::make_pair(v0, v1)] = unsigned(i / 3);
    }

    std::vector<int> triadj(_indices.size(), -1);

    for (size_t i = 0; i < _indices.size(); i += 3)
    {
        unsigned int v0 = shadowib[i + 0], v1 = shadowib[i + 1], v2 = shadowib[i + 2];

        std::map<std::pair<unsigned int, unsigned int>, unsigned int>::iterator oab = edges.find(std::make_pair(v1, v0));
        std::map<std::pair<unsigned int, unsigned int>, unsigned int>::iterator obc = edges.find(std::make_pair(v2, v1));
        std::map<std::pair<unsigned int, unsigned int>, unsigned int>::iterator oca = edges.find(std::make_pair(v0, v2));

        triadj[i + 0] = oab != edges.end() ? int(oab->second) : -1;
        triadj[i + 1] = obc != edges.end() ? int(obc->second) : -1;
        triadj[i + 2] = oca != edges.end() ? int(oca->second) : -1;
    }

    std::vector<int> triidx(_indices.size() / 3);
    for (size_t i = 0; i < _indices.size(); i += 3)
        triidx[i / 3] = int(i / 3);

    std::vector<NaniteCluster> result;
    clusterizeMetisRec(result, _indices, triidx, triadj);

    return result;
}

static std::vector<NaniteCluster> clusterize(const std::vector<NaniteVertex>& _vertices, const std::vector<uint32_t>& _indices)
{
    if (kUseMetisClusterize) {
        return clusterizeMetis(_vertices, _indices);
    }

    const size_t max_vertices = 192;
    const size_t max_triangles = kClusterSize;

    // compute meshlet bounds
    size_t max_clusters = meshopt_buildMeshletsBound(_indices.size(), max_vertices, max_triangles);

    std::vector<meshopt_Meshlet> meshlets(max_clusters);
    std::vector<uint32_t> meshlet_vertices(max_clusters * max_vertices);
    std::vector<unsigned char> meshlet_triangles(max_clusters * max_triangles * 3);

    size_t meshletCount = meshopt_buildMeshlets(
        meshlets.data()
        , meshlet_vertices.data()
        , meshlet_triangles.data()
        , _indices.data()
        , _indices.size()
        , &_vertices[0].px
        , _vertices.size()
        , sizeof(NaniteVertex)
        , max_vertices
        , max_triangles
        , 0.f
    );
    
    meshlets.resize(meshletCount);

    std::vector<NaniteCluster> clusters(meshletCount);

    for (size_t ii = 0; ii < meshletCount; ++ii)
    {
        const meshopt_Meshlet& meshlet = meshlets[ii];
        
        meshopt_optimizeMeshlet(
            &meshlet_vertices[meshlet.vertex_offset]
            , &meshlet_triangles[meshlet.triangle_offset]
            , meshlet.triangle_count
            , meshlet.vertex_count
        );

        clusters[ii].indices.resize(meshlet.triangle_count * 3);

        for (size_t jj =0; jj < meshlet.triangle_count * 3; ++ jj)
        {
            clusters[ii].indices[jj] 
                = meshlet_vertices[meshlet.vertex_offset + meshlet_triangles[meshlet.triangle_offset + jj]];
        }

        // set vertex and triangles
        for (size_t jj = 0; jj < meshlet.vertex_count; ++jj)
        {
            clusters[ii].data.push_back(meshlet_vertices[meshlet.vertex_offset + jj]);
        }

        const uint32_t* idxGroup = reinterpret_cast<const unsigned int*>(&meshlet_triangles[0] + meshlet.triangle_offset);
        uint32_t idxGroupCount = (meshlet.triangle_count * 3 + 3) / 4;
        for (size_t jj = 0; jj < idxGroupCount; ++jj)
        {
            clusters[ii].data.push_back(idxGroup[jj]);
        }

        clusters[ii].parent.error = FLT_MAX;
        clusters[ii].vertexCount = meshlet.vertex_count;
        clusters[ii].triangleCount = meshlet.triangle_count;
    }

    return clusters;
}


static LodBounds calcBounds(const std::vector<NaniteVertex>& _vertices, const std::vector<uint32_t>& _indices, float _err, uint32_t _lod)
{
    meshopt_Bounds bounds = meshopt_computeClusterBounds(
        _indices.data()
        , _indices.size()
        , &_vertices[0].px
        , _vertices.size()
        , sizeof(NaniteVertex)
    );

    LodBounds ret;
    ret.center = vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
    ret.radius = bounds.radius;
    ret.error = _err;
    ret.lod = _lod;

    return ret;
}

static LodBounds boundsMerge(const std::vector<NaniteCluster>& _clusters, const std::vector<int32_t>& _group)
{
    LodBounds result = {};

    float weight = 0.f;
    for (size_t ii = 0; ii < _group.size(); ++ii)
    {
        result.center[0] += _clusters[_group[ii]].self.center[0] * _clusters[_group[ii]].self.radius;
        result.center[1] += _clusters[_group[ii]].self.center[1] * _clusters[_group[ii]].self.radius;
        result.center[2] += _clusters[_group[ii]].self.center[2] * _clusters[_group[ii]].self.radius;
        weight += _clusters[_group[ii]].self.radius;
    }

    if (weight > 0)
    {
        result.center[0] /= weight;
        result.center[1] /= weight;
        result.center[2] /= weight;
    }

    // merged bounds must strictly contain all cluster bounds
    result.radius = 0.f;
    for (size_t ii = 0; ii < _group.size(); ++ii)
    {
        float dx = _clusters[_group[ii]].self.center[0] - result.center[0];
        float dy = _clusters[_group[ii]].self.center[1] - result.center[1];
        float dz = _clusters[_group[ii]].self.center[2] - result.center[2];
        result.radius = std::max(result.radius, _clusters[_group[ii]].self.radius + sqrtf(dx * dx + dy * dy + dz * dz));
    }

    // merged bounds error must be conservative wrt cluster errors
    result.error = 0.f;
    for (size_t ii = 0; ii < _group.size(); ++ii)
        result.error = std::max(result.error, _clusters[_group[ii]].self.error);

    return result;
}

static void lockBoundary(
    std::vector<uint8_t> _locks
    , const std::vector<std::vector<int32_t>>& _groups
    , const std::vector<NaniteCluster>& _clusters
    , const std::vector<uint32_t>& _remap
    )
{
    std::vector<int> groupmap(_locks.size(), -1);

    for (size_t ii = 0; ii < _groups.size(); ++ii) {
        for (size_t jj = 0; jj < _groups[ii].size(); ++jj) {
            const NaniteCluster& cluster = _clusters[_groups[ii][jj]];

            for (size_t kk = 0; kk < cluster.indices.size(); ++kk) {
                unsigned int v = cluster.indices[kk];
                unsigned int r = _remap[v];

                if (groupmap[r] == -1 || groupmap[r] == int(ii))
                    groupmap[r] = int(ii);
                else 
                    groupmap[r] = -2;
            }
        }
    }

    // note: we need to consistently lock all vertices with the same position to avoid holes
    for (size_t ii = 0; ii < _locks.size(); ++ii) {
        unsigned int r = _remap[ii];

        _locks[ii] = (groupmap[r] == -2); // 表明该顶点在多个 group 中出现
    }
}

static std::vector<std::vector<int32_t>> partitionMetis(
    const std::vector<NaniteCluster>& _clusters
    , const std::vector<int32_t>& _pending
    , const std::vector<uint32_t>& _remap
)
{
    std::vector<std::vector<int32_t>> result;

    std::vector<std::vector<int32_t> > vertices(_remap.size());

    // expand pending clusters into vertex lists
    for (size_t ii = 0; ii < _pending.size(); ++ii) {
        const NaniteCluster& cluster = _clusters[_pending[ii]];

        for (size_t jj = 0; jj < cluster.indices.size(); ++jj) {
            int32_t v = _remap[cluster.indices[jj]];

            std::vector<int32_t>& list = vertices[v];
            if (list.empty() || list.back() != int32_t(ii)) {
                list.push_back(int32_t(ii));
            }
        }
    }

    // adjacency map: key is edge, value is number of triangles sharing the edge
    std::map<std::pair<int, int>, int> adjacency;

    for (size_t vv = 0; vv < vertices.size(); ++vv)
    {
        const std::vector<int>& list = vertices[vv];

        for (size_t ii = 0; ii < list.size(); ++ii) {
            for (size_t jj = ii + 1; jj < list.size(); ++jj) {
                adjacency[std::make_pair(std::min(list[ii], list[jj]), std::max(list[ii], list[jj]))]++;
            }
        }
    }

    std::vector<int32_t> xadj(_pending.size() + 1); // store the loc for each vertex[i]
    std::vector<int32_t> adjncy; // adjacency list, flattened, for each vertex[i], it's adjacent vertices start from xadj[i], ends at xadj[i+1]
    std::vector<int32_t> adjwgt; // same as adjncy, but for edge weights
    std::vector<int32_t> part(_pending.size()); // output

    xadj.push_back(0);// start from 0

    for (size_t ii = 0; ii < _pending.size(); ++ii) {
        for (std::map<std::pair<int, int>, int>::iterator it = adjacency.begin(); it != adjacency.end(); ++it) {
            if (it->first.first == int(ii)) {
                adjncy.push_back(it->first.second);
                adjwgt.push_back(it->second);
            }
            else if (it->first.second == int(ii)) {
                adjncy.push_back(it->first.first);
                adjwgt.push_back(it->second);
            }
        }

        xadj[ii + 1] = (int32_t)adjncy.size();
    }

    int nvtxs = int(_pending.size());
    int ncon = 1;
    int32_t edgecut = 0;
    int32_t nparts = int32_t(result.size());
    int32_t options[METIS_NOPTIONS];
    METIS_SetDefaultOptions(options);
    options[METIS_OPTION_SEED] = 42;
    options[METIS_OPTION_UFACTOR] = 100;


    if (nparts <= 1)
    {
        result.push_back(_pending);
    }
    else {
        int32_t err = METIS_PartGraphKway(
            &nvtxs
            , &ncon
            , &xadj[0]
            , &adjncy[0]
            , NULL
            , NULL
            , &adjwgt[0]
            , &nparts
            , NULL
            , NULL
            , options
            , &edgecut
            , &part[0]
        );

        assert(err == METIS_OK);

        result.resize(nparts);
        for (size_t ii = 0; ii < part.size(); ++ii) {
            result[part[ii]].push_back(_pending[ii]);
        }
    }

    return result;
}

static std::vector<std::vector<int32_t>> partition(
    const std::vector<NaniteCluster>& _clusters
    , const std::vector<int32_t>& _pending
    , const std::vector<uint32_t>& _remap
)
{
    if (kUseMetisPartition)
    {
        return partitionMetis(_clusters, _pending, _remap);
    }

    //void(_remap);

    std::vector<std::vector<int32_t> > result;

    size_t last_indices = 0;

    // rough merge; while clusters are approximately spatially ordered, this should use a proper partitioning algorithm
    for (size_t ii = 0; ii < _pending.size(); ++ii)
    {
        if (result.empty() || last_indices + _clusters[_pending[ii]].indices.size() > kClusterSize * kGroupSize * 3)
        {
            result.emplace_back(std::vector<int32_t>());
            last_indices = 0;
        }

        result.back().push_back(_pending[ii]);
        last_indices += _clusters[_pending[ii]].indices.size();
    }

    return result;
}


static std::vector<uint32_t> simplify(
    const std::vector<NaniteVertex>& _vertices
    , const std::vector<uint32_t>& _indices
    , const std::vector<uint8_t>* _locks
    , size_t _tgt_count
    , float* error = nullptr
)
{
    if (_tgt_count > _indices.size()) {
        return _indices;
    }

    std::vector<uint32_t> lod(_indices.size());
    uint32_t options = meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute;
    float normal_weights[3] = { .5f, .5f, .5f };


    size_t sz = meshopt_simplifyWithAttributes(
        &lod[0]
        , _indices.data()
        , _indices.size()
        , &_vertices[0].px
        , _vertices.size()
        , sizeof(NaniteVertex)
        , &_vertices[0].nx
        , sizeof(NaniteVertex)
        , normal_weights
        , 3
        , _locks ? &(*_locks)[0] : nullptr
        , _tgt_count
        , FLT_MAX
        , options
        , error
    );
    
    lod.resize(sz);

    return lod;
}

static float boundsError(const LodBounds& bounds, float camera_x, float camera_y, float camera_z, float camera_proj, float camera_znear)
{
    float dx = bounds.center[0] - camera_x, dy = bounds.center[1] - camera_y, dz = bounds.center[2] - camera_z;
    float d = sqrtf(dx * dx + dy * dy + dz * dz) - bounds.radius;
    return bounds.error / (d > camera_znear ? d : camera_znear) * (camera_proj * 0.5f);
}

static FILE* outputFile = nullptr;
static FILE* getOutput()
{
    if (outputFile == nullptr)
    {
        FILE* file = fopen("output_dump.obj", "w");
        outputFile = file;
    }

    return outputFile;
}

void dumpObj(const std::vector<NaniteVertex>& vertices, const std::vector<unsigned int>& indices, bool recomputeNormals = false)
{
    std::vector<float> normals;

    if (recomputeNormals)
    {
        normals.resize(vertices.size() * 3);

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            unsigned int a = indices[i], b = indices[i + 1], c = indices[i + 2];

            const NaniteVertex& va = vertices[a];
            const NaniteVertex& vb = vertices[b];
            const NaniteVertex& vc = vertices[c];

            float nx = (vb.py - va.py) * (vc.pz - va.pz) - (vb.pz - va.pz) * (vc.py - va.py);
            float ny = (vb.pz - va.pz) * (vc.px - va.px) - (vb.px - va.px) * (vc.pz - va.pz);
            float nz = (vb.px - va.px) * (vc.py - va.py) - (vb.py - va.py) * (vc.px - va.px);

            for (int k = 0; k < 3; ++k)
            {
                unsigned int index = indices[i + k];

                normals[index * 3 + 0] += nx;
                normals[index * 3 + 1] += ny;
                normals[index * 3 + 2] += nz;
            }
        }
    }

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const NaniteVertex& v = vertices[i];

        float nx = v.nx, ny = v.ny, nz = v.nz;

        if (recomputeNormals)
        {
            nx = normals[i * 3 + 0];
            ny = normals[i * 3 + 1];
            nz = normals[i * 3 + 2];

            float l = sqrtf(nx * nx + ny * ny + nz * nz);
            float s = l == 0.f ? 0.f : 1.f / l;

            nx *= s;
            ny *= s;
            nz *= s;
        }

        fprintf(getOutput(), "v %f %f %f\n", v.px, v.py, v.pz);
        fprintf(getOutput(), "vn %f %f %f\n", nx, ny, nz);
    }

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        unsigned int a = indices[i], b = indices[i + 1], c = indices[i + 2];

        fprintf(getOutput(), "f %d//%d %d//%d %d//%d\n", a + 1, a + 1, b + 1, b + 1, c + 1, c + 1);
    }
}

void dumpObj(const char* section, const std::vector<unsigned int>& indices)
{
    fprintf(getOutput(), "o %s\n", section);

    for (size_t j = 0; j < indices.size(); j += 3)
    {
        unsigned int a = indices[j], b = indices[j + 1], c = indices[j + 2];

        fprintf(getOutput(), "f %d//%d %d//%d %d//%d\n", a + 1, a + 1, b + 1, b + 1, c + 1, c + 1);
    }
}

bool loadMeshNanite(Geometry& _outGeo, const char* _path)
{
    std::vector<NaniteVertex> vertices;
    std::vector<uint32_t> indices;
    parseObj(_path, vertices, indices);

    std::vector<NaniteCluster> clusters = clusterize(vertices, indices);
    for (NaniteCluster& c : clusters) {
        c.self = calcBounds(vertices, c.indices, 0.f, 0);
    }

    std::vector<int32_t> pending(clusters.size());
    for (size_t ii = 0; ii < clusters.size(); ++ii) {
        pending[ii] = int32_t(ii);
    }

    std::vector<uint8_t> locks(vertices.size());

    std::vector<uint32_t> remap(vertices.size());
    meshopt_Stream pos = { &vertices[0].px, sizeof(float) * 3, sizeof(NaniteVertex) };
    meshopt_generateVertexRemapMulti(
        remap.data()
        , indices.data()
        , indices.size()
        , vertices.size()
        , &pos
        , 1
    );

    int32_t depth = 0;


    kage::message(kage::info, "lod 0: %d clusters, %d triangles", int(clusters.size()), int(indices.size() / 3));

    std::vector<std::pair<int32_t, int32_t> > dag_debug;
    while (pending.size() > 1)
    {
        std::vector<std::vector<int32_t>> groups = partition(clusters, pending, remap);
        pending.clear();

        std::vector<int32_t> retry;

        size_t triangles = 0;
        size_t stuck_triangles = 0;
        int single_clusters = 0;
        int stuck_clusters = 0;
        int full_clusters = 0;

        lockBoundary(locks, groups, clusters, remap);
         
        for (size_t ii = 0; ii < groups.size(); ++ii)
        {
            if (groups[ii].empty()) {
                continue;
            }
            // skip only group with one cluster
            if (groups[ii].size() == 1)
            {
                retry.push_back(groups[ii][0]);

                single_clusters++;
                stuck_clusters++;
                stuck_triangles += clusters[groups[ii][0]].indices.size() / 3;

                continue;
            }

            std::vector<uint32_t> merged;
            for (size_t jj = 0; jj < groups[ii].size(); ++jj)
            {
                merged.insert(
                    merged.end()
                    , clusters[groups[ii][jj]].indices.begin()
                    , clusters[groups[ii][jj]].indices.end()
                );
            }


            size_t tgt_size = ((groups[ii].size() + 1) / 2) * kClusterSize * 3;
            float err = 0.f;
            std::vector<uint32_t> simplified = simplify(vertices, merged, &locks, tgt_size, &err);

            // if simplification failed, retry later
            if ( simplified.size() > merged.size() * .85f 
                || simplified.size() / (kClusterSize*3) >= merged.size() / (kClusterSize*3)
                ) {
                for (size_t jj = 0; jj < groups[ii].size(); ++ jj) {
                    retry.push_back(groups[ii][jj]);
                }
                stuck_clusters++;
                stuck_triangles += merged.size() / 3;

                continue;
            }

            // calculate merged bounds
            LodBounds mergedBounds = boundsMerge(clusters, groups[ii]);
            mergedBounds.error += err;
            mergedBounds.lod = depth + 1;

            std::vector<NaniteCluster> split = clusterize(vertices, simplified);

            // update dag
            for (size_t jj = 0; jj < groups[ii].size(); ++jj)
            {
                assert(clusters[groups[ii][jj]].parent.error == FLT_MAX);
                clusters[groups[ii][jj]].parent = mergedBounds;
            }

            for (size_t jj = 0; jj < groups[ii].size(); ++jj) {
                for (size_t kk = 0; kk < split.size(); ++kk) {
                    dag_debug.emplace_back(groups[ii][jj], int32_t(clusters.size() + kk));
                }
            }

            for (NaniteCluster& scRef: split) {
                scRef.self = mergedBounds;

                clusters.push_back(scRef);
                pending.push_back(int32_t(clusters.size() - 1));

                triangles += scRef.indices.size() / 3;
                full_clusters += scRef.indices.size() == kClusterSize * 3;
            }
        }

        depth++;

        kage::message(kage::info, "lod %d: simplified %d clusters (%d full, %.1f tri/cl), %d triangles; stuck %d clusters (%d single), %d triangles"
            , depth
            , int(pending.size())
            , full_clusters
            , pending.empty() ? 0 : double(triangles) / double(pending.size())
            , int(triangles)
            , stuck_clusters
            , single_clusters
            , int(stuck_triangles)
        );

        if (triangles < stuck_triangles / 3) {
            break;
        }
        pending.insert(pending.end(), retry.begin(), retry.end());
    }

    // fill the Geometry with the final clusters
    {
        uint32_t lodIdxOffset = (uint32_t)_outGeo.indices.size();
        uint32_t lodMeshletOffset = (uint32_t)_outGeo.meshlets.size();

        for (size_t ii = 0; ii < clusters.size(); ++ii)
        {
            const NaniteCluster& cluster = clusters[ii];
            Cluster c;
            c.self = cluster.self;
            c.parent = cluster.parent;
            c.dataOffset = (uint32_t)_outGeo.meshletdata.size();
            c.vertexCount = cluster.vertexCount;
            c.triangleCount = cluster.triangleCount;

            assert(cluster.triangleCount * 3 == cluster.indices.size());

            _outGeo.meshletdata.insert(_outGeo.meshletdata.end(), cluster.data.begin(), cluster.data.end());



            _outGeo.clusters.push_back(c);
        }

        Mesh mesh = {};

        mesh.vertexOffset = uint32_t(_outGeo.vertices.size());

        for (size_t ii = 0; ii < vertices.size(); ++ii)
        {
            _outGeo.vertices.push_back(Vertex{
                vertices[ii].px
                , vertices[ii].py
                , vertices[ii].pz
                , uint8_t(vertices[ii].nx * 127.f + 127.5f)
                , uint8_t(vertices[ii].ny * 127.f + 127.5f)
                , uint8_t(vertices[ii].nz * 127.f + 127.5f)
                , 0
                , meshopt_quantizeHalf(vertices[ii].tu)
                , meshopt_quantizeHalf(vertices[ii].tv)
                });
        }

        vec3 meshCenter = vec3(0.f);
        for (NaniteVertex& v : vertices) {
            meshCenter += vec3(v.px, v.py, v.pz);
        }
        meshCenter /= float(vertices.size());

        float radius = 0.0;
        for (NaniteVertex& v : vertices) {
            radius = glm::max(radius, glm::distance(meshCenter, vec3(v.px, v.py, v.pz)));
        }

        mesh.center = meshCenter;
        mesh.radius = radius;
        MeshLod lod;

        lod.indexCount = uint32_t(_outGeo.meshletdata.size());
        lod.indexOffset = lodIdxOffset;
        lod.meshletCount = uint32_t(clusters.size());
        lod.meshletOffset = lodMeshletOffset;

        mesh.seamlessLod = lod;

        _outGeo.meshes.push_back(mesh);

        while (_outGeo.clusters.size() % 64)
        {
            _outGeo.clusters.push_back(Cluster());
        }

    }

    {
        // for testing purposes, we can compute a DAG cut from a given viewpoint and dump it as an OBJ
        float maxx = 0.f, maxy = 0.f, maxz = 0.f;
        for (size_t ii = 0; ii < vertices.size(); ++ii)
        {
            maxx = std::max(maxx, vertices[ii].px * 2);
            maxy = std::max(maxy, vertices[ii].py * 2);
            maxz = std::max(maxz, vertices[ii].pz * 2);
        }

        // 
        float threshold = 2e-3f; // 2 pixels at 1080p
        float fovy = 60.f;
        float znear = 1e-2f;
        float proj = 1.f / tanf(fovy * 3.1415926f / 180.f * 0.5f);

        std::vector<unsigned int> cut;
        for (size_t ii = 0; ii < clusters.size(); ++ii) {
            if (boundsError(clusters[ii].self, maxx, maxy, maxz, proj, znear) <= threshold && boundsError(clusters[ii].parent, maxx, maxy, maxz, proj, znear) > threshold) {
                cut.insert(cut.end(), clusters[ii].indices.begin(), clusters[ii].indices.end());
            }
        }

        dumpObj(vertices, cut);
        for (size_t i = 0; i < clusters.size(); ++i)
            dumpObj("cluster", clusters[i].indices);

        fflush(outputFile);

        fclose(outputFile);
    }


    return true;
}

