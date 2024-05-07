#pragma once

#include "kage_inner.h"
#include "common.h"
#include "util.h"

#include "bx/allocator.h"
#include "bx/readerwriter.h"

namespace kage
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
        register_image_view,
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
        uint32_t    imgViewNum;

        uint16_t    presentImage;
        uint32_t    presentMipLevel;
    };

    // Register Info
    struct FGShaderCreateInfo
    {
        uint16_t    shaderId;
        uint16_t    strLen;
    };

    struct FGProgCreateInfo : public ProgramDesc
    {
    };

    struct FGBufferCreateInfo : public BufferDesc
    {
        uint16_t    bufId{ kInvalidHandle };
        void*       pData{ nullptr };
        
        ResourceLifetime    lifetime{ ResourceLifetime::transition };

        ResInteractDesc initialState;
    };

    struct FGImageCreateInfo : public ImageDesc
    {
        uint16_t    imgId{ kInvalidHandle };

        uint32_t    size;
        void*       pData;

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
        FGShaderCreateInfo      createInfo;
        uint16_t                pathIdx;
    };

    struct ProgramInfo
    {
        FGProgCreateInfo     createInfo;
        uint16_t   shaderIds[kMaxNumOfStageInPorgram]{kInvalidHandle};
    };

    struct PassMetaDataRef
    {
        PassMetaDataRef() = default;
        ~PassMetaDataRef()
        {
            vtxBindingIdxs.clear();
            vtxAttrIdxs.clear();
            pipelineSpecIdxs.clear();
        }

        uint16_t                passRegInfoIdx;
        stl::vector<uint16_t>   vtxBindingIdxs;
        stl::vector<uint16_t>   vtxAttrIdxs;
        stl::vector<int>        pipelineSpecIdxs;
    };

    struct RHIContext;
    
    class Framegraph
    {
    public:
        Framegraph(bx::AllocatorI* _allocator, bx::MemoryBlockI* _rhiMem)
            : m_pAllocator{ _allocator }
            , m_rhiMemWriter{ _rhiMem }
        { 
            m_pMemBlock = BX_NEW(m_pAllocator, bx::MemoryBlock)(m_pAllocator);
            m_pMemBlock->more(kInitialFrameGraphMemSize);
        }

        ~Framegraph();
        void bake();

    public:
        inline bx::MemoryBlockI* getMemoryBlock() const
        {
            return m_pMemBlock;
        }

        inline void setMemoryBlock(bx::MemoryBlockI* _memBlock)
        {
            m_pMemBlock = _memBlock;
        }

    private:
        void shutdown();

        void parseOp();

        // ==============================
        // process operations 
        void setBrief(bx::MemoryReader& _reader);

        void registerShader(bx::MemoryReader& _reader);
        void registerProgram(bx::MemoryReader& _reader);

        void registerPass(bx::MemoryReader& _reader);
        void registerBuffer(bx::MemoryReader& _reader);
        void registerImage(bx::MemoryReader& _reader);

        void registerSampler(bx::MemoryReader& _reader);
        void registerImageView(bx::MemoryReader& _reader);
        
        void storeBackBuffer(bx::MemoryReader& _reader);

        uint32_t readResource(const stl::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type);
        uint32_t writeResource(const stl::vector<PassResInteract>& _resVec, const uint16_t _passId, const ResourceType _type);
        uint32_t writeResForceAlias(const stl::vector<WriteOperationAlias>& _aliasMapVec, const uint16_t _passId, const ResourceType _type);

        void aliasResForce(bx::MemoryReader& _reader, ResourceType _type);

        bool isForceAliased(const CombinedResID& _res_0, const CombinedResID _res_1) const;
        // =======================================
        void buildGraph();
        void calcPriority();
        void calcCombinations(stl::vector<stl::vector<uint16_t>>& _vecs, const stl::vector<uint16_t>& _inVec);
        uint16_t findCommonParent(const uint16_t _a, uint16_t _b, const stl::unordered_map<uint16_t, uint16_t>& _sortedParentMap, const uint16_t _root);
        uint16_t validateSortedPasses(const stl::vector<uint16_t>& _sortedPassIdxes, const stl::unordered_map<uint16_t, uint16_t>& _sortedParentMap);
        void reverseTraversalDFSWithBackTrack();
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

            const FGImageCreateInfo& info = m_sparse_img_info[_res.id];
            return info.usage & ImageUsageFlagBits::depth_stencil_attachment;
        }

        bool isColorAttachment(const CombinedResID _res)
        {
            if (!isImage(_res))
            {
                return false;
            }

            const FGImageCreateInfo& info = m_sparse_img_info[_res.id];
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
            BufBucket() = default;
            ~BufBucket()
            {
                baseBufId = kInvalidHandle;

                size = 0;
                pData = nullptr;

                reses.clear();
            }

            uint16_t        baseBufId{kInvalidHandle};

            uint32_t        size{ 0 };
            void*           pData{ nullptr };

            BufferDesc      desc;
            ResInteractDesc initialBarrierState;

            bool            forceAliased{ false };
            stl::vector<CombinedResID> reses;
        };

        struct ImgBucket
        {
            ImgBucket() = default;
            ~ImgBucket()
            {
                baseImgId = kInvalidHandle;

                size = 0;
                pData = nullptr;

                reses.clear();
            }

            uint16_t    baseImgId;
            ImageDesc   desc;

            uint32_t    size;
            void*       pData;

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

        void createBufBkt(BufBucket& _bkt, const FGBufferCreateInfo& _info, const stl::vector<CombinedResID>& _res, const bool _forceAliased = false);
        void createImgBkt(ImgBucket& _bkt, const FGImageCreateInfo& _info, const stl::vector<CombinedResID>& _res, const bool _forceAliased = false);

        void aliasBuffers(stl::vector<BufBucket>& _buckets, const stl::vector<uint16_t>& _sortedBufList);
        void aliasImages(stl::vector<ImgBucket>& _buckets, const stl::vector< FGImageCreateInfo >& _infos, const stl::vector<uint16_t>& _sortedTexList, const ResourceType _type);

        void fillBufferBuckets();
        void fillImageBuckets();

        // for sort
        struct PassRWResource
        {
            void clear()
            {
                readInteractMap.clear();
                writeInteractMap.clear();
                readCombinedRes.clear();
                writeCombinedRes.clear();
                writeOpForcedAliasMap.clear();
                imageSamplerMap.clear();
            }

            PassRWResource() = default;
            ~PassRWResource()
            {
                clear();
            }

            stl::unordered_map<CombinedResID, ResInteractDesc> readInteractMap;
            stl::unordered_map<CombinedResID, ResInteractDesc> writeInteractMap;

            stl::unordered_set<CombinedResID> readCombinedRes;
            stl::unordered_set<CombinedResID> writeCombinedRes;

            ContinuousMap<CombinedResID, CombinedResID> writeOpForcedAliasMap;
            ContinuousMap<CombinedResID, uint16_t> imageSamplerMap;
        };

        struct PassDependency
        {
            void clear()
            {
                inPassIdxSet.clear();
                outPassIdxSet.clear();
            }

            PassDependency() = default;
            ~PassDependency()
            {
                clear();
            }

            stl::unordered_set<uint16_t> inPassIdxSet;
            stl::unordered_set<uint16_t> outPassIdxSet;
        };

        // for optimize
        struct PassInDependLevel
        {
            PassInDependLevel()
                : maxLevel{ 0 }
            {
                plainPassInLevel.clear();
            }

            PassInDependLevel(size_t _piqSz)
                : maxLevel{ 0 }
                , plainPassInLevel(_piqSz, kInvalidIndex)
            {
            }
            ~PassInDependLevel()
            {
                plainPassInLevel.clear();
            }

            stl::vector<uint16_t> plainPassInLevel;
            uint16_t maxLevel;
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
        bx::AllocatorI* m_pAllocator;
        bx::MemoryBlockI* m_pMemBlock;

        bx::MemoryWriter m_rhiMemWriter;

        CombinedResID  m_combinedPresentImage;
        uint32_t       m_presentMipLevel;
        PassHandle     m_finalPass{kInvalidHandle};

        stl::vector< ShaderHandle >     m_hShader;
        stl::vector< ProgramHandle >    m_hProgram;
        stl::vector< PassHandle >       m_hPass;
        stl::vector< BufferHandle >     m_hBuf;
        stl::vector< ImageHandle >      m_hTex;
        stl::vector< SamplerHandle >    m_hSampler;
        stl::vector< ImageViewHandle>   m_hImgView;

        stl::vector< ShaderInfo >           m_sparse_shader_info;
        stl::vector< ProgramInfo>           m_sparse_program_info;
        stl::vector< PassMetaData >         m_sparse_pass_meta;
        stl::vector< FGBufferCreateInfo >   m_sparse_buf_info;
        stl::vector< FGImageCreateInfo >    m_sparse_img_info;
        stl::vector< PassMetaDataRef>       m_sparse_pass_data_ref;
        stl::vector< SamplerMetaData >      m_sparse_sampler_meta;
        stl::vector< ImageViewDesc >        m_sparse_img_view_desc;
        
        stl::vector< String>                m_shader_path;

        stl::vector< CombinedResID>         m_combinedResId;

        stl::vector< VertexBindingDesc>     m_vtxBindingDesc;
        stl::vector< VertexAttributeDesc>   m_vtxAttrDesc;
        stl::vector< int>                   m_pipelineSpecData;

        stl::vector< PassRWResource>                m_pass_rw_res;
        stl::vector< PassDependency>                m_pass_dependency;
        stl::vector< CombinedResID>                 m_combinedForceAlias_base;
        stl::vector< stl::vector<CombinedResID>>    m_combinedForceAlias;
        stl::unordered_map<CombinedResID, CombinedResID> m_forceAliasMapToBase;

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
        struct PassInQueue
        {
            uint16_t arr[(uint16_t)PassExeQueue::count];
        };
        stl::vector< PassInQueue >    m_nearestSyncPassIdx;

        stl::vector< CombinedResID>     m_multiFrame_resList;


        stl::vector< CombinedResID>     m_resInUseUniList;
        stl::vector< CombinedResID>     m_resToOptmUniList;
        stl::vector< CombinedResID>     m_resInUseReadonlyList;
        stl::vector< CombinedResID>     m_resInUseMultiframeList;

        stl::vector< ResLifetime>       m_resLifeTime;

        ContinuousMap< CombinedResID, CombinedResID> m_plainResAliasToBase;

        stl::vector< BufBucket>          m_bufBuckets;
        stl::vector< ImgBucket>          m_imgBuckets;
    };


}; // namespace kage