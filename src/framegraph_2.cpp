#include "common.h"
#include "resources.h"
#include "config.h"

#include "memory_operation.h"
#include "handle.h"
#include "vkz.h"
#include "framegraph_2.h"



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

        m_pass_idx.push_back(info.idx);
        m_pass_rw_res.push_back({});
        m_pass_dependency.push_back({});
        m_pass_info[info.idx].queue = info.queue;
    }

    uint32_t getPlainResourceID(const uint16_t _resIdx, const RessourceType _resType)
    {
        uint32_t low = static_cast<uint32_t>(_resIdx);
        return (low | (static_cast<uint16_t>(_resType) << 16));
    }

    void Framegraph2::registerBuffer(MemoryReader& _reader)
    {
        BufRegisterInfo info;
        read(&_reader, info);

        m_buf_idx.push_back(info.idx);
        m_buf_info[info.idx] = info;

        uint32_t plainResIdx = getPlainResourceID(info.idx, RessourceType::Buffer);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerTexture(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_img_idx.push_back(info.idx);
        m_tex_info[info.idx] = info;

        uint32_t plainResIdx = getPlainResourceID(info.idx, RessourceType::Texture);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerRenderTarget(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_rt_idx.push_back(info.idx);
        m_rt_info[info.idx] = info;

        uint32_t plainResIdx = getPlainResourceID(info.idx, RessourceType::RenderTarget);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerDepthStencil(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_ds_idx.push_back(info.idx);
        m_ds_info[info.idx] = info;

        uint32_t plainResIdx = getPlainResourceID(info.idx, RessourceType::DepthStencil);
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
            int32_t idx = getIndex(m_pass_idx, info.pass);
            assert(idx != kInvalidIndex);
            
            uint16_t resIdx = resArr[ii];
            uint32_t plainIdx = getPlainResourceID(resIdx, _type);
            m_pass_rw_res[idx].readPlainRes.push_back(plainIdx);
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
            int32_t idx = getIndex(m_pass_idx, info.pass);
            assert(idx != kInvalidIndex);
            
            uint16_t resIdx = resArr[ii];
            uint32_t plainIdx = getPlainResourceID(resIdx, _type);
            m_pass_rw_res[idx].writePlainRes.push_back(plainIdx);
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

        uint32_t plainBaseIdx = getPlainResourceID(info.resBase, _type);
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
        int32_t idx = getIndex(m_rt_idx, rt);
        if (kInvalidIndex == idx)
        {
            message(DebugMessageType::error, "result rt is not registered!");
            return;
        }

        m_resultRT = rt;
    }


    void Framegraph2::buildDependency()
    {
        // make a plain table that can find producer of a resource easily.
        std::vector<uint32_t> linear_outResID;
        std::vector<uint16_t> linear_outPassID;

        for (uint16_t currPass : m_pass_idx)
        {
            PassRWResource rwRes = m_pass_rw_res[currPass];

            for (uint32_t plainRes : rwRes.writePlainRes)
            {
                linear_outResID.push_back(plainRes);
                linear_outPassID.push_back(currPass); // write by current pass
            }
        }

        for (uint16_t currPass : m_pass_idx)
        {
            PassRWResource rwRes = m_pass_rw_res[currPass];

            for (uint32_t plainRes : rwRes.readPlainRes)
            {
                int32_t idx = getIndex(linear_outResID, plainRes);
                if (kInvalidIndex == idx) {
                    continue;
                }

                uint16_t writePass = linear_outPassID[idx];

                m_pass_dependency[currPass].inPass.push_back(writePass);
                m_pass_dependency[writePass].outPass.push_back(currPass);
            }
        }
    }

    void Framegraph2::reverseDFSVisit(const uint16_t _currPass, std::vector<uint16_t>& _sortedPasses)
    {

    }

    void Framegraph2::reverseTraversalDFS()
    {
        uint16_t passNum = (uint16_t)m_pass_idx.size();

        std::vector<bool> visited(passNum, false);
        std::vector<bool> onStack(passNum, false);

        uint16_t currPass = m_resultRT;

        std::vector<uint16_t> sortedPass;

        if (!visited[currPass])
        {
            reverseDFSVisit(currPass, sortedPass);
        }

        m_sortedPass.insert(m_sortedPass.end(), sortedPass.begin(), sortedPass.end());

    }

} // namespace vkz
