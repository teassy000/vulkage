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
                aliasRes(reader, ResourceType::Buffer);
                break;
            }
            case MagicTag::AliasTexture: {
                aliasRes(reader, ResourceType::Texture);
                break;
            }
            case MagicTag::AliasRenderTarget: {
                aliasRes(reader, ResourceType::RenderTarget);
                break;
            }
            case MagicTag::AliasDepthStencil: {
                aliasRes(reader, ResourceType::DepthStencil);
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
    const uint16_t getIndex(const std::vector<T>& _vec, T _data)
    {
        auto it = std::find(begin(_vec), end(_vec), _data);
        if (it == end(_vec))
        {
            return kInvalidIndex;
        }

        return (uint16_t)std::distance(begin(_vec), it);
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

        CombinedResID plainResIdx = getPlainResourceID(info.idx, ResourceType::Buffer);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerTexture(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hTex.push_back({ info.idx });
        m_tex_info.push_back(info);

        CombinedResID plainResIdx = getPlainResourceID(info.idx, ResourceType::Texture);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerRenderTarget(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hRT.push_back({ info.idx });
        m_rt_info.push_back(info);

        CombinedResID plainResIdx = getPlainResourceID(info.idx, ResourceType::RenderTarget);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerDepthStencil(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hDS.push_back({ info.idx });
        m_ds_info.push_back(info);

        CombinedResID plainResIdx = getPlainResourceID(info.idx, ResourceType::DepthStencil);
        m_plain_resource_idx.push_back(plainResIdx);
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
            CombinedResID plainIdx = getPlainResourceID(resIdx, _type);
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
            CombinedResID plainIdx = getPlainResourceID(resIdx, _type);
            m_pass_rw_res[idx].writeCombinedRes.push_back(plainIdx);
        }

        delete[] resArr;
        resArr = nullptr;
    }

    void Framegraph2::aliasRes(MemoryReader& _reader, ResourceType _type)
    {
        ResAliasInfo info;
        read(&_reader, info);

        uint16_t* resArr = new uint16_t[info.aliasNum];
        read(&_reader, resArr, sizeof(uint16_t) * info.aliasNum);

        CombinedResID plainBaseIdx = getPlainResourceID(info.resBase, _type);
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

        m_resultRT = getPlainResourceID(rt, ResourceType::RenderTarget);
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
        for (uint16_t passIdx = 0; passIdx != m_sortedPassIdx.size(); ++passIdx)
        {
            const uint16_t pIdx = passIdx - 1;
            const PassInDependLevel& dpLv = m_passIdxInDpLevels[pIdx];
            
            
            if (dpLv.passInLv.empty() || dpLv.passInLv[0].empty())
                continue;

            // only level 0 would be sync
            m_passIdxToSync[passIdx].insert(m_passIdxToSync[passIdx].end(), dpLv.passInLv[0].begin(), dpLv.passInLv[0].end());
        }
    }

    void Framegraph2::optimizeSync()
    {
        // calc max level list
        std::vector<uint16_t> maxLvLst(m_sortedPassIdx.size(), 0);
        buildMaxLevelList(maxLvLst);

        // build dependency level
        formatDependency(maxLvLst);

        // init the nearest sync pass
        fillNearestSyncPass();

        // optimize sync pass
        optimizeSyncPass();
    }


} // namespace vkz
