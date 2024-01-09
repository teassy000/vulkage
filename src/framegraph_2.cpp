#include "common.h"
#include "macro.h"
#include "config.h"
#include "util.h"

#include "memory_operation.h"
#include "handle.h"
#include "vkz.h"
#include "framegraph_2.h"
#include <stack>
#include <algorithm>
#include "rhi_context.h"



namespace vkz
{

    Framegraph2::~Framegraph2()
    {
        deleteObject(m_pAllocator, m_pMemBlock);
    }

    void Framegraph2::bake()
    {
        // prepare
        parseOp();

        postParse();

        buildGraph();

        // sort and cut
        reverseTraversalDFS();

        // optimize
        optimizeSync();
        optimizeAlias();

        // actual create resources for renderer
        createResources();
    }

    void Framegraph2::parseOp()
    {
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
        FrameGraphBrief brief;
        read(&_reader, brief);

        // info data will use idx from handle as iterator
        m_sparse_shader_info.resize(brief.shaderNum);
        m_sparse_program_info.resize(brief.programNum);
        m_sparse_pass_meta.resize(brief.passNum);
        m_sparse_buf_info.resize(brief.bufNum);
        m_sparse_img_info.resize(brief.imgNum);

        m_sparse_pass_data_ref.resize(brief.passNum);

        m_combinedPresentImage = CombinedResID{ brief.presentImage, ResourceType::image };
    }


    void Framegraph2::registerShader(MemoryReader& _reader)
    {
        ShaderRegisterInfo info;
        read(&_reader, info);

        m_hShader.push_back({ info.shaderId });
        
        char path[kMaxPathLen];
        read(&_reader, (void*)(path), info.strLen);
        path[info.strLen] = '\0'; // null-terminated string

        m_shader_path.emplace_back(path);
        m_sparse_shader_info[info.shaderId].regInfo = info;
        m_sparse_shader_info[info.shaderId].pathIdx = (uint16_t)m_shader_path.size() - 1;
    }

    void Framegraph2::registerProgram(MemoryReader& _reader)
    {
        ProgramRegisterInfo regInfo;
        read(&_reader, regInfo);

        assert(regInfo.shaderNum <= kMaxNumOfStageInPorgram);

        ProgramInfo& progInfo = m_sparse_program_info[regInfo.progId];
        progInfo.regInfo = regInfo;

        read(&_reader, (void*)(progInfo.shaderIds), sizeof(uint16_t) * regInfo.shaderNum);

        m_hProgram.push_back({ regInfo.progId });
    }

    void Framegraph2::registerPass(MemoryReader& _reader)
    {
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
        std::vector<PassResInteract> writeImgVec(passMeta.writeImageNum);
        read(&_reader, writeImgVec.data(), sizeof(PassResInteract) * passMeta.writeImageNum);
        passMeta.writeImageNum = writeResource(writeImgVec, passMeta.passId, ResourceType::image);

        // read image
        std::vector<PassResInteract> readImgVec(passMeta.readImageNum);
        read(&_reader, readImgVec.data(), sizeof(PassResInteract) * passMeta.readImageNum);
        passMeta.readImageNum = readResource(readImgVec, passMeta.passId, ResourceType::image);

        // write buffer
        std::vector<PassResInteract> writeBufVec(passMeta.writeBufferNum);
        read(&_reader, writeBufVec.data(), sizeof(PassResInteract) * passMeta.writeBufferNum);
        passMeta.writeBufferNum = writeResource(writeBufVec, passMeta.passId, ResourceType::buffer);

        // read buffer
        std::vector<PassResInteract> readBufVec(passMeta.readBufferNum);
        read(&_reader, readBufVec.data(), sizeof(PassResInteract) * passMeta.readBufferNum);
        passMeta.readBufferNum = readResource(readBufVec, passMeta.passId, ResourceType::buffer);

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
        BufRegisterInfo info;
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
        ImgRegisterInfo info;
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



    const ResInteractDesc merge(const ResInteractDesc& _desc0, const ResInteractDesc& _desc1)
    {
        ResInteractDesc desc = _desc0;

        desc.access |= _desc1.access;
        desc.stage |= _desc1.stage;

        return std::move(desc);
    }

    bool isValidBinding(uint32_t _binding)
    {
        return _binding < kMaxDescriptorSetNum;
    }

    const ResInteractDesc mergeIfNoConflict(const ResInteractDesc& _desc0, const ResInteractDesc& _desc1)
    {
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

    uint32_t Framegraph2::readResource(std::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type)
    {
        uint16_t hPassIdx = getElemIndex(m_hPass, { _passId });
        assert(hPassIdx != kInvalidIndex);

        uint32_t actualSize = 0;
        for (const PassResInteract& prInteract: _resVec)
        {
            uint16_t passId = prInteract.passId;
            assert(passId == _passId);

            uint16_t resId = prInteract.resId;
            ResInteractDesc interact = prInteract.interact;

            CombinedResID plainId{ resId, _type };

            if (m_pass_rw_res[hPassIdx].readInteracts.exist(plainId))
            {
                const ResInteractDesc& orig = m_pass_rw_res[hPassIdx].readInteracts.getIdToData(plainId);

                interact = mergeIfNoConflict(orig, interact);
                m_pass_rw_res[hPassIdx].readInteracts.update_data(plainId, interact);
            }
            else
            {
                m_pass_rw_res[hPassIdx].readInteracts.push_back(plainId, interact);
                m_pass_rw_res[hPassIdx].readCombinedRes.push_back(plainId);
                actualSize++;
            }
        }

        // make sure vec elements are unique
        std::vector<CombinedResID>& readVecRef = m_pass_rw_res[hPassIdx].readCombinedRes;
        std::sort(readVecRef.begin(), readVecRef.end());
        readVecRef.erase(std::unique(readVecRef.begin(), readVecRef.end()), readVecRef.end());

        assert(readVecRef.size() == m_pass_rw_res[hPassIdx].readInteracts.size());

        return actualSize;
    }

    uint32_t Framegraph2::writeResource(std::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type)
    {
        uint16_t hPassIdx = getElemIndex(m_hPass, { _passId });
        assert(hPassIdx != kInvalidIndex);

        uint32_t actualSize = 0;
        for (const PassResInteract& prInteract : _resVec)
        {
            uint16_t passId = prInteract.passId;
            assert(passId == _passId);

            uint16_t resId = prInteract.resId;
            ResInteractDesc interact = prInteract.interact;

            CombinedResID plainId{ resId, _type };

            if (m_pass_rw_res[hPassIdx].writeInteracts.exist(plainId))
            {
                const ResInteractDesc& orig = m_pass_rw_res[hPassIdx].writeInteracts.getIdToData(plainId);

                interact = mergeIfNoConflict(orig, interact);
                m_pass_rw_res[hPassIdx].writeInteracts.update_data(plainId, interact);
            }
            else
            {
                m_pass_rw_res[hPassIdx].writeInteracts.push_back(plainId, interact);

                m_pass_rw_res[hPassIdx].writeCombinedRes.push_back(plainId);

                actualSize++;
            }
        }

        // make sure vec elements are unique
        std::vector<CombinedResID>& writeVecRef = m_pass_rw_res[hPassIdx].writeCombinedRes;
        std::sort(writeVecRef.begin(), writeVecRef.end());
        writeVecRef.erase(std::unique(writeVecRef.begin(), writeVecRef.end()), writeVecRef.end());

        assert(writeVecRef.size() == m_pass_rw_res[hPassIdx].writeInteracts.size());

        return actualSize;
    }

    void Framegraph2::aliasResForce(MemoryReader& _reader, ResourceType _type)
    {
        ResAliasInfo info;
        read(&_reader, info);

        void* mem = alloc(m_pAllocator, info.aliasNum * sizeof(uint16_t));
        read(&_reader, mem, info.aliasNum * sizeof(uint16_t));

        CombinedResID combinedBaseIdx{ info.resBase, _type };
        uint16_t idx = getElemIndex(m_combinedForceAlias_base, combinedBaseIdx);
        if (kInvalidIndex == idx)
        {
            m_combinedForceAlias_base.push_back(combinedBaseIdx);
            std::vector<CombinedResID> aliasVec({ combinedBaseIdx }); // store base resource into the list

            m_combinedForceAlias.push_back(aliasVec);
            idx = (uint16_t)m_combinedForceAlias.size() - 1;
        }

        for (uint16_t ii = 0; ii < info.aliasNum; ++ii)
        {
            uint16_t resId = *((uint16_t*)mem + ii);
            CombinedResID combinedId{resId, _type};
            push_back_unique(m_combinedForceAlias[idx], combinedId);
        }

        free(m_pAllocator, mem, info.aliasNum * sizeof(uint16_t));
    }

    void Framegraph2::buildGraph()
    {
        // make a plain table that can find producer of a resource easily.
        std::vector<CombinedResID> linear_outResCID;
        std::vector<uint16_t> linear_outPassIdx;

        for (uint16_t ii = 0; ii < m_hPass.size(); ++ii)
        {
            PassHandle currPass = m_hPass[ii];
            PassRWResource rwRes = m_pass_rw_res[ii];

            for (const CombinedResID CombindRes : rwRes.writeCombinedRes)
            {
                linear_outResCID.push_back(CombindRes);
                linear_outPassIdx.push_back(ii); // write by current pass

                if (CombindRes == m_combinedPresentImage)
                {
                    m_finalPass = currPass;
                }
            }
        }

        for (uint16_t ii = 0; ii < m_hPass.size(); ++ii)
        {
            PassRWResource rwRes = m_pass_rw_res[ii];

            for (CombinedResID plainRes : rwRes.readCombinedRes)
            {
                uint16_t idx = getElemIndex(linear_outResCID, plainRes);
                if (kInvalidIndex == idx) {
                    continue;
                }

                uint16_t currPassIdx = ii;
                uint16_t writePassIdx = linear_outPassIdx[idx];

                m_pass_dependency[currPassIdx].inPassIdxSet.insert(writePassIdx);
                m_pass_dependency[writePassIdx].outPassIdxSet.insert(currPassIdx);
            }
        }
    }

    void Framegraph2::reverseTraversalDFS()
    {
        uint16_t passNum = (uint16_t)m_hPass.size();

        std::vector<bool> visited(passNum, false);
        std::vector<bool> onStack(passNum, false);

        uint16_t finalPassIdx = (uint16_t)getElemIndex(m_hPass, m_finalPass);
        assert(finalPassIdx != kInvalidIndex);

        std::vector<uint16_t> sortedPassIdx;
        std::stack<uint16_t> passIdxStack;

        // start with the final pass
        passIdxStack.push(finalPassIdx);

        while (!passIdxStack.empty())
        {
            uint32_t currPassIdx = passIdxStack.top();

            onStack[currPassIdx] = true;
            visited[currPassIdx] = true;

            PassDependency passDep = m_pass_dependency[currPassIdx];

            bool inPassAllVisited = true;

            for (uint16_t parentPassIdx : passDep.inPassIdxSet)
            {
                if (!visited[parentPassIdx])
                {
                    passIdxStack.push(parentPassIdx);
                    inPassAllVisited = false;
                }
                else if (onStack[parentPassIdx])
                {
                    message(error, "cycle detected!");
                    return;
                }
            }

            if (inPassAllVisited)
            {
                sortedPassIdx.push_back(currPassIdx);
                
                passIdxStack.pop();
                onStack[currPassIdx] = false;
            }
        }
        
        // fill the sorted and clipped pass
        for (uint16_t idx : sortedPassIdx)
        {
            m_sortedPass.push_back(m_hPass[idx]);
        }
        m_sortedPassIdx = sortedPassIdx;

    }

    void Framegraph2::buildMaxLevelList(std::vector<uint16_t>& _maxLvLst)
    {
        for (uint16_t passIdx : m_sortedPassIdx)
        {
            uint16_t maxLv = 0;

            for (uint16_t inPassIdx : m_pass_dependency[passIdx].inPassIdxSet)
            {
                uint16_t dd = _maxLvLst[inPassIdx] + 1;
                maxLv = std::max(maxLv, dd);
            }

            _maxLvLst[passIdx] = maxLv;
        }
    }

    void Framegraph2::formatDependency(const std::vector<uint16_t>& _maxLvLst)
    {
        // initialize the dependency level
        uint16_t maxLv = *std::max_element(begin(_maxLvLst), end(_maxLvLst));
        m_passIdxInDpLevels = std::vector<PassInDependLevel>(m_sortedPassIdx.size(), PassInDependLevel{});
        for (uint16_t passIdx : m_sortedPassIdx)
        {
            m_passIdxInDpLevels[passIdx].passInLv = std::vector<std::vector<uint16_t>>(maxLv, std::vector<uint16_t>());
        }


        for (uint16_t passIdx : m_sortedPassIdx)
        {
            std::stack<uint16_t> passStack;
            passStack.push(passIdx);


            uint16_t baseLv = _maxLvLst[passIdx];

            while (!passStack.empty())
            {
                uint16_t currPassIdx = passStack.top();
                passStack.pop();

                const uint16_t currLv = _maxLvLst[currPassIdx];
                for (const uint16_t inPassIdx : m_pass_dependency[currPassIdx].inPassIdxSet)
                {
                    uint16_t idx = getElemIndex(m_passIdxInDpLevels[passIdx].passInLv[baseLv - currLv], inPassIdx);
                    if (currLv - 1 != _maxLvLst[inPassIdx] || kInvalidIndex != idx)
                    {
                        continue;
                    }

                    m_passIdxInDpLevels[passIdx].passInLv[baseLv - currLv].push_back(inPassIdx);
                }

                for (const uint16_t inPassIdx : m_pass_dependency[currPassIdx].inPassIdxSet)
                {
                    passStack.push(inPassIdx);
                }
            }
        }
    }

    void Framegraph2::fillNearestSyncPass()
    {
        // init nearest sync pass
        m_nearestSyncPassIdx.resize(m_sortedPass.size());
        for (auto& arr : m_nearestSyncPassIdx)
        {
            arr.fill(kInvalidIndex);
        }

        for (const uint16_t passIdx : m_sortedPassIdx)
        {
            const PassInDependLevel& passDep = m_passIdxInDpLevels[passIdx];

            // iterate each level
            for (const auto & passInCurrLv : passDep.passInLv)
            {
                // iterate each pass in current level
                for (uint16_t pIdx : passInCurrLv)
                {
                    // check if pass in m_passIdxInDLevels can match with m_passIdxInQueue
                    for (uint16_t qIdx = 0; qIdx < m_passIdxInQueue.size(); ++qIdx)
                    {
                        const uint16_t idx = getElemIndex(m_passIdxInQueue[qIdx], pIdx);

                        bool isMatch = 
                               (kInvalidIndex == m_nearestSyncPassIdx[passIdx][qIdx]) // not set yet
                            && (kInvalidIndex != idx); // found

                        // only first met would set the value
                        if ( isMatch) {
                            m_nearestSyncPassIdx[passIdx][qIdx] = pIdx;
                            break;
                        }
                    }
                }
            }
        }
        // cool nesting :)
    }

    void Framegraph2::optimizeSyncPass()
    {
        for (uint16_t pIdx : m_sortedPassIdx)
        {
            const PassInDependLevel& dpLv = m_passIdxInDpLevels[pIdx];
            
            
            if (dpLv.passInLv.empty() || dpLv.passInLv[0].empty())
                continue;

            // only level 0 would be sync
            m_passIdxToSync[pIdx].insert(m_passIdxToSync[pIdx].end(), dpLv.passInLv[0].begin(), dpLv.passInLv[0].end());
        }

        // TODO: 
        //      if m_passIdxToSync for one pass needs to sync with multiple queue
        // =========================
        //      **Fence** required
    }

    void Framegraph2::postParse()
    {
        std::vector<CombinedResID> plainAliasRes;
        std::vector<uint16_t> plainAliasResIdx;

        for (uint16_t ii = 0; ii < m_combinedForceAlias_base.size(); ++ii)
        {
            CombinedResID base = m_combinedForceAlias_base[ii];

            const std::vector<CombinedResID>& aliasVec = m_combinedForceAlias[ii];

            plainAliasRes.insert(end(plainAliasRes), begin(aliasVec), end(aliasVec));
            plainAliasResIdx.insert(end(plainAliasResIdx), aliasVec.size(), ii);

            for (CombinedResID alias : aliasVec)
            {
                m_plainResAliasToBase.push_back(alias, base);
            }
        }

        std::vector<CombinedResID> fullMultiFrameRes = m_multiFrame_resList;
        for (const CombinedResID cid : m_multiFrame_resList)
        {
            /* TODO: remove manually mapping to other vectors
            if (!m_plainResAliasToBase.isValidId(cid))
            {
                continue;
            }
            */
            const uint16_t idx = getElemIndex(plainAliasRes, cid);
            if (kInvalidHandle == idx)
            {
                continue;
            }

            const uint16_t alisIdx = plainAliasResIdx[idx];
            const std::vector<CombinedResID>& aliasVec = m_combinedForceAlias[alisIdx];
            for (CombinedResID alias : aliasVec)
            {
                push_back_unique(fullMultiFrameRes, alias);
            }
        }

        m_multiFrame_resList = fullMultiFrameRes;
    }

    void Framegraph2::buildResLifetime()
    {
        std::vector<CombinedResID> resInUseUniList;
        std::vector<CombinedResID> resToOptmUniList; // all used resources except: force alias, multi-frame, read-only
        std::vector<CombinedResID> readResUniList;
        std::vector<CombinedResID> writeResUniList;
        
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
        }

        std::vector<std::vector<uint16_t>> resToOptmPassIdxByOrder; // share the same idx with resToOptmUniList
        resToOptmPassIdxByOrder.assign(resToOptmUniList.size(), std::vector<uint16_t>());
        for(uint16_t ii = 0; ii < m_sortedPassIdx.size(); ++ii)
        {
            const uint16_t pIdx = m_sortedPassIdx[ii];
            PassRWResource rwRes = m_pass_rw_res[pIdx];

            for (const CombinedResID combRes : rwRes.writeCombinedRes)
            {
                uint16_t usedIdx = getElemIndex(resToOptmUniList, combRes);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }

            for (const CombinedResID combRes : rwRes.readCombinedRes)
            {
                uint16_t usedIdx = getElemIndex(resToOptmUniList, combRes);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }
        }

        std::vector<CombinedResID> readonlyResUniList;
        for (const CombinedResID readRes : readResUniList)
        {
            // not found in writeResUniList: no one write to it
            if (kInvalidIndex != getElemIndex(writeResUniList, readRes)) {
                continue;
            }
            readonlyResUniList.push_back(readRes);
        }

        // remove read-only resource from resToOptmUniList
        for (const CombinedResID readonlyRes : readonlyResUniList)
        {
            uint16_t idx = getElemIndex(resToOptmUniList, readonlyRes);
            if (kInvalidIndex == idx) {
                continue;
            }
            resToOptmUniList.erase(resToOptmUniList.begin() + idx);
            resToOptmPassIdxByOrder.erase(resToOptmPassIdxByOrder.begin() + idx);
        }

        // remove force alias resource from resToOptmUniList
        for (size_t ii = 0; ii < m_plainResAliasToBase.size(); ++ii)
        {
            // find current id in resToOptmUniList
            const CombinedResID plainAlias = m_plainResAliasToBase.getIdAt(ii);
            uint16_t idx = getElemIndex(resToOptmUniList, plainAlias);
            if (kInvalidIndex == idx) {
                continue;
            }

            resToOptmUniList.erase(resToOptmUniList.begin() + idx);
            resToOptmPassIdxByOrder.erase(resToOptmPassIdxByOrder.begin() + idx);
        }

        // remove multi-frame resource from resToOptmUniList
        const std::vector<CombinedResID> multiFrameResUniList = m_multiFrame_resList;
        for (const CombinedResID multiFrameRes : multiFrameResUniList)
        {
            uint16_t idx = getElemIndex(resToOptmUniList, multiFrameRes);
            if (kInvalidIndex == idx) {
                continue;
            }
            resToOptmUniList.erase(resToOptmUniList.begin() + idx);
            resToOptmPassIdxByOrder.erase(resToOptmPassIdxByOrder.begin() + idx);
        }

        assert(resToOptmPassIdxByOrder.size() == resToOptmUniList.size());

        std::vector < ResLifetime > resLifeTime;
        // only transient resource would be in the lifetime list
        for (const std::vector<uint16_t> & passIdxByOrder : resToOptmPassIdxByOrder)
        {
            assert(!passIdxByOrder.empty());

            uint16_t maxIdx = *std::max_element(begin(passIdxByOrder), end(passIdxByOrder));
            uint16_t minIdx = *std::min_element(begin(passIdxByOrder), end(passIdxByOrder));

            resLifeTime.push_back({ minIdx, maxIdx });
        }

        // set the value
        m_resInUseUniList = resInUseUniList;
        m_resToOptmUniList = resToOptmUniList;
        m_readonly_resList = readonlyResUniList;
        m_readResUniList = readResUniList;
        m_writeResUniList = writeResUniList;

        m_resLifeTime = resLifeTime;
    }

    void Framegraph2::fillBucketForceAlias()
    {
        std::vector<CombinedResID> actualAliasBase;
        std::vector<std::vector<CombinedResID>> actualAlias;
        for (const CombinedResID combRes : m_resInUseUniList)
        {
            if (!m_plainResAliasToBase.exist(combRes))
            {
                continue;
            }

            CombinedResID base2 = m_plainResAliasToBase.getIdToData(combRes);

            uint16_t usedBaseIdx = push_back_unique(actualAliasBase, base2);
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
            const std::vector<CombinedResID>& aliasVec = actualAlias[ii];

            // buffers
            if (isBuffer(base)) 
            {
                const BufRegisterInfo info = m_sparse_buf_info[base.id];

                BufBucket bucket;
                createBufBkt(bucket, info, aliasVec, true);
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (isImage(base))
            {
                const ImgRegisterInfo info = m_sparse_img_info[base.id];

                ImgBucket bucket;
                createImgBkt(bucket, info, aliasVec, true);
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph2::fillBucketReadonly()
    {
        for (const CombinedResID cid : m_readonly_resList)
        {
            // buffers
            if (isBuffer(cid))
            {
                const BufRegisterInfo info = m_sparse_buf_info[cid.id];

                BufBucket bucket;
                createBufBkt(bucket, info, {cid});
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (isImage(cid))
            {
                const ImgRegisterInfo info = m_sparse_img_info[cid.id];

                ImgBucket bucket;
                createImgBkt(bucket, info, {cid});
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph2::fillBucketMultiFrame()
    {
        for (const CombinedResID cid : m_multiFrame_resList)
        {
            // buffers
            if (isBuffer(cid))
            {
                const BufRegisterInfo info = m_sparse_buf_info[cid.id];

                BufBucket bucket;
                createBufBkt(bucket, info, { cid });
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (isImage(cid))
            {
                const ImgRegisterInfo info = m_sparse_img_info[cid.id];

                ImgBucket bucket;
                createImgBkt(bucket, info, { cid });
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph2::createBufBkt(BufBucket& _bkt, const BufRegisterInfo& _info, const std::vector<CombinedResID>& _reses, const bool _forceAliased /*= false*/)
    {
        assert(!_reses.empty());

        _bkt.desc.size = _info.size;
        _bkt.desc.memFlags = _info.memFlags;
        _bkt.desc.usage = _info.usage;

        _bkt.initialBarrierState = _info.initialState;
        _bkt.baseBufId = _info.bufId;
        _bkt.reses = _reses;
        _bkt.forceAliased = _forceAliased;
    }

    void Framegraph2::createImgBkt(ImgBucket& _bkt, const ImgRegisterInfo& _info, const std::vector<CombinedResID>& _reses, const bool _forceAliased /*= false*/)
    {
        assert(!_reses.empty());

        _bkt.desc.mips = _info.mips;
        _bkt.desc.width = _info.width;
        _bkt.desc.height = _info.height;
        _bkt.desc.depth = _info.depth;
        _bkt.desc.arrayLayers = _info.arrayLayers;

        _bkt.desc.type = _info.type;
        _bkt.desc.viewType = _info.viewType;
        _bkt.desc.format = _info.format;
        _bkt.desc.usage = _info.usage;
        _bkt.desc.layout = _info.layout;

        _bkt.aspectFlags = _info.aspectFlags;
        _bkt.initialBarrierState = _info.initialState;
        _bkt.baseImgId = _info.imgId;
        _bkt.reses = _reses;
        _bkt.forceAliased = _forceAliased;
    }

    void Framegraph2::aliasBuffers(std::vector<BufBucket>& _buckets, const std::vector<uint16_t>& _sortedBufList)
    {
        std::vector<uint16_t> restRes = _sortedBufList;

        std::vector<std::vector<CombinedResID>> resInLevel{{}};

        if (restRes.empty()) {
            return;
        }

        // now process normal alias resources
        std::vector<BufBucket> buckets;
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
                    uint16_t idx = getElemIndex(restRes, res.id);
                    if(kInvalidIndex == idx)
                        continue;

                    restRes.erase(begin(restRes) + idx);
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
            const BufRegisterInfo& info = m_sparse_buf_info[baseRes];
            createBufBkt(bucket, info, { baseCid });
            buckets.push_back(bucket);

            restRes.erase(begin(restRes));

            resInLevel.clear();
            resInLevel.emplace_back();
        }

        _buckets = _buckets;
    }

    void Framegraph2::aliasImages(std::vector<ImgBucket>& _buckets,const std::vector< ImgRegisterInfo >& _infos, const std::vector<uint16_t>& _sortedTexList, const ResourceType _type)
    {
        std::vector<uint16_t> restRes = _sortedTexList;

        std::vector<std::vector<CombinedResID>> resInLevel{{}};

        if (restRes.empty()) {
            return;
        }

        // now process normal alias resources
        std::vector<ImgBucket> buckets;
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
                    uint16_t idx = getElemIndex(restRes, res.id);
                    if (kInvalidIndex == idx)
                        continue;

                    restRes.erase(begin(restRes) + idx);
                }

                break;
            }

            if (aliased)
            {
                continue;
            }

            // no suitable bucket found, create a new one
            ImgBucket bucket;
            const ImgRegisterInfo& info = _infos[baseRes];
            createImgBkt(bucket, info, { baseCid });
            buckets.push_back(bucket);

            restRes.erase(begin(restRes));

            resInLevel.clear();
            resInLevel.emplace_back();
        }

        _buckets.insert(end(_buckets), begin(buckets), end(buckets));
    }

    void Framegraph2::fillBufferBuckets()
    {
        // sort the resource by size
        std::vector<uint16_t> sortedBufIdx;
        for (const CombinedResID& crid : m_resToOptmUniList)
        {
            if (!isBuffer(crid)) {
                continue;
            }
            sortedBufIdx.push_back(crid.id);
        }

        std::sort(begin(sortedBufIdx), end(sortedBufIdx), [&](uint16_t _l, uint16_t _r) {
            return m_sparse_buf_info[_l].size > m_sparse_buf_info[_r].size;
        });

        std::vector<BufBucket> buckets;
        aliasBuffers(buckets,sortedBufIdx);

        m_bufBuckets.insert(end(m_bufBuckets), begin(buckets), end(buckets));
    }

    void Framegraph2::fillImageBuckets()
    {
        const ResourceType type = ResourceType::image;

        std::vector<uint16_t> sortedTexIdx;
        for (const CombinedResID& crid : m_resToOptmUniList)
        {
            if (!(isImage(crid))) {
                continue;
            }
            sortedTexIdx.push_back(crid.id);
        }

        std::sort(begin(sortedTexIdx), end(sortedTexIdx), [&](uint16_t _l, uint16_t _r) {
            ImgRegisterInfo info_l = m_sparse_img_info[_l];
            ImgRegisterInfo info_r = m_sparse_img_info[_r];

            size_t size_l = info_l.width * info_l.height * info_l.depth * info_l.mips * info_l.bpp;
            size_t size_r = info_r.width * info_r.height * info_r.depth * info_r.mips * info_r.bpp;

            return size_l > size_r;
        });

        std::vector<ImgBucket> buckets;
        aliasImages(buckets, m_sparse_img_info, sortedTexIdx, type);

        m_imgBuckets.insert(end(m_imgBuckets), begin(buckets), end(buckets));
    }

    void Framegraph2::optimizeSync()
    {
        // calc max level list
        std::vector<uint16_t> maxLvLst(m_sortedPassIdx.size(), 0);
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
        for (const BufBucket& bkt : m_bufBuckets)
        {
            RHIContextOpMagic magic{ RHIContextOpMagic::create_buffer };

            write(&m_rhiMemWriter, magic);

            BufferCreateInfo info;
            info.bufId = bkt.baseBufId;
            info.size = bkt.desc.size;
            info.data = bkt.desc.data;
            info.memFlags = bkt.desc.memFlags;
            info.usage = bkt.desc.usage;
            info.aliasNum = (uint16_t)bkt.reses.size();

            info.barrierState = bkt.initialBarrierState;

            write(&m_rhiMemWriter, info);

            if (info.aliasNum > 0)
            {
                assert(info.data == nullptr);
            }

            std::vector<BufferAliasInfo> aliasInfo;
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
        for (const ImgBucket& bkt : m_imgBuckets)
        {
            RHIContextOpMagic magic{ RHIContextOpMagic::create_image };

            write(&m_rhiMemWriter, magic);

            ImageCreateInfo info;
            info.imgId = bkt.baseImgId;
            info.mips = bkt.desc.mips;
            info.width = bkt.desc.width;
            info.height = bkt.desc.height;
            info.depth = bkt.desc.depth;
            info.arrayLayers = bkt.desc.arrayLayers;

            info.type = bkt.desc.type;
            info.viewType = bkt.desc.viewType;
            info.format = bkt.desc.format;
            info.usage = bkt.desc.usage;
            info.layout = bkt.desc.layout;

            info.aliasNum = (uint16_t)bkt.reses.size();

            info.aspectFlags = bkt.aspectFlags;
            info.barrierState = bkt.initialBarrierState;

            write(&m_rhiMemWriter, info);

            std::vector<ImageAliasInfo> aliasInfo;
            for (const CombinedResID cid : bkt.reses)
            {
                ImageAliasInfo alias;
                alias.imgId = cid.id;
                aliasInfo.push_back(alias);
            }

            write(&m_rhiMemWriter, (void*)aliasInfo.data(), int32_t(sizeof(ImageAliasInfo) * bkt.reses.size()));

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }
    }

    void Framegraph2::createShaders()
    {
        std::vector<ProgramHandle> usedProgram{};
        std::vector<ShaderHandle> usedShaders{};
        for ( PassHandle pass : m_sortedPass)
        {
            const PassMetaData& passMeta = m_sparse_pass_meta[pass.id];

            const ProgramInfo& progInfo = m_sparse_program_info[passMeta.programId];
            usedProgram.push_back({ passMeta.programId });

            for (uint16_t ii = 0; ii < progInfo.regInfo.shaderNum; ++ii)
            {
                const uint16_t shaderId = progInfo.shaderIds[ii];

                usedShaders.push_back({ shaderId });
            }
        }

        // write shader info
        for (ShaderHandle shader : usedShaders)
        {
            const ShaderInfo& info = m_sparse_shader_info[shader.id];
            assert(shader.id == info.regInfo.shaderId);

            std::string& path = m_shader_path[info.pathIdx];
            
            ShaderCreateInfo createInfo;
            createInfo.shaderId = info.regInfo.shaderId;
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
            assert(prog.id == info.regInfo.progId);

            ProgramCreateInfo createInfo;
            createInfo.progId = info.regInfo.progId;
            createInfo.shaderNum = info.regInfo.shaderNum;
            createInfo.sizePushConstants = info.regInfo.sizePushConstants;
            createInfo.pPushConstants = info.regInfo.pPushConstants;

            RHIContextOpMagic magic{ RHIContextOpMagic::create_program };

            write(&m_rhiMemWriter, magic);

            write(&m_rhiMemWriter, createInfo);

            write(&m_rhiMemWriter, (void*)info.shaderIds, (int32_t)(info.regInfo.shaderNum * sizeof(uint16_t)));

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }
    }

    void Framegraph2::createPasses()
    {
        std::vector<PassMetaData> passMetaDataVec{};
        std::vector<std::vector<VertexBindingDesc>> passVertexBinding(m_sortedPass.size());
        std::vector<std::vector<VertexAttributeDesc>> passVertexAttribute(m_sortedPass.size());
        std::vector<std::vector<int>> passPipelineSpecData(m_sortedPass.size());

        std::vector<std::pair<ImageHandle, ResInteractDesc>> writeDSPair(m_sortedPass.size());
        std::vector<UniDataContainer< ImageHandle, ResInteractDesc> > writeImageList(m_sortedPass.size());
        std::vector<UniDataContainer< ImageHandle, ResInteractDesc> > readImageList(m_sortedPass.size());

        std::vector<UniDataContainer< BufferHandle, ResInteractDesc> > readBufferList(m_sortedPass.size());
        std::vector<UniDataContainer< BufferHandle, ResInteractDesc> > writeBufferList(m_sortedPass.size());
        
        for (uint32_t ii = 0; ii < m_sortedPass.size(); ++ii)
        {
            PassHandle pass = m_sortedPass[ii];
            uint16_t passIdx = getElemIndex(m_hPass, pass);
            const PassMetaData& passMeta = m_sparse_pass_meta[pass.id];
            assert(pass.id == passMeta.passId);

            std::pair<ImageHandle, ResInteractDesc>& writeDS = writeDSPair[ii];
            UniDataContainer< ImageHandle, ResInteractDesc>& writeImg = writeImageList[ii];
            UniDataContainer< ImageHandle, ResInteractDesc>& readImg = readImageList[ii];

            UniDataContainer< BufferHandle, ResInteractDesc>& readBuf = readBufferList[ii];
            UniDataContainer< BufferHandle, ResInteractDesc>& writeBuf = writeBufferList[ii];
            {
                writeDS.first = { kInvalidHandle };
                writeImg.clear();
                readImg.clear();
                readBuf.clear();
                writeBuf.clear();

                const PassRWResource& rwRes = m_pass_rw_res[passIdx];
                // set write resources
                for (const CombinedResID writeRes : rwRes.writeCombinedRes)
                {
                    if (isDepthStencil(writeRes)) {

                        assert(writeDS.first.id == kInvalidHandle);
                        writeDS.first = { writeRes.id };
                        writeDS.second = rwRes.writeInteracts.getIdToData(writeRes);
                        
                        writeImg.push_back({ writeRes.id }, rwRes.writeInteracts.getIdToData(writeRes));
                    }
                    else if (isColorAttachment(writeRes))
                    {
                        writeImg.push_back({ writeRes.id }, rwRes.writeInteracts.getIdToData(writeRes));
                    }
                    else if (isNormalImage(writeRes)
                        && (passMeta.queue == PassExeQueue::compute || passMeta.queue == PassExeQueue::copy))
                    {
                        writeImg.push_back({ writeRes.id }, rwRes.writeInteracts.getIdToData(writeRes));
                    }
                    else if (isBuffer(writeRes))
                    {
                        writeBuf.push_back({ writeRes.id }, rwRes.writeInteracts.getIdToData(writeRes));
                    }
                    else
                    {
                        vkz::message(DebugMessageType::error, "invalid write resource mapping: Res %4d, PassExeQueue: %d\n", writeRes.id, passMeta.queue);
                    }
                }

                // set read resources
                for (const CombinedResID readRes : rwRes.readCombinedRes)
                {
                    if (  isColorAttachment(readRes) 
                        || isDepthStencil(readRes)
                        || isImage(readRes))
                    {
                        readImg.push_back({ readRes.id }, rwRes.readInteracts.getIdToData(readRes));
                    }
                    else if (isBuffer(readRes))
                    {
                        assert(!readBuf.exist({ readRes.id }));
                        readBuf.push_back({ readRes.id }, rwRes.readInteracts.getIdToData(readRes));
                    }
                }
            }

            // create pass info
            assert(writeDS.first.id == passMeta.writeDepthId);
            assert((uint16_t)writeImg.size() == passMeta.writeImageNum);
            assert((uint16_t)readImg.size() == passMeta.readImageNum);
            assert((uint16_t)readBuf.size() == passMeta.readBufferNum);
            assert((uint16_t)writeBuf.size() == passMeta.writeBufferNum);

            passMetaDataVec.emplace_back(passMeta);

            const PassMetaDataRef& createDataRef = m_sparse_pass_data_ref[pass.id];

            // vertex binding           
            std::vector<VertexBindingDesc>& bindingDesc = passVertexBinding[ii];
            for (const uint16_t& bindingIdx : createDataRef.vtxBindingIdxs)
            {
                bindingDesc.push_back(m_vtxBindingDesc[bindingIdx]);
            }
            assert(bindingDesc.size() == passMeta.vertexBindingNum);

            // vertex attribute
            std::vector<VertexAttributeDesc>& attributeDesc = passVertexAttribute[ii];
            for (const uint16_t& attributeIdx : createDataRef.vtxAttrIdxs)
            {
                attributeDesc.push_back(m_vtxAttrDesc[attributeIdx]);
            }
            assert(attributeDesc.size() == passMeta.vertexAttributeNum);

            // pipeline constants
            std::vector<int>& pipelineSpecData = passPipelineSpecData[ii];
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
            write(&m_rhiMemWriter, (void*)writeImageList[ii].getIdPtr(), (int32_t)(createInfo.writeImageNum * sizeof(ImageHandle)));
            write(&m_rhiMemWriter, (void*)readImageList[ii].getIdPtr(), (int32_t)(createInfo.readImageNum * sizeof(ImageHandle)));
            write(&m_rhiMemWriter, (void*)readBufferList[ii].getIdPtr(), (int32_t)(createInfo.readBufferNum * sizeof(BufferHandle)));
            write(&m_rhiMemWriter, (void*)writeBufferList[ii].getIdPtr(), (int32_t)(createInfo.writeBufferNum * sizeof(BufferHandle)));

            write(&m_rhiMemWriter, writeDSPair[ii].second);
            write(&m_rhiMemWriter, (void*)writeImageList[ii].getDataPtr(), (int32_t)(createInfo.writeImageNum * sizeof(ResInteractDesc)));
            write(&m_rhiMemWriter, (void*)readImageList[ii].getDataPtr(), (int32_t)(createInfo.readImageNum * sizeof(ResInteractDesc)));
            write(&m_rhiMemWriter, (void*)readBufferList[ii].getDataPtr(), (int32_t)(createInfo.readBufferNum * sizeof(ResInteractDesc)));
            write(&m_rhiMemWriter, (void*)writeBufferList[ii].getDataPtr(), (int32_t)(createInfo.writeBufferNum * sizeof(ResInteractDesc)));

            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }
    }

    void Framegraph2::createResources()
    {
        createBuffers();
        createImages();
        createShaders();
        createPasses();

        // set brief
        {
            write(&m_rhiMemWriter, RHIContextOpMagic::set_brief);
            RHIBrief brief;
            brief.finalPassId = m_finalPass.id;
            brief.presentImageId = m_combinedPresentImage.id;

            write(&m_rhiMemWriter, brief);
            write(&m_rhiMemWriter, RHIContextOpMagic::magic_body_end);
        }

        // end mem tag
        RHIContextOpMagic magic{ RHIContextOpMagic::end };
        write(&m_rhiMemWriter, magic);
    }

    bool Framegraph2::isBufInfoAliasable(uint16_t _idx, const BufBucket& _bucket, const std::vector<CombinedResID> _resInCurrStack) const
    {
        // size check in current stack
        const BufRegisterInfo& info = m_sparse_buf_info[_idx];

        bool bCondMatch = (info.size <= _bucket.desc.size);

        for (const CombinedResID res : _resInCurrStack)
        {
            const BufRegisterInfo& stackInfo = m_sparse_buf_info[res.id];

            bCondMatch &= (info.data == nullptr && stackInfo.data == nullptr);
            bCondMatch &= (info.memFlags == stackInfo.memFlags);
            bCondMatch &= (info.usage == stackInfo.usage);

        }

        return bCondMatch;
    }

    bool Framegraph2::isImgInfoAliasable(uint16_t _ImgId, const ImgBucket& _bucket, const std::vector<CombinedResID> _resInCurrStack) const
    {
        bool bCondMatch = true;

        // condition check
        const ImgRegisterInfo& info = m_sparse_img_info[_ImgId];
        for (const CombinedResID res : _resInCurrStack)
        {
            const ImgRegisterInfo& stackInfo = m_sparse_img_info[res.id];

            bCondMatch &= (info.mips == stackInfo.mips);
            bCondMatch &= (info.width == stackInfo.width && info.height == stackInfo.height && info.depth == stackInfo.depth);
            bCondMatch &= (info.arrayLayers == stackInfo.arrayLayers);
            bCondMatch &= (info.format == stackInfo.format);

            bCondMatch &= (info.usage == stackInfo.usage);
            bCondMatch &= (info.type == stackInfo.type);
        }
        
        return bCondMatch;
    }

    bool Framegraph2::isStackAliasable(const CombinedResID& _res, const std::vector<CombinedResID>& _reses) const
    {
        bool bStackMatch = true;
        
        // life time check in entire bucket
        const uint16_t idx = getElemIndex(m_resToOptmUniList, _res);
        const ResLifetime& resLifetime = m_resLifeTime[idx];
        for (const CombinedResID res : _reses)
        {
            const uint16_t resIdx = getElemIndex(m_resToOptmUniList, res);
            const ResLifetime& stackLifetime = m_resLifeTime[resIdx];

            // if lifetime overlap with any resource in current bucket, then it's not alias-able
            bStackMatch &= (stackLifetime.startIdx > resLifetime.endIdx || stackLifetime.endIdx < resLifetime.startIdx);
        }

        return bStackMatch;
    }

    bool Framegraph2::isAliasable(const CombinedResID& _res, const BufBucket& _bucket, const std::vector<CombinedResID>& _resInCurrStack) const
    {
        // no stack checking for stacks, because each stack only contains 1 resource
        // bool bInfoMatch = isBufInfoAliasable(_res.id, _bucket, _resInCurrStack);
        
        bool bInfoMatch = isBufInfoAliasable(_res.id, _bucket, _bucket.reses);
        bool bStackMatch = isStackAliasable(_res, _bucket.reses);

        return bInfoMatch && bStackMatch;
    }

    bool Framegraph2::isAliasable(const CombinedResID& _res, const ImgBucket& _bucket, const std::vector<CombinedResID>& _resInCurrStack) const
    {
        // no stack checking for stacks, because each stack only contains 1 resource
        // bool bInfoMatch = isImgInfoAliasable(_res.id, _bucket, _resInCurrStack);

        bool bInfoMatch = isImgInfoAliasable(_res.id, _bucket, _bucket.reses);
        bool bStackMatch = isStackAliasable(_res, _bucket.reses);

        return bInfoMatch && bStackMatch;
    }

} // namespace vkz
