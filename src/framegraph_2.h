#pragma once

#include "memory_operation.h"
#include <unordered_set>

namespace vkz
{
    typedef uint32_t DependLevel;

    enum class MagicTag : uint32_t
    {
        InvalidMagic = 0x00000000,

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
        Buffer          = 1,
        Texture,
        RenderTarget,
        DepthStencil,
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

    private:
        
        void parseOp();

        // ==============================
        // process operations 
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

        // ==============================
        void buildGraph();
        void reverseTraversalDFS();
        void buildMaxLevelList(std::vector<uint16_t>& _maxLvLst);
        void formatDependency(const std::vector<uint16_t>& _maxLvLst);
        void fillNearestSyncPass();
        void optimizeSyncPass();

        void processMultiFrameRes();
        void buildResLifetime();

        void fillResourceBuckets();

        void optimizeSync();
        void optimizeAlias();
        void allocate_resources();

    private:
        union CombinedResID
        {
            struct {
                uint16_t      idx;
                ResourceType type;
            };

            uint32_t combined;

            bool operator == (const CombinedResID& rhs) const {
                return combined == rhs.combined;
            }
        };

        inline CombinedResID getCombinedResID(const uint16_t _resIdx, const ResourceType _resType) const
        {
            CombinedResID handle;
            handle.idx = _resIdx;
            handle.type = _resType;

            return handle;
        }

        inline bool isBufferAliasable(uint16_t _idx0, uint16_t _idx1) const;

        inline bool isTextureAliasable(uint16_t _idx0, uint16_t _idx1) const;

        bool isAlisable(const CombinedResID& _r0, const CombinedResID& _r1);


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
            uint16_t    lasting;  // how many passes it would last
        };

    private:
        MemoryBlockI* m_pMemBlock;

        CombinedResID  m_resultRT;
        PassHandle     m_finalPass;

        std::vector< PassHandle         >   m_hPass;
        std::vector< BufferHandle       >   m_hBuf;
        std::vector< TextureHandle      >   m_hTex;
        std::vector< RenderTargetHandle >   m_hRT;
        std::vector< DepthStencilHandle >   m_hDS;

        std::vector< PassRegisterInfo>  m_pass_info;
        std::vector< BufRegisterInfo >  m_buf_info;
        std::vector< ImgRegisterInfo >  m_tex_info;
        std::vector< ImgRegisterInfo >  m_rt_info;
        std::vector< ImgRegisterInfo >  m_ds_info;
                     
        std::vector< CombinedResID>     m_plain_resource_id;
                     
        std::vector< PassRWResource>                 m_pass_rw_res;
        std::vector< PassDependency>                 m_pass_dependency;
        std::vector< CombinedResID>                  m_plain_force_alias_base;
        std::vector< std::unordered_set<uint16_t>>   m_plain_force_alias;

        std::vector< PassRenderData> m_passData;

        // sorted and cut passes
        std::vector< PassHandle>    m_sortedPass;
        std::vector< uint16_t>      m_sortedPassIdx;

        std::vector<PassInDependLevel>          m_passIdxInDpLevels;

        std::vector<std::vector<uint16_t>>      m_passIdxToSync;

        // =====================
        // lv0: each queue
        // lv1: passes in queue
        std::array< std::vector<uint16_t>, (uint16_t)PassExeQueue::Count >    m_passIdxInQueue; 

        // =====================
        // lv0 index: each pass would use
        // lv1 index: one pass in each queue
        std::vector< std::array<uint16_t, (uint16_t)PassExeQueue::Count> >    m_nearestSyncPassIdx;

        std::vector< CombinedResID>     m_multiFrame_res;
                     
        std::vector< CombinedResID>     m_usedResUniList;
        std::vector< CombinedResID>     m_readResUniList;
        std::vector< CombinedResID>     m_writeResUniList;
        std::vector< ResLifetime>       m_resLifeTime;
    };


}; // namespace vkz