#pragma once

#include "memory_operation.h"
#include "vkz_structs_inner.h"
#include "common.h"
#include "util.h"

namespace vkz
{
    enum class MagicTag : uint32_t
    {
        invalid_magic = 0x00000000,

        magic_body_end,

        set_brief,

        register_shader,
        register_program,

        register_pass,
        register_buffer,
        register_image,
        register_sampler,

        force_alias_buffer,
        force_alias_image,

        end,
    };

    struct FrameGraphBrief
    {
        uint32_t    version;
        uint32_t    shaderNum;
        uint32_t    programNum;
        uint32_t    passNum;
        uint32_t    bufNum;
        uint32_t    imgNum;
        uint32_t    samplerNum;

        uint16_t    presentImage;
    };

    // Register Info
    struct ShaderRegisterInfo
    {
        uint16_t    shaderId;
        uint16_t    strLen;
    };

    struct ProgramRegisterInfo : public ProgramDesc
    {
    };

    struct BufRegisterInfo : public BufferDesc
    {
        uint16_t    bufId{ kInvalidHandle };
        
        ResourceLifetime    lifetime{ ResourceLifetime::transition };

        ResInteractDesc initialState;
    };

    struct ImgRegisterInfo : public ImageDesc
    {
        uint16_t    imgId{ kInvalidHandle };

        uint16_t    bpp{ 4u };

        ResourceLifetime    lifetime{ ResourceLifetime::transition };

        ImageAspectFlags    aspectFlags{ 0 };
        ResInteractDesc     initialState;
    };

    struct PassResInteract
    {
        uint16_t    passId;
        uint16_t    resId;
        uint16_t    samplerId;

        // for specific image view creation
        SpecificImageViewInfo   specImgViewInfo;

        ResInteractDesc interact;
    };

    // read/write resource
    struct RWResInfo
    {
        uint16_t    passId;
        uint16_t    resId;
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

    struct ShaderInfo
    {
        ShaderRegisterInfo      regInfo;
        uint16_t                pathIdx;
    };

    struct ProgramInfo
    {
        ProgramRegisterInfo     regInfo;
        uint16_t   shaderIds[kMaxNumOfStageInPorgram];
    };

    struct PassMetaDataRef
    {
        uint16_t                passRegInfoIdx;
        stl::vector<uint16_t>   vtxBindingIdxs;
        stl::vector<uint16_t>   vtxAttrIdxs;
        stl::vector<int>   pipelineSpecIdxs;
    };

    class RHIContext;
    
    class Framegraph2
    {
    public:
        Framegraph2(AllocatorI* _allocator, MemoryBlockI* _rhiMem)
            : m_pAllocator{ _allocator }
            , m_pCreatorMemBlock {_rhiMem}
            , m_rhiMemWriter{ _rhiMem }
        { 
            m_pMemBlock = VKZ_NEW(m_pAllocator, MemoryBlock(m_pAllocator));
            m_pMemBlock->expand(kInitialFrameGraphMemSize);
        }

        ~Framegraph2();
        void bake();

    public:
        inline MemoryBlockI* getMemoryBlock() const
        {
            return m_pMemBlock;
        }

        inline void setMemoryBlock(MemoryBlockI* _memBlock)
        {
            m_pMemBlock = _memBlock;
        }

    private:
        void parseOp();

        // ==============================
        // process operations 
        void setBrief(MemoryReader& _reader);

        void registerShader(MemoryReader& _reader);
        void registerProgram(MemoryReader& _reader);

        void registerPass(MemoryReader& _reader);
        void registerBuffer(MemoryReader& _reader);
        void registerImage(MemoryReader& _reader);

        void registerSampler(MemoryReader& _reader);

        uint32_t readResource(const stl::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type);
        uint32_t writeResource(const stl::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type);
        uint32_t writeResAlias(const stl::vector<WriteOperationAlias>& _aliasMapVec, const uint16_t _passId, const ResourceType _type);

        void aliasResForce(MemoryReader& _reader, ResourceType _type);

        // =======================================
        void buildGraph();
        void reverseTraversalDFS();
        void buildMaxLevelList(stl::vector<uint16_t>& _maxLvLst);
        void formatDependency(const stl::vector<uint16_t>& _maxLvLst);
        void fillNearestSyncPass();
        void optimizeSyncPass();

        // =======================================
        void postParse();

        void buildResLifetime();

        // =======================================
        void optimizeSync();
        void optimizeAlias();

        // =======================================
        void createBuffers();
        void createImages();
        void createShaders();
        void createSamplers();
        void createPasses();

        void createResources();

    private:
        inline bool isDepthStencil(const CombinedResID _res)
        {
            if (!isImage(_res))
            {
                return false;
            }

            const ImgRegisterInfo& info = m_sparse_img_info[_res.id];
            return info.usage & ImageUsageFlagBits::depth_stencil_attachment;
        }

        bool isColorAttachment(const CombinedResID _res)
        {
            if (!isImage(_res))
            {
                return false;
            }

            const ImgRegisterInfo& info = m_sparse_img_info[_res.id];
            return info.usage & ImageUsageFlagBits::color_attachment;
        }

        bool isNormalImage(const CombinedResID _res)
        {
            if (!isImage(_res))
            {
                return false;
            }

            return !isDepthStencil(_res) && !isColorAttachment(_res);
        }

        struct BufBucket
        {
            uint32_t        idx;
            uint16_t        baseBufId;
            BufferDesc      desc;
            ResInteractDesc    initialBarrierState;

            bool            forceAliased{ false };
            stl::vector<CombinedResID> reses;
        };

        struct ImgBucket
        {
            uint32_t    idx;
            uint16_t    baseImgId;
            ImageDesc   desc;

            ImageAspectFlags   aspectFlags;
            ResInteractDesc    initialBarrierState;

            bool        forceAliased{ false };

            stl::vector<CombinedResID> reses;
        };

        bool isBufInfoAliasable(uint16_t _idx, const BufBucket& _bucket, const stl::vector<CombinedResID> _resInCurrStack) const;
        bool isImgInfoAliasable(uint16_t _idx, const ImgBucket& _bucket, const stl::vector<CombinedResID> _resInCurrStack) const;

        bool isStackAliasable(const CombinedResID& _res, const stl::vector<CombinedResID>& _reses) const;

        bool isAliasable(const CombinedResID& _res, const BufBucket& _bucket, const stl::vector<CombinedResID>& _reses) const;
        bool isAliasable(const CombinedResID& _res, const ImgBucket& _bucket, const stl::vector<CombinedResID>& _reses) const;

        void fillBucketForceAlias();
        void fillBucketReadonly();
        void fillBucketMultiFrame();

        void createBufBkt(BufBucket& _bkt, const BufRegisterInfo& _info, const stl::vector<CombinedResID>& _res, const bool _forceAliased = false);
        void createImgBkt(ImgBucket& _bkt, const ImgRegisterInfo& _info, const stl::vector<CombinedResID>& _res, const bool _forceAliased = false);

        void aliasBuffers(stl::vector<BufBucket>& _buckets, const stl::vector<uint16_t>& _sortedBufList);
        void aliasImages(stl::vector<ImgBucket>& _buckets, const stl::vector< ImgRegisterInfo >& _infos, const stl::vector<uint16_t>& _sortedTexList, const ResourceType _type);

        void fillBufferBuckets();
        void fillImageBuckets();

        // for sort
        struct PassRWResource
        {
            stl::unordered_map<CombinedResID, ResInteractDesc> readInteractMap;
            stl::unordered_map<CombinedResID, ResInteractDesc> writeInteractMap;

            stl::unordered_set<CombinedResID> readCombinedRes;
            stl::unordered_set<CombinedResID> writeCombinedRes;

            UniDataContainer<CombinedResID, CombinedResID> writeOpAliasMap;

            UniDataContainer<CombinedResID, uint16_t> imageSamplerMap;
            UniDataContainer<CombinedResID, SpecificImageViewInfo> specImgViewMap;
        };

        struct PassDependency
        {
            std::set<uint16_t> inPassIdxSet;
            std::set<uint16_t> outPassIdxSet;
        };

        // for optimize
        struct PassInDependLevel
        {
            stl::vector<stl::vector<uint16_t>> passInLv;

            stl::vector<uint16_t> plainPassInLevel;
        };

        struct PassToSync
        {
            stl::vector<uint16_t> passToSync;
        };

        struct ResLifetime
        {
            uint16_t    startIdx; // the index in m_sortedPassIdx
            uint16_t    endIdx;  // how many passes it would last
        };

    private:
        AllocatorI* m_pAllocator;
        MemoryBlockI* m_pMemBlock;

        MemoryBlockI* m_pCreatorMemBlock;
        MemoryWriter m_rhiMemWriter;

        CombinedResID  m_combinedPresentImage;
        PassHandle     m_finalPass{kInvalidHandle};

        stl::vector< ShaderHandle >     m_hShader;
        stl::vector< ProgramHandle >    m_hProgram;
        stl::vector< PassHandle >      m_hPass;
        stl::vector< BufferHandle >     m_hBuf;
        stl::vector< ImageHandle >      m_hTex;
        stl::vector< SamplerHandle >    m_hSampler;

        stl::vector< ShaderInfo >       m_sparse_shader_info;
        stl::vector< ProgramInfo>       m_sparse_program_info;
        stl::vector< PassMetaData >     m_sparse_pass_meta;
        stl::vector< BufRegisterInfo >  m_sparse_buf_info;
        stl::vector< ImgRegisterInfo >  m_sparse_img_info;
        stl::vector< PassMetaDataRef>   m_sparse_pass_data_ref;
        stl::vector< SamplerMetaData >  m_sparse_sampler_meta;
        
        stl::vector< std::string>           m_shader_path;

        stl::vector< CombinedResID>         m_combinedResId;

        stl::vector< VertexBindingDesc>     m_vtxBindingDesc;
        stl::vector< VertexAttributeDesc>   m_vtxAttrDesc;
        stl::vector< int>                   m_pipelineSpecData;

        stl::vector< PassRWResource>                m_pass_rw_res;
        stl::vector< PassDependency>                m_pass_dependency;
        stl::vector< CombinedResID>                 m_combinedForceAlias_base;
        stl::vector< stl::vector<CombinedResID>>    m_combinedForceAlias;

        // sorted and cut passes
        stl::vector< PassHandle>    m_sortedPass;
        stl::vector< uint16_t>      m_sortedPassIdx;

        stl::vector< PassInDependLevel>          m_passIdxInDpLevels;

        stl::vector< stl::vector<uint16_t>>      m_passIdxToSync;

        // =====================
        // lv0: each queue
        // lv1: passes in queue
        std::array< stl::vector<uint16_t>, (uint16_t)PassExeQueue::count >    m_passIdxInQueue; 

        // =====================
        // lv0 index: each pass would use
        // lv1 index: one pass in each queue
        stl::vector< std::array<uint16_t, (uint16_t)PassExeQueue::count> >    m_nearestSyncPassIdx;

        stl::vector< CombinedResID>     m_multiFrame_resList;


        stl::vector< CombinedResID>     m_resInUseUniList;
        stl::vector< CombinedResID>     m_resToOptmUniList;
        stl::vector< CombinedResID>     m_resInUseReadonlyList;
        stl::vector< CombinedResID>     m_resInUseMultiframeList;

        stl::vector< ResLifetime>       m_resLifeTime;

        UniDataContainer< CombinedResID, CombinedResID> m_plainResAliasToBase;

        stl::vector< BufBucket>          m_bufBuckets;
        stl::vector< ImgBucket>          m_imgBuckets;
    };


}; // namespace vkz