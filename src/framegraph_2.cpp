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
    void Framegraph2::build()
    {
        parseOperations();

        buildDependency();
        reverseTraversalDFS();
    }

    void Framegraph2::parseOperations()
    {
        assert(m_pMemBlock != nullptr);
        MemoryReader reader(m_pMemBlock->expand(0), m_pMemBlock->size());

        while (true)
        {
            MagicTag magic = MagicTag::InvalidMagic;
            read(&reader, magic);

            bool finish = false;
            switch (magic)
            {
            // check
            
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
                aliasRes(reader, RessourceType::Buffer);
                break;
            }
            case MagicTag::AliasTexture: {
                aliasRes(reader, RessourceType::Texture);
                break;
            }
            case MagicTag::AliasRenderTarget: {
                aliasRes(reader, RessourceType::RenderTarget);
                break;
            }
            case MagicTag::AliasDepthStencil: {
                aliasRes(reader, RessourceType::DepthStencil);
                break;
            }
            // Read
            case MagicTag::PassReadBuffer: {
                passReadRes(reader, RessourceType::Buffer);
                break;
            }
            case MagicTag::PassReadTexture: {
                passReadRes(reader, RessourceType::Texture);
                break;
            }
            case MagicTag::PassReadRenderTarget: {
                passReadRes(reader, RessourceType::RenderTarget);
                break;
            }
            case MagicTag::PassReadDepthStencil: {
                passReadRes(reader, RessourceType::DepthStencil);
                break;
            }
            // Write
            case MagicTag::PassWriteBuffer: {
                passWriteRes(reader, RessourceType::Buffer);
                break;
            }
            case MagicTag::PassWriteTexture: {
                passWriteRes(reader, RessourceType::Texture);
                break;
            }
            case MagicTag::PassWriteRenderTarget: {
                passWriteRes(reader, RessourceType::RenderTarget);
                break;
            }
            case MagicTag::PassWriteDepthStencil: {
                passWriteRes(reader, RessourceType::DepthStencil);
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
                finish = true;
                break;
            }
            
            if (finish)
            {
                break;
            }
        }
    }

    constexpr int32_t kInvalidIndex = 0xFFFFFFFF;
    template<typename T>
    int32_t getIndex(const std::vector<T>& _vec, T _data)
    {
        auto it = std::find(begin(_vec), end(_vec), _data);
        if (it == end(_vec))
        {
            return kInvalidIndex;
        }

        return (int32_t)std::distance(begin(_vec), it);
    }

    void Framegraph2::registerPass(MemoryReader& _reader)
    {
        PassRegisterInfo info;
        read(&_reader, info);

        m_hPass.push_back({ info.idx });
        m2_pass_info.push_back(info);

        m_pass_rw_res.push_back({});
        m_pass_dependency.push_back({});
    }

    void Framegraph2::registerBuffer(MemoryReader& _reader)
    {
        BufRegisterInfo info;
        read(&_reader, info);

        m_hBuf.push_back({ info.idx });
        m2_buf_info.push_back(info);

        CombinedResID plainResIdx = getPlainResourceID(info.idx, RessourceType::Buffer);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerTexture(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hTex.push_back({ info.idx });
        m2_tex_info.push_back(info);

        CombinedResID plainResIdx = getPlainResourceID(info.idx, RessourceType::Texture);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerRenderTarget(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hRT.push_back({ info.idx });
        m2_rt_info.push_back(info);

        CombinedResID plainResIdx = getPlainResourceID(info.idx, RessourceType::RenderTarget);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerDepthStencil(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_hDS.push_back({ info.idx });
        m2_ds_info.push_back(info);

        CombinedResID plainResIdx = getPlainResourceID(info.idx, RessourceType::DepthStencil);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::passReadRes(MemoryReader& _reader, RessourceType _type)
    {
        PassRWInfo info;
        read(&_reader, info);
        alloc(sizeof(uint16_t) * info.resNum);
        
        uint16_t* resArr = new uint16_t[info.resNum];
        read(&_reader, resArr, sizeof(uint16_t) * info.resNum);

        for (int16_t ii = 0; ii < info.resNum; ++ii)
        {
            int32_t idx = getIndex(m_hPass, { info.pass });
            assert(idx != kInvalidIndex);
            
            uint16_t resIdx = resArr[ii];
            CombinedResID plainIdx = getPlainResourceID(resIdx, _type);
            m_pass_rw_res[idx].readCombinedRes.push_back(plainIdx);
        }

        delete[] resArr;
        resArr = nullptr;
    }

    void Framegraph2::passWriteRes(MemoryReader& _reader, RessourceType _type)
    {
        PassRWInfo info;
        read(&_reader, info);

        uint16_t* resArr = new uint16_t[info.resNum];
        read(&_reader, resArr, sizeof(uint16_t) * info.resNum);

        for (int16_t ii = 0; ii < info.resNum; ++ii)
        {
            int32_t idx = getIndex(m_hPass, { info.pass });
            assert(idx != kInvalidIndex);
            
            uint16_t resIdx = resArr[ii];
            CombinedResID plainIdx = getPlainResourceID(resIdx, _type);
            m_pass_rw_res[idx].writeCombinedRes.push_back(plainIdx);
        }

        delete[] resArr;
        resArr = nullptr;
    }

    void Framegraph2::aliasRes(MemoryReader& _reader, RessourceType _type)
    {
        ResAliasInfo info;
        read(&_reader, info);

        uint16_t* resArr = new uint16_t[info.aliasNum];
        read(&_reader, resArr, sizeof(uint16_t) * info.aliasNum);

        CombinedResID plainBaseIdx = getPlainResourceID(info.resBase, _type);
        int32_t idx = getIndex(m_plain_force_alias_base, plainBaseIdx);
        if (kInvalidIndex == idx)
        {
            m_plain_force_alias_base.push_back(plainBaseIdx);
            std::unordered_set<uint16_t> aliasSet(resArr, resArr + info.aliasNum);
            m_plain_force_alias.push_back(aliasSet);
        }
        else // found the base
        {
            for (int16_t ii = 0; ii < info.aliasNum; ++ii)
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
        int32_t idx = getIndex(m_hRT, { rt });
        if (kInvalidIndex == idx)
        {
            message(DebugMessageType::error, "result rt is not registered!");
            return;
        }

        m_resultRT = RenderTargetHandle{ rt };
    }


    void Framegraph2::buildDependency()
    {
        // make a plain table that can find producer of a resource easily.
        std::vector<CombinedResID> linear_outResCID;
        std::vector<uint16_t> linear_outPassIdx;

        for (uint16_t ii = 0; ii < m_hPass.size(); ++ii)
        {
            PassHandle currPass = m_hPass[ii];
            PassRWResource rwRes = m_pass_rw_res[ii];

            for (CombinedResID CombindRes : rwRes.writeCombinedRes)
            {
                linear_outResCID.push_back(CombindRes);
                linear_outPassIdx.push_back(ii); // write by current pass
            }
        }

        for (uint16_t ii = 0; ii < m_hPass.size(); ++ii)
        {
            PassRWResource rwRes = m_pass_rw_res[ii];

            for (CombinedResID plainRes : rwRes.readCombinedRes)
            {
                int32_t idx = getIndex(linear_outResCID, plainRes);
                if (kInvalidIndex == idx) {
                    continue;
                }


                int16_t currPassIdx = ii;
                
                uint16_t writePassIdx = linear_outPassIdx[idx];

                m_pass_dependency[currPassIdx].inPassIdx.push_back(writePassIdx);
                m_pass_dependency[writePassIdx].outPassIdx.push_back(currPassIdx);
            }
        }
    }

    void Framegraph2::reverseDFSVisit(const PassHandle _currPass, std::vector<PassHandle>& _sortedPasses)
    {

        
    }

    void Framegraph2::reverseTraversalDFS()
    {
        uint16_t passNum = (uint16_t)m_hPass.size();

        std::vector<bool> visited(passNum, false);
        std::vector<bool> onStack(passNum, false);



        std::vector<PassHandle> sortedPass;


        std::stack<uint16_t> passStack;
       

    }

    Framegraph2::CombinedResID Framegraph2::getPlainResourceID(const uint16_t _resIdx, const RessourceType _resType)
    {
        CombinedResID handle;
        handle.idx = _resIdx;
        handle.type = _resType;

        return handle;
    }

} // namespace vkz
