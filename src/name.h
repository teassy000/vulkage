#pragma once
#include "common.h"
#include "string.h"

namespace vkz
{
    namespace NameTags
    {
        // tag for varies of handle 
        static const char* kShader          = "[sh]";
        static const char* kRenderPass      = "[pass]";
        static const char* kProgram         = "[prog]";
        static const char* kImage           = "[img]";
        static const char* kBuffer          = "[buf]";
        static const char* kSampler         = "[samp]";
        static const char* kImageView       = "[imgv]";
        
        // tag for alias: buffer, image
        static const char* kAlias           = "[alias]";
    }

    enum class HandleType : uint16_t
    {
        unknown = 0,

        pass,
        shader,
        program,
        image,
        buffer,
        sampler,
        image_view,
    };

    struct HandleSignature
    {
        HandleType  type;
        uint16_t    id;

        // for tinystl::hash(const T& value)
        // which requires to cast to size_t
        operator size_t() const
        {
            return ((size_t)(type) << 16) | id;
        }
    };

    extern AllocatorI* s_allocator;
    using String = SimpleString<&s_allocator>;

    class NameManager
    {
    public:
        NameManager();
        ~NameManager();

        inline void setName(const char* _name, const int32_t _len, PassHandle _hPass) { setName(_name, _len, { HandleType::pass, _hPass.id });}
        inline void setName(const char* _name, const int32_t _len, ShaderHandle _hShader) { setName(_name, _len, { HandleType::shader, _hShader.id }); }
        inline void setName(const char* _name, const int32_t _len, ProgramHandle _hProgram) { setName(_name, _len, { HandleType::program, _hProgram.id }); }
        inline void setName(const char* _name, const int32_t _len, ImageHandle _hImage, bool _isAlias) { setName(_name, _len, { HandleType::image, _hImage.id }), _isAlias; }
        inline void setName(const char* _name, const int32_t _len, BufferHandle _hBuffer, bool _isAlias) { setName(_name, _len, { HandleType::buffer, _hBuffer.id }), _isAlias; }
        inline void setName(const char* _name, const int32_t _len, SamplerHandle _hSampler) { setName(_name, _len, { HandleType::sampler, _hSampler.id }); }
        inline void setName(const char* _name, const int32_t _len, ImageViewHandle _hImageView) { setName(_name, _len, { HandleType::image_view , _hImageView.id }); }

        inline const char* getName(PassHandle _hPass) { return getName({ HandleType::pass, _hPass.id}); }
        inline const char* getName(ShaderHandle _hShader) { return getName({ HandleType::shader, _hShader.id }); }
        inline const char* getName(ProgramHandle _hProgram) { return getName({ HandleType::program, _hProgram.id }); }
        inline const char* getName(ImageHandle _hImage) { return getName({ HandleType::image, _hImage.id }); }
        inline const char* getName(BufferHandle _hBuffer) { return getName({ HandleType::buffer, _hBuffer.id }); }
        inline const char* getName(SamplerHandle _hSampler) { return getName({ HandleType::sampler, _hSampler.id }); }
        inline const char* getName(ImageViewHandle _hImageView) { return getName({ HandleType::image_view, _hImageView.id }); }

    private:
        inline void setName(const char* _name, const int32_t _len, const HandleSignature _signature, bool _isAlias = false);

        inline const char* getName(const HandleSignature id);

        void getPrefix(String& _outStr, HandleType _type, bool _isAlias);
        
    private:
        stl::unordered_map<HandleSignature, String> _idToName;
    };

    NameManager::NameManager()
    {
        _idToName.clear();
    }

    NameManager::~NameManager()
    {
        _idToName.clear();
    }

    inline void NameManager::setName(const char* _name, const int32_t _len, const HandleSignature _signature, bool _isAlias /* = isFalse */)
    {
        if (_signature.type == HandleType::unknown)
        {
            message(error, "invalid handle type");
            return;
        }

        String name;
        getPrefix(name, _signature.type, _isAlias);
        name.append(_name);

        _idToName.insert({ _signature, name });
    }

    inline const char* NameManager::getName(const HandleSignature id)
    {
        return _idToName[id].getCStr();
    }

    void NameManager::getPrefix(String& _inStr , HandleType _type, bool _isAlias)
    {
        if (HandleType::unknown == _type)
        {
            message(error, "invalid handle type");
            return;
        }

        if (_isAlias)
        {
            _inStr.append(NameTags::kAlias);
        }

        if (HandleType::pass == _type)
        {
            _inStr.append(NameTags::kRenderPass);
        }
        else if (HandleType::shader == _type)
        {
            _inStr.append(NameTags::kShader);
        }
        else if (HandleType::program == _type)
        {
            _inStr.append(NameTags::kProgram);
        }
        else if (HandleType::image == _type)
        {
            _inStr.append(NameTags::kImage);
        }
        else if (HandleType::buffer == _type)
        {
            _inStr.append(NameTags::kBuffer);
        }
        else if (HandleType::sampler == _type)
        {
            _inStr.append(NameTags::kSampler);
        }
        else if (HandleType::image_view == _type)
        {
            _inStr.append(NameTags::kImageView);
        }
    }

    inline void setName(NameManager* _nameMngr, const char* _name, const size_t _len, PassHandle _hPass)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hPass);
    }

    inline void setName(NameManager* _nameMngr, const char* _name, const size_t _len, ShaderHandle _hShader)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hShader);
    }

    inline void setName(NameManager* _nameMngr, const char* _name, const size_t _len, ProgramHandle _hProgram)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hProgram);
    }

    inline void setName(NameManager* _nameMngr, const char* _name, const size_t _len, ImageHandle _hImage, bool _isAlias = false)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hImage, _isAlias);
    }

    inline void setName(NameManager* _nameMngr, const char* _name, const size_t _len, BufferHandle _hBuffer, bool _isAlias = false)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hBuffer, _isAlias);
    }

    inline void setName(NameManager* _nameMngr, const char* _name, const size_t _len, SamplerHandle _hSampler)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hSampler);
    }

    inline void setName(NameManager* _nameMngr, const char* _name, const size_t _len, ImageViewHandle _hImageView)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hImageView);
    }

    inline const char* getName(NameManager* _nameMngr, PassHandle _hPass)
    {
        return _nameMngr->getName(_hPass);
    }

    inline const char* getName(NameManager* _nameMngr, ShaderHandle _hShader)
    {
        return _nameMngr->getName(_hShader);
    }

    inline const char* getName(NameManager* _nameMngr, ProgramHandle _hProgram)
    {
        return _nameMngr->getName(_hProgram);
    }

    inline const char* getName(NameManager* _nameMngr, ImageHandle _hImage)
    {
        return _nameMngr->getName(_hImage);
    }

    inline const char* getName(NameManager* _nameMngr, BufferHandle _hBuffer)
    {
        return _nameMngr->getName(_hBuffer);
    }

    inline const char* getName(NameManager* _nameMngr, SamplerHandle _hSampler)
    {
        return _nameMngr->getName(_hSampler);
    }

    inline const char* getName(NameManager* _nameMngr, ImageViewHandle _hImageView)
    {
        return _nameMngr->getName(_hImageView);
    }
} // namespace vkz


