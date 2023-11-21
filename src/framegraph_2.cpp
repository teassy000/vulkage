#include "common.h"
#include "resources.h"
#include "config.h"

#include "memory_operation.h"
#include "handle.h"
#include "vkz.h"
#include "framegraph_2.h"
#include <stack>



namespace vkz
{
    void Framegraph2::bake()
    {
        // prepare
        parseOp();

        buildGraph();

        // sort and cut
        reverseTraversalDFS();

        // optimize
        optimizeSync();

    }

    void Framegraph2::parseOp()
    {
        assert(m_pMemBlock != nullptr);
        MemoryReader reader(m_pMemBlock->expand(0), m_pMemBlock->size());

        while (true)
        {
            MagicTag magic = MagicTag::InvalidMagic;
            read(&reader, magic);

            bool finished = false;
            switch (magic)
            {
            // Register
            case MagicTag::RegisterPass: {
                registerPass(reader);
                break;
            }
            case MagicTag::RegisterBuffer: {
                registerBuffer(reader);
                break;
            }
            case MagicTag::RegisterTexture: {
                registerTexture(reader);
                break;
            }
            case MagicTag::RegisterRenderTarget: {
                registerRenderTarget(reader);
                break;
            }
            case MagicTag::RegisterDepthStencil: {
                registerDepthStencil(reader);
                break;
            }
            // Alias
            case MagicTag::AliasBuffer: {
                aliasResForce(reader, ResourceType::Buffer);
                break;
            }
            case MagicTag::AliasTexture: {
                aliasResForce(reader, ResourceType::Texture);
                break;
            }
            case MagicTag::AliasRenderTarget: {
                aliasResForce(reader, ResourceType::RenderTarget);
                break;
            }
            case MagicTag::AliasDepthStencil: {
                aliasResForce(reader, ResourceType::DepthStencil);
                break;
            }
            // Read
            case MagicTag::PassReadBuffer: {
                passReadRes(reader, ResourceType::Buffer);
                break;
            }
            case MagicTag::PassReadTexture: {
                passReadRes(reader, ResourceType::Texture);
                break;
            }
            case MagicTag::PassReadRenderTarget: {
                passReadRes(reader, ResourceType::RenderTarget);
                break;
            }
            case MagicTag::PassReadDepthStencil: {
                passReadRes(reader, ResourceType::DepthStencil);
                break;
            }
            // Write
            case MagicTag::PassWriteBuffer: {
                passWriteRes(reader, ResourceType::Buffer);
                break;
            }
            case MagicTag::PassWriteTexture: {
                passWriteRes(reader, ResourceType::Texture);
                break;
            }
            case MagicTag::PassWriteRenderTarget: {
                passWriteRes(reader, ResourceType::RenderTarget);
                break;
            }
            case MagicTag::PassWriteDepthStencil: {
                passWriteRes(reader, ResourceType::DepthStencil);
                break;
            }
            case MagicTag::SetResultRenderTarget: {
                setResultRT(reader);
                break;
            }
            // MultiFrame
            case MagicTag::SetMuitiFrameBuffer: {
                setMultiFrameRes(reader, ResourceType::Buffer);
                break;
            }
            case MagicTag::SetMuitiFrameTexture: {
                setMultiFrameRes(reader, ResourceType::Texture);
                break;
            }
            case MagicTag::SetMultiFrameRenderTarget: {
                setMultiFrameRes(reader, ResourceType::RenderTarget);
                break;
            }
            case MagicTag::SetMultiFrameDepthStencil: {
                setMultiFrameRes(reader, ResourceType::DepthStencil);
                break;
            }
            // End
            case MagicTag::InvalidMagic:
                message(DebugMessageType::warning, "invalid magic tag, data incorrect!");
            case MagicTag::End:
            default:
                finished = true;
                break;
            }
            
            if (finished)
            {
                break;
            }
        }
    }

    constexpr uint16_t kInvalidIndex = 0xffff;
    template<typename T>
    const uint16_t getIndex(const std::vector<T>& _vec, const T _data)
    {
        auto it = std::find(begin(_vec), end(_vec), _data);
        if (it == end(_vec))
        {
            return kInvalidIndex;
        }

        return (uint16_t)std::distance(begin(_vec), it);
    }

    template<typename T>
    const uint16_t push_back_unique(std::vector<T>& _vec, const T _data)
    {
        uint16_t idx = getIndex(_vec, _data);

        if (kInvalidIndex == idx)
        {
            idx = (uint16_t)_vec.size();
            _vec.push_back(_data);
        }

        return idx;
    }

    void Framegraph2::registerPass(MemoryReader& _reader)
    {
        PassRegisterInfo info;
        read(&_reader, info);

        // fill pass idx in queue
        uint16_t qIdx = (uint16_t)info.queue;
        m_passIdxInQueue[qIdx].push_back((uint16_t)m_hPass.size());

        // fill pass info
        m_hPass.push_back({ info.idx });
        m_pass_info.push_back(info);

        m_pass_rw_res.push_back({});
        m_pass_dependency.push_back({});
        m_passIdxToSync.push_back({});
    }

    void Framegraph2::registerBuffer(MemoryReader& _reader)
    {
        BufRegisterInfo info;
        read(&_reader, info);

        m_hBuf.push_back({ info.idx });
        m_buf_info.push_back(info);

        CombinedResID plainResID = getCombinedResID(info.idx, ResourceType::Buffer);
        m_plain_resource_id.push_back(plainResID);
    }

    void Framegraph2::registerTexture(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hTex.push_back({ info.idx });
        m_tex_info.push_back(info);

        CombinedResID plainResIdx = getCombinedResID(info.idx, ResourceType::Texture);
        m_plain_resource_id.push_back(plainResIdx);
    }

    void Framegraph2::registerRenderTarget(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hRT.push_back({ info.idx });
        m_rt_info.push_back(info);

        CombinedResID plainResIdx = getCombinedResID(info.idx, ResourceType::RenderTarget);
        m_plain_resource_id.push_back(plainResIdx);
    }

    void Framegraph2::registerDepthStencil(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hDS.push_back({ info.idx });
        m_ds_info.push_back(info);

        CombinedResID plainResIdx = getCombinedResID(info.idx, ResourceType::DepthStencil);
        m_plain_resource_id.push_back(plainResIdx);
    }

    void Framegraph2::passReadRes(MemoryReader& _reader, ResourceType _type)
    {
        PassRWInfo info;
        read(&_reader, info);
        alloc(sizeof(uint16_t) * info.resNum);
        
        uint16_t* resArr = new uint16_t[info.resNum];
        read(&_reader, resArr, sizeof(uint16_t) * info.resNum);

        for (uint16_t ii = 0; ii < info.resNum; ++ii)
        {
            uint16_t idx = getIndex(m_hPass, { info.pass });
            assert(idx != kInvalidIndex);
            
            uint16_t resIdx = resArr[ii];
            CombinedResID plainIdx = getCombinedResID(resIdx, _type);
            m_pass_rw_res[idx].readCombinedRes.push_back(plainIdx);
        }

        delete[] resArr;
        resArr = nullptr;
    }

    void Framegraph2::passWriteRes(MemoryReader& _reader, ResourceType _type)
    {
        PassRWInfo info;
        read(&_reader, info);

        uint16_t* resArr = new uint16_t[info.resNum];
        read(&_reader, resArr, sizeof(uint16_t) * info.resNum);

        for (uint16_t ii = 0; ii < info.resNum; ++ii)
        {
            uint16_t idx = getIndex(m_hPass, { info.pass });
            assert(idx != kInvalidIndex);
            
            uint16_t resIdx = resArr[ii];
            CombinedResID plainIdx = getCombinedResID(resIdx, _type);
            m_pass_rw_res[idx].writeCombinedRes.push_back(plainIdx);
        }

        delete[] resArr;
        resArr = nullptr;
    }

    void Framegraph2::aliasResForce(MemoryReader& _reader, ResourceType _type)
    {
        ResAliasInfo info;
        read(&_reader, info);

        uint16_t* resArr = new uint16_t[info.aliasNum];
        read(&_reader, resArr, sizeof(uint16_t) * info.aliasNum);

        CombinedResID plainBaseIdx = getCombinedResID(info.resBase, _type);
        uint16_t idx = getIndex(m_plain_force_alias_base, plainBaseIdx);
        if (kInvalidIndex == idx)
        {
            m_plain_force_alias_base.push_back(plainBaseIdx);
            std::unordered_set<uint16_t> aliasSet(resArr, resArr + info.aliasNum);
            m_plain_force_alias.push_back(aliasSet);
        }
        else // found the base
        {
            for (uint16_t ii = 0; ii < info.aliasNum; ++ii)
            {
                uint16_t resIdx = resArr[ii];
                m_plain_force_alias[idx].insert(resIdx);
            }
        }

        delete[] resArr;
        resArr = nullptr;
    }


    void Framegraph2::setMultiFrameRes(MemoryReader& _reader, ResourceType _type)
    {
        uint16_t resNum = 0;
        read(&_reader, resNum);

        uint16_t* resArr = new uint16_t[resNum];
        read(&_reader, resArr, sizeof(uint16_t) * resNum);

        for (uint16_t ii = 0; ii < resNum; ++ii)
        {
            uint16_t resIdx = resArr[ii];
            CombinedResID plainIdx = getCombinedResID(resIdx, _type);
            m_multiFrame_res.push_back(plainIdx);
        }
    }

    void Framegraph2::setResultRT(MemoryReader& _reader)
    {
        uint16_t rt;
        read(&_reader, rt);

        // check if rt is ready resisted
        uint16_t idx = getIndex(m_hRT, { rt });
        if (kInvalidIndex == idx)
        {
            message(DebugMessageType::error, "result rt is not registered!");
            return;
        }

        m_resultRT = getCombinedResID(rt, ResourceType::RenderTarget);
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

                if (CombindRes == m_resultRT)
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
                uint16_t idx = getIndex(linear_outResCID, plainRes);
                if (kInvalidIndex == idx) {
                    continue;
                }

                uint16_t currPassIdx = ii;
                uint16_t writePassIdx = linear_outPassIdx[idx];

                m_pass_dependency[currPassIdx].inPassIdxLst.push_back(writePassIdx);
                m_pass_dependency[writePassIdx].outPassIdxLst.push_back(currPassIdx);
            }
        }
    }

    void Framegraph2::reverseTraversalDFS()
    {
        uint16_t passNum = (uint16_t)m_hPass.size();

        std::vector<bool> visited(passNum, false);
        std::vector<bool> onStack(passNum, false);

        uint16_t finalPassIdx = (uint16_t)getIndex(m_hPass, m_finalPass);

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

            for (uint16_t parentPassIdx : passDep.inPassIdxLst)
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

            for (uint16_t inPassIdx : m_pass_dependency[passIdx].inPassIdxLst)
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

                for (const uint16_t inPassIdx : m_pass_dependency[currPassIdx].inPassIdxLst)
                {
                    uint16_t idx = getIndex(m_passIdxInDpLevels[passIdx].passInLv[baseLv - currLv], inPassIdx);
                    if (currLv - 1 != _maxLvLst[inPassIdx] || kInvalidIndex != idx)
                    {
                        continue;
                    }

                    m_passIdxInDpLevels[passIdx].passInLv[baseLv - currLv].push_back(inPassIdx);
                }

                for (const uint16_t inPassIdx : m_pass_dependency[currPassIdx].inPassIdxLst)
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
            for (uint16_t ii = 0; ii < passDep.passInLv.size(); ++ii)
            {
                // iterate each pass in current level
                for (uint16_t jj = 0; jj < passDep.passInLv[ii].size(); ++jj)
                {
                    const uint16_t inPassInCurrLevel = passDep.passInLv[ii][jj];

                    // check if pass in m_passIdxInDLevels can match with m_passIdxInQueue
                    for (uint16_t qIdx = 0; qIdx < m_passIdxInQueue.size(); ++qIdx)
                    {
                        const uint16_t idx = getIndex(m_passIdxInQueue[qIdx], inPassInCurrLevel);

                        bool isMatch = 
                               (kInvalidIndex == m_nearestSyncPassIdx[passIdx][qIdx]) // not set yet
                            && (kInvalidIndex != idx); // found

                        // only first met would set the value
                        if ( isMatch) {
                            m_nearestSyncPassIdx[passIdx][qIdx] = inPassInCurrLevel;
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

    void Framegraph2::processMultiFrameRes()
    {
        std::vector<CombinedResID> plainAliasRes;
        std::vector<uint16_t> aliasResIdx;
        for (uint16_t ii = 0; ii < m_plain_force_alias_base.size(); ++ii)
        {
            CombinedResID base = m_plain_force_alias_base[ii];
            plainAliasRes.push_back(base);

            const std::unordered_set<uint16_t>& aliasSet = m_plain_force_alias[ii];

            for (uint16_t alias : aliasSet)
            {
                CombinedResID plainAlias = getCombinedResID(alias, base.type);
                plainAliasRes.push_back(plainAlias);
            }

            aliasResIdx.insert(end(aliasResIdx), aliasSet.size() + 1, ii);
        }

        std::vector<CombinedResID> fullMultiFrameRes = m_multiFrame_res;
        for (const CombinedResID cid : m_multiFrame_res)
        {
            const uint16_t idx = getIndex(plainAliasRes, cid);
            if (kInvalidHandle == idx)
            {
                continue;
            }

            const uint16_t alisIdx = aliasResIdx[idx];
            CombinedResID base = m_plain_force_alias_base[alisIdx];

            push_back_unique(fullMultiFrameRes, base);
            const std::unordered_set<uint16_t>& aliasSet = m_plain_force_alias[alisIdx];
            for (uint16_t alias : aliasSet)
            {
                CombinedResID plainAlias = getCombinedResID(alias, base.type);
                push_back_unique(fullMultiFrameRes, plainAlias);
            }
        }

        m_multiFrame_res = fullMultiFrameRes;
    }

    void Framegraph2::buildResLifetime()
    {
        std::vector<CombinedResID> usedResUniList;
        std::vector<CombinedResID> readResUniList;
        std::vector<CombinedResID> writeResUniList;
        
        for (const uint16_t pIdx : m_sortedPassIdx)
        {
            PassRWResource rwRes = m_pass_rw_res[pIdx];

            for (const CombinedResID combRes : rwRes.writeCombinedRes)
            {
                push_back_unique(writeResUniList, combRes);
                push_back_unique(usedResUniList, combRes);
            }

            for (const CombinedResID combRes : rwRes.readCombinedRes)
            {
                push_back_unique(readResUniList, combRes);
                push_back_unique(usedResUniList, combRes);
            }
        }

        std::vector<std::vector<uint16_t>> usedResPassIdxByOrder; // share the same idx with usedResUniList
        usedResPassIdxByOrder.assign(usedResUniList.size(), std::vector<uint16_t>());
        for(uint16_t ii = 0; ii < m_sortedPassIdx.size(); ++ii)
        {
            const uint16_t pIdx = m_sortedPassIdx[ii];
            PassRWResource rwRes = m_pass_rw_res[pIdx];

            for (const CombinedResID combRes : rwRes.writeCombinedRes)
            {
                uint16_t usedIdx = getIndex(usedResUniList, combRes);
                usedResPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }

            for (const CombinedResID combRes : rwRes.readCombinedRes)
            {
                uint16_t usedIdx = getIndex(usedResUniList, combRes);
                usedResPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }
        }

        std::vector<CombinedResID> readonlyResUniList;
        for (const CombinedResID readRes : readResUniList)
        {
            if (kInvalidIndex == getIndex(writeResUniList, readRes)) {
                continue;
            }
            readonlyResUniList.push_back(readRes);
        }

        // remove read-only resource from usedResUniList
        for (const CombinedResID readonlyRes : readonlyResUniList)
        {
            uint16_t idx = getIndex(usedResUniList, readonlyRes);
            if (kInvalidIndex == idx) {
                continue;
            }
            usedResUniList.erase(usedResUniList.begin() + idx);
        }
        
        // remove multi-frame resource from usedResUniList
        const std::vector<CombinedResID> multiFrameResUniList = m_multiFrame_res;
        for (const CombinedResID multiFrameRes : multiFrameResUniList)
        {
            uint16_t idx = getIndex(usedResUniList, multiFrameRes);
            if (kInvalidIndex == idx) {
                continue;
            }
            usedResUniList.erase(usedResUniList.begin() + idx);
        }

        assert(usedResPassIdxByOrder.size() == usedResUniList.size());

        std::vector < ResLifetime > resLifeTime;
        // only transient resource would be in the lifetime list
        for (uint16_t ii = 0; ii < usedResPassIdxByOrder.size(); ++ii)
        {
            const std::vector<uint16_t>& passIdxByOrder = usedResPassIdxByOrder[ii];

            assert(!passIdxByOrder.empty());

            uint16_t maxIdx = *std::max_element(begin(passIdxByOrder), end(passIdxByOrder));
            uint16_t minIdx = *std::min_element(begin(passIdxByOrder), end(passIdxByOrder));

            uint16_t lasting = maxIdx - minIdx;
            resLifeTime.push_back({ minIdx, lasting });
        }

        // set the value
        m_usedResUniList = usedResUniList;
        m_readResUniList = readResUniList;
        m_writeResUniList = writeResUniList;

        m_resLifeTime = resLifeTime;
    }

    void Framegraph2::fillResourceBuckets()
    {

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

    }

    bool Framegraph2::isBufferAliasable(uint16_t _idx0, uint16_t _idx1) const
    {
        //

        return false;
    }

    bool Framegraph2::isTextureAliasable(uint16_t _idx0, uint16_t _idx1) const
    {
        return false;
    }

    bool Framegraph2::isAlisable(const CombinedResID& _r0, const CombinedResID& _r1)
    {
        if (_r0.type != _r1.type)
        {
            return false;
        }

        if (_r0.type == ResourceType::Buffer)
        {
            return isBufferAliasable(_r0.idx, _r1.idx);
        }
        if (_r0.type == ResourceType::Texture)
        {
            return isTextureAliasable(_r0.idx, _r1.idx);
        }
        /*
        if (_r0.type == ResourceType::RenderTarget)
        {
            return false;
        }
        if (_r0.type == ResourceType::DepthStencil)
        {
            return false;
        }
        */

        return false;
    }

} // namespace vkz
