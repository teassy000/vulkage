#pragma once
#include "common.h"

namespace vkz
{
    namespace NameTags
    {
        // tag for varies of handle 
        static const char* kShader       = "sh";
        static const char* kRenderPass   = "rp";
        static const char* kProgram      = "prog";
        static const char* kPipeline     = "pip";
        static const char* kRenderTarget = "rt";
        static const char* kDepthStencil = "dp";
        static const char* kTexture      = "tex";
        static const char* kBuffer       = "buf";
        
        // tag for alias, avliable for buffer, texture, renderTarget, depthStencil
        static const char* kAlias        = "alias";
    }

    enum HandleType
    {
        HandleType_Shader,
        HandleType_RenderPass,
        HandleType_Program,
        HandleType_Pipeline,
        HandleType_RenderTarget,
        HandleType_DepthStencil,
        HandleType_Texture,
        HandleType_Buffer,
        
        HandleType_Unknown,
    };

    struct HandleSignature
    {
        HandleType  type;
        uint16_t    idx;
    };

    class NameManager
    {
    public:
        inline void setShaderName(const char* _name, uint16_t idx);
        inline void setRenderPassName(const char* _name, uint16_t idx);
        inline void setProgramName(const char* _name, uint16_t idx);
        inline void setPipelineName(const char* _name, uint16_t idx);
        inline void setRenderTargetName(const char* _name, uint16_t idx);
        inline void setDepthStencilName(const char* _name, uint16_t idx);
        inline void setTextureName(const char* _name, uint16_t idx);
        inline void setBufferName(const char* _name, uint16_t idx);

        inline std::string getShaderName(const uint16_t idx) { return getName({ HandleType_Shader, idx }); }
        inline std::string getRenderPassName(const uint16_t idx) { return getName({ HandleType_RenderPass, idx }); }
        inline std::string getProgramName(const uint16_t idx) { return getName({ HandleType_Program, idx }); }
        inline std::string getPipelineName(const uint16_t idx) { return getName({ HandleType_Pipeline, idx }); }
        inline std::string getRenderTargetName(const uint16_t idx) { return getName({ HandleType_RenderTarget, idx }); }
        inline std::string getDepthStencilName(const uint16_t idx) { return getName({ HandleType_DepthStencil, idx }); }
        inline std::string getTextureName(const uint16_t idx) { return getName({ HandleType_Texture, idx }); }
        inline std::string getBufferName(const uint16_t idx) { return getName({ HandleType_Buffer, idx }); }

        inline const uint16_t getID(const std::string& name)
        {
            return _nameToID[name].idx;
        }

    private:
        inline void addResource(const std::string& _name, const HandleSignature _signature)
        {
            _nameToID[_name] = _signature;
            HandleType type = _signature.type;
            assert(type != HandleType_Unknown);
            getVector(type).emplace_back(_name);
        }

        inline std::string getName(const HandleSignature id)
        {
            HandleType type = id.type;
            auto& vec = getVector(type);
            assert(id.idx < vec.size());
            return vec[id.idx];
        }

        inline std::vector<std::string>& getVector(HandleType type);
        
    private:
        std::unordered_map<std::string, HandleSignature> _nameToID;
        std::vector<std::string> _idToName_sh;
        std::vector<std::string> _idToName_rp;
        std::vector<std::string> _idToName_prog;
        std::vector<std::string> _idToName_pip;
        std::vector<std::string> _idToName_rt;
        std::vector<std::string> _idToName_dp;
        std::vector<std::string> _idToName_tex;
        std::vector<std::string> _idToName_buf;
        std::vector<std::string> _idToName_unknown;
    };

    inline void NameManager::setShaderName(const char* _name, uint16_t idx)
    {
        std::string name = _name;
        addResource(name, { HandleType_Shader, idx });
    }

    inline void NameManager::setRenderPassName(const char* _name, uint16_t idx)
    {
        std::string name = _name;
        addResource(name, { HandleType_RenderPass, idx });
    }

    inline void NameManager::setProgramName(const char* _name, uint16_t idx)
    {
        std::string name = _name;
        addResource(name, { HandleType_Program, idx });
    }

    inline void NameManager::setPipelineName(const char* _name, uint16_t idx)
    {
        std::string name = _name;
        addResource(name, { HandleType_Pipeline, idx });
    }

    inline void NameManager::setRenderTargetName(const char* _name, uint16_t idx)
    {
        std::string name = _name;
        addResource(name, { HandleType_RenderTarget, idx });
    }

    inline void NameManager::setDepthStencilName(const char* _name, uint16_t idx)
    {
        std::string name = _name;
        addResource(name, { HandleType_DepthStencil, idx });
    }

    inline void NameManager::setTextureName(const char* _name, uint16_t idx)
    {
        std::string name = _name;
        addResource(name, { HandleType_Texture, idx });
    }

    inline void NameManager::setBufferName(const char* _name, uint16_t idx)
    {
        std::string name = _name;
        addResource(name, { HandleType_Buffer, idx });
    }

    inline std::vector<std::string>& NameManager::getVector(HandleType type)
    {
        switch (type)
        {
        case HandleType_Shader:
            return _idToName_sh;
        case HandleType_RenderPass:
            return _idToName_rp;
        case HandleType_Program:
            return _idToName_prog;
        case HandleType_Pipeline:
            return _idToName_pip;
        case HandleType_RenderTarget:
            return _idToName_rt;
        case HandleType_DepthStencil:
            return _idToName_dp;
        case HandleType_Texture:
            return _idToName_tex;
        case HandleType_Buffer:
            return _idToName_buf;
        default:
            assert(false);
            return _idToName_unknown;
        }
    }
}
