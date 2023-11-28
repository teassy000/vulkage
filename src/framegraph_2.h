#pragma once

#include "memory_operation.h"
#include <unordered_set>

namespace vkz
{
    typedef uint32_t DependLevel;

    enum class MagicTag : uint32_t
    {
        InvalidMagic = 0x00000000,

        SetBrief,

        RegisterPass,
        RegisterBuffer,
        RegisterTexture,
        RegisterRenderTarget,
        RegisterDepthStencil,

        PassReadBuffer,
        PassWriteBuffer,
        PassReadTexture,
        PassWriteTexture,
        PassReadRenderTarget,
        PassWriteRenderTarget,
        PassReadDepthStencil,
        PassWriteDepthStencil,

        AliasBuffer,
        AliasTexture,
        AliasRenderTarget,
        AliasDepthStencil,

        SetMuitiFrameBuffer,
        SetMuitiFrameTexture,
        SetMultiFrameRenderTarget,
        SetMultiFrameDepthStencil,

        SetResultRenderTarget,

        End,
    };

    enum class ResourceType : uint16_t
    {
        Buffer = 0,
        Texture,
        RenderTarget,
        DepthStencil,
    };

    struct FrameGraphBrief
    {
        uint32_t    version;
        uint32_t    passNum;
        uint32_t    bufNum;
        uint32_t    imgNum;
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
        uint16_t    bpp{ 4u };

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

    class Framegraph2
    {
    public:
        void bake();
        void execute(); // execute passes

    public:

        inline PassRenderData& getPassData(uint32_t p)
        {
            return m_passData[p];
        }

        inline void setMemoryBlock(MemoryBlockI* _memBlock)
        {
            m_pMemBlock = _memBlock;
        }

        inline void setBrief(MemoryBlock* _briefBlock)
        {
            m_pBirefMemBlock = _briefBlock;
        }

    private:
        void parseOp();

        // ==============================
        // process operations 
        void setBrief(MemoryReader& _reader);
        void registerPass(MemoryReader& _reader);
        void registerBuffer(MemoryReader& _reader);
        void registerTexture(MemoryReader& _reader);
        void registerRenderTarget(MemoryReader& _reader);
        void registerDepthStencil(MemoryReader& _reader);

        void passReadRes(MemoryReader& _reader, ResourceType _type);
        void passWriteRes(MemoryReader& _reader, ResourceType _type);

        void aliasResForce(MemoryReader& _reader, ResourceType _type);

        void setMultiFrameRes(MemoryReader& _reader, ResourceType _type);

        void setResultRT(MemoryReader& _reader);

        // =======================================
        void buildGraph();
        void reverseTraversalDFS();
        void buildMaxLevelList(std::vector<uint16_t>& _maxLvLst);
        void formatDependency(const std::vector<uint16_t>& _maxLvLst);
        void fillNearestSyncPass();
        void optimizeSyncPass();

        // =======================================
        void postParse();
        void postParseMultiFrameRes();

        void buildResLifetime();

        // =======================================
        void optimizeSync();
        void optimizeAlias();
        void allocate_resources();

    private:
        union CombinedResID
        {
            struct {
                uint16_t        id;
                ResourceType    type;
            };

            uint32_t combined;

            bool operator == (const CombinedResID& rhs) const {
                return combined == rhs.combined;
            }
        };

        struct BufBucket
        {
            uint32_t        idx;

            size_t          size;

            bool            forceAliased{ false };

            std::vector<CombinedResID> reses;
        };

        struct ImgBucket
        {
            uint32_t        idx;

            uint16_t    mips{ 1u };

            uint32_t    x{ 0u };
            uint32_t    y{ 0u };
            uint32_t    z{ 1u };

            uint32_t    format;
            uint32_t    usage;
            uint32_t    type{ VK_IMAGE_TYPE_2D };

            bool        forceAliased{ false };

            std::vector<CombinedResID> reses;
        };

        inline CombinedResID getCombinedResID(const uint16_t _resIdx, const ResourceType _resType) const
        {
            CombinedResID handle;
            handle.id = _resIdx;
            handle.type = _resType;

            return handle;
        }

        bool isBufInfoAliasable(uint16_t _idx, const BufBucket& _bucket, const std::vector<CombinedResID> _resInCurrStack) const;
        bool isImgInfoAliasable(uint16_t _idx, const ImgBucket& _bucket, const std::vector<CombinedResID> _resInCurrStack) const;

        bool isStackAliasable(const CombinedResID& _res, const std::vector<CombinedResID>& _reses) const;

        bool isAliasable(const CombinedResID& _res, const BufBucket& _bucket, const std::vector<CombinedResID>& _reses) const;
        bool isAliasable(const CombinedResID& _res, const ImgBucket& _bucket, const std::vector<CombinedResID>& _reses) const;

        void fillBucketForceAlias();
        void fillBucketReadonly();
        void fillBucketMultiFrame();

        void createBufBkt(BufBucket& _bkt, const BufRegisterInfo& _info, const std::vector<CombinedResID>& _res, const bool _forceAliased = false);
        void createImgBkt(ImgBucket& _bkt, const ImgRegisterInfo& _info, const std::vector<CombinedResID>& _res, const bool _forceAliased = false);

        void aliasBuffers(const std::vector<uint16_t>& _sortedBufList);
        void aliasImages(std::vector<ImgBucket>& _buckets, const std::vector< ImgRegisterInfo >& _infos, const std::vector<uint16_t>& _sortedTexList, const ResourceType _type);

        void fillBufferBuckets();
        void fillImageBuckets();


        // for sort
        struct PassRWResource
        {
            std::vector<CombinedResID> readCombinedRes;
            std::vector<CombinedResID> writeCombinedRes;
        };

        struct PassDependency
        {
            std::vector<uint16_t> inPassIdxLst;
            std::vector<uint16_t> outPassIdxLst;
        };

        // for optimize
        struct PassInDependLevel
        {
            std::vector<std::vector<uint16_t>> passInLv;
        };

        struct PassToSync
        {
            std::vector<uint16_t> passToSync;
        };

        struct ResLifetime
        {
            uint16_t    startIdx; // the index in m_sortedPassIdx
            uint16_t    endIdx;  // how many passes it would last
        };

    private:
        MemoryBlockI* m_pMemBlock;
        MemoryBlockI* m_pBirefMemBlock;

        CombinedResID  m_resultRT;
        PassHandle     m_finalPass;

        std::vector< PassHandle         >   m_hPass;
        std::vector< BufferHandle       >   m_hBuf;
        std::vector< TextureHandle      >   m_hTex;
        std::vector< RenderTargetHandle >   m_hRT;
        std::vector< DepthStencilHandle >   m_hDS;

        std::vector< PassRegisterInfo>  m_pass_info;
        std::vector< BufRegisterInfo >  m_buf_info;
        std::vector< ImgRegisterInfo >  m_img_info;
        std::vector< ImgRegisterInfo >  m_rt_info;
        std::vector< ImgRegisterInfo >  m_ds_info;
                     
        std::vector< CombinedResID>     m_combinedResId;

        std::vector< PassRWResource>                m_pass_rw_res;
        std::vector< PassDependency>                m_pass_dependency;
        std::vector< CombinedResID>                 m_combinedForceAlias_base;
        std::vector< std::vector<CombinedResID>>    m_combinedForceAlias;

        std::vector< PassRenderData> m_passData;

        // sorted and cut passes
        std::vector< PassHandle>    m_sortedPass;
        std::vector< uint16_t>      m_sortedPassIdx;

        std::vector< PassInDependLevel>          m_passIdxInDpLevels;

        std::vector< std::vector<uint16_t>>      m_passIdxToSync;

        // =====================
        // lv0: each queue
        // lv1: passes in queue
        std::array< std::vector<uint16_t>, (uint16_t)PassExeQueue::Count >    m_passIdxInQueue; 

        // =====================
        // lv0 index: each pass would use
        // lv1 index: one pass in each queue
        std::vector< std::array<uint16_t, (uint16_t)PassExeQueue::Count> >    m_nearestSyncPassIdx;

        std::vector< CombinedResID>     m_multiFrame_resList;
        std::vector< CombinedResID>     m_readonly_resList;

        std::vector< CombinedResID>     m_resInUseUniList;
        std::vector< CombinedResID>     m_resToOptmUniList;

        std::vector< CombinedResID>     m_readResUniList;
        std::vector< CombinedResID>     m_writeResUniList;
        std::vector< ResLifetime>       m_resLifeTime;

        std::vector< CombinedResID>      m_plainAliasRes;
        std::vector< uint16_t>           m_plainAliasResIdx;

        std::vector< BufBucket>          m_bufBuckets;
        std::vector< ImgBucket>          m_imgBuckets;
    };


}; // namespace vkz