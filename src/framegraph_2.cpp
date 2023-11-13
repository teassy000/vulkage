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
                
                break;
            }
            case MagicTag::AliasTexture: {
                break;
            }
            case MagicTag::AliasRenderTarget: {
                break;
            }
            case MagicTag::AliasDepthStencil: {
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
            case MagicTag::End:
            case MagicTag::InvalidMagic:
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
        m_pass_info[info.idx].queue = info.queue;
    }

    uint32_t getPlainResourceIdx(const uint16_t _resIdx, const RessourceType _resType)
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

        uint32_t plainResIdx = getPlainResourceIdx(info.idx, RessourceType::Buffer);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerTexture(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_img_idx.push_back(info.idx);
        m_img_info[info.idx] = info;

        uint32_t plainResIdx = getPlainResourceIdx(info.idx, RessourceType::Texture);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerRenderTarget(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_rt_idx.push_back(info.idx);
        m_rt_info[info.idx] = info;

        uint32_t plainResIdx = getPlainResourceIdx(info.idx, RessourceType::RenderTarget);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::registerDepthStencil(MemoryReader& _reader)
    {
        ImgRegisterInfo info;
        read(&_reader, info);

        m_ds_idx.push_back(info.idx);
        m_ds_info[info.idx] = info;

        uint32_t plainResIdx = getPlainResourceIdx(info.idx, RessourceType::DepthStencil);
        m_plain_resource_idx.push_back(plainResIdx);
    }

    void Framegraph2::passReadRes(MemoryReader& _reader, RessourceType _type)
    {
        PassRWInfo info;
        read(&_reader, info);

        for (int16_t ii = 0; ii < info.resNum; ++ii)
        {
            uint16_t resIdx;
            read(&_reader, resIdx);

            uint32_t plainIdx = getPlainResourceIdx(resIdx, _type);

            int32_t idx = getIndex(m_pass_idx, info.pass);
            assert(idx != kInvalidIndex);
            m_pass_rw_res[idx].readPlainRes.push_back(plainIdx);
        }
    }

    void Framegraph2::passWriteRes(MemoryReader& _reader, RessourceType _type)
    {
        PassRWInfo info;
        read(&_reader, info);

        for (int16_t ii = 0; ii < info.resNum; ++ii)
        {
            uint16_t resIdx;
            read(&_reader, resIdx);

            uint32_t plainIdx = getPlainResourceIdx(resIdx, _type);

            int32_t idx = getIndex(m_pass_idx, info.pass);
            assert(idx != kInvalidIndex);
            m_pass_rw_res[idx].writePlainRes.push_back(plainIdx);
        }
    }

    void Framegraph2::aliasBuffer(MemoryReader& _reader)
    {

    }

    void Framegraph2::aliasImage(MemoryReader& _reader)
    {

    }

    void Framegraph2::aliasRenderTargets(MemoryReader& _reader)
    {

    }

    void Framegraph2::aliasDepthStencil(MemoryReader& _reader)
    {

    }
} // namespace vkz
