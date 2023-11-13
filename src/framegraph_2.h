#pragma once

#include "memory_operation.h"

namespace vkz
{
    typedef uint32_t DependLevel;

    enum class MagicTag : uint32_t
    {
        InvalidMagic = 0x00000000,

        RegisterPass = 0x00000001,
        RegisterBuffer = 0x00000002,
        RegisterTexture = 0x00000003,
        RegisterRenderTarget = 0x00000004,
        RegisterDepthStencil = 0x00000005,

        PassReadBuffer = 0x00000006,
        PassWriteBuffer = 0x00000007,
        PassReadTexture = 0x00000008,
        PassWriteTexture = 0x00000009,
        PassReadRenderTarget = 0x0000000A,
        PassWriteRenderTarget = 0x0000000B,
        PassReadDepthStencil = 0x0000000C,
        PassWriteDepthStencil = 0x0000000D,

        AliasBuffer = 0x0000000E,
        AliasTexture = 0x0000000F,
        AliasRenderTarget = 0x00000010,
        AliasDepthStencil = 0x00000011,

        End = 0x00000012,
    };

    enum class RessourceType : uint16_t
    {
        Buffer          = 1 << 0,
        Texture         = 1 << 1,
        RenderTarget    = 1 << 2,
        DepthStencil    = 1 << 3,
    };

    // Register Info
    struct PassRegisterInfo
    {
        uint16_t     idx;
        PassExeQueue queue;
    };

    struct FGBarrierState
    {
        uint32_t    layout;
        uint32_t    stage;
        uint32_t    access;
    };

    struct BufRegisterInfo : public FGBarrierState
    {
        uint16_t    idx;
        uint16_t    padding;
        
        uint32_t    size;
    };

    struct ImgRegisterInfo : public FGBarrierState
    {
        uint16_t    idx;

        uint16_t    mips{ 1u };

        uint32_t    x{ 0u };
        uint32_t    y{ 0u };
        uint32_t    z{ 1u };

        uint32_t    format;
        uint32_t    usage;
        uint32_t    type{ VK_IMAGE_TYPE_2D };
        uint32_t    viewType{ VK_IMAGE_VIEW_TYPE_2D };
    };

    // Read/Write Info
    struct PassRWInfo
    {
        uint16_t    pass;
        uint16_t    resNum;
    };

    // Alias Info
    struct ResAliasInfo
    {
        uint16_t    resBase;
        uint16_t    aliasNum;
    };

    // for sort
    struct PassRWResource
    {
        uint16_t passIdx;
        std::vector<uint32_t> readPlainRes;
        std::vector<uint32_t> writePlainRes;
    };

    struct PassRenderData
    {
        uint16_t        _idx;
        PassExeQueue _queue{ PassExeQueue::Graphics };
        std::vector<uint32_t> _resources;
        
        // constants

        // resource state in current pass
        std::unordered_map<uint32_t, FGBarrierState>     _resourceState;
        // optimal pass to sync with, no redundant sync
        std::vector<uint32_t>                            _passToSync; 

        std::vector<uint32_t>         _parentPassIDs;
        std::vector<uint32_t>         _childPassIDs;

        std::vector<uint32_t>     _inputResIDs;
        std::vector<uint32_t>     _outputResIDs;
    };

    struct BufBucket
    {
        uint32_t        idx;
        
        size_t          size;

        std::vector<uint32_t> _reses;
        std::vector<uint32_t> _forceAliasedReses;
    };

    struct ImgBucket
    {
        uint32_t        idx;

        uint32_t        x{ 0u };
        uint32_t        y{ 0u };
        uint32_t        z{ 1u };
        uint16_t        mips{ 1u };

        std::vector<uint32_t> _reses;
        std::vector<uint32_t> _forceAliasedReses;
    };

    class Framegraph2
    {
    public:
        void build();
        void execute(); // execute passes


    private:
        void parseOperations();

        void registerPass(MemoryReader& _reader);
        void registerBuffer(MemoryReader& _reader);
        void registerTexture(MemoryReader& _reader);
        void registerRenderTarget(MemoryReader& _reader);
        void registerDepthStencil(MemoryReader& _reader);

        void passReadRes(MemoryReader& _reader, RessourceType _type);
        void passWriteRes(MemoryReader& _reader, RessourceType _type);

        void aliasBuffer(MemoryReader& _reader);
        void aliasImage(MemoryReader& _reader);
        void aliasRenderTargets(MemoryReader& _reader);
        void aliasDepthStencil(MemoryReader& _reader);


    private:
        void sort(); // reverse dfs
        void optimize(); // alias and sync 
        void pre_execute(); // allocate resource and prepare passes, maybe merge passes, detect transist resources

        void reverseDFSVisit(const uint16_t currPass, std::vector<uint16_t>& sortedPasses);
        void reverseTraversalDFS();
        void buildDenpendencyLevel();
        void buildResourceLifetime();
        void buildResourceBuckets();

        void optimize_sync();
        void optimize_alias();
        void allocate_resources();

    public:
        inline void setFinalPass(uint32_t p)
        {
            m_finalPass = p;
        }

        inline PassRenderData& getPassData(uint32_t p)
        {
            return m_passData[p];
        }

        inline void setMemoryBlock(MemoryBlockI* _memBlock)
        {
            m_pMemBlock = _memBlock;
        }

    private:
        std::vector<uint16_t> m_pass_idx;
        std::vector<uint16_t> m_buf_idx;
        std::vector<uint16_t> m_img_idx;
        std::vector<uint16_t> m_rt_idx;
        std::vector<uint16_t> m_ds_idx;

        std::vector<PassRWResource> m_pass_rw_res;

        std::vector<uint32_t> m_plain_resource_idx;

        PassRegisterInfo  m_pass_info[kMaxNumOfBufferHandle];
        BufRegisterInfo   m_buf_info[kMaxNumOfTextureHandle];
        ImgRegisterInfo   m_img_info[kMaxNumOfRenderTargetHandle];
        ImgRegisterInfo   m_rt_info[kMaxNumOfDepthStencilHandle];
        ImgRegisterInfo   m_ds_info[kMaxNumOfPassHandle];

        std::vector<PassRenderData> m_passData;

        // sorted and cut passes
        std::vector<uint32_t> m_sortedPass;

        uint32_t            m_finalPass;

        MemoryBlockI*        m_pMemBlock;
    };


}; // namespace vkz