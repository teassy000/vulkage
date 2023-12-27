#pragma once

#include "memory_operation.h"
#include "vkz_structs_inner.h"
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
        register_texture,
        register_render_target,
        register_depth_stencil,

        pass_read_buffer,
        pass_write_buffer,
        pass_read_texture,
        pass_write_texture,
        pass_read_render_target,
        pass_write_render_target,
        pass_read_depth_stencil,
        pass_write_depth_stencil,

        force_alias_buffer,
        force_alias_texture,
        force_alias_render_target,
        force_alias_depth_stencil,

        set_non_transition_buffer,
        set_non_transition_texture,
        set_non_transition_render_target,
        set_non_transition_depth_stencil,

        set_present,

        end,
    };

    enum class ResourceType : uint16_t
    {
        buffer = 0,
        texture,
        render_target,
        depth_stencil,
    };

    struct FrameGraphBrief
    {
        uint32_t    version;
        uint32_t    shaderNum;
        uint32_t    programNum;
        uint32_t    passNum;
        uint32_t    bufNum;
        uint32_t    imgNum;
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

    struct PassRegisterInfo : public PassDesc
    {
        uint16_t    passId;

        // TODO: pipeline rendering create info
        // write depth stencil format, color attachment format, and count
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

    struct PassCreateDataRef
    {
        uint16_t                passRegInfoIdx;
        std::vector<uint16_t>   vtxBindingIdxs;
        std::vector<uint16_t>   vtxAttrIdxs;
        std::vector<uint32_t>   pushConstantIdxs;
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
        void registerTexture(MemoryReader& _reader);
        void registerRenderTarget(MemoryReader& _reader);
        void registerDepthStencil(MemoryReader& _reader);

        void passReadRes(MemoryReader& _reader, ResourceType _type);
        void passWriteRes(MemoryReader& _reader, ResourceType _type);

        void readResource(MemoryReader& _reader, ResourceType _type);
        void writeResource(MemoryReader& _reader, ResourceType _type);

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

        void buildResLifetime();

        // =======================================
        void optimizeSync();
        void optimizeAlias();

        // =======================================
        void createBuffers();
        void createImages();
        void createShaders();
        void createPasses();

        void createResources();

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

        inline bool isBuffer(const CombinedResID& _res) const
        {
            return _res.type == ResourceType::buffer;
        }

        inline bool isTexture(const CombinedResID& _res) const
        {
            return _res.type == ResourceType::texture;
        }

        inline bool isRenderTarget(const CombinedResID& _res) const
        {
            return _res.type == ResourceType::render_target;
        }

        inline bool isDepthStencil(const CombinedResID& _res) const
        {
            return _res.type == ResourceType::depth_stencil;
        }

        struct BufBucket
        {
            uint32_t        idx;
            uint16_t        baseBufId;
            BufferDesc      desc;
            ResInteractDesc    initialBarrierState;

            bool            forceAliased{ false };
            std::vector<CombinedResID> reses;
        };

        struct ImgBucket
        {
            uint32_t    idx;
            uint16_t    baseImgId;
            ImageDesc   desc;

            ImageAspectFlags   aspectFlags;
            ResInteractDesc    initialBarrierState;

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

        void aliasBuffers(std::vector<BufBucket>& _buckets, const std::vector<uint16_t>& _sortedBufList);
        void aliasImages(std::vector<ImgBucket>& _buckets, const std::vector< ImgRegisterInfo >& _infos, const std::vector<uint16_t>& _sortedTexList, const ResourceType _type);

        void fillBufferBuckets();
        void fillImageBuckets();


        // for sort
        struct PassRWResource
        {
            UniDataContainer<CombinedResID, ResInteractDesc> readInteracts;
            UniDataContainer<CombinedResID, ResInteractDesc> writeInteracts;

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
        AllocatorI* m_pAllocator;
        MemoryBlockI* m_pMemBlock;

        MemoryBlockI* m_pCreatorMemBlock;
        MemoryWriter m_rhiMemWriter;

        CombinedResID  m_resultRT;
        PassHandle     m_finalPass;

        std::vector< ShaderHandle       >   m_hShader;
        std::vector< ProgramHandle      >   m_hProgram;
        std::vector< PassHandle         >   m_hPass;
        std::vector< BufferHandle       >   m_hBuf;
        std::vector< TextureHandle      >   m_hTex;
        std::vector< RenderTargetHandle >   m_hRT;
        std::vector< DepthStencilHandle >   m_hDS;

        std::vector< ShaderInfo >       m_sparse_shader_info;
        std::vector< ProgramInfo>       m_sparse_program_info;
        std::vector< PassRegisterInfo > m_sparse_pass_info;
        std::vector< BufRegisterInfo >  m_sparse_buf_info;
        std::vector< ImgRegisterInfo >  m_sparse_img_info;
        std::vector< PassCreateDataRef> m_sparse_pass_data_ref;
        
        std::vector< std::string>           m_shader_path;

        std::vector< CombinedResID>         m_combinedResId;

        std::vector< VertexBindingDesc>     m_vtxBindingDesc;
        std::vector< VertexAttributeDesc>   m_vtxAttrDesc;
        std::vector< int>                   m_pushConstants;

        std::vector< PassRWResource>                m_pass_rw_res;
        std::vector< PassDependency>                m_pass_dependency;
        std::vector< CombinedResID>                 m_combinedForceAlias_base;
        std::vector< std::vector<CombinedResID>>    m_combinedForceAlias;

        // sorted and cut passes
        std::vector< PassHandle>    m_sortedPass;
        std::vector< uint16_t>      m_sortedPassIdx;

        std::vector< PassInDependLevel>          m_passIdxInDpLevels;

        std::vector< std::vector<uint16_t>>      m_passIdxToSync;

        // =====================
        // lv0: each queue
        // lv1: passes in queue
        std::array< std::vector<uint16_t>, (uint16_t)PassExeQueue::count >    m_passIdxInQueue; 

        // =====================
        // lv0 index: each pass would use
        // lv1 index: one pass in each queue
        std::vector< std::array<uint16_t, (uint16_t)PassExeQueue::count> >    m_nearestSyncPassIdx;

        std::vector< CombinedResID>     m_multiFrame_resList;
        std::vector< CombinedResID>     m_readonly_resList;

        std::vector< CombinedResID>     m_resInUseUniList;
        std::vector< CombinedResID>     m_resToOptmUniList;

        std::vector< CombinedResID>     m_readResUniList;
        std::vector< CombinedResID>     m_writeResUniList;
        std::vector< ResLifetime>       m_resLifeTime;

        UniDataContainer< CombinedResID, CombinedResID> m_plainResAliasToBase;

        std::vector< BufBucket>          m_bufBuckets;
        std::vector< ImgBucket>          m_imgBuckets;
    };


    /////////////////////////////////////////////////////////////////////////////////////////
    struct PassMetaData
    {
        uint16_t passId;
        uint16_t programId;
        uint16_t pipelineId;

        uint32_t pushConstantOffset;
        uint32_t pushConstantSize;

        uint32_t descriptorSetNum;
        uint32_t descriptorSetOffset;
        uint32_t descriptorSetSize;

        MemoryBlockI*   pMemBlock;
    };

    struct ComputePassData : public PassMetaData
    {
        uint32_t groupCountX;
        uint32_t groupCountY;
        uint32_t groupCountZ;
    };

    struct GraphicPassData : public PassMetaData
    {
        uint32_t vertexBindingNum;
        uint32_t vertexAttributeNum;
        uint32_t vertexBindingOffset;
        uint32_t vertexAttributeOffset;
    };

    class Framegraph2Executor
    {

    };


}; // namespace vkz