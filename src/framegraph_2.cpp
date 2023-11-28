#include "common.h"
#include "resources.h"
#include "config.h"

#include "memory_operation.h"
#include "handle.h"
#include "vkz.h"
#include "framegraph_2.h"
#include <stack>
#include <algorithm>



namespace vkz
{
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
            // Brief
            case MagicTag::SetBrief: {
                setBrief(reader);
                break;
            }
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


    void Framegraph2::setBrief(MemoryReader& _reader)
    {
        FrameGraphBrief brief;
        read(&_reader, brief);

        m_pass_info.resize(brief.passNum);
        m_buf_info.resize(brief.bufNum);
        m_img_info.resize(brief.imgNum);
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
        m_pass_info[info.idx] = info;

        m_pass_rw_res.emplace_back();
        m_pass_dependency.emplace_back();
        m_passIdxToSync.emplace_back();
    }

    void Framegraph2::registerBuffer(MemoryReader& _reader)
    {
        BufRegisterInfo info;
        read(&_reader, info);

        m_hBuf.push_back({ info.idx });
        m_buf_info[info.idx] = info;

        CombinedResID plainResID = getCombinedResID(info.idx, ResourceType::Buffer);
        m_combinedResId.push_back(plainResID);
    }

    void Framegraph2::registerTexture(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hTex.push_back({ info.idx });
        m_img_info[info.idx] = info;

        CombinedResID plainResIdx = getCombinedResID(info.idx, ResourceType::Texture);
        m_combinedResId.push_back(plainResIdx);
    }

    void Framegraph2::registerRenderTarget(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hTex.push_back({ info.idx });
        m_img_info[info.idx] = info;

        CombinedResID plainResIdx = getCombinedResID(info.idx, ResourceType::RenderTarget);
        m_combinedResId.push_back(plainResIdx);
    }

    void Framegraph2::registerDepthStencil(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hTex.push_back({ info.idx });
        m_img_info[info.idx] = info;

        CombinedResID plainResIdx = getCombinedResID(info.idx, ResourceType::DepthStencil);
        m_combinedResId.push_back(plainResIdx);
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

        CombinedResID combinedBaseIdx = getCombinedResID(info.resBase, _type);
        uint16_t idx = getIndex(m_combinedForceAlias_base, combinedBaseIdx);
        if (kInvalidIndex == idx)
        {
            m_combinedForceAlias_base.push_back(combinedBaseIdx);
            std::vector<CombinedResID> aliasVec({ combinedBaseIdx }); // store base resource!

            m_combinedForceAlias.push_back(aliasVec);
            idx = (uint16_t)m_combinedForceAlias.size() - 1;
        }

        for (uint16_t ii = 0; ii < info.aliasNum; ++ii)
        {
            uint16_t resIdx = resArr[ii];
            CombinedResID combinedIdx = getCombinedResID(resIdx, _type);
            push_back_unique(m_combinedForceAlias[idx], combinedIdx);
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
            m_multiFrame_resList.push_back(plainIdx);
        }
    }

    void Framegraph2::setResultRT(MemoryReader& _reader)
    {
        uint16_t rt;
        read(&_reader, rt);

        // check if rt is ready resisted
        uint16_t idx = getIndex(m_hTex, { rt });
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
        }

        std::vector<CombinedResID> fullMultiFrameRes = m_multiFrame_resList;
        for (const CombinedResID cid : m_multiFrame_resList)
        {
            const uint16_t idx = getIndex(plainAliasRes, cid);
            if (kInvalidHandle == idx)
            {
                continue;
            }

            const uint16_t alisIdx = plainAliasResIdx[idx];
            CombinedResID base = m_combinedForceAlias_base[alisIdx];

            const std::vector<CombinedResID>& aliasVec = m_combinedForceAlias[alisIdx];
            for (CombinedResID alias : aliasVec)
            {
                push_back_unique(fullMultiFrameRes, alias);
            }
        }

        m_plainAliasRes = plainAliasRes;
        m_plainAliasResIdx = plainAliasResIdx;
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
                uint16_t usedIdx = getIndex(resToOptmUniList, combRes);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
            }

            for (const CombinedResID combRes : rwRes.readCombinedRes)
            {
                uint16_t usedIdx = getIndex(resToOptmUniList, combRes);
                resToOptmPassIdxByOrder[usedIdx].push_back(ii); // store the idx in ordered pass
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

        // remove read-only resource from resToOptmUniList
        for (const CombinedResID readonlyRes : readonlyResUniList)
        {
            uint16_t idx = getIndex(resToOptmUniList, readonlyRes);
            if (kInvalidIndex == idx) {
                continue;
            }
            resToOptmUniList.erase(resToOptmUniList.begin() + idx);
            resToOptmPassIdxByOrder.erase(resToOptmPassIdxByOrder.begin() + idx);
        }

        // remove force alias resource from resToOptmUniList
        const std::vector<CombinedResID> plainAliasRes = m_plainAliasRes;
        for (const CombinedResID plainAlias : plainAliasRes)
        {
            uint16_t idx = getIndex(resToOptmUniList, plainAlias);
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
            uint16_t idx = getIndex(resToOptmUniList, multiFrameRes);
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
            const uint16_t idx = getIndex(m_plainAliasRes, combRes);
            if (kInvalidIndex == idx)
            {
                continue;
            }

            const uint16_t alisIdx = m_plainAliasResIdx[idx];
            CombinedResID base = m_combinedForceAlias_base[alisIdx];

            uint16_t usedBaseIdx = push_back_unique(actualAliasBase, base);
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
            if (base.type == ResourceType::Buffer) 
            {
                const BufRegisterInfo info = m_buf_info[base.id];

                BufBucket bucket;
                createBufBkt(bucket, info, aliasVec, true);
                m_bufBuckets.push_back(bucket);
            }

            // textures
            if (base.type == ResourceType::Texture 
                || base.type == ResourceType::RenderTarget
                || base.type == ResourceType::DepthStencil)
            {
                const ImgRegisterInfo info = m_img_info[base.id];

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
            if (cid.type == ResourceType::Buffer)
            {
                const BufRegisterInfo info = m_buf_info[cid.id];

                BufBucket bucket;
                createBufBkt(bucket, info, {cid});
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (cid.type == ResourceType::Texture
                || cid.type == ResourceType::RenderTarget
                || cid.type == ResourceType::DepthStencil)
            {
                const ImgRegisterInfo info = m_img_info[cid.id];

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
            if (cid.type == ResourceType::Buffer)
            {
                const BufRegisterInfo info = m_buf_info[cid.id];

                BufBucket bucket;
                createBufBkt(bucket, info, { cid });
                m_bufBuckets.push_back(bucket);
            }

            // images
            if (cid.type == ResourceType::Texture
                || cid.type == ResourceType::RenderTarget
                || cid.type == ResourceType::DepthStencil)
            {
                const ImgRegisterInfo info = m_img_info[cid.id];

                ImgBucket bucket;
                createImgBkt(bucket, info, { cid });
                m_imgBuckets.push_back(bucket);
            }
        }
    }

    void Framegraph2::createBufBkt(BufBucket& _bkt, const BufRegisterInfo& _info, const std::vector<CombinedResID>& _reses, const bool _forceAliased /*= false*/)
    {
        _bkt.size = _info.size;
        _bkt.reses = _reses;
        _bkt.forceAliased = _forceAliased;
    }

    void Framegraph2::createImgBkt(ImgBucket& _bkt, const ImgRegisterInfo& _info, const std::vector<CombinedResID>& _reses, const bool _forceAliased /*= false*/)
    {
        _bkt.mips = _info.mips;
        _bkt.x = _info.x;
        _bkt.y = _info.y;
        _bkt.z = _info.z;
        _bkt.format = _info.format;
        _bkt.usage = _info.usage;
        _bkt.type = _info.type;

        _bkt.reses = _reses;
        _bkt.forceAliased = _forceAliased;
    }

    void Framegraph2::aliasBuffers(const std::vector<uint16_t>& _sortedBufList)
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
            const CombinedResID baseCid = getCombinedResID(baseRes, ResourceType::Buffer);

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
                    uint16_t idx = getIndex(restRes, res.id);
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
            const BufRegisterInfo& info = m_buf_info[baseRes];
            createBufBkt(bucket, info, { baseCid });
            buckets.push_back(bucket);

            restRes.erase(begin(restRes));

            resInLevel.clear();
            resInLevel.emplace_back();
        }

        m_bufBuckets.insert(end(m_bufBuckets), begin(buckets), end(buckets));
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
            const CombinedResID baseCid = getCombinedResID(baseRes, _type);

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
                    uint16_t idx = getIndex(restRes, res.id);
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
            if (crid.type != ResourceType::Buffer) {
                continue;
            }
            sortedBufIdx.push_back(crid.id);
        }

        std::sort(begin(sortedBufIdx), end(sortedBufIdx), [&](uint16_t _l, uint16_t _r) {
            return m_buf_info[_l].size > m_buf_info[_r].size;
        });

        aliasBuffers(sortedBufIdx);
    }

    void Framegraph2::fillImageBuckets()
    {
        const ResourceType type = ResourceType::Texture;

        std::vector<uint16_t> sortedTexIdx;
        for (const CombinedResID& crid : m_resToOptmUniList)
        {
            if (crid.type != ResourceType::Texture 
                && crid.type != ResourceType::RenderTarget
                && crid.type != ResourceType::DepthStencil) {
                continue;
            }
            sortedTexIdx.push_back(crid.id);
        }

        std::sort(begin(sortedTexIdx), end(sortedTexIdx), [&](uint16_t _l, uint16_t _r) {
            ImgRegisterInfo info_l = m_img_info[_l];
            ImgRegisterInfo info_r = m_img_info[_r];

            size_t size_l = info_l.x * info_l.y * info_l.z * info_l.mips * info_l.bpp;
            size_t size_r = info_r.x * info_r.y * info_r.z * info_r.mips * info_r.bpp;

            return size_l > size_r;
        });

        aliasImages(m_imgBuckets, m_img_info, sortedTexIdx, type);
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

    bool Framegraph2::isBufInfoAliasable(uint16_t _idx, const BufBucket& _bucket, const std::vector<CombinedResID> _resInCurrStack) const
    {
        bool bSzMatch = true;

        // size check in current stack
        const BufRegisterInfo& info = m_buf_info[_idx];
        size_t remaingSize = _bucket.size;
        for (const CombinedResID res : _resInCurrStack)
        {
            remaingSize -= m_buf_info[res.id].size;
        }

        bSzMatch = info.size <= remaingSize;

        return bSzMatch;
    }

    bool Framegraph2::isImgInfoAliasable(uint16_t _idx, const ImgBucket& _bucket, const std::vector<CombinedResID> _resInCurrStack) const
    {
        bool bCondMatch = true;

        // condition check
        const ImgRegisterInfo& info = m_img_info[_idx];
        for (const CombinedResID res : _resInCurrStack)
        {
            const ImgRegisterInfo& stackInfo = m_img_info[res.id];

            bCondMatch &= (info.x == stackInfo.x && info.y == stackInfo.y && info.z == stackInfo.z);
            bCondMatch &= (info.mips == stackInfo.mips);
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
        const uint16_t idx = getIndex(m_resToOptmUniList, _res);
        const ResLifetime& resLifetime = m_resLifeTime[idx];
        for (const CombinedResID res : _reses)
        {
            const uint16_t resIdx = getIndex(m_resToOptmUniList, res);
            const ResLifetime& stackLifetime = m_resLifeTime[resIdx];

            // if lifetime overlap with any resource in current bucket, then it's not alias-able
            bStackMatch &= (stackLifetime.startIdx > resLifetime.endIdx || stackLifetime.endIdx < resLifetime.startIdx);
        }

        return bStackMatch;
    }

    bool Framegraph2::isAliasable(const CombinedResID& _res, const BufBucket& _bucket, const std::vector<CombinedResID>& _resInCurrStack) const
    {
        bool bInfoMatch = isBufInfoAliasable(_res.id, _bucket, _resInCurrStack);
        bool bStackMatch = isStackAliasable(_res, _bucket.reses);

        return bInfoMatch && bStackMatch;
    }

    bool Framegraph2::isAliasable(const CombinedResID& _res, const ImgBucket& _bucket, const std::vector<CombinedResID>& _resInCurrStack) const
    {
        bool bInfoMatch = isImgInfoAliasable(_res.id, _bucket, _resInCurrStack);
        bool bStackMatch = isStackAliasable(_res, _bucket.reses);

        return bInfoMatch && bStackMatch;
    }

} // namespace vkz
