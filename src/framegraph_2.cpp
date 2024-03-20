#include "common.h"
#include "macro.h"
#include "config.h"
#include "util.h"

#include "memory_operation.h"
#include "handle.h"
#include "framegraph_2.h"
#include "rhi_context.h"

#include "profiler.h"

#include <algorithm>



namespace vkz
{

    Framegraph2::~Framegraph2()
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        deleteObject(m_pAllocator, m_pMemBlock);
    }

    void Framegraph2::bake()
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        // prepare
        parseOp();

        postParse();

        buildGraph();

        // sort and cut
        reverseTraversalDFSWithBackTrack();

        // optimize
        optimizeSync();
        optimizeAlias();

        // actual create resources for renderer
        createResources();
    }

    void Framegraph2::parseOp()
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        assert(m_pMemBlock != nullptr);
        MemoryReader reader(m_pMemBlock->expand(0), m_pMemBlock->size());

        while (true)
        {
            MagicTag magic = MagicTag::invalid_magic;
            read(&reader, magic);

            bool finished = false;
            switch (magic)
            {
            // Brief
            case MagicTag::set_brief: {
                setBrief(reader);
                break;
            }
            // Register
            case MagicTag::register_shader: {
                registerShader(reader);
                break;
            }
            case MagicTag::register_program: {
                registerProgram(reader);
                break;
            }
            case MagicTag::register_pass: {
                registerPass(reader);
                break;
            }
            case MagicTag::register_buffer: {
                registerBuffer(reader);
                break;
            }
            case MagicTag::register_image: {
                registerImage(reader);
                break;
            }
            case MagicTag::register_sampler: {
                registerSampler(reader);
                break;
            }
            case MagicTag::register_image_view: {
                registerImageView(reader);
                break;
            }
            // back-buffers
            case MagicTag::store_back_buffer: {
                storeBackBuffer(reader);
                break;
            }
            // Alias
            case MagicTag::force_alias_buffer: {
                aliasResForce(reader, ResourceType::buffer);
                break;
            }
            case MagicTag::force_alias_image: {
                aliasResForce(reader, ResourceType::image);
                break;
            }
            // End
            case MagicTag::invalid_magic:
                message(DebugMessageType::warning, "invalid magic tag, data incorrect!");
            case MagicTag::end:
            default:
                finished = true;
                break;
            }

            if (finished)
            {
                break;
            }

            MagicTag bodyEnd;
            read(&reader, bodyEnd);
            assert(MagicTag::magic_body_end == bodyEnd);
        }
    }

    void Framegraph2::setBrief(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        FrameGraphBrief brief;
        read(&_reader, brief);

        // info data will use idx from handle as iterator
        m_sparse_shader_info.resize(brief.shaderNum);
        m_sparse_program_info.resize(brief.programNum);
        m_sparse_pass_meta.resize(brief.passNum);
        m_sparse_buf_info.resize(brief.bufNum);
        m_sparse_img_info.resize(brief.imgNum);
        m_sparse_sampler_meta.resize(brief.samplerNum);
        m_sparse_img_view_desc.resize(brief.imgViewNum);

        m_sparse_pass_data_ref.resize(brief.passNum);

        m_combinedPresentImage = CombinedResID{ brief.presentImage, ResourceType::image };
        m_presentMipLevel = brief.presentMipLevel;
    }


    void Framegraph2::registerShader(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        FGShaderCreateInfo info;
        read(&_reader, info);

        m_hShader.push_back({ info.shaderId });
        
        char path[kMaxPathLen];
        read(&_reader, (void*)(path), info.strLen);
        path[info.strLen] = '\0'; // null-terminated string

        m_shader_path.emplace_back(path);
        m_sparse_shader_info[info.shaderId].createInfo = info;
        m_sparse_shader_info[info.shaderId].pathIdx = (uint16_t)m_shader_path.size() - 1;
    }

    void Framegraph2::registerProgram(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        FGProgCreateInfo regInfo;
        read(&_reader, regInfo);

        assert(regInfo.shaderNum <= kMaxNumOfStageInPorgram);

        ProgramInfo& progInfo = m_sparse_program_info[regInfo.progId];
        progInfo.createInfo = regInfo;

        read(&_reader, (void*)(progInfo.shaderIds), sizeof(uint16_t) * regInfo.shaderNum);

        m_hProgram.push_back({ regInfo.progId });
    }

    void Framegraph2::registerPass(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        PassMetaData passMeta;
        read(&_reader, passMeta);

        // process order is important
        for (uint32_t ii = 0; ii < passMeta.vertexBindingNum; ++ii)
        {
            VertexBindingDesc bInfo;
            read(&_reader, bInfo);
            m_vtxBindingDesc.emplace_back(bInfo);
            m_sparse_pass_data_ref[passMeta.passId].vtxBindingIdxs.push_back((uint16_t)m_vtxBindingDesc.size() - 1);
        }

        for (uint32_t ii = 0; ii < passMeta.vertexAttributeNum; ++ii)
        {
            VertexAttributeDesc aInfo;
            read(&_reader, aInfo);
            m_vtxAttrDesc.emplace_back(aInfo);
            m_sparse_pass_data_ref[passMeta.passId].vtxAttrIdxs.push_back((uint16_t)m_vtxAttrDesc.size() - 1);
        }

        for (uint32_t ii = 0; ii < passMeta.pipelineSpecNum; ++ii)
        {
            int pipelineSpec;
            read(&_reader, pipelineSpec);
            m_pipelineSpecData.emplace_back(pipelineSpec);
            m_sparse_pass_data_ref[passMeta.passId].pipelineSpecIdxs.push_back((uint16_t)m_pipelineSpecData.size() - 1);
        }

        m_hPass.push_back({ passMeta.passId });
        m_pass_rw_res.emplace_back();

        // write image
        // include color attachment and depth stencil attachment
        stl::vector<PassResInteract> writeImgVec(passMeta.writeImageNum);
        read(&_reader, writeImgVec.data(), sizeof(PassResInteract) * passMeta.writeImageNum);
        passMeta.writeImageNum = writeResource(writeImgVec, passMeta.passId, ResourceType::image);

        // read image
        stl::vector<PassResInteract> readImgVec(passMeta.readImageNum);
        read(&_reader, readImgVec.data(), sizeof(PassResInteract) * passMeta.readImageNum);
        passMeta.readImageNum = readResource(readImgVec, passMeta.passId, ResourceType::image);

        // write buffer
        stl::vector<PassResInteract> writeBufVec(passMeta.writeBufferNum);
        read(&_reader, writeBufVec.data(), sizeof(PassResInteract) * passMeta.writeBufferNum);
        passMeta.writeBufferNum = writeResource(writeBufVec, passMeta.passId, ResourceType::buffer);

        // read buffer
        stl::vector<PassResInteract> readBufVec(passMeta.readBufferNum);
        read(&_reader, readBufVec.data(), sizeof(PassResInteract) * passMeta.readBufferNum);
        passMeta.readBufferNum = readResource(readBufVec, passMeta.passId, ResourceType::buffer);

        // write resource alias
        stl::vector<WriteOperationAlias> writeImgAliasVec(passMeta.writeImgAliasNum);
        read(&_reader, writeImgAliasVec.data(), sizeof(WriteOperationAlias) * passMeta.writeImgAliasNum);
        passMeta.writeImgAliasNum = writeResForceAlias(writeImgAliasVec, passMeta.passId, ResourceType::image);

        stl::vector<WriteOperationAlias> writeBufAliasVec(passMeta.writeBufAliasNum);
        read(&_reader, writeBufAliasVec.data(), sizeof(WriteOperationAlias) * passMeta.writeBufAliasNum);
        passMeta.writeBufAliasNum = writeResForceAlias(writeBufAliasVec, passMeta.passId, ResourceType::buffer);

        // fill pass idx in queue
        uint16_t qIdx = (uint16_t)passMeta.queue;
        m_passIdxInQueue[qIdx].push_back((uint16_t)m_hPass.size());

        // fill pass info
        m_sparse_pass_data_ref[passMeta.passId].passRegInfoIdx = passMeta.passId;
        m_sparse_pass_meta[passMeta.passId] = passMeta;

        m_pass_dependency.emplace_back();
        m_passIdxToSync.emplace_back();
    }

    void Framegraph2::registerBuffer(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        FGBufferCreateInfo info;
        read(&_reader, info);

        m_hBuf.push_back({ info.bufId });
        m_sparse_buf_info[info.bufId] = info;

        CombinedResID plainResID{ info.bufId, ResourceType::buffer };
        m_combinedResId.push_back(plainResID);

        if (info.lifetime == ResourceLifetime::non_transition)
        {
            CombinedResID plainIdx{ info.bufId, ResourceType::buffer };
            m_multiFrame_resList.push_back(plainIdx);
        }
    }

    void Framegraph2::registerImage(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        FGImageCreateInfo info;
        read(&_reader, info);

        m_hTex.push_back({ info.imgId });
        m_sparse_img_info[info.imgId] = info;

        CombinedResID plainResIdx{ info.imgId, ResourceType::image };
        m_combinedResId.push_back(plainResIdx);

        if (info.lifetime == ResourceLifetime::non_transition)
        {
            CombinedResID plainIdx{ info.imgId, ResourceType::image };
            m_multiFrame_resList.push_back(plainIdx);
        }
    }

    void Framegraph2::registerSampler(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        SamplerMetaData meta;
        read(&_reader, meta);

        m_hSampler.push_back({ meta.samplerId });
        m_sparse_sampler_meta[meta.samplerId] = meta;
    }

    void Framegraph2::registerImageView(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        ImageViewDesc meta;
        read(&_reader, meta);

        m_hImgView.push_back({ meta.imgViewId });
        m_sparse_img_view_desc[meta.imgViewId] = meta;
    }

    void Framegraph2::storeBackBuffer(MemoryReader& _reader)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        uint16_t id;
        read(&_reader, id);

        m_backBufferSet.insert( id );
    }

    const ResInteractDesc merge(const ResInteractDesc& _desc0, const ResInteractDesc& _desc1)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        ResInteractDesc desc = _desc0;

        desc.access |= _desc1.access;
        desc.stage |= _desc1.stage;

        return std::move(desc);
    }

    bool isValidBinding(uint32_t _binding)
    {
        return _binding < kDescriptorSetBindingDontCare;
    }

    const ResInteractDesc mergeIfNoConflict(const ResInteractDesc& _desc0, const ResInteractDesc& _desc1)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        ResInteractDesc retVar = _desc1;

        if (_desc0.layout == retVar.layout)
        {

            if (_desc0.binding == retVar.binding)
            {
                message(DebugMessageType::info, "resource interaction in current pass already exist! Now merging!");
                retVar = merge(_desc0, retVar);
            }
            else
            {
                if (isValidBinding(_desc0.binding)) {
                    message(DebugMessageType::info, "resource interaction in current pass already exist! Old: %d, New: %d, Using: %d", _desc0.binding, retVar.binding, _desc0.binding);
                    retVar = merge(_desc0, retVar);
                    retVar.binding = _desc0.binding;
                }
                else if (isValidBinding(retVar.binding)) {
                    message(DebugMessageType::info, "resource interaction in current pass already exist! Now merging!");
                    retVar = merge(_desc0, retVar);

                }
                else {
                    message(DebugMessageType::error, "resource interaction in current pass already exist! conflicts detected!");
                }
            }
        }
        else
        {
            message(DebugMessageType::error, "resource interaction in current pass already exist! conflicts detected!");
        }

        return retVar;
    }

    uint32_t Framegraph2::readResource(const stl::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        const size_t hPassIdx = getElemIndex(m_hPass, { _passId });
        assert(hPassIdx != kInvalidIndex);

        uint32_t actualSize = 0;
        for (const PassResInteract& prInteract: _resVec)
        {
            uint16_t passId = prInteract.passId;
            assert(passId == _passId);

            uint16_t resId = prInteract.resId;
            ResInteractDesc interact = prInteract.interact;

            CombinedResID plainId{ resId, _type };


            auto existent = m_pass_rw_res[hPassIdx].readInteractMap.find(plainId);
            if (existent != m_pass_rw_res[hPassIdx].readInteractMap.end())
            {
                // update the existing interaction
                ResInteractDesc& orig = existent->second;
                orig = mergeIfNoConflict(orig, interact);
            }
            else
            {
                m_pass_rw_res[hPassIdx].readInteractMap.insert({ plainId, interact });
                m_pass_rw_res[hPassIdx].readCombinedRes.insert(plainId);
                actualSize++;
            }

            if (isImage(plainId))
            {
                assert(kInvalidHandle != prInteract.samplerId);
                m_pass_rw_res[hPassIdx].imageSamplerMap.push_back(plainId, prInteract.samplerId);
            }
        }

        assert(m_pass_rw_res[hPassIdx].readCombinedRes.size() == m_pass_rw_res[hPassIdx].readInteractMap.size());

        return actualSize;
    }

    uint32_t Framegraph2::writeResource(const stl::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        const size_t hPassIdx = getElemIndex(m_hPass, { _passId });
        assert(hPassIdx != kInvalidIndex);

        uint32_t actualSize = 0;
        for (const PassResInteract& prInteract : _resVec)
        {
            uint16_t passId = prInteract.passId;
            assert(passId == _passId);

            uint16_t resId = prInteract.resId;
            ResInteractDesc interact = prInteract.interact;

            CombinedResID plainId{ resId, _type };

            auto existent = m_pass_rw_res[hPassIdx].writeInteractMap.find(plainId);
            if (existent != m_pass_rw_res[hPassIdx].writeInteractMap.end())
            {
                // update the existing interaction
                ResInteractDesc& orig = existent->second;
                orig = mergeIfNoConflict(orig, interact);
            }
            else
            {
                m_pass_rw_res[hPassIdx].writeInteractMap.insert({ plainId, interact });
                m_pass_rw_res[hPassIdx].writeCombinedRes.insert(plainId);
                actualSize++;
            }
        }

        assert(m_pass_rw_res[hPassIdx].writeCombinedRes.size() == m_pass_rw_res[hPassIdx].writeInteractMap.size());

        return actualSize;
    }

    uint32_t Framegraph2::writeResForceAlias(const stl::vector<WriteOperationAlias>& _aliasMapVec, const uint16_t _passId, const ResourceType _type)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        const size_t hPassIdx =getElemIndex(m_hPass, { _passId });
        assert(hPassIdx != kInvalidIndex);

        uint32_t actualSize = 0;
        for (const WriteOperationAlias& wra : _aliasMapVec)
        {
            CombinedResID basdCombined{ wra.writeOpIn, _type };
            CombinedResID aliasCombined{ wra.writeOpOut, _type };

            if (m_pass_rw_res[hPassIdx].writeOpForcedAliasMap.exist(basdCombined))
            {
                message(error, "write to same resource in same pass multiple times!");
                continue;
            }

            m_pass_rw_res[hPassIdx].writeOpForcedAliasMap.push_back(basdCombined, aliasCombined);
            actualSize++;
        }

        return actualSize;
    }

    void Framegraph2::aliasResForce(MemoryReader& _reader, ResourceType _type)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        ResAliasInfo info;
        read(&_reader, info);

        void* mem = alloc(m_pAllocator, info.aliasNum * sizeof(uint16_t));
        read(&_reader, mem, info.aliasNum * sizeof(uint16_t));

        CombinedResID combinedBaseIdx{ info.resBase, _type };
        size_t idx = getElemIndex(m_combinedForceAlias_base, combinedBaseIdx);
        if (kInvalidIndex == idx)
        {
            m_combinedForceAlias_base.push_back(combinedBaseIdx);
            stl::vector<CombinedResID> aliasVec({ 1, combinedBaseIdx }); // store base resource into the list

            m_combinedForceAlias.push_back(aliasVec);
            idx = m_combinedForceAlias.size() - 1;
        }

        for (uint16_t ii = 0; ii < info.aliasNum; ++ii)
        {
            uint16_t resId = *((uint16_t*)mem + ii);
            CombinedResID combinedId{resId, _type};
            push_back_unique(m_combinedForceAlias[idx], combinedId);

            m_forceAliasMapToBase.insert({ combinedId, combinedBaseIdx });
        }

        free(m_pAllocator, mem);
    }

    bool Framegraph2::isForceAliased(const CombinedResID& _res_0, const CombinedResID _res_1) const
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        const CombinedResID base_0 = m_forceAliasMapToBase.find(_res_0)->second;
        const CombinedResID base_1 = m_forceAliasMapToBase.find(_res_1)->second;

        return base_0 == base_1;
    }

    void Framegraph2::buildGraph()
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        stl::unordered_map<CombinedResID, uint16_t> linear_writeResPassIdxMap;

        stl::unordered_set<CombinedResID> writeOpInSetO{};
        for (uint16_t ii = 0; ii < m_hPass.size(); ++ii)
        {
            PassRWResource rwRes = m_pass_rw_res[ii];
            stl::unordered_set<CombinedResID> writeOpInSet{};

            size_t aliasMapNum = rwRes.writeOpForcedAliasMap.size();
            for (uint32_t jj = 0; jj < aliasMapNum; ++jj)
            {
                CombinedResID writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(jj);
                CombinedResID writeOpOut = rwRes.writeOpForcedAliasMap.getDataAt(jj);

                writeOpInSet.insert(writeOpIn);

                linear_writeResPassIdxMap.insert({ writeOpOut , ii });
            }

            for (const CombinedResID combinedRes : rwRes.writeCombinedRes)
            {
                if(writeOpInSet.find(combinedRes) == writeOpInSet.end()){
                    message(error, "what? there is res written but not added to writeOpInSet? check the vkz part!");
                }
            }
        }

        for (const auto& resPassPair : linear_writeResPassIdxMap)
        {
            CombinedResID cid = resPassPair.first;
            uint16_t passIdx = resPassPair.second;

            if (cid == m_combinedPresentImage)
            {
                m_finalPass = m_hPass[passIdx];
                break;
            }
        }

        for (uint16_t ii = 0; ii < m_hPass.size(); ++ii)
        {
            const PassRWResource rwRes = m_pass_rw_res[ii];

            stl::unordered_set<CombinedResID> resReadInPass{}; // includes two type of resources: 1. read in current pass. 2. resource alias before write.
            
            size_t aliasMapNum = rwRes.writeOpForcedAliasMap.size();
            
            // writeOpIn is not in the readCombinedRes, should affect the graph here
            for (uint32_t jj = 0; jj < aliasMapNum; ++jj)
            {
                CombinedResID writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(jj);

                resReadInPass.insert(writeOpIn);
            }

            for (CombinedResID plainRes : rwRes.readCombinedRes)
            {
                resReadInPass.insert(plainRes);
            }

            for (CombinedResID resReadIn : resReadInPass)
            {
                auto it = linear_writeResPassIdxMap.find(resReadIn);
                if (it == linear_writeResPassIdxMap.end()) {
                    continue;
                }

                uint16_t currPassIdx = ii;
                uint16_t writePassIdx = it->second;

                m_pass_dependency[currPassIdx].inPassIdxSet.insert(writePassIdx);
                m_pass_dependency[writePassIdx].outPassIdxSet.insert(currPassIdx);
            }
        }
    }

    void Framegraph2::calcPriority()
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        uint16_t passNum = (uint16_t)m_hPass.size();

        for (uint16_t ii = 0; ii < passNum; ++ ii)
        {
            PassDependency passDep = m_pass_dependency[ii];
        }
    }

    void Framegraph2::calcCombinations(stl::vector<stl::vector<uint16_t>>& _vecs, const stl::vector<uint16_t>& _inVec)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        uint16_t n = (uint16_t)_inVec.size();
        stl::vector<uint16_t> indices(n, 0);
        stl::vector<uint16_t> nums (_inVec);

        std::sort(nums.begin(), nums.end());
        _vecs.push_back(nums);  // add the sorted one as first

        uint16_t i = 0;
        while (i < n) {
            if (indices[i] < i) {
                if (i % 2 == 0) {
                    std::swap(nums[0], nums[i]);
                }
                else {
                    std::swap(nums[indices[i]], nums[i]);
                }
                _vecs.push_back(nums);

                indices[i]++;
                i = 0;
            }
            else {
                indices[i] = 0;
                i++;
            }
        }

        for (const stl::vector<uint16_t>& orders : _vecs)
        {
            message(info, "combs ======");
            for (uint16_t id : orders)
            {
                message(info, "%d", id);
            }
        }
    }

    uint16_t Framegraph2::findCommonParent(const uint16_t _a, uint16_t _b, const stl::unordered_map<uint16_t, uint16_t>& _sortedParentMap, const uint16_t _root)
    {
        VKZ_ZoneScopedC(Color::light_yellow);
        uint16_t commonNode = _root;

        uint16_t bp = 0;

        stl::unordered_set<uint16_t> apSet;
        uint16_t curr = _a;
        while (curr != _root)
        {
            curr =  _sortedParentMap.find(curr)->second;
            apSet.insert(curr);
        }

        curr = _b;
        while (curr != _root)
        {
            curr = _sortedParentMap.find(curr)->second;
            if (apSet.find(curr) != apSet.end())
            {
                commonNode = curr;
                break;
            }
        }

        message(info, "%d, %d shares the common parent node: %d", _a, _b, commonNode);

        return commonNode;
    }

    uint16_t Framegraph2::validateSortedPasses(const stl::vector<uint16_t>& _sortedPassIdxes, const stl::unordered_map<uint16_t, uint16_t>& _sortedParentMap)
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        for (uint16_t idx : _sortedPassIdxes)
        {
            message(info, "sorted pass Idx: %d/%d, Id: %d", idx, _sortedPassIdxes.size(), m_hPass[idx].id);
        }

        for (const auto& p : _sortedParentMap)
        {
            message(info, "sorted Idx: %d/%d", p.first, p.second);
        }

        stl::unordered_map<CombinedResID, uint16_t> writeResPassIdxMap;

        uint16_t order = 0;
        for (uint16_t passIdx : _sortedPassIdxes)
        {
            PassRWResource rwRes = m_pass_rw_res[passIdx];
            PassHandle pass = m_hPass[passIdx];

            // check if there any resource is read after a flip executed;
            // all flip point
            for (uint16_t ii = 0; ii < rwRes.writeOpForcedAliasMap.size(); ++ii)
            {
                CombinedResID writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(ii);

                StringView resName;
                if (writeOpIn.type == ResourceType::image)
                {
                    resName = getName( ImageHandle{ writeOpIn.id });
                }
                else if (writeOpIn.type == ResourceType::buffer)
                {
                    resName = getName(BufferHandle{ writeOpIn.id });
                }
                else
                {
                    assert(0);
                }
                StringView passName = getName(pass);

                message(info, "res : 0x%8x:%s in pass %d:%s", writeOpIn, resName.getPtr(), pass.id, passName.getPtr());

                writeResPassIdxMap.insert({ writeOpIn, order });
            }
            order++;
        }

        order = 0;
        uint16_t root = _sortedPassIdxes.back();
        uint16_t commonNodeIdx = kInvalidIndex;
        for (uint16_t passIdx : _sortedPassIdxes)
        {
            PassRWResource rwRes = m_pass_rw_res[passIdx];
            PassHandle pass = m_hPass[passIdx];

            for (const CombinedResID& plainRes : rwRes.readCombinedRes)
            {
                if (writeResPassIdxMap.find(plainRes) == writeResPassIdxMap.end())
                {
                    continue;
                }
                uint16_t writeOrder = writeResPassIdxMap[plainRes];
                if (writeOrder < order)
                {
                    message(warning, "resource 0x%8x in pass %d/%d is written before read @ pass %d/%d!", plainRes, pass.id, order, _sortedPassIdxes[writeOrder], writeOrder);
                    commonNodeIdx = findCommonParent(passIdx, _sortedPassIdxes[writeOrder], _sortedParentMap, root);
                }
            }
            order++;
        }

        return commonNodeIdx;
    }

    uint32_t permutation(uint32_t _v)
    {
        uint32_t ret = 1;
        for (uint32_t ii = 1; ii <= _v; ++ii)
        {
            ret *= ii;
        }
        return ret;
    }

    void Framegraph2::reverseTraversalDFSWithBackTrack()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        uint16_t passNum = (uint16_t)m_hPass.size();
        uint16_t commonIdx = kInvalidIndex;

        stl::unordered_map<uint16_t, stl::vector<stl::vector<uint16_t>>> passCombinationMap;
        stl::unordered_map<uint16_t, uint32_t> passCombinationIdx;

        for (uint16_t ii = 0; ii < passNum; ++ii)
        {
            PassDependency passDep = m_pass_dependency[ii];

            stl::vector<uint16_t> inPassIdxVec;
            for (uint16_t idx : passDep.inPassIdxSet)
            {
                inPassIdxVec.push_back(idx);
            }

            stl::vector<stl::vector<uint16_t>> orders;
            calcCombinations(orders, inPassIdxVec);

            passCombinationMap.insert({ ii, orders });
            passCombinationIdx.insert({ ii, 0 });

        }

        for (PassHandle pidx : m_hPass)
        {
            message(info, "pass: %d", pidx.id);
        }

        while (true)
        {
            stl::vector<bool> visited(passNum, false);
            stl::vector<bool> onStack(passNum, false);

            uint16_t finalPassIdx = (uint16_t)getElemIndex(m_hPass, m_finalPass);
            assert(finalPassIdx != kInvalidIndex);

            stl::vector<uint16_t> sortedPassIdx;
            stl::vector<uint16_t> passIdxStack;
            stl::unordered_map<uint16_t, uint16_t> passIdxToNode;

            // start with the final pass
            passIdxStack.push_back(finalPassIdx);

            uint16_t levelIndex = 0;
            while (!passIdxStack.empty())
            {
                uint16_t currPassIdx = passIdxStack.back();

                visited[currPassIdx] = true;
                onStack[currPassIdx] = true;

                PassDependency passDep = m_pass_dependency[currPassIdx];

                bool inPassAllVisited = true;
                uint32_t combIdx = passCombinationIdx.find(currPassIdx)->second;
                const stl::vector<uint16_t>& inPassIdxVec = passCombinationMap.find(currPassIdx)->second[combIdx];

                for (uint16_t childPassIdx : inPassIdxVec)
                {
                    if (!visited[childPassIdx])
                    {
                        passIdxToNode.insert({ childPassIdx, currPassIdx });
                        passIdxStack.push_back(childPassIdx);
                        onStack[childPassIdx] = true;

                        inPassAllVisited = false;
                    }
                    else if (onStack[childPassIdx])
                    {
                        message(error, "cycle detected! pass %d depends by %d!", childPassIdx, currPassIdx);
                        return;
                    }
                }

                if (inPassAllVisited)
                {
                    if (kInvalidIndex == getElemIndex(sortedPassIdx, currPassIdx)) {
                        sortedPassIdx.push_back(currPassIdx);
                    }

                    passIdxStack.pop_back();
                    onStack[currPassIdx] = false;
                }
            }


            commonIdx =  validateSortedPasses(sortedPassIdx, passIdxToNode);

            if (commonIdx == kInvalidIndex)
            {
                m_sortedPass.clear();
                m_sortedPassIdx.clear();

                // fill the sorted and clipped pass
                for (uint16_t idx : sortedPassIdx)
                {
                    m_sortedPass.push_back(m_hPass[idx]);
                }
                m_sortedPassIdx = sortedPassIdx;

                break;
            }
            else
            {
                uint32_t& combIdx = passCombinationIdx.find(commonIdx)->second;
                combIdx++;

                const stl::vector < stl::vector<uint16_t>>& combs = passCombinationMap.find(commonIdx)->second;
                if (combIdx >= combs.size())
                {
                    message(error, "no valid combination found!");
                    break;
                }
            }
        }
    }

    void Framegraph2::buildMaxLevelList(stl::vector<uint16_t>& _maxLvLst)
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        for (uint16_t passIdx : m_sortedPassIdx)
        {
            uint16_t maxLv = 0;

            for (uint16_t inPassIdx : m_pass_dependency[passIdx].inPassIdxSet)
            {
                uint16_t dd = _maxLvLst[inPassIdx] + 1;
                maxLv = glm::max(maxLv, dd);
            }

            _maxLvLst[passIdx] = maxLv;
        }
    }

    void Framegraph2::formatDependency(const stl::vector<uint16_t>& _maxLvLst)
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        // initialize the dependency level
        const uint16_t maxLv = *std::max_element(_maxLvLst.begin(), _maxLvLst.end());

        stl::vector<PassInDependLevel> passInDpLevel(m_sortedPassIdx.size(), PassInDependLevel{ (uint32_t)(maxLv * m_sortedPassIdx.size()) });

        const uint32_t stride = (uint32_t)m_sortedPassIdx.size();
        for (uint16_t passIdx : m_sortedPassIdx)
        {
            stl::vector<uint16_t> passStack;
            passStack.push_back(passIdx);

            uint16_t baseLv = _maxLvLst[passIdx];

            stl::unordered_map<uint16_t, uint16_t> lvToIdx;
            for (uint16_t lv = 0; lv < maxLv; ++lv)
            {
                lvToIdx.insert({ lv, 0 });
            }

            while (!passStack.empty())
            {
                uint16_t currPassIdx = passStack.back();
                passStack.pop_back();

                const uint16_t currLv = _maxLvLst[currPassIdx];
                for (const uint16_t inPassIdx : m_pass_dependency[currPassIdx].inPassIdxSet)
                {
                    uint16_t lvDist = baseLv - currLv;

                    stl::vector<uint16_t>::iterator beginIt = passInDpLevel[passIdx].plainPassInLevel.begin();
                    size_t offset = stride * lvDist;
                    stl::vector<uint16_t> range(beginIt + offset, beginIt + offset + stride);

                    const size_t idx = getElemIndex(range, inPassIdx);
                    
                    if (currLv - 1 != _maxLvLst[inPassIdx] || kInvalidIndex != idx)
                    {
                        continue;
                    }
                    
                    uint16_t size = lvToIdx.find(lvDist)->second;
                    size_t finalIdx = offset + size;
                    passInDpLevel[passIdx].plainPassInLevel[finalIdx] = inPassIdx;
                    lvToIdx.emplace({ lvDist, uint16_t(size + 1) });
                }

                for (const uint16_t inPassIdx : m_pass_dependency[currPassIdx].inPassIdxSet)
                {
                    passStack.push_back(inPassIdx);
                }
            }
        }

        m_passIdxInDpLevels.resize(passInDpLevel.size());
        for (uint32_t ii = 0; ii < passInDpLevel.size(); ++ii)
        {
            const PassInDependLevel& pidl = passInDpLevel[ii];
            PassInDependLevel& target = m_passIdxInDpLevels[ii];
            target.plainPassInLevel.assign(pidl.plainPassInLevel.begin(), pidl.plainPassInLevel.end());
            target.maxLevel = maxLv;
        }

    }

    void Framegraph2::fillNearestSyncPass()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        // init nearest sync pass
        m_nearestSyncPassIdx.resize(m_sortedPass.size());
        for (auto& piq : m_nearestSyncPassIdx)
        {
            for (uint16_t & pIdx : piq.arr)
            {
                pIdx = kInvalidIndex;
            }
        }

        for (const uint16_t passIdx : m_sortedPassIdx)
        {
            const PassInDependLevel& passDep = m_passIdxInDpLevels[passIdx];

            for (const uint16_t pIdx : passDep.plainPassInLevel)
            {
                size_t passCount = m_sortedPassIdx.size();

                for (uint16_t qIdx = 0; qIdx < m_passIdxInQueue.size(); ++qIdx)
                {
                    const size_t idx = getElemIndex(m_passIdxInQueue[qIdx], pIdx);

                    bool isMatch =
                        (kInvalidIndex == m_nearestSyncPassIdx[passIdx].arr[qIdx]) // not set yet
                        && (kInvalidIndex != idx); // found

                    // only first met would set the value
                    if (isMatch) {
                        m_nearestSyncPassIdx[passIdx].arr[qIdx] = pIdx;

                        message(info, "pass %d to queue %d: %d", passIdx, qIdx, pIdx);
                        break;
                    }
                }
            }
        }
        // cool nesting :)
    }

    void Framegraph2::optimizeSyncPass()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        for (uint16_t pIdx : m_sortedPassIdx)
        {
            const PassInDependLevel& dpLv = m_passIdxInDpLevels[pIdx];

            if (dpLv.plainPassInLevel.empty())
                continue;

            stl::vector<uint16_t> lv0;
            for (uint16_t ii=0; ii < m_sortedPassIdx.size(); ++ii)
            {
                if (dpLv.plainPassInLevel[ii] == kInvalidIndex)
                    break;

                lv0.push_back(dpLv.plainPassInLevel[ii]);

                message(info, "pass: %d to sync %d", pIdx, dpLv.plainPassInLevel[ii]);
            }

            m_passIdxToSync[pIdx].insert(m_passIdxToSync[pIdx].end(), lv0.begin(), lv0.end());
        }

        // TODO: 
        //      if m_passIdxToSync for one pass needs to sync with multiple queue
        // =========================
        //      **Fence** required
    }

    void Framegraph2::postParse()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        stl::vector<CombinedResID> plainAliasRes;
        stl::vector<uint16_t> plainAliasResIdx;

        for (uint16_t ii = 0; ii < m_combinedForceAlias_base.size(); ++ii)
        {
            CombinedResID base = m_combinedForceAlias_base[ii];

            const stl::vector<CombinedResID>& aliasVec = m_combinedForceAlias[ii];

            plainAliasRes.insert(plainAliasRes.end(), aliasVec.begin(), aliasVec.end());

            for (CombinedResID alias : aliasVec)
            {
                plainAliasResIdx.push_back(ii);
                m_plainResAliasToBase.push_back(alias, base);
            }
        }

        stl::vector<CombinedResID> fullMultiFrameRes = m_multiFrame_resList;
        for (const CombinedResID cid : m_multiFrame_resList)
        {
            const size_t idx = getElemIndex(plainAliasRes, cid);
            if (kInvalidHandle == idx)
            {
                continue;
            }

            const uint16_t alisIdx = plainAliasResIdx[idx];
            const stl::vector<CombinedResID>& aliasVec = m_combinedForceAlias[alisIdx];
            for (CombinedResID alias : aliasVec)
            {
                push_back_unique(fullMultiFrameRes, alias);
            }
        }

        m_multiFrame_resList = fullMultiFrameRes;
    }

    void Framegraph2::buildResLifetime()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        stl::vector<CombinedResID> resInUseUniList;
        stl::vector<CombinedResID> resToOptmUniList; // all used resources except: force alias, multi-frame, read-only
        stl::vector<CombinedResID> readResUniList;
        stl::vector<CombinedResID> writeResUniList;
        
        for (const uint16_t pIdx : m_sortedPassIdx)
        {
            PassRWResource rwRes = m_pass_rw_res[pIdx];

            for (const CombinedResID combRes : rwRes.writeCombinedRes)
            {
                push_back_unique(writeResUniList, combRes);
                push_back_unique(resToOptmUniList, combRes);
                push_back_unique(resInUseUniList, combRes);
            }

            for (const CombinedResID combRes : rwRes.readCombinedRes)
            {
                push_back_unique(readResUniList, combRes);
                push_back_unique(resToOptmUniList, combRes);
                push_back_unique(resInUseUniList, combRes);
            }

            size_t aliasMapNum = rwRes.writeOpForcedAliasMap.size();
            for (uint32_t ii = 0; ii < aliasMapNum; ++ii)
            {
                CombinedResID writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(ii);
                CombinedResID writeOpOut = rwRes.writeOpForcedAliasMap.getDataAt(ii);

                push_back_unique(writeResUniList, writeOpOut);
                push_back_unique(resToOptmUniList, writeOpOut);
                push_back_unique(resInUseUniList, writeOpOut);
            }
        }

        stl::vector<stl::vector<uint16_t>> resToOptmPassIdxByOrder(resToOptmUniList.size(), stl::vector<uint16_t>()); // share the same idx with resToOptmUniList
        for(uint16_t ii = 0; ii < m_sortedPassIdx.size(); ++ii)
        {
            const uint16_t pIdx = m_sortedPassIdx[ii];
            PassRWResource rwRes = m_pass_rw_res[pIdx];

            for (const CombinedResID combRes : rwRes.writeCombinedRes)
            {
                const size_t usedIdx = getElemIndex(resToOptmUniList, combRes);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }

            for (const CombinedResID combRes : rwRes.readCombinedRes)
            {
                const size_t usedIdx = getElemIndex(resToOptmUniList, combRes);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }

            size_t aliasMapNum = rwRes.writeOpForcedAliasMap.size();
            for (uint32_t alisMapIdx = 0; alisMapIdx < aliasMapNum; ++alisMapIdx)
            {
                CombinedResID writeOpOut = rwRes.writeOpForcedAliasMap.getDataAt(alisMapIdx);
                const size_t usedIdx = getElemIndex(resToOptmUniList, writeOpOut);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }
        }

        stl::vector<CombinedResID> readonlyResUniList;
        for (const CombinedResID readRes : readResUniList)
        {
            // not found in writeResUniList: no one write to it
            if (kInvalidIndex != getElemIndex(writeResUniList, readRes)) {
                continue;
            }

            // force alias
            // read-only has lower priority than force alias, if a resource is force aliased, it definitely will alias.
            // if not, then we consider to fill a individual bucket for it.
            if (m_plainResAliasToBase.exist(readRes)) {
                message(warning, "%s %d is force aliased but it's read-only, is this by intention", isImage(readRes) ? "image" :"buffer", readRes.id );
                continue;
            }

            readonlyResUniList.push_back(readRes);
        }


        stl::vector<CombinedResID> multiframeResUniList;
        for (const CombinedResID multiFrameRes : m_multiFrame_resList)
        {
            // not in use 
            if (kInvalidIndex == getElemIndex(resInUseUniList, multiFrameRes)) {
                continue;
            }
            // force alias
            // multi-frame has lower priority than force alias, if a resource is force aliased, it definitely will alias.
            // if not, then we consider to fill a individual bucket for it.
            if (m_plainResAliasToBase.exist(multiFrameRes)) {
                continue;
            }

            multiframeResUniList.push_back(multiFrameRes);
        }

        // remove force alias resource from resToOptmUniList
        for (size_t ii = 0; ii < m_plainResAliasToBase.size(); ++ii)
        {
            // find current id in resToOptmUniList
            const CombinedResID plainAlias = m_plainResAliasToBase.getIdAt(ii);
            const size_t idx = getElemIndex(resToOptmUniList, plainAlias);
            if (kInvalidIndex == idx) {
                continue;
            }

            resToOptmUniList.erase(resToOptmUniList.begin() + idx);
            resToOptmPassIdxByOrder.erase(resToOptmPassIdxByOrder.begin() + idx);
        }

        // remove read-only resource from resToOptmUniList
        for (const CombinedResID readonlyRes : readonlyResUniList)
        {
            const size_t idx = getElemIndex(resToOptmUniList, readonlyRes);
            if (kInvalidIndex == idx) {
                continue;
            }

            resToOptmUniList.erase(resToOptmUniList.begin() + idx);
            resToOptmPassIdxByOrder.erase(resToOptmPassIdxByOrder.begin() + idx);
        }

        // remove multi-frame resource from resToOptmUniList
        for (const CombinedResID multiFrameRes : multiframeResUniList)
        {
            const size_t idx = getElemIndex(resToOptmUniList, multiFrameRes);
            if (kInvalidIndex == idx) {
                continue;
            }
            resToOptmUniList.erase(resToOptmUniList.begin() + idx);
            resToOptmPassIdxByOrder.erase(resToOptmPassIdxByOrder.begin() + idx);
        }

        assert(resToOptmPassIdxByOrder.size() == resToOptmUniList.size());

        stl::vector < ResLifetime > resLifeTime;
        // only transient resource would be in the lifetime list
        for (const stl::vector<uint16_t> & passIdxByOrder : resToOptmPassIdxByOrder)
        {
            assert(!passIdxByOrder.empty());

            uint16_t maxIdx = *std::max_element(passIdxByOrder.begin(), passIdxByOrder.end());
            uint16_t minIdx = *std::min_element(passIdxByOrder.begin(), passIdxByOrder.end());

            resLifeTime.push_back({ minIdx, maxIdx });
        }

        // set the value
        m_resInUseUniList = resInUseUniList;
        m_resToOptmUniList = resToOptmUniList;
        m_resInUseReadonlyList = readonlyResUniList;
        m_resInUseMultiframeList = multiframeResUniList;

        m_resLifeTime = resLifeTime;
    }

    void Framegraph2::fillBucketForceAlias()
    {
        stl::vector<CombinedResID> actualAliasBase;
        stl::vector<stl::vector<CombinedResID>> actualAlias;
        for (const CombinedResID combRes : m_resInUseUniList)
        {
            if (!m_plainResAliasToBase.exist(combRes))
            {
                continue;
            }

            CombinedResID base2 = m_plainResAliasToBase.getIdToData(combRes);

            const size_t usedBaseIdx = push_back_unique(actualAliasBase, base2);
            if (actualAlias.size() == usedBaseIdx) // if it's new one
            {
                actualAlias.emplace_back();
            }

            push_back_unique(actualAlias[usedBaseIdx], combRes);
        }

        // process force alias
        for (uint16_t ii = 0; ii < actualAliasBase.size(); ++ii)
        {
            const CombinedResID base = actualAliasBase[ii];
            const stl::vector<CombinedResID>& aliasVec = actualAlias[ii];

            // buffers
            if (isBuffer(base)) 
            {
                const FGBufferCreateInfo info = m_sparse_buf_info[base.id];

                BufBucket bucket;
                createBufBkt(bucket, info, aliasVec, true);
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (isImage(base))
            {
                const FGImageCreateInfo info = m_sparse_img_info[base.id];

                ImgBucket bucket;
                createImgBkt(bucket, info, aliasVec, true);
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph2::fillBucketReadonly()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        for (const CombinedResID cid : m_resInUseReadonlyList)
        {
            // buffers
            if (isBuffer(cid))
            {
                const FGBufferCreateInfo info = m_sparse_buf_info[cid.id];

                BufBucket bucket;
                createBufBkt(bucket, info, { 1, cid });
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (isImage(cid))
            {
                const FGImageCreateInfo info = m_sparse_img_info[cid.id];

                ImgBucket bucket;
                createImgBkt(bucket, info, {1, cid});
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph2::fillBucketMultiFrame()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        for (const CombinedResID cid : m_resInUseMultiframeList)
        {
            // buffers
            if (isBuffer(cid))
            {
                const FGBufferCreateInfo info = m_sparse_buf_info[cid.id];

                BufBucket bucket;
                createBufBkt(bucket, info, { 1, cid });
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (isImage(cid))
            {
                const FGImageCreateInfo info = m_sparse_img_info[cid.id];

                ImgBucket bucket;
                createImgBkt(bucket, info, { 1, cid });
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph2::createBufBkt(BufBucket& _bkt, const FGBufferCreateInfo& _info, const stl::vector<CombinedResID>& _reses, const bool _forceAliased /*= false*/)
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        assert(!_reses.empty());

        _bkt.desc.size = _info.size;
        _bkt.pData = _info.pData;

        _bkt.desc.memFlags = _info.memFlags;
        _bkt.desc.usage = _info.usage;

        _bkt.initialBarrierState = _info.initialState;
        _bkt.baseBufId = _info.bufId;
        _bkt.reses = _reses;
        _bkt.forceAliased = _forceAliased;
    }

    void Framegraph2::createImgBkt(ImgBucket& _bkt, const FGImageCreateInfo& _info, const stl::vector<CombinedResID>& _reses, const bool _forceAliased /*= false*/)
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        assert(!_reses.empty());

        _bkt.desc.width = _info.width;
        _bkt.desc.height = _info.height;
        _bkt.desc.depth = _info.depth;
        _bkt.desc.numMips = _info.numMips;
        _bkt.desc.numLayers = _info.numLayers;

        _bkt.size = _info.size;
        _bkt.pData = _info.pData;

        _bkt.desc.type = _info.type;
        _bkt.desc.viewType = _info.viewType;
        _bkt.desc.format = _info.format;
        _bkt.desc.usage = _info.usage;
        _bkt.desc.layout = _info.layout;
        _bkt.desc.viewCount = _info.viewCount;


        for (uint32_t mipIdx = 0; mipIdx < kMaxNumOfImageMipLevel; ++mipIdx)
        {
            _bkt.desc.mipViews[mipIdx] = mipIdx < _bkt.desc.viewCount ? _info.mipViews[mipIdx] : ImageViewHandle{ kInvalidHandle };
        }

        _bkt.aspectFlags = _info.aspectFlags;
        _bkt.initialBarrierState = _info.initialState;
        _bkt.baseImgId = _info.imgId;
        _bkt.reses = _reses;
        _bkt.forceAliased = _forceAliased;


    }

    void Framegraph2::aliasBuffers(stl::vector<BufBucket>& _buckets, const stl::vector<uint16_t>& _sortedBufList)
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        stl::vector<uint16_t> restRes = _sortedBufList;

        stl::vector<stl::vector<CombinedResID>> resInLevel{{}};

        if (restRes.empty()) {
            return;
        }

        // now process normal alias resources
        stl::vector<BufBucket> buckets;
        while (!restRes.empty())
        {
            uint16_t baseRes = *restRes.begin();
            const CombinedResID baseCid{ baseRes, ResourceType::buffer };

            // check if current resource can alias into any existing bucket
            bool aliased = false;
            for (BufBucket& bkt : buckets)
            {
                if (!isAliasable(baseCid, bkt, resInLevel.back()))
                {
                    continue;
                }

                // alias-able, add current resource into current bucket
                bkt.reses.push_back(baseCid);

                resInLevel.back().push_back(baseCid);

                for (CombinedResID res : bkt.reses)
                {
                    const size_t idx = getElemIndex(restRes, res.id);
                    if(kInvalidIndex == idx)
                        continue;

                    restRes.erase(restRes.begin() + idx);
                }
                aliased = true;
                break;
            }

            if (aliased)
            {
                continue;
            }

            // no suitable bucket found, create a new one
            BufBucket bucket;
            const FGBufferCreateInfo& info = m_sparse_buf_info[baseRes];
            createBufBkt(bucket, info, { 1, baseCid });
            buckets.push_back(bucket);

            restRes.erase(restRes.begin());

            resInLevel.clear();
            resInLevel.emplace_back();
        }

        _buckets = _buckets;
    }

    void Framegraph2::aliasImages(stl::vector<ImgBucket>& _buckets,const stl::vector< FGImageCreateInfo >& _infos, const stl::vector<uint16_t>& _sortedTexList, const ResourceType _type)
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        stl::vector<uint16_t> restRes = _sortedTexList;

        stl::vector<stl::vector<CombinedResID>> resInLevel{{}};

        if (restRes.empty()) {
            return;
        }

        // now process normal alias resources
        stl::vector<ImgBucket> buckets;
        while (!restRes.empty())
        {
            uint16_t baseRes = *restRes.begin();
            const CombinedResID baseCid{ baseRes, _type };

            // check if current resource can alias into any existing bucket
            bool aliased = false;
            for (ImgBucket& bkt : buckets)
            {
                if (!isAliasable(baseCid, bkt, resInLevel.back()))
                {
                    continue;
                }

                aliased = true;

                // alias-able, add current resource into current bucket
                bkt.reses.push_back(baseCid);

                resInLevel.back().push_back(baseCid);

                for (CombinedResID res : bkt.reses)
                {
                    const size_t idx = getElemIndex(restRes, res.id);
                    if (kInvalidIndex == idx)
                        continue;

                    restRes.erase(restRes.begin() + idx);
                }

                break;
            }

            if (aliased)
            {
                continue;
            }

            // no suitable bucket found, create a new one
            ImgBucket bucket;
            const FGImageCreateInfo& info = _infos[baseRes];
            createImgBkt(bucket, info, { 1, baseCid });
            buckets.push_back(bucket);

            restRes.erase(restRes.begin());

            resInLevel.clear();
            resInLevel.emplace_back();
        }

        _buckets.insert(_buckets.end(), buckets.begin(), buckets.end());
    }

    void Framegraph2::fillBufferBuckets()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        // sort the resource by size
        stl::vector<uint16_t> sortedBufIdx;
        for (const CombinedResID& crid : m_resToOptmUniList)
        {
            if (!isBuffer(crid)) {
                continue;
            }
            sortedBufIdx.push_back(crid.id);
        }

        std::sort(sortedBufIdx.begin(), sortedBufIdx.end(), [&](uint16_t _l, uint16_t _r) {
            return m_sparse_buf_info[_l].size > m_sparse_buf_info[_r].size;
        });

        stl::vector<BufBucket> buckets;
        aliasBuffers(buckets,sortedBufIdx);

        m_bufBuckets.insert(m_bufBuckets.end(), buckets.begin(), buckets.end());
    }

    void Framegraph2::fillImageBuckets()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        const ResourceType type = ResourceType::image;

        stl::vector<uint16_t> sortedTexIdx;
        for (const CombinedResID& crid : m_resToOptmUniList)
        {
            if (!(isImage(crid))) {
                continue;
            }
            sortedTexIdx.push_back(crid.id);
        }

        std::sort(sortedTexIdx.begin(), sortedTexIdx.end(), [&](uint16_t _l, uint16_t _r) {
            FGImageCreateInfo info_l = m_sparse_img_info[_l];
            FGImageCreateInfo info_r = m_sparse_img_info[_r];

            size_t size_l = info_l.width * info_l.height * info_l.depth * info_l.numMips * info_l.bpp;
            size_t size_r = info_r.width * info_r.height * info_r.depth * info_r.numMips * info_r.bpp;

            return size_l > size_r;
        });

        stl::vector<ImgBucket> buckets;
        aliasImages(buckets, m_sparse_img_info, sortedTexIdx, type);

        m_imgBuckets.insert(m_imgBuckets.end(), buckets.begin(), buckets.end());
    }

    void Framegraph2::optimizeSync()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        // calc max level list
        stl::vector<uint16_t> maxLvLst(m_sortedPassIdx.size(), 0);
        buildMaxLevelList(maxLvLst);

        // build dependency level
        formatDependency(maxLvLst);

        // init the nearest sync pass
        fillNearestSyncPass(); // TODO: remove this

        // optimize sync pass
        optimizeSyncPass();
    }

    void Framegraph2::optimizeAlias()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        buildResLifetime();

        // static aliases ========================
        // force aliases 
        fillBucketForceAlias();
        // bucket for read-only
        fillBucketReadonly();
        // bucket for multi-frame resources
        fillBucketMultiFrame();

        // dynamic aliases =======================
        // buffers
        fillBufferBuckets();
        // images
        fillImageBuckets();
    }

    void Framegraph2::createBuffers()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        for (const BufBucket& bkt : m_bufBuckets)
        {
            RHIContextOpMagic magic{ RHIContextOpMagic::create_buffer };

            write(&m_rhiMemWriter, magic);

            BufferCreateInfo info;
            info.bufId = bkt.baseBufId;
            info.size = bkt.desc.size;
            info.pData = bkt.pData;
            info.fillVal = bkt.desc.fillVal;
            info.memFlags = bkt.desc.memFlags;
            info.usage = bkt.desc.usage;
            info.resCount = (uint16_t)bkt.reses.size();

            info.barrierState = bkt.initialBarrierState;

            write(&m_rhiMemWriter, info);

            if (info.resCount > 1)
            {
                assert(info.pData == nullptr);
            }

            stl::vector<BufferAliasInfo> aliasInfo;
            for (const CombinedResID cid : bkt.reses)
            {
                BufferAliasInfo alias;
                alias.bufId = cid.id;
                alias.size = m_sparse_buf_info[cid.id].size;
                aliasInfo.push_back(alias);
            }

            write(&m_rhiMemWriter, (void*)aliasInfo.data(), int32_t(sizeof(BufferAliasInfo) * bkt.reses.size()));

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }
    }

    void Framegraph2::createImages()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        // setup image view metas
        stl::vector<ImageViewDesc> imgViewDescs{};
        stl::vector<uint16_t> backbuffers{};
        imgViewDescs.reserve(m_sparse_img_view_desc.size());
        for (const ImgBucket& bkt : m_imgBuckets)
        {
            for (const CombinedResID cid : bkt.reses)
            {
                const FGImageCreateInfo& info = m_sparse_img_info[cid.id];

                for (uint32_t ii = 0 ; ii < info.viewCount; ++ii )
                {
                    imgViewDescs.push_back(m_sparse_img_view_desc[info.mipViews[ii].id]);
                }

                // if it is back buffer
                if (m_backBufferSet.find(cid.id) != m_backBufferSet.end() )
                {
                    backbuffers.push_back(cid.id);
                }
            }
        }

        for (ImageViewDesc desc : imgViewDescs)
        {
            RHIContextOpMagic magic{ RHIContextOpMagic::create_image_view };

            write(&m_rhiMemWriter, magic);

            write(&m_rhiMemWriter, desc);

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }


        for (const ImgBucket& bkt : m_imgBuckets)
        {
            RHIContextOpMagic magic{ RHIContextOpMagic::create_image };

            write(&m_rhiMemWriter, magic);

            ImageCreateInfo info;
            info.imgId = bkt.baseImgId;
            info.numMips = bkt.desc.numMips;
            info.width = bkt.desc.width;
            info.height = bkt.desc.height;
            info.depth = bkt.desc.depth;
            info.numLayers = bkt.desc.numLayers;
            info.pData = bkt.pData;
            info.size = bkt.size;

            info.type = bkt.desc.type;
            info.viewType = bkt.desc.viewType;
            info.format = bkt.desc.format;
            info.usage = bkt.desc.usage;
            info.layout = bkt.desc.layout;

            info.resCount = (uint16_t)bkt.reses.size();

            info.aspectFlags = bkt.aspectFlags;
            info.barrierState = bkt.initialBarrierState;

            info.viewCount = bkt.desc.viewCount;
            for (uint32_t mipIdx = 0; mipIdx < kMaxNumOfImageMipLevel; ++mipIdx)
            {
                info.mipViews[mipIdx] = mipIdx < info.viewCount ? bkt.desc.mipViews[mipIdx] : ImageViewHandle{ kInvalidHandle };
            }

            write(&m_rhiMemWriter, info);

            stl::vector<ImageAliasInfo> aliasInfo;
            for (const CombinedResID cid : bkt.reses)
            {
                ImageAliasInfo alias;
                alias.imgId = cid.id;
                aliasInfo.push_back(alias);
            }

            write(&m_rhiMemWriter, (void*)aliasInfo.data(), int32_t(sizeof(ImageAliasInfo) * bkt.reses.size()));

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }

        if (!backbuffers.empty())
        {
            write(&m_rhiMemWriter, RHIContextOpMagic::set_back_buffers);

            uint32_t count = (uint32_t)backbuffers.size();
            write(&m_rhiMemWriter, count);

            write(&m_rhiMemWriter, (void*)backbuffers.data(), int32_t(sizeof(uint16_t) * backbuffers.size()));

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }

    }

    void Framegraph2::createShaders()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        stl::vector<ProgramHandle> usedProgram{};
        stl::vector<ShaderHandle> usedShaders{};
        for ( PassHandle pass : m_sortedPass)
        {
            const PassMetaData& passMeta = m_sparse_pass_meta[pass.id];

            if (passMeta.queue == PassExeQueue::copy || passMeta.queue == PassExeQueue::fill_buffer)
            {
                continue;
            }

            const ProgramInfo& progInfo = m_sparse_program_info[passMeta.programId];
            usedProgram.push_back({ passMeta.programId });

            for (uint16_t ii = 0; ii < progInfo.createInfo.shaderNum; ++ii)
            {
                const uint16_t shaderId = progInfo.shaderIds[ii];

                usedShaders.push_back({ shaderId });
            }
        }

        // write shader info
        for (ShaderHandle shader : usedShaders)
        {
            const ShaderInfo& info = m_sparse_shader_info[shader.id];
            assert(shader.id == info.createInfo.shaderId);

            std::string& path = m_shader_path[info.pathIdx];
            
            ShaderCreateInfo createInfo;
            createInfo.shaderId = info.createInfo.shaderId;
            createInfo.pathLen = (uint16_t)path.length();

            RHIContextOpMagic magic{ RHIContextOpMagic::create_shader };
            write(&m_rhiMemWriter, magic);

            write(&m_rhiMemWriter, createInfo);

            write(&m_rhiMemWriter, (void*)path.c_str(), (int32_t)path.length());

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }


        // write program info
        for (ProgramHandle prog : usedProgram)
        {
            const ProgramInfo& info = m_sparse_program_info[prog.id];
            assert(prog.id == info.createInfo.progId);

            ProgramCreateInfo createInfo;
            createInfo.progId = info.createInfo.progId;
            createInfo.shaderNum = info.createInfo.shaderNum;
            createInfo.sizePushConstants = info.createInfo.sizePushConstants;

            RHIContextOpMagic magic{ RHIContextOpMagic::create_program };

            write(&m_rhiMemWriter, magic);

            write(&m_rhiMemWriter, createInfo);

            write(&m_rhiMemWriter, (void*)info.shaderIds, (int32_t)(info.createInfo.shaderNum * sizeof(uint16_t)));

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }
    }

    void Framegraph2::createSamplers()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        stl::unordered_set<uint16_t> usedSamplers{};
        for (PassHandle pass : m_sortedPass)
        {
            const size_t passIdx = getElemIndex(m_hPass, pass);
            const PassRWResource& rwRes = m_pass_rw_res[passIdx];

            uint32_t usedSamplerCount = (uint32_t)rwRes.imageSamplerMap.size();
            for (uint32_t ii = 0; ii < usedSamplerCount; ++ii)
            {
                const uint16_t& samplerId = rwRes.imageSamplerMap.getDataAt(ii);
                usedSamplers.insert(samplerId);
            }
        }

        for (uint16_t samplerId : usedSamplers)
        {
            const SamplerMetaData& meta = m_sparse_sampler_meta[samplerId];

            RHIContextOpMagic magic{ RHIContextOpMagic::create_sampler };

            write(&m_rhiMemWriter, magic);
            write(&m_rhiMemWriter, meta);
            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }
    }

    void Framegraph2::createPasses()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        stl::vector<PassMetaData> passMetaDataVec{};
        stl::vector<stl::vector<VertexBindingDesc>> passVertexBinding(m_sortedPass.size());
        stl::vector<stl::vector<VertexAttributeDesc>> passVertexAttribute(m_sortedPass.size());
        stl::vector<stl::vector<int>> passPipelineSpecData(m_sortedPass.size());

        stl::vector<stl::pair<ImageHandle, ResInteractDesc>> writeDSPair(m_sortedPass.size());
        stl::vector<UniDataContainer< ImageHandle, ResInteractDesc> > writeImageVec(m_sortedPass.size());
        stl::vector<UniDataContainer< ImageHandle, ResInteractDesc> > readImageVec(m_sortedPass.size());

        stl::vector<UniDataContainer< BufferHandle, ResInteractDesc> > readBufferVec(m_sortedPass.size());
        stl::vector<UniDataContainer< BufferHandle, ResInteractDesc> > writeBufferVec(m_sortedPass.size());
        stl::vector<UniDataContainer< ImageHandle, SamplerHandle> > imageSamplerVec(m_sortedPass.size());
        stl::vector< UniDataContainer<CombinedResID, CombinedResID> >  writeOpAliasMapVec(m_sortedPass.size());

        
        for (uint32_t ii = 0; ii < m_sortedPass.size(); ++ii)
        {
            PassHandle pass = m_sortedPass[ii];
            const size_t passIdx = getElemIndex(m_hPass, pass);
            const PassMetaData& passMeta = m_sparse_pass_meta[pass.id];
            assert(pass.id == passMeta.passId);

            stl::pair<ImageHandle, ResInteractDesc>& writeDS = writeDSPair[ii];
            UniDataContainer< ImageHandle, ResInteractDesc>& writeColor = writeImageVec[ii];
            UniDataContainer< ImageHandle, ResInteractDesc>& readImg = readImageVec[ii];

            UniDataContainer< BufferHandle, ResInteractDesc>& readBuf = readBufferVec[ii];
            UniDataContainer< BufferHandle, ResInteractDesc>& writeBuf = writeBufferVec[ii];
            UniDataContainer< ImageHandle, SamplerHandle>& imgSamplerMap = imageSamplerVec[ii];

            UniDataContainer<CombinedResID, CombinedResID>& writeOpAliasMap = writeOpAliasMapVec[ii];
            {
                writeDS.first = { kInvalidHandle };
                writeColor.clear();
                readImg.clear();
                readBuf.clear();
                writeBuf.clear();
                imgSamplerMap.clear();

                const PassRWResource& rwRes = m_pass_rw_res[passIdx];
                // set write resources
                for (const CombinedResID writeRes : rwRes.writeCombinedRes)
                {
                    auto writeInteractPair = rwRes.writeInteractMap.find(writeRes);
                    assert(writeInteractPair != rwRes.writeInteractMap.end());

                    if (isDepthStencil(writeRes)) {

                        assert(writeDS.first.id == kInvalidHandle);
                        writeDS.first = { writeRes.id };
                        writeDS.second = writeInteractPair->second;
                        
                        writeColor.push_back({ writeRes.id }, writeInteractPair->second);
                    }
                    else if (isColorAttachment(writeRes))
                    {
                        writeColor.push_back({ writeRes.id }, writeInteractPair->second);
                    }
                    else if (isNormalImage(writeRes)
                        && (passMeta.queue == PassExeQueue::compute || passMeta.queue == PassExeQueue::copy))
                    {
                        writeColor.push_back({ writeRes.id }, writeInteractPair->second);
                    }
                    else if (isBuffer(writeRes))
                    {
                        writeBuf.push_back({ writeRes.id }, writeInteractPair->second);
                    }
                    else
                    {
                        vkz::message(DebugMessageType::error, "invalid write resource mapping: Res %4d, PassExeQueue: %d\n", writeRes.id, passMeta.queue);
                    }
                }

                // set read resources
                for (const CombinedResID readRes : rwRes.readCombinedRes)
                {
                    auto readInteractPair = rwRes.readInteractMap.find(readRes);
                    assert(readInteractPair != rwRes.readInteractMap.end());

                    if (  isColorAttachment(readRes) 
                        || isDepthStencil(readRes)
                        || isNormalImage(readRes))
                    {
                        readImg.push_back({ readRes.id }, readInteractPair->second);
                    }
                    else if (isBuffer(readRes))
                    {
                        assert(!readBuf.exist({ readRes.id }));
                        readBuf.push_back({ readRes.id }, readInteractPair->second);
                    }
                }

                uint32_t usedSamplerNum = (uint32_t)rwRes.imageSamplerMap.size();
                for (uint32_t ii = 0; ii < usedSamplerNum; ++ii)
                {
                    const CombinedResID& image = rwRes.imageSamplerMap.getIdAt(ii);
                    const uint16_t& samplerId = rwRes.imageSamplerMap.getDataAt(ii);
                    imgSamplerMap.push_back({ image.id }, {samplerId });
                }

                // write operation out aliases
                uint32_t aliasMapNum = (uint32_t)rwRes.writeOpForcedAliasMap.size();
                for (uint32_t ii = 0; ii < aliasMapNum; ++ii)
                {
                    const CombinedResID writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(ii);
                    const CombinedResID writeOpOut = rwRes.writeOpForcedAliasMap.getDataAt(ii);

                    writeOpAliasMap.push_back(writeOpIn, writeOpOut);
                }
            }

            // create pass info
            assert(writeDS.first.id == passMeta.writeDepthId);
            assert((uint16_t)writeColor.size() == passMeta.writeImageNum);
            assert((uint16_t)readImg.size() == passMeta.readImageNum);
            assert((uint16_t)readBuf.size() == passMeta.readBufferNum);
            assert((uint16_t)writeBuf.size() == passMeta.writeBufferNum);
            assert((uint16_t)imgSamplerMap.size() == passMeta.sampleImageNum);
            assert((uint16_t)writeOpAliasMap.size() == (passMeta.writeBufAliasNum + passMeta.writeImgAliasNum));

            passMetaDataVec.emplace_back(passMeta);

            const PassMetaDataRef& createDataRef = m_sparse_pass_data_ref[pass.id];

            // vertex binding           
            stl::vector<VertexBindingDesc>& bindingDesc = passVertexBinding[ii];
            for (const uint16_t& bindingIdx : createDataRef.vtxBindingIdxs)
            {
                bindingDesc.push_back(m_vtxBindingDesc[bindingIdx]);
            }
            assert(bindingDesc.size() == passMeta.vertexBindingNum);

            // vertex attribute
            stl::vector<VertexAttributeDesc>& attributeDesc = passVertexAttribute[ii];
            for (const uint16_t& attributeIdx : createDataRef.vtxAttrIdxs)
            {
                attributeDesc.push_back(m_vtxAttrDesc[attributeIdx]);
            }
            assert(attributeDesc.size() == passMeta.vertexAttributeNum);

            // pipeline constants
            stl::vector<int>& pipelineSpecData = passPipelineSpecData[ii];
            for (const uint16_t& pcIdx : createDataRef.pipelineSpecIdxs)
            {
                pipelineSpecData.push_back(m_pipelineSpecData[pcIdx]);
            }
            assert(pipelineSpecData.size() == passMeta.pipelineSpecNum);
        }

        // create things via pass info
        for (uint16_t ii = 0; ii < passMetaDataVec.size(); ++ii)
        {
            const PassMetaData& createInfo = passMetaDataVec[ii];

            RHIContextOpMagic magic{ RHIContextOpMagic::create_pass };

            write(&m_rhiMemWriter, magic);

            write(&m_rhiMemWriter, createInfo);

            // vertex binding
            write(&m_rhiMemWriter, (void*)passVertexBinding[ii].data(), (int32_t)(createInfo.vertexBindingNum * sizeof(VertexBindingDesc)));

            // vertex attribute
            write(&m_rhiMemWriter, (void*)passVertexAttribute[ii].data(), (int32_t)(createInfo.vertexAttributeNum * sizeof(VertexAttributeDesc)));

            // push constants
            write(&m_rhiMemWriter, (void*)passPipelineSpecData[ii].data(), (int32_t)(createInfo.pipelineSpecNum * sizeof(int)));

            // pass read/write resources and it's interaction
            write(&m_rhiMemWriter, (void*)writeImageVec[ii].getIdPtr(), (int32_t)(createInfo.writeImageNum * sizeof(ImageHandle)));
            write(&m_rhiMemWriter, (void*)readImageVec[ii].getIdPtr(), (int32_t)(createInfo.readImageNum * sizeof(ImageHandle)));
            write(&m_rhiMemWriter, (void*)readBufferVec[ii].getIdPtr(), (int32_t)(createInfo.readBufferNum * sizeof(BufferHandle)));
            write(&m_rhiMemWriter, (void*)writeBufferVec[ii].getIdPtr(), (int32_t)(createInfo.writeBufferNum * sizeof(BufferHandle)));

            //write(&m_rhiMemWriter, writeDSPair[ii].second);
            write(&m_rhiMemWriter, (void*)writeImageVec[ii].getDataPtr(), (int32_t)(createInfo.writeImageNum * sizeof(ResInteractDesc)));
            write(&m_rhiMemWriter, (void*)readImageVec[ii].getDataPtr(), (int32_t)(createInfo.readImageNum * sizeof(ResInteractDesc)));
            write(&m_rhiMemWriter, (void*)readBufferVec[ii].getDataPtr(), (int32_t)(createInfo.readBufferNum * sizeof(ResInteractDesc)));
            write(&m_rhiMemWriter, (void*)writeBufferVec[ii].getDataPtr(), (int32_t)(createInfo.writeBufferNum * sizeof(ResInteractDesc)));

            // samplers
            write(&m_rhiMemWriter, (void*)imageSamplerVec[ii].getIdPtr(), (int32_t)(createInfo.sampleImageNum * sizeof(ImageHandle)));
            write(&m_rhiMemWriter, (void*)imageSamplerVec[ii].getDataPtr(), (int32_t)(createInfo.sampleImageNum * sizeof(SamplerHandle)));

            // write op alias
            write(&m_rhiMemWriter, (void*)writeOpAliasMapVec[ii].getIdPtr(), (int32_t)(createInfo.writeBufAliasNum + createInfo.writeImgAliasNum) * sizeof(CombinedResID));
            write(&m_rhiMemWriter, (void*)writeOpAliasMapVec[ii].getDataPtr(), (int32_t)(createInfo.writeBufAliasNum + createInfo.writeImgAliasNum) * sizeof(CombinedResID));

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }
    }

    void Framegraph2::createResources()
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        createBuffers();
        createImages();
        createShaders();
        createSamplers();
        createPasses();

        // set brief
        {
            write(&m_rhiMemWriter, RHIContextOpMagic::set_brief);
            RHIBrief brief;
            brief.finalPassId = m_finalPass.id;
            brief.presentImageId = m_combinedPresentImage.id;
            brief.presentMipLevel = m_presentMipLevel;

            write(&m_rhiMemWriter, brief);
            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }

        // end mem tag
        write(&m_rhiMemWriter,  RHIContextOpMagic::end );
    }

    bool Framegraph2::isBufInfoAliasable(uint16_t _idx, const BufBucket& _bucket, const stl::vector<CombinedResID> _resInCurrStack) const
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        // size check in current stack
        const FGBufferCreateInfo& info = m_sparse_buf_info[_idx];

        bool bCondMatch = (info.size <= _bucket.desc.size);

        for (const CombinedResID res : _resInCurrStack)
        {
            const FGBufferCreateInfo& stackInfo = m_sparse_buf_info[res.id];

            bCondMatch &= (info.pData == nullptr && stackInfo.pData == nullptr);
            bCondMatch &= (info.memFlags == stackInfo.memFlags);
            bCondMatch &= (info.usage == stackInfo.usage);

        }

        return bCondMatch;
    }

    bool Framegraph2::isImgInfoAliasable(uint16_t _ImgId, const ImgBucket& _bucket, const stl::vector<CombinedResID> _resInCurrStack) const
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        bool bCondMatch = true;

        // condition check
        const FGImageCreateInfo& info = m_sparse_img_info[_ImgId];
        for (const CombinedResID res : _resInCurrStack)
        {
            const FGImageCreateInfo& stackInfo = m_sparse_img_info[res.id];

            bCondMatch &= (info.numMips == stackInfo.numMips);
            bCondMatch &= (info.width == stackInfo.width && info.height == stackInfo.height && info.depth == stackInfo.depth);
            bCondMatch &= (info.numLayers == stackInfo.numLayers);
            bCondMatch &= (info.format == stackInfo.format);

            bCondMatch &= (info.usage == stackInfo.usage);
            bCondMatch &= (info.type == stackInfo.type);
        }
        
        return bCondMatch;
    }

    bool Framegraph2::isStackAliasable(const CombinedResID& _res, const stl::vector<CombinedResID>& _reses) const
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        bool bStackMatch = true;
        
        // life time check in entire bucket
        const size_t idx = getElemIndex(m_resToOptmUniList, _res);
        const ResLifetime& resLifetime = m_resLifeTime[idx];
        for (const CombinedResID resInStack : _reses)
        {
            const size_t resIdx = getElemIndex(m_resToOptmUniList, resInStack);
            const ResLifetime& stackLifetime = m_resLifeTime[resIdx];

            // if lifetime overlap with any resource in current bucket, then it's not alias-able
            bStackMatch &= (stackLifetime.startIdx > resLifetime.endIdx || stackLifetime.endIdx < resLifetime.startIdx);
        }

        return bStackMatch;
    }

    bool Framegraph2::isAliasable(const CombinedResID& _res, const BufBucket& _bucket, const stl::vector<CombinedResID>& _resInCurrStack) const
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        // no stack checking for stacks, because each stack only contains 1 resource
        // bool bInfoMatch = isBufInfoAliasable(_res.id, _bucket, _resInCurrStack);
        
        bool bInfoMatch = isBufInfoAliasable(_res.id, _bucket, _bucket.reses);
        bool bStackMatch = isStackAliasable(_res, _bucket.reses);

        return bInfoMatch && bStackMatch;
    }

    bool Framegraph2::isAliasable(const CombinedResID& _res, const ImgBucket& _bucket, const stl::vector<CombinedResID>& _resInCurrStack) const
    {
        VKZ_ZoneScopedC(Color::light_yellow);

        // no stack checking for stacks, because each stack only contains 1 resource
        // bool bInfoMatch = isImgInfoAliasable(_res.id, _bucket, _resInCurrStack);

        bool bInfoMatch = isImgInfoAliasable(_res.id, _bucket, _bucket.reses);
        bool bStackMatch = isStackAliasable(_res, _bucket.reses);

        return bInfoMatch && bStackMatch;
    }

} // namespace vkz
