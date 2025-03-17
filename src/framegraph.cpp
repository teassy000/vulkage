#include "common.h"
#include "macro.h"
#include "config.h"
#include "util.h"

#include "framegraph.h"
#include "rhi_context.h"

#include "kage_math.h"

#include "profiler.h"
#include "bx/readerwriter.h"
#include <algorithm>


namespace kage
{
    Framegraph::~Framegraph()
    {
        KG_ZoneScopedC(Color::light_yellow);
        shutdown();
    }

    void Framegraph::bake()
    {
        KG_ZoneScopedC(Color::light_yellow);
        // prepare
        parseOp();

        postParse();

        buildGraph();

        // sort and cut
        reverseTraversalDFSWithBackTrack();

        // optimize
        // optimizeSync(); // TODO: this would cause out of range access due to using the wrong index, fix it later
        optimizeAlias();

        // actual create resources for renderer
        createResources();
    }

    void Framegraph::shutdown()
    {
        KG_ZoneScopedC(Color::light_yellow);

        m_hShader.clear();
        m_hProgram.clear();
        m_hPass.clear();
        m_hBuf.clear();
        m_hTex.clear();
        m_hSampler.clear();

        m_sparse_shader_info.clear();
        m_sparse_program_info.clear();
        m_sparse_pass_meta.clear();
        m_sparse_buf_info.clear();
        m_sparse_img_info.clear();
        m_sparse_sampler_meta.clear();
        m_sparse_pass_data_ref.clear();
        m_sparse_bindless_meta.clear();
        
        m_shader_path.clear();
        m_unifiedResId.clear();
        m_vtxBindingDesc.clear();
        m_vtxAttrDesc.clear();
        m_pipelineSpecData.clear();

        m_pass_rw_res.clear();
        m_pass_dependency.clear();
        m_unifiedForceAlias_base.clear();
        m_unifiedForceAlias.clear();
        m_forceAliasMapToBase.clear();

        m_sortedPass.clear();
        m_sortedPassIdx.clear();

        m_passIdxInDpLevels.clear();
        m_passIdxToSync.clear();

        m_nearestSyncPassIdx.clear();
        m_multiFrame_resList.clear();

        m_resInUseUniList.clear();
        m_resToOptmUniList.clear();
        m_resInUseReadonlyList.clear();
        m_resInUseMultiframeList.clear();

        m_resLifeTime.clear();
        m_plainResAliasToBase.clear();
        m_bufBuckets.clear();
        m_imgBuckets.clear();

        bx::deleteObject(m_pAllocator, m_pMemBlock);
        m_pAllocator = nullptr;
    }

    void Framegraph::parseOp()
    {
        KG_ZoneScopedC(Color::light_yellow);
        assert(m_pMemBlock != nullptr);
        bx::MemoryReader reader(m_pMemBlock->more(), m_pMemBlock->getSize());

        while (true)
        {
            MagicTag magic = MagicTag::invalid_magic;
            bx::read(&reader, magic, nullptr);

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
            case MagicTag::register_bindless: {
                registerBindless(reader);
                break;
            }
            case MagicTag::register_static: {
                registerStaticResources(reader);
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
                message(DebugMsgType::warning, "invalid magic tag, data incorrect!");
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
            bx::read(&reader, bodyEnd, nullptr);
            assert(MagicTag::magic_body_end == bodyEnd);
        }
    }

    void Framegraph::setBrief(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::light_yellow);
        FrameGraphBrief brief;
        bx::read(&_reader, brief, nullptr);

        // info data will use idx from handle as iterator
        m_sparse_shader_info.resize(brief.shaderNum);
        m_sparse_program_info.resize(brief.programNum);
        m_sparse_pass_meta.resize(brief.passNum);
        m_sparse_buf_info.resize(brief.bufNum);
        m_sparse_img_info.resize(brief.imgNum);
        m_sparse_sampler_meta.resize(brief.samplerNum);
        m_sparse_bindless_meta.resize(brief.bindlessNum);

        m_sparse_pass_data_ref.resize(brief.passNum);

        m_presentImage = { brief.presentImage };
        m_presentMipLevel = brief.presentMipLevel;
    }

    void Framegraph::registerShader(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::light_yellow);
        FGShaderCreateInfo info;
        bx::read(&_reader, info, nullptr);

        m_hShader.push_back({ info.shaderId });
        
        char path[kMaxPathLen];
        bx::read(&_reader, (void*)(path), info.strLen, nullptr);
        path[info.strLen] = '\0';

        m_shader_path.emplace_back(path);
        m_sparse_shader_info[info.shaderId].createInfo = info;
        m_sparse_shader_info[info.shaderId].pathIdx = (uint16_t)m_shader_path.size() - 1;
    }

    void Framegraph::registerProgram(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::light_yellow);
        FGProgCreateInfo regInfo;
        bx::read(&_reader, regInfo, nullptr);

        assert(regInfo.shaderNum <= kMaxNumOfStageInPorgram);

        ProgramInfo& progInfo = m_sparse_program_info[regInfo.progId];
        progInfo.createInfo = regInfo;

        bx::read(&_reader, (void*)(progInfo.shaderIds), sizeof(uint16_t) * regInfo.shaderNum, nullptr);

        m_hProgram.push_back({ regInfo.progId });
    }

    void Framegraph::registerPass(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::light_yellow);
        PassMetaData passMeta;
        bx::read(&_reader, passMeta, nullptr);

        // process order is important
        for (uint32_t ii = 0; ii < passMeta.vertexBindingNum; ++ii)
        {
            VertexBindingDesc bInfo;
            bx::read(&_reader, bInfo, nullptr);
            m_vtxBindingDesc.emplace_back(bInfo);
            m_sparse_pass_data_ref[passMeta.passId].vtxBindingIdxs.push_back((uint16_t)m_vtxBindingDesc.size() - 1);
        }

        for (uint32_t ii = 0; ii < passMeta.vertexAttributeNum; ++ii)
        {
            VertexAttributeDesc aInfo;
            bx::read(&_reader, aInfo, nullptr);
            m_vtxAttrDesc.emplace_back(aInfo);
            m_sparse_pass_data_ref[passMeta.passId].vtxAttrIdxs.push_back((uint16_t)m_vtxAttrDesc.size() - 1);
        }

        for (uint32_t ii = 0; ii < passMeta.pipelineSpecNum; ++ii)
        {
            int pipelineSpec;
            bx::read(&_reader, pipelineSpec, nullptr);
            m_pipelineSpecData.emplace_back(pipelineSpec);
            m_sparse_pass_data_ref[passMeta.passId].pipelineSpecIdxs.push_back((uint16_t)m_pipelineSpecData.size() - 1);
        }

        m_hPass.push_back({ passMeta.passId });
        m_pass_rw_res.emplace_back();

        // write image
        // include color attachment and depth stencil attachment
        stl::vector<PassResInteract> writeImgVec(passMeta.writeImageNum);
        bx::read(&_reader, writeImgVec.data(), sizeof(PassResInteract) * passMeta.writeImageNum, nullptr);
        passMeta.writeImageNum = writeResource(writeImgVec, passMeta.passId, ResourceType::image);

        // read image
        stl::vector<PassResInteract> readImgVec(passMeta.readImageNum);
        bx::read(&_reader, readImgVec.data(), sizeof(PassResInteract) * passMeta.readImageNum, nullptr);
        passMeta.readImageNum = readResource(readImgVec, passMeta.passId, ResourceType::image);

        // write buffer
        stl::vector<PassResInteract> writeBufVec(passMeta.writeBufferNum);
        bx::read(&_reader, writeBufVec.data(), sizeof(PassResInteract) * passMeta.writeBufferNum, nullptr);
        passMeta.writeBufferNum = writeResource(writeBufVec, passMeta.passId, ResourceType::buffer);

        // read buffer
        stl::vector<PassResInteract> readBufVec(passMeta.readBufferNum);
        bx::read(&_reader, readBufVec.data(), sizeof(PassResInteract) * passMeta.readBufferNum, nullptr);
        passMeta.readBufferNum = readResource(readBufVec, passMeta.passId, ResourceType::buffer);

        // write resource alias
        stl::vector<WriteOperationAlias> writeImgAliasVec(passMeta.writeImgAliasNum);
        bx::read(&_reader, writeImgAliasVec.data(), sizeof(WriteOperationAlias) * passMeta.writeImgAliasNum, nullptr);
        passMeta.writeImgAliasNum = writeResForceAlias(writeImgAliasVec, passMeta.passId, ResourceType::image);

        stl::vector<WriteOperationAlias> writeBufAliasVec(passMeta.writeBufAliasNum);
        bx::read(&_reader, writeBufAliasVec.data(), sizeof(WriteOperationAlias) * passMeta.writeBufAliasNum, nullptr);
        passMeta.writeBufAliasNum = writeResForceAlias(writeBufAliasVec, passMeta.passId, ResourceType::buffer);

        // set bindless res
        if(passMeta.queue != PassExeQueue::extern_abstract) {
            const ProgramInfo& progInfo = m_sparse_program_info[passMeta.programId];

            if (progInfo.createInfo.bindlessId != kInvalidHandle)
            {
                BindlessMetaData bMeta = m_sparse_bindless_meta[progInfo.createInfo.bindlessId];

                if (bMeta.resType == ResourceType::image)
                {
                    stl::vector<ImageHandle> bindlessVec(bMeta.resCount);
                    memcpy(bindlessVec.data(), bMeta.resIdMem->data, sizeof(ImageHandle) * bMeta.resCount);

                    for (ImageHandle hRes : bindlessVec)
                    {
                        m_pass_rw_res[passMeta.passId].bindlessRes.insert({ hRes.id, bMeta.resType });
                    }
                }
                else
                {
                    assert(0); // not suport yet
                }
            }
        }

        // fill pass idx in queue
        uint16_t qIdx = (uint16_t)passMeta.queue;
        m_passIdxInQueue[qIdx].push_back((uint16_t)m_hPass.size());

        // fill pass info
        m_sparse_pass_data_ref[passMeta.passId].passId = passMeta.passId;
        m_sparse_pass_meta[passMeta.passId] = passMeta;

        m_pass_dependency.emplace_back();
        m_passIdxToSync.emplace_back();
    }

    void Framegraph::registerBuffer(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::light_yellow);
        FGBufferCreateInfo info;
        bx::read(&_reader, info, nullptr);

        m_hBuf.push_back({ info.hBuf });
        m_sparse_buf_info[info.hBuf] = info;

        UnifiedResHandle plainResID{ info.hBuf};
        m_unifiedResId.push_back(plainResID);

        if (info.lifetime == ResourceLifetime::non_transition)
        {
            UnifiedResHandle plainIdx{ info.hBuf};
            m_multiFrame_resList.push_back(plainIdx);
        }
    }

    void Framegraph::registerImage(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::light_yellow);
        FGImageCreateInfo info;
        bx::read(&_reader, info, nullptr);

        m_hTex.push_back({ info.hImg });
        m_sparse_img_info[info.hImg] = info;

        UnifiedResHandle plainResIdx{ info.hImg};
        m_unifiedResId.push_back(plainResIdx);

        if (info.lifetime == ResourceLifetime::non_transition)
        {
            UnifiedResHandle plainIdx{ info.hImg };
            m_multiFrame_resList.push_back(plainIdx);
        }
    }

    void Framegraph::registerSampler(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::light_yellow);
        SamplerMetaData meta;
        bx::read(&_reader, meta, nullptr);

        m_hSampler.push_back({ meta.samplerId });
        m_sparse_sampler_meta[meta.samplerId] = meta;
    }

    void Framegraph::registerBindless(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::light_yellow);
        BindlessMetaData meta;
        bx::read(&_reader, meta, nullptr);

        m_hBindless.push_back({ meta.bindlessId });
        m_sparse_bindless_meta[meta.bindlessId] = meta;
    }

    void Framegraph::registerStaticResources(bx::MemoryReader& _reader)
    {
        KG_ZoneScopedC(Color::light_yellow);
        uint32_t resNum;
        bx::read(&_reader, resNum, nullptr);

        m_staticResources.resize(resNum);
        bx::read(&_reader, m_staticResources.data(), sizeof(UnifiedResHandle) * resNum, nullptr);
    }

    const ResInteractDesc merge(const ResInteractDesc& _desc0, const ResInteractDesc& _desc1)
    {
        KG_ZoneScopedC(Color::light_yellow);
        ResInteractDesc desc = _desc0;

        desc.access |= _desc1.access;
        desc.stage |= _desc1.stage;

        return std::move(desc);
    }

    bool isValidBinding(uint32_t _binding)
    {
        return _binding < kDescriptorSetBindingDontCare;
    }

    uint32_t Framegraph::readResource(const stl::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type)
    {
        KG_ZoneScopedC(Color::light_yellow);
        const size_t hPassIdx = getElemIndex(m_hPass, { _passId });
        assert(hPassIdx != kInvalidIndex);

        uint32_t actualSize = 0;
        for (const PassResInteract& prInteract: _resVec)
        {
            uint16_t passId = prInteract.passId;
            assert(passId == _passId);

            uint16_t resId = prInteract.resId;
            ResInteractDesc interact = prInteract.interact;

            UnifiedResHandle plainId{ resId, _type };


            auto existent = m_pass_rw_res[hPassIdx].readInteractMap.find(plainId);
            if (existent != m_pass_rw_res[hPassIdx].readInteractMap.end())
            {
                // update the existing interaction
                ResInteractDesc& orig = existent->second;
                orig = merge(orig, interact);
            }
            else
            {
                m_pass_rw_res[hPassIdx].readInteractMap.insert({ plainId, interact });
                m_pass_rw_res[hPassIdx].readUnifiedRes.insert(plainId);
                actualSize++;
            }
        }

        assert(m_pass_rw_res[hPassIdx].readUnifiedRes.size() == m_pass_rw_res[hPassIdx].readInteractMap.size());

        return actualSize;
    }

    uint32_t Framegraph::writeResource(const stl::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type)
    {
        KG_ZoneScopedC(Color::light_yellow);
        const size_t hPassIdx = getElemIndex(m_hPass, { _passId });
        assert(hPassIdx != kInvalidIndex);

        uint32_t actualSize = 0;
        for (const PassResInteract& prInteract : _resVec)
        {
            uint16_t passId = prInteract.passId;
            assert(passId == _passId);

            uint16_t resId = prInteract.resId;
            ResInteractDesc interact = prInteract.interact;

            UnifiedResHandle plainId{ resId, _type };

            auto existent = m_pass_rw_res[hPassIdx].writeInteractMap.find(plainId);
            if (existent != m_pass_rw_res[hPassIdx].writeInteractMap.end())
            {
                // update the existing interaction
                ResInteractDesc& orig = existent->second;
                orig = merge(orig, interact);
            }
            else
            {
                m_pass_rw_res[hPassIdx].writeInteractMap.insert({ plainId, interact });
                m_pass_rw_res[hPassIdx].writeUnifiedRes.insert(plainId);
                actualSize++;
            }
        }

        assert(m_pass_rw_res[hPassIdx].writeUnifiedRes.size() == m_pass_rw_res[hPassIdx].writeInteractMap.size());

        return actualSize;
    }

    uint32_t Framegraph::writeResForceAlias(const stl::vector<WriteOperationAlias>& _aliasMapVec, const uint16_t _passId, const ResourceType _type)
    {
        KG_ZoneScopedC(Color::light_yellow);
        const size_t hPassIdx =getElemIndex(m_hPass, { _passId });
        assert(hPassIdx != kInvalidIndex);

        uint32_t actualSize = 0;
        for (const WriteOperationAlias& wra : _aliasMapVec)
        {
            UnifiedResHandle basdCombined{ wra.writeOpIn, _type };
            UnifiedResHandle aliasCombined{ wra.writeOpOut, _type };

            if (m_pass_rw_res[hPassIdx].writeOpForcedAliasMap.exist(basdCombined))
            {
                message(error, "write to same resource in same pass multiple times!");
                continue;
            }

            m_pass_rw_res[hPassIdx].writeOpForcedAliasMap.addOrUpdate(basdCombined, aliasCombined);
            actualSize++;
        }

        return actualSize;
    }

    void Framegraph::aliasResForce(bx::MemoryReader& _reader, ResourceType _type)
    {
        KG_ZoneScopedC(Color::light_yellow);
        ResAliasInfo info;
        bx::read(&_reader, info, nullptr);

        void* mem = alloc(m_pAllocator, info.aliasNum * sizeof(uint16_t));
        bx::read(&_reader, mem, info.aliasNum * sizeof(uint16_t), nullptr);

        UnifiedResHandle combinedBaseIdx{ info.resBase, _type };
        size_t idx = getElemIndex(m_unifiedForceAlias_base, combinedBaseIdx);
        if (kInvalidIndex == idx)
        {
            m_unifiedForceAlias_base.push_back(combinedBaseIdx);
            stl::vector<UnifiedResHandle> aliasVec({ 1, combinedBaseIdx }); // store base resource into the list

            m_unifiedForceAlias.push_back(aliasVec);
            idx = m_unifiedForceAlias.size() - 1;
        }

        for (uint16_t ii = 0; ii < info.aliasNum; ++ii)
        {
            uint16_t resId = *((uint16_t*)mem + ii);
            UnifiedResHandle combinedId{resId, _type};
            push_back_unique(m_unifiedForceAlias[idx], combinedId);

            m_forceAliasMapToBase.insert({ combinedId, combinedBaseIdx });
        }

        free(m_pAllocator, mem);
    }

    bool Framegraph::isForceAliased(const UnifiedResHandle& _res_0, const UnifiedResHandle _res_1) const
    {
        KG_ZoneScopedC(Color::light_yellow);
        const UnifiedResHandle base_0 = m_forceAliasMapToBase.find(_res_0)->second;
        const UnifiedResHandle base_1 = m_forceAliasMapToBase.find(_res_1)->second;

        return base_0 == base_1;
    }

    void Framegraph::buildGraph()
    {
        KG_ZoneScopedC(Color::light_yellow);
        stl::unordered_map<UnifiedResHandle, uint16_t> linear_writeResPassIdxMap;

        stl::unordered_set<UnifiedResHandle> writeOpInSetO{};
        for (uint16_t ii = 0; ii < m_hPass.size(); ++ii)
        {
            const PassRWResource rwRes = m_pass_rw_res[ii];
            stl::unordered_set<UnifiedResHandle> writeOpInSet{};

            size_t aliasMapNum = rwRes.writeOpForcedAliasMap.size();
            for (uint32_t jj = 0; jj < aliasMapNum; ++jj)
            {
                UnifiedResHandle writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(jj);
                UnifiedResHandle writeOpOut = rwRes.writeOpForcedAliasMap.getDataAt(jj);

                writeOpInSet.insert(writeOpIn);

                linear_writeResPassIdxMap.insert({ writeOpOut , ii });
            }

            for (const UnifiedResHandle combinedRes : rwRes.writeUnifiedRes)
            {
                if(writeOpInSet.find(combinedRes) == writeOpInSet.end()){
                    message(error, "what? there is res written but not added to writeOpInSet? check the vkz part!");
                }
            }
        }

        for (const auto& resPassPair : linear_writeResPassIdxMap)
        {
            UnifiedResHandle cid = resPassPair.first;
            uint16_t passIdx = resPassPair.second;

            if (cid.rawId == m_presentImage.id)
            {
                m_finalPass = m_hPass[passIdx];
                break;
            }
        }

        for (uint16_t ii = 0; ii < m_hPass.size(); ++ii)
        {
            const PassRWResource rwRes = m_pass_rw_res[ii];

            stl::unordered_set<UnifiedResHandle> resReadInPass{}; // includes two type of resources: 1. read in current pass. 2. resource alias before write.
            
            size_t aliasMapNum = rwRes.writeOpForcedAliasMap.size();
            
            // writeOpIn is not in the readCombinedRes, should affect the graph here
            for (uint32_t jj = 0; jj < aliasMapNum; ++jj)
            {
                const UnifiedResHandle writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(jj);

                resReadInPass.insert(writeOpIn);
            }

            for (const UnifiedResHandle plainRes : rwRes.readUnifiedRes)
            {
                resReadInPass.insert(plainRes);
            }

            for (const UnifiedResHandle resReadIn : resReadInPass)
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

    void Framegraph::calcPriority()
    {
        KG_ZoneScopedC(Color::light_yellow);
        uint16_t passNum = (uint16_t)m_hPass.size();

        for (uint16_t ii = 0; ii < passNum; ++ ii)
        {
            PassDependency passDep = m_pass_dependency[ii];
        }
    }

    void Framegraph::calcCombinations(stl::vector<stl::vector<uint16_t>>& _vecs, const stl::vector<uint16_t>& _inVec)
    {
        KG_ZoneScopedC(Color::light_yellow);
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

    uint16_t Framegraph::findCommonParent(const uint16_t _a, uint16_t _b, const stl::unordered_map<uint16_t, uint16_t>& _sortedParentMap, const uint16_t _root)
    {
        KG_ZoneScopedC(Color::light_yellow);
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

    uint16_t Framegraph::validateSortedPasses(const stl::vector<uint16_t>& _sortedPassIdxes, const stl::unordered_map<uint16_t, uint16_t>& _sortedParentMap)
    {
        KG_ZoneScopedC(Color::light_yellow);

        for (uint16_t idx : _sortedPassIdxes)
        {
            message(info, "sorted pass Idx: %d/%d, Id: %d", idx, _sortedPassIdxes.size(), m_hPass[idx].id);
        }

        for (const auto& p : _sortedParentMap)
        {
            message(info, "sorted Idx: %d/%d", p.first, p.second);
        }

        stl::unordered_map<UnifiedResHandle, uint16_t> writeResPassIdxMap;

        uint16_t order = 0;
        for (uint16_t passIdx : _sortedPassIdxes)
        {
            PassRWResource rwRes = m_pass_rw_res[passIdx];
            PassHandle pass = m_hPass[passIdx];

            // check if there any resource is read after a flip executed;
            // all flip point
            for (uint16_t ii = 0; ii < rwRes.writeOpForcedAliasMap.size(); ++ii)
            {
                UnifiedResHandle writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(ii);

                bx::StringView resName;
                if (writeOpIn.isImage())
                {
                    resName = getName(writeOpIn.img);
                }
                else if (writeOpIn.isBuffer())
                {
                    resName = getName(writeOpIn.buf);
                }
                else
                {
                    assert(0);
                }
                bx::StringView passName = getName(pass);

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

            for (const UnifiedResHandle& plainRes : rwRes.readUnifiedRes)
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

    void Framegraph::reverseTraversalDFSWithBackTrack()
    {
        KG_ZoneScopedC(Color::light_yellow);

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
                    message(info, "- sorted idx: %d", idx);
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

    void Framegraph::buildMaxLevelList(stl::vector<uint16_t>& _maxLvLst)
    {
        KG_ZoneScopedC(Color::light_yellow);

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

    void Framegraph::formatDependency(const stl::vector<uint16_t>& _maxLvLst)
    {
        KG_ZoneScopedC(Color::light_yellow);

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

                    lvToIdx.insert({ lvDist, uint16_t(size + 1) });
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

    void Framegraph::fillNearestSyncPass()
    {
        KG_ZoneScopedC(Color::light_yellow);

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

    void Framegraph::optimizeSyncPass()
    {
        KG_ZoneScopedC(Color::light_yellow);

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

    void Framegraph::postParse()
    {
        KG_ZoneScopedC(Color::light_yellow);

        stl::vector<UnifiedResHandle> plainAliasRes;
        stl::vector<uint16_t> plainAliasResIdx;

        for (uint16_t ii = 0; ii < m_unifiedForceAlias_base.size(); ++ii)
        {
            UnifiedResHandle base = m_unifiedForceAlias_base[ii];

            const stl::vector<UnifiedResHandle>& aliasVec = m_unifiedForceAlias[ii];

            plainAliasRes.insert(plainAliasRes.end(), aliasVec.begin(), aliasVec.end());

            for (UnifiedResHandle alias : aliasVec)
            {
                plainAliasResIdx.push_back(ii);
                m_plainResAliasToBase.addOrUpdate(alias, base);
            }
        }

        stl::vector<UnifiedResHandle> fullMultiFrameRes = m_multiFrame_resList;
        for (const UnifiedResHandle cid : m_multiFrame_resList)
        {
            const size_t idx = getElemIndex(plainAliasRes, cid);
            if (kInvalidHandle == idx)
            {
                continue;
            }

            const uint16_t alisIdx = plainAliasResIdx[idx];
            const stl::vector<UnifiedResHandle>& aliasVec = m_unifiedForceAlias[alisIdx];
            for (UnifiedResHandle alias : aliasVec)
            {
                push_back_unique(fullMultiFrameRes, alias);
            }
        }

        m_multiFrame_resList = fullMultiFrameRes;
    }

    void Framegraph::buildResLifetime()
    {
        KG_ZoneScopedC(Color::light_yellow);

        stl::vector<UnifiedResHandle> resInUseUniList;
        stl::vector<UnifiedResHandle> resToOptmUniList; // all used resources except: force alias, multi-frame, read-only
        stl::vector<UnifiedResHandle> readResUniList;
        stl::vector<UnifiedResHandle> writeResUniList;

        // add resources used by passes
        for (const uint16_t pIdx : m_sortedPassIdx)
        {
            PassRWResource rwRes = m_pass_rw_res[pIdx];

            for (const UnifiedResHandle uniRes : rwRes.writeUnifiedRes)
            {
                push_back_unique(writeResUniList, uniRes);
                push_back_unique(resToOptmUniList, uniRes);
                push_back_unique(resInUseUniList, uniRes);
            }

            for (const UnifiedResHandle uniRes : rwRes.readUnifiedRes)
            {
                push_back_unique(readResUniList, uniRes);
                push_back_unique(resToOptmUniList, uniRes);
                push_back_unique(resInUseUniList, uniRes);
            }

            size_t aliasMapNum = rwRes.writeOpForcedAliasMap.size();
            for (uint32_t ii = 0; ii < aliasMapNum; ++ii)
            {
                UnifiedResHandle writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(ii);
                UnifiedResHandle writeOpOut = rwRes.writeOpForcedAliasMap.getDataAt(ii);

                push_back_unique(writeResUniList, writeOpOut);
                push_back_unique(resToOptmUniList, writeOpOut);
                push_back_unique(resInUseUniList, writeOpOut);
            }

            // bindless resources
            for (const UnifiedResHandle uniRes : rwRes.bindlessRes)
            {
                push_back_unique(resInUseUniList, uniRes);
            }
        }

        stl::vector<stl::vector<uint16_t>> resToOptmPassIdxByOrder(resToOptmUniList.size(), stl::vector<uint16_t>()); // share the same idx with resToOptmUniList
        for(uint16_t ii = 0; ii < m_sortedPassIdx.size(); ++ii)
        {
            const uint16_t pIdx = m_sortedPassIdx[ii];
            PassRWResource rwRes = m_pass_rw_res[pIdx];

            for (const UnifiedResHandle uniRes : rwRes.writeUnifiedRes)
            {
                const size_t usedIdx = getElemIndex(resToOptmUniList, uniRes);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }

            for (const UnifiedResHandle uniRes : rwRes.readUnifiedRes)
            {
                const size_t usedIdx = getElemIndex(resToOptmUniList, uniRes);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }

            size_t aliasMapNum = rwRes.writeOpForcedAliasMap.size();
            for (uint32_t alisMapIdx = 0; alisMapIdx < aliasMapNum; ++alisMapIdx)
            {
                UnifiedResHandle writeOpOut = rwRes.writeOpForcedAliasMap.getDataAt(alisMapIdx);
                const size_t usedIdx = getElemIndex(resToOptmUniList, writeOpOut);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }
        }

        stl::vector<UnifiedResHandle> readonlyResUniList;
        for (const UnifiedResHandle readRes : readResUniList)
        {
            // not found in writeResUniList: no one write to it
            if (kInvalidIndex != getElemIndex(writeResUniList, readRes)) {
                continue;
            }

            // force alias
            // read-only has lower priority than force alias, if a resource is force aliased, it definitely will alias.
            // if not, then we consider to fill a individual bucket for it.
            if (m_plainResAliasToBase.exist(readRes)) {
                message(warning, "%s %d is force aliased but it's read-only, is this by intention", readRes.isImage() ? "image" :"buffer", readRes.rawId );
                continue;
            }

            // skip statics
            // leave it handle by the m_staticResources later, we don't need to consider it here.
            if (kInvalidIndex != getElemIndex(m_staticResources, readRes)) {
                continue;
            }

            readonlyResUniList.push_back(readRes);
        }


        stl::vector<UnifiedResHandle> multiframeResUniList;
        for (const UnifiedResHandle multiFrameRes : m_multiFrame_resList)
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
            const UnifiedResHandle plainAlias = m_plainResAliasToBase.getIdAt(ii);
            const size_t idx = getElemIndex(resToOptmUniList, plainAlias);
            if (kInvalidIndex == idx) {
                continue;
            }

            resToOptmUniList.erase(resToOptmUniList.begin() + idx);
            resToOptmPassIdxByOrder.erase(resToOptmPassIdxByOrder.begin() + idx);
        }

        // remove read-only resource from resToOptmUniList
        for (const UnifiedResHandle readonlyRes : readonlyResUniList)
        {
            const size_t idx = getElemIndex(resToOptmUniList, readonlyRes);
            if (kInvalidIndex == idx) {
                continue;
            }

            resToOptmUniList.erase(resToOptmUniList.begin() + idx);
            resToOptmPassIdxByOrder.erase(resToOptmPassIdxByOrder.begin() + idx);
        }

        // remove multi-frame resource from resToOptmUniList
        for (const UnifiedResHandle multiFrameRes : multiframeResUniList)
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

    void Framegraph::fillBucketStaticRes()
    {
        KG_ZoneScopedC(Color::light_yellow);

        for (const UnifiedResHandle uniRes : m_staticResources)
        {
            // buffers
            if (uniRes.isBuffer()) {
                const FGBufferCreateInfo info = m_sparse_buf_info[uniRes.buf];
                BufBucket bucket;
                createBufBkt(bucket, info, { 1, uniRes }, true);
                m_bufBuckets.push_back(bucket);
            }
            // images
            if (uniRes.isImage()) {
                const FGImageCreateInfo info = m_sparse_img_info[uniRes.img];
                ImgBucket bucket;
                createImgBkt(bucket, info, { 1, uniRes }, true);
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph::fillBucketForceAlias()
    {
        stl::vector<UnifiedResHandle> actualAliasBase;
        stl::vector<stl::vector<UnifiedResHandle>> actualAlias;
        for (const UnifiedResHandle uniRes : m_resInUseUniList)
        {
            if (!m_plainResAliasToBase.exist(uniRes))
            {
                continue;
            }

            UnifiedResHandle base2 = m_plainResAliasToBase.getIdToData(uniRes);

            const size_t usedBaseIdx = push_back_unique(actualAliasBase, base2);
            if (actualAlias.size() == usedBaseIdx) // if it's new one
            {
                actualAlias.emplace_back();
            }

            push_back_unique(actualAlias[usedBaseIdx], uniRes);
        }

        // process force alias
        for (uint16_t ii = 0; ii < actualAliasBase.size(); ++ii)
        {
            const UnifiedResHandle base = actualAliasBase[ii];
            const stl::vector<UnifiedResHandle>& aliasVec = actualAlias[ii];

            // buffers
            if (base.isBuffer()) 
            {
                const FGBufferCreateInfo info = m_sparse_buf_info[base.buf];

                BufBucket bucket;
                createBufBkt(bucket, info, aliasVec, true);
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (base.isImage())
            {
                const FGImageCreateInfo info = m_sparse_img_info[base.img];

                ImgBucket bucket;
                createImgBkt(bucket, info, aliasVec, true);
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph::fillBucketReadonly()
    {
        KG_ZoneScopedC(Color::light_yellow);

        for (const UnifiedResHandle cid : m_resInUseReadonlyList)
        {
            // buffers
            if (cid.isBuffer())
            {
                const FGBufferCreateInfo info = m_sparse_buf_info[cid.buf];

                BufBucket bucket;
                createBufBkt(bucket, info, { 1, cid });
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (cid.isImage())
            {
                const FGImageCreateInfo info = m_sparse_img_info[cid.img];

                ImgBucket bucket;
                createImgBkt(bucket, info, {1, cid});
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph::fillBucketMultiFrame()
    {
        KG_ZoneScopedC(Color::light_yellow);

        for (const UnifiedResHandle cid : m_resInUseMultiframeList)
        {
            // buffers
            if (cid.isBuffer())
            {
                const FGBufferCreateInfo info = m_sparse_buf_info[cid.buf];

                BufBucket bucket;
                createBufBkt(bucket, info, { 1, cid });
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (cid.isImage())
            {
                const FGImageCreateInfo info = m_sparse_img_info[cid.img];

                ImgBucket bucket;
                createImgBkt(bucket, info, { 1, cid });
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph::createBufBkt(BufBucket& _bkt, const FGBufferCreateInfo& _info, const stl::vector<UnifiedResHandle>& _reses, const bool _forceAliased /*= false*/)
    {
        KG_ZoneScopedC(Color::light_yellow);

        assert(!_reses.empty());

        _bkt.desc.size = _info.size;
        _bkt.pData = _info.pData;

        _bkt.desc.memFlags = _info.memFlags;
        _bkt.desc.usage = _info.usage;
        _bkt.desc.format = _info.format;

        _bkt.initialBarrierState = _info.initialState;
        _bkt.base_hbuf = _info.hBuf;
        _bkt.reses = _reses;
        _bkt.forceAliased = _forceAliased;
    }

    void Framegraph::createImgBkt(ImgBucket& _bkt, const FGImageCreateInfo& _info, const stl::vector<UnifiedResHandle>& _reses, const bool _forceAliased /*= false*/)
    {
        KG_ZoneScopedC(Color::light_yellow);

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

        _bkt.aspectFlags = _info.aspectFlags;
        _bkt.initialBarrierState = _info.initialState;
        _bkt.basse_himg = _info.hImg;
        _bkt.reses = _reses;
        _bkt.forceAliased = _forceAliased;
    }

    void Framegraph::aliasBuffers(stl::vector<BufBucket>& _buckets, const stl::vector<BufferHandle>& _sortedBufList)
    {
        KG_ZoneScopedC(Color::light_yellow);

        stl::vector<BufferHandle> restRes = _sortedBufList;

        stl::vector<stl::vector<UnifiedResHandle>> resInLevel{{}};

        if (restRes.empty()) {
            return;
        }

        // now process normal alias resources
        stl::vector<BufBucket> buckets;
        while (!restRes.empty())
        {
            BufferHandle baseRes = { *restRes.begin() };
            const UnifiedResHandle baseCid{ baseRes};

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

                for (UnifiedResHandle res : bkt.reses)
                {
                    const size_t idx = getElemIndex(restRes, res.buf);
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

        _buckets.insert(_buckets.end(), buckets.begin(), buckets.end());
    }

    void Framegraph::aliasImages(stl::vector<ImgBucket>& _buckets,const stl::vector< FGImageCreateInfo >& _infos, const stl::vector<ImageHandle>& _sortedTexList, const ResourceType _type)
    {
        KG_ZoneScopedC(Color::light_yellow);

        stl::vector<ImageHandle> restRes = _sortedTexList;

        stl::vector<stl::vector<UnifiedResHandle>> resInLevel{{}};

        if (restRes.empty()) {
            return;
        }

        // now process normal alias resources
        stl::vector<ImgBucket> buckets;
        while (!restRes.empty())
        {
            ImageHandle baseRes = { *restRes.begin() };
            const UnifiedResHandle baseCid{ baseRes };

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

                for (UnifiedResHandle res : bkt.reses)
                {
                    const size_t idx = getElemIndex(restRes, res.img);
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

    void Framegraph::fillBufferBuckets()
    {
        KG_ZoneScopedC(Color::light_yellow);

        // sort the resource by size
        stl::vector<BufferHandle> sortedBufIdx;
        for (const UnifiedResHandle& crid : m_resToOptmUniList)
        {
            if (! crid.isBuffer()) {
                continue;
            }
            sortedBufIdx.push_back(crid.buf);
        }

        std::sort(sortedBufIdx.begin(), sortedBufIdx.end(), [&](BufferHandle _l, BufferHandle _r) {
            return m_sparse_buf_info[_l].size > m_sparse_buf_info[_r].size;
        });

        stl::vector<BufBucket> buckets;
        aliasBuffers(buckets,sortedBufIdx);

        m_bufBuckets.insert(m_bufBuckets.end(), buckets.begin(), buckets.end());
    }

    void Framegraph::fillImageBuckets()
    {
        KG_ZoneScopedC(Color::light_yellow);

        const ResourceType type = ResourceType::image;

        stl::vector<ImageHandle> sortedTexIdx;
        for (const UnifiedResHandle& crid : m_resToOptmUniList)
        {
            if ( ! crid.isImage()) {
                continue;
            }
            sortedTexIdx.push_back(crid.img);
        }

        std::sort(sortedTexIdx.begin(), sortedTexIdx.end(), [&](ImageHandle _l, ImageHandle _r) {
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

    void Framegraph::optimizeSync()
    {
        KG_ZoneScopedC(Color::light_yellow);

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

    void Framegraph::optimizeAlias()
    {
        KG_ZoneScopedC(Color::light_yellow);

        buildResLifetime();

        // static resources ======================
        // for those resources will handle by external libs(e.g. ffx brixelizer)
        fillBucketStaticRes();

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

    void Framegraph::createBuffers()
    {
        KG_ZoneScopedC(Color::light_yellow);

        for (const BufBucket& bkt : m_bufBuckets)
        {
            RHIContextOpMagic magic{ RHIContextOpMagic::create_buffer };

            bx::write(&m_rhiMemWriter, magic, nullptr);

            BufferCreateInfo info;
            info.hbuf = bkt.base_hbuf;
            info.size = bkt.desc.size;
            info.format = bkt.desc.format;
            info.fillVal = bkt.desc.fillVal;
            info.memFlags = bkt.desc.memFlags;
            info.usage = bkt.desc.usage;
            info.pData = bkt.pData;
            info.resCount = (uint16_t)bkt.reses.size();


            info.barrierState = bkt.initialBarrierState;

            bx::write(&m_rhiMemWriter, info, nullptr);

            if (info.resCount > 1)
            {
                assert(info.pData == nullptr);
            }

            stl::vector<BufferAliasInfo> aliasInfo;
            for (const UnifiedResHandle cid : bkt.reses)
            {
                BufferAliasInfo alias;
                alias.hbuf = cid.buf;
                alias.size = m_sparse_buf_info[cid.buf].size;
                aliasInfo.push_back(alias);
            }

            bx::write(&m_rhiMemWriter, (void*)aliasInfo.data(), int32_t(sizeof(BufferAliasInfo) * bkt.reses.size()), nullptr);

            bx::write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end, nullptr);
        }
    }

    void Framegraph::createImages()
    {
        KG_ZoneScopedC(Color::light_yellow);

        for (const ImgBucket& bkt : m_imgBuckets)
        {
            RHIContextOpMagic magic{ RHIContextOpMagic::create_image };

            bx::write(&m_rhiMemWriter, magic, nullptr);

            ImageCreateInfo info;
            info.himg = bkt.basse_himg;
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

            bx::write(&m_rhiMemWriter, info, nullptr);

            stl::vector<ImageAliasInfo> aliasInfo;
            for (const UnifiedResHandle cid : bkt.reses)
            {
                ImageAliasInfo alias;
                alias.himg = cid.img;
                aliasInfo.push_back(alias);
            }

            bx::write(&m_rhiMemWriter, (void*)aliasInfo.data(), int32_t(sizeof(ImageAliasInfo) * bkt.reses.size()), nullptr);

            bx::write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end, nullptr);
        }
    }

    void Framegraph::createShaders()
    {
        KG_ZoneScopedC(Color::light_yellow);

        stl::vector<ProgramHandle> usedProgram{};
        stl::vector<ShaderHandle> usedShaders{};
        stl::unordered_map<BindlessHandle, stl::unordered_set<ShaderHandle>> bindlessToShaders{};

        for ( PassHandle pass : m_sortedPass)
        {
            const PassMetaData& passMeta = m_sparse_pass_meta[pass.id];

            if (passMeta.queue == PassExeQueue::extern_abstract)
                continue;

            const ProgramInfo& progInfo = m_sparse_program_info[passMeta.programId];
            usedProgram.push_back({ passMeta.programId });

            for (uint16_t ii = 0; ii < progInfo.createInfo.shaderNum; ++ii)
            {
                const uint16_t shaderId = progInfo.shaderIds[ii];
                usedShaders.push_back({ shaderId });
            }

            if (progInfo.createInfo.bindlessId != kInvalidHandle)
            {
                BindlessHandle bindless = { progInfo.createInfo.bindlessId };

                for (uint16_t ii = 0; ii < progInfo.createInfo.shaderNum; ++ii)
                {
                    const uint16_t shaderId = progInfo.shaderIds[ii];
                    bindlessToShaders[bindless].insert({ shaderId });
                }
            }
        }

        // write shader info
        for (ShaderHandle shader : usedShaders)
        {
            const ShaderInfo& info = m_sparse_shader_info[shader.id];
            assert(shader.id == info.createInfo.shaderId);

            String& path = m_shader_path[info.pathIdx];
            
            ShaderCreateInfo createInfo;
            createInfo.shaderId = info.createInfo.shaderId;
            createInfo.pathLen = (uint16_t)path.getLength();

            RHIContextOpMagic magic{ RHIContextOpMagic::create_shader };
            bx::write(&m_rhiMemWriter, magic, nullptr);

            bx::write(&m_rhiMemWriter, createInfo, nullptr);

            bx::write(&m_rhiMemWriter, (void*)path.getCPtr(), (int32_t)path.getLength(), nullptr);

            bx::write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end, nullptr);
        }

        // write bindless info
        for ( const stl::unordered_hash_node<BindlessHandle, stl::unordered_set<ShaderHandle>>& bltoSh 
            : bindlessToShaders)
        {
            BindlessHandle bindlessId = bltoSh.first;
            stl::unordered_set<ShaderHandle> shaders = bltoSh.second;

            const BindlessMetaData& meta = m_sparse_bindless_meta[bindlessId.id];
            RHIContextOpMagic magic{ RHIContextOpMagic::create_bindless };
            bx::write(&m_rhiMemWriter, magic, nullptr);
            bx::write(&m_rhiMemWriter, meta, nullptr);

            stl::vector<uint16_t> shaderIds;
            for (ShaderHandle shader : shaders)
            {
                shaderIds.push_back(shader.id);
            }
            uint32_t shaderNum = (uint32_t)shaderIds.size();
            bx::write(&m_rhiMemWriter, shaderNum, nullptr);
            bx::write(&m_rhiMemWriter, (void*)shaderIds.data(), (int32_t)(shaderIds.size() * sizeof(uint16_t)), nullptr);

            bx::write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end, nullptr);
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
            createInfo.bindlessId = info.createInfo.bindlessId;

            RHIContextOpMagic magic{ RHIContextOpMagic::create_program };

            bx::write(&m_rhiMemWriter, magic, nullptr);

            bx::write(&m_rhiMemWriter, createInfo, nullptr);

            bx::write(&m_rhiMemWriter, (void*)info.shaderIds, (int32_t)(info.createInfo.shaderNum * sizeof(uint16_t)), nullptr);

            bx::write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end, nullptr);
        }
    }

    void Framegraph::createSamplers()
    {
        KG_ZoneScopedC(Color::light_yellow);

        // all samplers will pass to RHI level, then a sampler cache will be use there.
        for (SamplerHandle hSamp : m_hSampler)
        {
            const SamplerMetaData& meta = m_sparse_sampler_meta[hSamp.id];

            RHIContextOpMagic magic{ RHIContextOpMagic::create_sampler };

            bx::write(&m_rhiMemWriter, magic, nullptr);
            bx::write(&m_rhiMemWriter, meta, nullptr);
            bx::write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end, nullptr);
        }
    }

    void Framegraph::createPasses()
    {
        KG_ZoneScopedC(Color::light_yellow);

        stl::vector<PassMetaData> passMetaDataVec{};
        stl::vector<stl::vector<VertexBindingDesc>> passVertexBinding(m_sortedPass.size());
        stl::vector<stl::vector<VertexAttributeDesc>> passVertexAttribute(m_sortedPass.size());
        stl::vector<stl::vector<int>> passPipelineSpecData(m_sortedPass.size());

        stl::vector<stl::pair<ImageHandle, ResInteractDesc>> writeDSPair(m_sortedPass.size());
        stl::vector<ContinuousMap< ImageHandle, ResInteractDesc> > writeImageVec(m_sortedPass.size());
        stl::vector<ContinuousMap< ImageHandle, ResInteractDesc> > readImageVec(m_sortedPass.size());

        stl::vector<ContinuousMap< BufferHandle, ResInteractDesc> > readBufferVec(m_sortedPass.size());
        stl::vector<ContinuousMap< BufferHandle, ResInteractDesc> > writeBufferVec(m_sortedPass.size());
        stl::vector< ContinuousMap<UnifiedResHandle, UnifiedResHandle> >  writeOpAliasMapVec(m_sortedPass.size());

        
        for (uint32_t ii = 0; ii < m_sortedPass.size(); ++ii)
        {
            PassHandle pass = m_sortedPass[ii];
            const size_t passIdx = getElemIndex(m_hPass, pass);
            const PassMetaData& passMeta = m_sparse_pass_meta[pass.id];
            assert(pass.id == passMeta.passId);

            stl::pair<ImageHandle, ResInteractDesc>& writeDS = writeDSPair[ii];
            ContinuousMap< ImageHandle, ResInteractDesc>& writeColor = writeImageVec[ii];
            ContinuousMap< ImageHandle, ResInteractDesc>& readImg = readImageVec[ii];

            ContinuousMap< BufferHandle, ResInteractDesc>& readBuf = readBufferVec[ii];
            ContinuousMap< BufferHandle, ResInteractDesc>& writeBuf = writeBufferVec[ii];

            ContinuousMap<UnifiedResHandle, UnifiedResHandle>& writeOpAliasMap = writeOpAliasMapVec[ii];
            {
                writeDS.first = { kInvalidHandle };
                writeColor.clear();
                readImg.clear();
                readBuf.clear();
                writeBuf.clear();
                writeOpAliasMap.clear();

                const PassRWResource& rwRes = m_pass_rw_res[passIdx];
                // set write resources
                for (const UnifiedResHandle writeRes : rwRes.writeUnifiedRes)
                {
                    auto writeInteractPair = rwRes.writeInteractMap.find(writeRes);
                    assert(writeInteractPair != rwRes.writeInteractMap.end());


                    if (writeRes.isImage())
                    {
                        if (isDepthStencil(writeRes))
                        {
                            assert(writeDS.first.id == kInvalidHandle);
                            writeDS.first = { writeRes.img };
                            writeDS.second = writeInteractPair->second;
                        }

                        writeColor.addOrUpdate({ writeRes.img }, writeInteractPair->second);

                    }
                    else if (writeRes.isBuffer())
                    {
                        writeBuf.addOrUpdate({ writeRes.buf }, writeInteractPair->second);
                    }
                    else
                    {
                        kage::message(DebugMsgType::error, "invalid write resource mapping: Res %4d, PassExeQueue: %d\n", writeRes.buf, passMeta.queue);
                    }
                }

                // set read resources
                for (const UnifiedResHandle readRes : rwRes.readUnifiedRes)
                {
                    auto readInteractPair = rwRes.readInteractMap.find(readRes);
                    assert(readInteractPair != rwRes.readInteractMap.end());

                    if (readRes.isImage())
                    {
                        readImg.addOrUpdate({ readRes.img}, readInteractPair->second);
                    }
                    else if (readRes.isBuffer())
                    {
                        assert(!readBuf.exist({ readRes.buf }));
                        readBuf.addOrUpdate({ readRes.buf }, readInteractPair->second);
                    }
                }

                // write operation out aliases
                uint32_t aliasMapNum = (uint32_t)rwRes.writeOpForcedAliasMap.size();
                for (uint32_t ii = 0; ii < aliasMapNum; ++ii)
                {
                    const UnifiedResHandle writeOpIn = rwRes.writeOpForcedAliasMap.getIdAt(ii);
                    const UnifiedResHandle writeOpOut = rwRes.writeOpForcedAliasMap.getDataAt(ii);

                    writeOpAliasMap.addOrUpdate(writeOpIn, writeOpOut);
                }
            }

            // create pass info
            assert(writeDS.first.id == passMeta.writeDepthId);
            assert((uint16_t)writeColor.size() == passMeta.writeImageNum);
            assert((uint16_t)readImg.size() == passMeta.readImageNum);
            assert((uint16_t)readBuf.size() == passMeta.readBufferNum);
            assert((uint16_t)writeBuf.size() == passMeta.writeBufferNum);
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

            bx::write(&m_rhiMemWriter, magic, nullptr);

            bx::write(&m_rhiMemWriter, createInfo, nullptr);

            // vertex binding
            bx::write(&m_rhiMemWriter, (void*)passVertexBinding[ii].data(), (int32_t)(createInfo.vertexBindingNum * sizeof(VertexBindingDesc)), nullptr);

            // vertex attribute
            bx::write(&m_rhiMemWriter, (void*)passVertexAttribute[ii].data(), (int32_t)(createInfo.vertexAttributeNum * sizeof(VertexAttributeDesc)), nullptr);

            // push constants
            bx::write(&m_rhiMemWriter, (void*)passPipelineSpecData[ii].data(), (int32_t)(createInfo.pipelineSpecNum * sizeof(int)), nullptr);

            // pass read/write resources and it's interaction
            bx::write(&m_rhiMemWriter, (void*)writeImageVec[ii].getIdPtr(), (int32_t)(createInfo.writeImageNum * sizeof(ImageHandle)), nullptr);
            bx::write(&m_rhiMemWriter, (void*)readImageVec[ii].getIdPtr(), (int32_t)(createInfo.readImageNum * sizeof(ImageHandle)), nullptr);
            bx::write(&m_rhiMemWriter, (void*)readBufferVec[ii].getIdPtr(), (int32_t)(createInfo.readBufferNum * sizeof(BufferHandle)), nullptr);
            bx::write(&m_rhiMemWriter, (void*)writeBufferVec[ii].getIdPtr(), (int32_t)(createInfo.writeBufferNum * sizeof(BufferHandle)), nullptr);

            //write(&m_rhiMemWriter, writeDSPair[ii].second);
            bx::write(&m_rhiMemWriter, (void*)writeImageVec[ii].getDataPtr(), (int32_t)(createInfo.writeImageNum * sizeof(ResInteractDesc)), nullptr);
            bx::write(&m_rhiMemWriter, (void*)readImageVec[ii].getDataPtr(), (int32_t)(createInfo.readImageNum * sizeof(ResInteractDesc)), nullptr);
            bx::write(&m_rhiMemWriter, (void*)readBufferVec[ii].getDataPtr(), (int32_t)(createInfo.readBufferNum * sizeof(ResInteractDesc)), nullptr);
            bx::write(&m_rhiMemWriter, (void*)writeBufferVec[ii].getDataPtr(), (int32_t)(createInfo.writeBufferNum * sizeof(ResInteractDesc)), nullptr);

            // write op alias
            bx::write(&m_rhiMemWriter, (void*)writeOpAliasMapVec[ii].getIdPtr(), (int32_t)(createInfo.writeBufAliasNum + createInfo.writeImgAliasNum) * sizeof(UnifiedResHandle), nullptr);
            bx::write(&m_rhiMemWriter, (void*)writeOpAliasMapVec[ii].getDataPtr(), (int32_t)(createInfo.writeBufAliasNum + createInfo.writeImgAliasNum) * sizeof(UnifiedResHandle), nullptr);

            bx::write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end, nullptr);
        }
    }

    void Framegraph::createResources()
    {
        KG_ZoneScopedC(Color::light_yellow);

        createBuffers();
        createImages();
        createShaders();
        createSamplers();
        createPasses();

        // set brief
        {
            bx::write(&m_rhiMemWriter, RHIContextOpMagic::set_brief, nullptr);
            RHIBrief brief;
            brief.finalPass = m_finalPass;
            brief.presentImage = m_presentImage;
            brief.presentMipLevel = m_presentMipLevel;

            bx::write(&m_rhiMemWriter, brief, nullptr);
            bx::write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end, nullptr);
        }

        // end mem tag
        bx::write(&m_rhiMemWriter,  RHIContextOpMagic::end , nullptr);
    }

    bool Framegraph::isBufInfoAliasable(BufferHandle _hbuf, const BufBucket& _bucket, const stl::vector<UnifiedResHandle> _resInCurrStack) const
    {
        KG_ZoneScopedC(Color::light_yellow);

        // size check in current stack
        const FGBufferCreateInfo& info = m_sparse_buf_info[_hbuf];

        bool bCondMatch = (info.size <= _bucket.desc.size);

        for (const UnifiedResHandle res : _resInCurrStack)
        {
            const FGBufferCreateInfo& stackInfo = m_sparse_buf_info[res.buf];

            bCondMatch &= (info.pData == nullptr && stackInfo.pData == nullptr);
            bCondMatch &= (info.memFlags == stackInfo.memFlags);
            bCondMatch &= (info.usage == stackInfo.usage);
        }

        return bCondMatch;
    }

    bool Framegraph::isImgInfoAliasable(ImageHandle _himg, const ImgBucket& _bucket, const stl::vector<UnifiedResHandle> _resInCurrStack) const
    {
        KG_ZoneScopedC(Color::light_yellow);

        bool bCondMatch = true;

        // condition check
        const FGImageCreateInfo& info = m_sparse_img_info[_himg];
        for (const UnifiedResHandle res : _resInCurrStack)
        {
            const FGImageCreateInfo& stackInfo = m_sparse_img_info[res.img];

            bCondMatch &= (info.numMips == stackInfo.numMips);
            bCondMatch &= (info.width == stackInfo.width && info.height == stackInfo.height && info.depth == stackInfo.depth);
            bCondMatch &= (info.numLayers == stackInfo.numLayers);
            bCondMatch &= (info.format == stackInfo.format);

            bCondMatch &= (info.usage == stackInfo.usage);
            bCondMatch &= (info.type == stackInfo.type);
        }
        
        return bCondMatch;
    }

    bool Framegraph::isStackAliasable(const UnifiedResHandle& _res, const stl::vector<UnifiedResHandle>& _reses) const
    {
        KG_ZoneScopedC(Color::light_yellow);

        bool bStackMatch = true;
        
        // life time check in entire bucket
        const size_t idx = getElemIndex(m_resToOptmUniList, _res);
        const ResLifetime& resLifetime = m_resLifeTime[idx];
        for (const UnifiedResHandle resInStack : _reses)
        {
            const size_t resIdx = getElemIndex(m_resToOptmUniList, resInStack);
            const ResLifetime& stackLifetime = m_resLifeTime[resIdx];

            // if lifetime overlap with any resource in current bucket, then it's not alias-able
            bStackMatch &= (stackLifetime.startIdx > resLifetime.endIdx || stackLifetime.endIdx < resLifetime.startIdx);
        }

        return bStackMatch;
    }

    bool Framegraph::isAliasable(const UnifiedResHandle& _res, const BufBucket& _bucket, const stl::vector<UnifiedResHandle>& _resInCurrStack) const
    {
        KG_ZoneScopedC(Color::light_yellow);

        // no stack checking for stacks, because each stack only contains 1 resource
        // bool bInfoMatch = isBufInfoAliasable(_res.id, _bucket, _resInCurrStack);
        
        bool bInfoMatch = isBufInfoAliasable(_res.buf, _bucket, _bucket.reses);
        bool bStackMatch = isStackAliasable(_res, _bucket.reses);

        return bInfoMatch && bStackMatch;
    }

    bool Framegraph::isAliasable(const UnifiedResHandle& _res, const ImgBucket& _bucket, const stl::vector<UnifiedResHandle>& _resInCurrStack) const
    {
        KG_ZoneScopedC(Color::light_yellow);

        // no stack checking for stacks, because each stack only contains 1 resource
        // bool bInfoMatch = isImgInfoAliasable(_res.id, _bucket, _resInCurrStack);

        bool bInfoMatch = isImgInfoAliasable(_res.img, _bucket, _bucket.reses);
        bool bStackMatch = isStackAliasable(_res, _bucket.reses);

        return bInfoMatch && bStackMatch;
    }

} // namespace kage
