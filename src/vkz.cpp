#include "common.h"

#include "vkz_inner.h"

#include "config.h"

#include "util.h"

#include "rhi_context.h"
#include "framegraph_2.h"
#include "rhi_context_vk.h"


#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#include "string.h"


#include "bx/allocator.h"
#include "bx/readerwriter.h"
#include "bx/settings.h"
#include "bx/string.h"
#include "bx/handlealloc.h"


namespace vkz
{
    static bx::AllocatorI* s_bxAllocator = nullptr;
    static bx::AllocatorI* getBxAllocator()
    {
        if (s_bxAllocator == nullptr)
        {
            bx::DefaultAllocator allocator;
            s_bxAllocator = BX_NEW(&allocator, bx::DefaultAllocator)();
        }
        return s_bxAllocator;
    }

    static void shutdownAllocator()
    {
        s_bxAllocator = nullptr;
    }

    using String = bx::StringT<&s_bxAllocator>;

    //using String = SimpleString<&s_bxAllocator>;

    struct NameMgr
    {
        NameMgr();
        ~NameMgr();

        void setName(const char* _name, const int32_t _len, PassHandle _hPass) { setName(_name, _len, { HandleType::pass, _hPass.id }); }
        void setName(const char* _name, const int32_t _len, ShaderHandle _hShader) { setName(_name, _len, { HandleType::shader, _hShader.id }); }
        void setName(const char* _name, const int32_t _len, ProgramHandle _hProgram) { setName(_name, _len, { HandleType::program, _hProgram.id }); }
        void setName(const char* _name, const int32_t _len, ImageHandle _hImage, bool _isAlias) { setName(_name, _len, { HandleType::image, _hImage.id }, _isAlias) ; }
        void setName(const char* _name, const int32_t _len, BufferHandle _hBuffer, bool _isAlias) { setName(_name, _len, { HandleType::buffer, _hBuffer.id }, _isAlias); }
        void setName(const char* _name, const int32_t _len, SamplerHandle _hSampler) { setName(_name, _len, { HandleType::sampler, _hSampler.id }); }
        void setName(const char* _name, const int32_t _len, ImageViewHandle _hImageView) { setName(_name, _len, { HandleType::image_view , _hImageView.id }); }

        const char* getName(PassHandle _hPass) const { return getName({ HandleType::pass, _hPass.id }); }
        const char* getName(ShaderHandle _hShader) const { return getName({ HandleType::shader, _hShader.id }); }
        const char* getName(ProgramHandle _hProgram) const { return getName({ HandleType::program, _hProgram.id }); }
        const char* getName(ImageHandle _hImage) const { return getName({ HandleType::image, _hImage.id }); }
        const char* getName(BufferHandle _hBuffer) const { return getName({ HandleType::buffer, _hBuffer.id }); }
        const char* getName(SamplerHandle _hSampler) const { return getName({ HandleType::sampler, _hSampler.id }); }
        const char* getName(ImageViewHandle _hImageView) const { return getName({ HandleType::image_view, _hImageView.id }); }

    private:
        void setName(const char* _name, const int32_t _len, const HandleSignature _signature, bool _isAlias = false);

        const char* getName(const HandleSignature id) const;

        void getPrefix(String& _outStr, HandleType _type, bool _isAlias);

    private:
        stl::unordered_map<HandleSignature, String> _idToName;
    };

    NameMgr::NameMgr()
    {
        _idToName.clear();
    }

    NameMgr::~NameMgr()
    {
        _idToName.clear();
    }

    void NameMgr::setName(const char* _name, const int32_t _len, const HandleSignature _signature, bool _isAlias /* = false */)
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

    const char* NameMgr::getName(const HandleSignature id) const
    {
        const String& str = _idToName.find(id)->second;
        return str.getCPtr();
    }

    void NameMgr::getPrefix(String& _inStr, HandleType _type, bool _isAlias)
    {
        if (HandleType::unknown == _type)
        {
            message(error, "invalid handle type");
            return;
        }

        if (_isAlias)
        {
            _inStr.append(NameTags::kAlias);
            return;
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

    inline void setName(NameMgr* _nameMngr, const char* _name, const size_t _len, PassHandle _hPass)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hPass);
    }

    void setName(NameMgr* _nameMngr, const char* _name, const size_t _len, ShaderHandle _hShader)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hShader);
    }

    void setName(NameMgr* _nameMngr, const char* _name, const size_t _len, ProgramHandle _hProgram)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hProgram);
    }

    void setName(NameMgr* _nameMngr, const char* _name, const size_t _len, ImageHandle _hImage, bool _isAlias = false)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hImage, _isAlias);
    }

    void setName(NameMgr* _nameMngr, const char* _name, const size_t _len, BufferHandle _hBuffer, bool _isAlias = false)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hBuffer, _isAlias);
    }

    void setName(NameMgr* _nameMngr, const char* _name, const size_t _len, SamplerHandle _hSampler)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hSampler);
    }

    void setName(NameMgr* _nameMngr, const char* _name, const size_t _len, ImageViewHandle _hImageView)
    {
        _nameMngr->setName(_name, (int32_t)_len, _hImageView);
    }

    static NameMgr* s_nameManager = nullptr;
    static NameMgr* getNameManager()
    {
        if (s_nameManager == nullptr)
        {
            s_nameManager = BX_NEW(getBxAllocator(), NameMgr)();
        }

        return s_nameManager;
    }

    uint16_t getBytesPerPixel(ResourceFormat _format)
    {
        uint16_t bpp = 0;
        switch (_format)
        {
        case ResourceFormat::undefined:
            bpp = 0;
            break;
        case ResourceFormat::r8_sint:
            bpp = 1;
            break;
        case ResourceFormat::r8_uint:
            bpp = 1;
            break;
        case ResourceFormat::r16_uint:
            bpp = 2;
            break;
        case ResourceFormat::r16_sint:
            bpp = 2;
            break;
        case ResourceFormat::r16_snorm:
            bpp = 2;
            break;
        case ResourceFormat::r16_unorm:
            bpp = 2;
            break;
        case ResourceFormat::r32_uint:
            bpp = 4;
            break;
        case ResourceFormat::r32_sint:
            bpp = 4;
            break;
        case ResourceFormat::r32_sfloat:
            bpp = 4;
            break;
        case ResourceFormat::b8g8r8a8_snorm:
            bpp = 4;
            break;
        case ResourceFormat::b8g8r8a8_unorm:
            bpp = 4;
            break;
        case ResourceFormat::b8g8r8a8_sint:
            bpp = 4;
            break;
        case ResourceFormat::b8g8r8a8_uint:
            bpp = 4;
            break;
        case ResourceFormat::r8g8b8a8_snorm:
            bpp = 4;
            break;
        case ResourceFormat::r8g8b8a8_unorm:
            bpp = 4;
            break;
        case ResourceFormat::r8g8b8a8_sint:
            bpp = 4;
            break;
        case ResourceFormat::r8g8b8a8_uint:
            bpp = 4;
            break;
        case ResourceFormat::unknown_depth:
            bpp = 0;
            break;
        case ResourceFormat::d16:
            bpp = 2;
            break;
        case ResourceFormat::d32:
            bpp = 4;
            break;
        default:
            bpp = 0;
            break;
        }
        
        return bpp;
    }

    struct MemoryRef
    {
        Memory mem;
        ReleaseFn releaseFn;
        void* userData;
    };

    bool isMemoryRef(const Memory* _mem)
    {
        return _mem->data != (uint8_t*)_mem + sizeof(Memory);
    }

    void release(const Memory* _mem)
    {
        assert(nullptr != _mem);
        Memory* mem = const_cast<Memory*>(_mem);
        if (isMemoryRef(mem))
        {
            MemoryRef* memRef = reinterpret_cast<MemoryRef*>(mem);
            if (nullptr != memRef->releaseFn)
            {
                memRef->releaseFn(mem->data, memRef->userData);
            }
        }
        free(getBxAllocator(), mem);
    }

    // GLFW 
    KeyEnum getKey(int _glfwKey)
    {
        switch (_glfwKey) {
        case GLFW_KEY_UNKNOWN: return KeyEnum::key_unknown;
        case GLFW_KEY_A: return KeyEnum::key_a;
        case GLFW_KEY_B: return KeyEnum::key_b;
        case GLFW_KEY_C: return KeyEnum::key_c;
        case GLFW_KEY_D: return KeyEnum::key_d;
        case GLFW_KEY_E: return KeyEnum::key_e;
        case GLFW_KEY_F: return KeyEnum::key_f;
        case GLFW_KEY_G: return KeyEnum::key_g;
        case GLFW_KEY_H: return KeyEnum::key_h;
        case GLFW_KEY_I: return KeyEnum::key_i;
        case GLFW_KEY_J: return KeyEnum::key_j;
        case GLFW_KEY_K: return KeyEnum::key_k;
        case GLFW_KEY_L: return KeyEnum::key_l;
        case GLFW_KEY_M: return KeyEnum::key_m;
        case GLFW_KEY_N: return KeyEnum::key_n;
        case GLFW_KEY_O: return KeyEnum::key_o;
        case GLFW_KEY_P: return KeyEnum::key_p;
        case GLFW_KEY_Q: return KeyEnum::key_q;
        case GLFW_KEY_R: return KeyEnum::key_r;
        case GLFW_KEY_S: return KeyEnum::key_s;
        case GLFW_KEY_T: return KeyEnum::key_t;
        case GLFW_KEY_U: return KeyEnum::key_u;
        case GLFW_KEY_V: return KeyEnum::key_v;
        case GLFW_KEY_W: return KeyEnum::key_w;
        case GLFW_KEY_X: return KeyEnum::key_x;
        case GLFW_KEY_Y: return KeyEnum::key_y;
        case GLFW_KEY_Z: return KeyEnum::key_z;
        case GLFW_KEY_0: return KeyEnum::key_0;
        case GLFW_KEY_1: return KeyEnum::key_1;
        case GLFW_KEY_2: return KeyEnum::key_2;
        case GLFW_KEY_3: return KeyEnum::key_3;
        case GLFW_KEY_4: return KeyEnum::key_4;
        case GLFW_KEY_5: return KeyEnum::key_5;
        case GLFW_KEY_6: return KeyEnum::key_6;
        case GLFW_KEY_7: return KeyEnum::key_7;
        case GLFW_KEY_8: return KeyEnum::key_8;
        case GLFW_KEY_9: return KeyEnum::key_9;
        case GLFW_KEY_F1: return KeyEnum::key_f1;
        case GLFW_KEY_F2: return KeyEnum::key_f2;
        case GLFW_KEY_F3: return KeyEnum::key_f3;
        case GLFW_KEY_F4: return KeyEnum::key_f4;
        case GLFW_KEY_F5: return KeyEnum::key_f5;
        case GLFW_KEY_F6: return KeyEnum::key_f6;
        case GLFW_KEY_F7: return KeyEnum::key_f7;
        case GLFW_KEY_F8: return KeyEnum::key_f8;
        case GLFW_KEY_F9: return KeyEnum::key_f9;
        case GLFW_KEY_F10: return KeyEnum::key_f10;
        case GLFW_KEY_F11: return KeyEnum::key_f11;
        case GLFW_KEY_F12: return KeyEnum::key_f12;
        case GLFW_KEY_LEFT: return KeyEnum::key_left;
        case GLFW_KEY_RIGHT: return KeyEnum::key_right;
        case GLFW_KEY_UP: return KeyEnum::key_up;
        case GLFW_KEY_DOWN: return KeyEnum::key_down;
        case GLFW_KEY_SPACE: return KeyEnum::key_space;
        case GLFW_KEY_ESCAPE: return KeyEnum::key_esc;
        case GLFW_KEY_ENTER: return KeyEnum::key_enter;
        case GLFW_KEY_BACKSPACE: return KeyEnum::key_backspace;
        case GLFW_KEY_TAB: return KeyEnum::key_tab;
        case GLFW_KEY_CAPS_LOCK: return KeyEnum::key_capslock;
        case GLFW_KEY_LEFT_SHIFT: return KeyEnum::key_shift;
        case GLFW_KEY_RIGHT_SHIFT: return KeyEnum::key_shift;
        case GLFW_KEY_LEFT_CONTROL: return KeyEnum::key_ctrl;
        case GLFW_KEY_RIGHT_CONTROL: return KeyEnum::key_ctrl;
        case GLFW_KEY_LEFT_ALT: return KeyEnum::key_alt;
        case GLFW_KEY_RIGHT_ALT: return KeyEnum::key_alt;

        default: return KeyEnum::key_unknown;
        }
    }

    KeyState getKeyState(int _glfwKeyState)
    {
        switch (_glfwKeyState) {
        case GLFW_PRESS: return KeyState::press;
        case GLFW_RELEASE: return KeyState::release;
        case GLFW_REPEAT: return KeyState::repeat;
        default: return KeyState::unknown;
        }
    }

    KeyModFlags getKeyModFlag(int _glfwModFlag)
    {
        KeyModFlags modFlags = KeyModBits::none;
        if (_glfwModFlag & GLFW_MOD_SHIFT)
        {
            modFlags |= KeyModBits::shift;
        }
        if (_glfwModFlag & GLFW_MOD_CONTROL)
        {
            modFlags |= KeyModBits::ctrl;
        }
        if (_glfwModFlag & GLFW_MOD_ALT)
        {
            modFlags |= KeyModBits::alt;
        }
        if (_glfwModFlag & GLFW_MOD_CAPS_LOCK)
        {
            modFlags |= KeyModBits::capslock;
        }
        if (_glfwModFlag & GLFW_MOD_NUM_LOCK)
        {
            modFlags |= KeyModBits::numlock;
        }

        return modFlags;
    }

    void defaultKeyCallback(KeyEnum _key, KeyState _state, KeyModFlags _mods) {}
    void defaultMouseMoveCallback(double xpos, double ypos) {}

    static KeyCallbackFunc s_keyCallback = defaultKeyCallback;
    static MouseMoveCallbackFunc s_mouseMoveCallback = defaultMouseMoveCallback;
    static MouseButtonCallbackFunc s_mouseButtonCallback = nullptr;
    class GLFWManager
    {
    public:
        bool init(uint32_t _w, uint32_t _h, const char* _name);
        bool shouldClose();
        void update();
        void shutdown();
        void getWindowSize(uint32_t& _width, uint32_t& _height);
        GLFWwindow* getWindow() { return m_pWindow; }

    private:
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);

        GLFWwindow* m_pWindow;
        uint32_t m_width;
        uint32_t m_height;
    };

    bool GLFWManager::init(uint32_t _w, uint32_t _h, const char* _name)
    {
        int rc = glfwInit();
        assert(rc);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_pWindow = glfwCreateWindow(_w, _h, _name, nullptr, nullptr);
        assert(m_pWindow);

        m_width = _w;
        m_height = _h;

        glfwSetKeyCallback(m_pWindow, keyCallback);
        glfwSetCursorPosCallback(m_pWindow, mouseMoveCallback);

        // disable cursor
        glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        return m_pWindow != nullptr;
    }

    bool GLFWManager::shouldClose()
    {
        return glfwWindowShouldClose(m_pWindow);
    }

    void GLFWManager::update()
    {
        glfwPollEvents();
        int newWindowWidth = 0, newWindowHeight = 0;
        glfwGetWindowSize(m_pWindow, &newWindowWidth, &newWindowHeight);
    }

    void GLFWManager::shutdown()
    {
        glfwDestroyWindow(m_pWindow);
        glfwTerminate();
    }

    void GLFWManager::getWindowSize(uint32_t& _width, uint32_t& _height)
    {
        _width = m_width;
        _height = m_height;
    }

    void GLFWManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (s_keyCallback)
        {
            const KeyEnum keyEnum = getKey(key);
            const KeyState keyState = getKeyState(action);
            const KeyModFlags keyModFlags = getKeyModFlag(mods);

            s_keyCallback(keyEnum, keyState, keyModFlags);
        }
    }

    void GLFWManager::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
    {
        if (s_mouseMoveCallback)
        {
            s_mouseMoveCallback(xpos, ypos);
        }
    }

    constexpr uint8_t kTouched = 1;
    constexpr uint8_t kUnTouched = 0;

    struct Context
    {
        // conditions
        bool isCompute(const PassHandle _hPass);
        bool isGraphics(const PassHandle _hPass);
        bool isFillBuffer(const PassHandle _hPass);
        bool isOneOpPass(const PassHandle _hPass);
        bool isRead(const AccessFlags _access);
        bool isWrite(const AccessFlags _access, const PipelineStageFlags _stage);

        bool isDepthStencil(const ImageHandle _hImg);
        bool isColorAttachment(const ImageHandle _hImg);
        bool isNormalImage(const ImageHandle _hImg);

        bool isAliasFromBuffer(const BufferHandle _hBase, const BufferHandle _hAlias);
        bool isAliasFromImage(const ImageHandle _hBase, const ImageHandle _hAlias);

        // Context API
        void setRenderSize(uint32_t _width, uint32_t _height);
        void setWindowName(const char* _name);

        bool checkSupports(VulkanSupportExtension _ext);

        ShaderHandle registShader(const char* _name, const char* _path);
        ProgramHandle registProgram(const char* _name, const Memory* _shaders, const uint16_t _shaderCount, const uint32_t _sizePushConstants = 0);

        PassHandle registPass(const char* _name, const PassDesc& _desc);

        BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const Memory* _mem, const ResourceLifetime _lifetime);

        ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const Memory* _mem, const ResourceLifetime _lifetime);
        ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);
        ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime);

        ImageViewHandle registImageView(const char* _name, const ImageHandle _hImg, const uint32_t _baseMip, const uint32_t _mipLevel);

        SamplerHandle requestSampler(const SamplerDesc& _desc);

        BufferHandle aliasBuffer(const BufferHandle _baseBuf);
        ImageHandle aliasImage(const ImageHandle  _baseImg);

        // read-only data, no barrier required
        void bindVertexBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _vtxCount);
        void bindIndexBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _idxCount);

        void setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _defaultMaxCount);
        void setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset);

        void bindBuffer(PassHandle _hPass, BufferHandle _buf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias);
        SamplerHandle sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, ImageLayout _layout, SamplerReductionMode _reductionMode);
        void bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
            , const ImageHandle _outAlias, const ImageViewHandle _hImgView);

        void setCustomRenderFunc(const PassHandle _hPass, RenderFuncPtr _func, const Memory* _dataMem);

        void setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias);

        void resizeTexture(ImageHandle _hImg, uint32_t _width, uint32_t _height);

        void fillBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _offset, const uint32_t _size, const uint32_t _value, const BufferHandle _outAlias);

        void setMultiFrame(ImageHandle _img);
        void setMultiFrame(BufferHandle _buf);
        
        void setPresentImage(ImageHandle _rt, uint32_t _mipLv);

        void updateCustomRenderFuncData(const PassHandle _hPass, const Memory* _dataMem);

        void updateBuffer(const BufferHandle _hbuf, const Memory* _mem);

        void updatePushConstants(const PassHandle _hPass, const Memory* _mem);

        void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ);

        // actual write to memory
        void storeBrief();
        void storeShaderData();
        void storeProgramData();
        void storePassData();
        void storeBufferData();
        void storeImageData();
        void storeAliasData();
        void storeSamplerData();
        void storeBackBufferData();

        void init();
        bool shouldClose();

        void run();
        void bake();
        void getWindowSize(uint32_t& _width, uint32_t _height);
        void resizeBackBuffer(uint32_t _windth, uint32_t _height);
        void render();
        void shutdown();

        void setRenderGraphDataDirty() { m_isRenderGraphDataDirty = true; }
        bool isRenderGraphDataDirty() const { return m_isRenderGraphDataDirty; }

        // callbacks
        void setKeyCallback(KeyCallbackFunc _func);
        void setMouseMoveCallback(MouseMoveCallbackFunc _func);
        void setMouseButtonCallback(MouseButtonCallbackFunc _func);

        double getPassTime(const PassHandle _hPass);

        bx::HandleAllocT<kMaxNumOfShaderHandle> m_shaderHandles;
        bx::HandleAllocT<kMaxNumOfProgramHandle> m_programHandles;
        bx::HandleAllocT<kMaxNumOfPassHandle> m_passHandles;
        bx::HandleAllocT<kMaxNumOfBufferHandle> m_bufferHandles;
        bx::HandleAllocT<kMaxNumOfImageHandle> m_imageHandles;
        bx::HandleAllocT<kMaxNumOfImageViewHandle> m_imageViewHandles;
        bx::HandleAllocT<kMaxNumOfSamplerHandle> m_samplerHandles;
        
        ImageHandle m_presentImage{kInvalidHandle};
        uint32_t m_presentMipLevel{ 0 };

        // resource Info
        ContinuousMap<ShaderHandle, std::string> m_shaderDescs;
        ContinuousMap<ProgramHandle, ProgramDesc> m_programDescs;
        ContinuousMap<BufferHandle, BufferMetaData> m_bufferMetas;
        ContinuousMap<ImageHandle, ImageMetaData> m_imageMetas;
        ContinuousMap<PassHandle, PassMetaData> m_passMetas;
        ContinuousMap<SamplerHandle, SamplerDesc> m_samplerDescs;
        ContinuousMap<ImageViewHandle, ImageViewDesc> m_imageViewDesc;

        // alias
        stl::unordered_map<uint16_t, BufferHandle> m_bufferAliasMapToBase;
        stl::unordered_map<uint16_t, ImageHandle> m_imageAliasMapToBase;

        ContinuousMap<BufferHandle, stl::vector<BufferHandle>> m_aliasBuffers;
        ContinuousMap<ImageHandle, stl::vector<ImageHandle>> m_aliasImages;

        // read/write resources
        ContinuousMap<PassHandle, stl::vector<PassResInteract>> m_readBuffers;
        ContinuousMap<PassHandle, stl::vector<PassResInteract>> m_writeBuffers;
        ContinuousMap<PassHandle, stl::vector<PassResInteract>> m_readImages;
        ContinuousMap<PassHandle, stl::vector<PassResInteract>> m_writeImages;
        ContinuousMap<PassHandle, stl::vector<WriteOperationAlias>> m_writeForcedBufferAliases; // add a dependency line to the write resource
        ContinuousMap<PassHandle, stl::vector<WriteOperationAlias>> m_writeForcedImageAliases; // add a dependency line to the write resource

        // back buffers
        stl::vector<ImageHandle> m_backBuffers;

        // binding
        ContinuousMap<PassHandle, stl::vector<uint32_t>> m_usedBindPoints;
        ContinuousMap<PassHandle, stl::vector<uint32_t>> m_usedAttchBindPoints;

        // push constants
        ContinuousMap<ProgramHandle, void *> m_pushConstants;

        // 1-operation pass: blit/copy/fill 
        ContinuousMap<PassHandle, uint8_t> m_oneOpPassTouched;

        // Memories
        stl::vector<const Memory*> m_inputMemories;

        // render config
        uint32_t m_renderWidth{ 0 };
        uint32_t m_renderHeight{ 0 };

        // render data status
        bool    m_isRenderGraphDataDirty{ false };

        // frame graph
        bx::MemoryBlockI* m_pFgMemBlock{ nullptr };
        bx::MemoryWriter* m_fgMemWriter{ nullptr };

        RHIContext* m_rhiContext{ nullptr };

        Framegraph2* m_frameGraph{ nullptr };

        bx::AllocatorI* m_pAllocator{ nullptr };

        // name manager
        NameMgr* m_pNameManager{ nullptr };

        // glfw
        char m_windowTitle[256];
        GLFWManager m_glfwManager;
    };

    bool Context::isCompute(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas.getIdToData(_hPass);
        return meta.queue == PassExeQueue::compute;
    }

    bool Context::isGraphics(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas.getIdToData(_hPass);
        return meta.queue == PassExeQueue::graphics;
    }

    bool Context::isFillBuffer(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas.getIdToData(_hPass);
        return meta.queue == PassExeQueue::fill_buffer;
    }

    bool Context::isOneOpPass(const PassHandle _hPass)
    {
        const PassMetaData& meta = m_passMetas.getIdToData(_hPass);

        return (
            meta.queue == PassExeQueue::fill_buffer
            || meta.queue == PassExeQueue::copy
            );
    }

    bool Context::isRead(const AccessFlags _access)
    {
        return _access & AccessFlagBits::read_only;
    }

    bool Context::isWrite(const AccessFlags _access, const PipelineStageFlags _stage)
    {
        return (_access & AccessFlagBits::write_only) || (_stage & PipelineStageFlagBits::color_attachment_output);
    }

    bool Context::isDepthStencil(const ImageHandle _hImg)
    {
        const ImageMetaData& meta = m_imageMetas.getIdToData(_hImg);
        return meta.usage & ImageUsageFlagBits::depth_stencil_attachment;
    }

    bool Context::isColorAttachment(const ImageHandle _hImg)
    {
        const ImageMetaData& meta = m_imageMetas.getIdToData(_hImg);
        return meta.usage & ImageUsageFlagBits::color_attachment;
    }

    bool Context::isNormalImage(const ImageHandle _hImg)
    {
        return !isDepthStencil(_hImg) && !isColorAttachment(_hImg);
    }

    bool Context::isAliasFromBuffer(const BufferHandle _hBase, const BufferHandle _hAlias)
    {
        bool result = false;

        if (m_aliasBuffers.exist(_hBase))
        {
            const stl::vector<BufferHandle>& aliasVec = m_aliasBuffers.getIdToData(_hBase);
            for (const BufferHandle& alias : aliasVec)
            {
                if ( alias == _hAlias)
                {
                    result = true;
                    break;
                }
            }
        }

        return result;
    }

    bool Context::isAliasFromImage(const ImageHandle _hBase, const ImageHandle _hAlias)
    {
        bool result = false;

        if (m_aliasImages.exist(_hBase))
        {
            const stl::vector<ImageHandle>& aliasVec = m_aliasImages.getIdToData(_hBase);
            for (const ImageHandle& alias : aliasVec)
            {
                if (alias == _hAlias)
                {
                    result = true;
                    break;
                }
            }
        }

        return result;
    }

    void Context::setRenderSize(uint32_t _width, uint32_t _height)
    {
        m_renderWidth = _width;
        m_renderHeight = _height;
    }

    void Context::setWindowName(const char* _name)
    {
        if (nullptr == _name)
        {
            memcpy(m_windowTitle, "vkz", 3);
            m_windowTitle[3] = '\0';
            return;
        }

        size_t length = strlen(_name);
        memcpy(m_windowTitle, _name, length);
        m_windowTitle[length] = '\0';
    }

    bool Context::checkSupports(VulkanSupportExtension _ext)
    {
        return m_rhiContext->checkSupports(_ext);
    }

    ShaderHandle Context::registShader(const char* _name, const char* _path)
    {
        uint16_t idx = m_shaderHandles.alloc();

        ShaderHandle handle = ShaderHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ShaderHandle failed!");
            return ShaderHandle{ kInvalidHandle };
        }

        m_shaderDescs.addOrUpdate({ idx }, _path);
        
        setName(m_pNameManager, _name, strlen(_name), handle);

        setRenderGraphDataDirty();

        return handle;
    }

    ProgramHandle Context::registProgram(const char* _name, const Memory* _mem, const uint16_t _shaderNum, const uint32_t _sizePushConstants /*= 0*/)
    {
        uint16_t idx = m_programHandles.alloc();

        ProgramHandle handle = ProgramHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ProgramHandle failed!");
            return ProgramHandle{ kInvalidHandle };
        }

        ProgramDesc desc{};
        desc.progId = idx;
        desc.shaderNum = _shaderNum;
        desc.sizePushConstants = _sizePushConstants;
        memcpy_s(desc.shaderIds, COUNTOF(desc.shaderIds), _mem->data, _mem->size);
        release(_mem);
        m_programDescs.addOrUpdate({ idx }, desc);

        setName(m_pNameManager, _name, strlen(_name), handle);

        setRenderGraphDataDirty();

        return handle;
    }

    PassHandle Context::registPass(const char* _name, const PassDesc& _desc)
    {
        uint16_t idx = m_passHandles.alloc();
        PassHandle handle = PassHandle{ idx };
        
        // check if pass is valid
        if (!isValid(handle))
        {
            message(error, "create PassHandle failed!");
            return PassHandle{ kInvalidHandle };
        }

        PassMetaData meta{ _desc };
        meta.passId = idx;
        m_passMetas.addOrUpdate({ idx }, meta);

        m_oneOpPassTouched.addOrUpdate({ idx }, kUnTouched);

        setName(m_pNameManager, _name, strlen(_name), handle);

        setRenderGraphDataDirty();

        return handle;
    }

    vkz::BufferHandle Context::registBuffer(const char* _name, const BufferDesc& _desc, const Memory* _mem, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_bufferHandles.alloc();

        BufferHandle handle = BufferHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create BufferHandle failed!");
            return BufferHandle{ kInvalidHandle };
        }

        BufferMetaData meta{ _desc };
        meta.bufId = idx;
        meta.lifetime = _lifetime;

        if (_mem != nullptr)
        {
            meta.pData = _mem->data;
            meta.usage |= BufferUsageFlagBits::transfer_dst;

            m_inputMemories.push_back(_mem);
        }

        m_bufferMetas.addOrUpdate({ idx }, meta);

        setName(m_pNameManager, _name, strlen(_name), handle);

        setRenderGraphDataDirty();

        return handle;
    }

    ImageHandle Context::registTexture(const char* _name, const ImageDesc& _desc, const Memory* _mem, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        ImageHandle handle = ImageHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ImageHandle failed!");
            return ImageHandle{ kInvalidHandle };
        }

        ImageMetaData meta = _desc;
        meta.imgId = idx;
        meta.aspectFlags = ImageAspectFlagBits::color;
        meta.lifetime = _lifetime;

        if (_mem != nullptr)
        {
            meta.usage |= ImageUsageFlagBits::transfer_dst;
            meta.size = _mem->size;
            meta.pData = _mem->data;

            m_inputMemories.push_back(_mem);
        }

        m_imageMetas.addOrUpdate({ idx }, meta);

        setName(m_pNameManager, _name, strlen(_name), handle);

        setRenderGraphDataDirty();

        return handle;
    }

    ImageHandle Context::registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        ImageHandle handle = ImageHandle{ idx };

        if (!isValid(handle))
        {
            message(error, "create ImageHandle failed!");
            return ImageHandle{ kInvalidHandle };
        }

        // do not set the texture format
        if (_desc.format != ResourceFormat::undefined)
        {
            message(error, "do not set the resource format for render target");
            return ImageHandle{ kInvalidHandle };
        }

        if (m_renderWidth != _desc.width || m_renderHeight != _desc.height)
        {
            message(warning, "no need to set width and height for render target, will use render size instead");
        }

        ImageMetaData meta = _desc;
        meta.width = m_renderWidth;
        meta.height = m_renderHeight;
        meta.imgId = idx;
        meta.format = ResourceFormat::undefined;
        meta.usage = ImageUsageFlagBits::color_attachment | _desc.usage;
        meta.aspectFlags = ImageAspectFlagBits::color;
        meta.lifetime = _lifetime;

        m_imageMetas.addOrUpdate(handle, meta);

        m_backBuffers.push_back(handle);

        setName(m_pNameManager, _name, strlen(_name), handle);

        setRenderGraphDataDirty();

        return handle;
    }

    ImageHandle Context::registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime)
    {
        uint16_t idx = m_imageHandles.alloc();

        ImageHandle handle = ImageHandle{ idx };

        if (m_renderWidth != _desc.width || m_renderHeight != _desc.height)
        {
            message(warning, "no need to set width and height for depth stencil, will use render size instead");
        }

        // check if img is valid
        if (!isValid(handle))
        {
            message(error, "create ImageHandle failed!");
            return ImageHandle{ kInvalidHandle };
        }

        // do not set the texture format
        if (_desc.format != ResourceFormat::undefined)
        {
            message(error, "do not set the resource format for depth stencil!");
            return ImageHandle{ kInvalidHandle };
        }

        ImageMetaData meta{ _desc };
        meta.width = m_renderWidth;
        meta.height = m_renderHeight;
        meta.imgId = idx;
        meta.format = ResourceFormat::d32;
        meta.usage = ImageUsageFlagBits::depth_stencil_attachment | _desc.usage;
        meta.aspectFlags = ImageAspectFlagBits::depth;
        meta.lifetime = _lifetime;

        m_imageMetas.addOrUpdate(handle, meta);

        m_backBuffers.push_back(handle);
        setName(m_pNameManager, _name, strlen(_name), handle);


        setRenderGraphDataDirty();

        return handle;
    }

    vkz::ImageViewHandle Context::registImageView(const char* _name, const ImageHandle _hImg, const uint32_t _baseMip, const uint32_t _mipLevel)
    {
        if (!isValid(_hImg))
        {
            message(error, "_hImg must be valid to create image view");
            return ImageViewHandle{ kInvalidHandle };
        }

        uint16_t idx = m_imageViewHandles.alloc();
        ImageViewHandle handle { idx };
        // check if img view is valid
        if (!isValid(handle))
        {
            message(error, "create ImageViewHandle failed!");
            return ImageViewHandle{ kInvalidHandle };
        }

        ImageViewDesc desc{ _hImg.id, idx, _baseMip, _mipLevel };
        m_imageViewDesc.addOrUpdate({ idx }, desc);


        ImageMetaData& imgMeta = m_imageMetas.getDataRef(_hImg);
        if (imgMeta.viewCount >= kMaxNumOfImageMipLevel)
        {
            message(error, "too many mip views for image!");
            return ImageViewHandle{ kInvalidHandle };
        }

        imgMeta.mipViews[imgMeta.viewCount] = handle;
        imgMeta.viewCount++;

        setName(m_pNameManager, _name, strlen(_name), handle);

        setRenderGraphDataDirty();

        return handle;
    }

    SamplerHandle Context::requestSampler(const SamplerDesc& _desc)
    {
        // find if the sampler already exist
        uint16_t samplerId = kInvalidHandle;
        for (uint16_t ii = 0; ii < m_samplerHandles.getNumHandles(); ++ii)
        {
            SamplerHandle handle{ m_samplerHandles.getHandleAt(ii) };
            const SamplerDesc& desc = m_samplerDescs.getIdToData(handle);

            if (desc == _desc)
            {
                samplerId = handle.id;
                break;
            }
        }

        if (kInvalidHandle == samplerId)
        {
            samplerId = m_samplerHandles.alloc();

            if (kInvalidHandle == samplerId)
            {
                message(error, "create SamplerHandle failed!");
                return SamplerHandle{ kInvalidHandle };
            }
            
            m_samplerDescs.addOrUpdate({ samplerId }, _desc);
        }

        return SamplerHandle{ samplerId };
    }

    void Context::storeBrief()
    {
        MagicTag magic{ MagicTag::set_brief };
        bx::write(m_fgMemWriter, magic, nullptr);

        // set the brief data
        FrameGraphBrief brief;
        brief.version = 1u;
        brief.passNum = m_passHandles.getNumHandles();
        brief.bufNum = m_bufferHandles.getNumHandles();
        brief.imgNum = m_imageHandles.getNumHandles();
        brief.shaderNum = m_shaderHandles.getNumHandles();
        brief.programNum = m_programHandles.getNumHandles();
        brief.samplerNum = m_samplerHandles.getNumHandles();
        brief.imgViewNum = m_imageViewHandles.getNumHandles();

        brief.presentImage = m_presentImage.id;
        brief.presentMipLevel = m_presentMipLevel;


        bx::write(m_fgMemWriter, brief, nullptr);

        bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
    }

    void Context::storeShaderData()
    {
        uint16_t shaderCount = m_shaderHandles.getNumHandles();
        
        for (uint16_t ii = 0; ii < shaderCount; ++ii)
        {
            ShaderHandle shader { (m_shaderHandles.getHandleAt(ii)) };
            const std::string& path = m_shaderDescs.getIdToData(shader);

            // magic tag
            MagicTag magic{ MagicTag::register_shader };
            bx::write(m_fgMemWriter, magic, nullptr);

            FGShaderCreateInfo info;
            info.shaderId = shader.id;
            info.strLen = (uint16_t)strnlen_s(path.c_str(), kMaxPathLen);

            bx::write(m_fgMemWriter, info, nullptr);
            bx::write(m_fgMemWriter, (void*)path.c_str(), (int32_t)info.strLen, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeProgramData()
    {
        uint16_t programCount = m_programHandles.getNumHandles();

        for (uint16_t ii = 0; ii < programCount; ++ii)
        {
            ProgramHandle program { (m_programHandles.getHandleAt(ii)) };
            const ProgramDesc& desc = m_programDescs.getIdToData(program);

            // magic tag
            MagicTag magic{ MagicTag::register_program };
            bx::write(m_fgMemWriter, magic, nullptr);

            // info
            FGProgCreateInfo info;
            info.progId = desc.progId;
            info.sizePushConstants = desc.sizePushConstants;
            info.shaderNum = desc.shaderNum;
            bx::write(m_fgMemWriter, info, nullptr);

            // actual shader indexes
            bx::write(m_fgMemWriter, desc.shaderIds, sizeof(uint16_t) * desc.shaderNum, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storePassData()
    {
        uint16_t passCount = m_passHandles.getNumHandles();

        for (uint16_t ii = 0; ii < passCount; ++ii)
        {
            auto getResInteractNum = [](
                const ContinuousMap<PassHandle, stl::vector<PassResInteract>>& __cont
                , const PassHandle __pass) -> uint32_t
            {
                if (__cont.exist(__pass)) {
                    return (uint32_t)(__cont.getIdToData(__pass).size());
                }
                
                return 0u;
            };

            PassHandle pass{ (m_passHandles.getHandleAt(ii)) };
            const PassMetaData& passMeta = m_passMetas.getIdToData(pass);

            assert(getResInteractNum(m_writeImages, pass) == passMeta.writeImageNum);
            assert(getResInteractNum(m_readImages, pass) == passMeta.readImageNum);
            assert(getResInteractNum(m_writeBuffers, pass) == passMeta.writeBufferNum);
            assert(getResInteractNum(m_readBuffers, pass) == passMeta.readBufferNum);

            // frame graph data
            MagicTag magic{ MagicTag::register_pass };
            bx::write(m_fgMemWriter, magic, nullptr);

            bx::write(m_fgMemWriter, passMeta, nullptr);

            if (0 != passMeta.vertexBindingNum)
            {
                bx::write(m_fgMemWriter, passMeta.vertexBindings, sizeof(VertexBindingDesc) * passMeta.vertexBindingNum, nullptr);
            }
            if (0 != passMeta.vertexAttributeNum)
            {
                bx::write(m_fgMemWriter, passMeta.vertexAttributes, sizeof(VertexAttributeDesc) * passMeta.vertexAttributeNum, nullptr);
            }
            if (0 != passMeta.pipelineSpecNum)
            {
                bx::write(m_fgMemWriter, passMeta.pipelineSpecData, sizeof(int) * passMeta.pipelineSpecNum, nullptr);
            }

            // write image
            if (0 != passMeta.writeImageNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_writeImages.getIdToData(pass).data()
                    , (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_writeImages, pass))
                    , nullptr
                );
            }
            // read image
            if (0 != passMeta.readImageNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_readImages.getIdToData(pass).data()
                    , (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_readImages, pass))
                    , nullptr
                );
            }
            // write buffer
            if (0 != passMeta.writeBufferNum)
            {
                bx::write(
                    m_fgMemWriter, 
                    (const void*)m_writeBuffers.getIdToData(pass).data(), 
                    (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_writeBuffers, pass))
                    , nullptr
                );
            }
            // read buffer
            if (0 != passMeta.readBufferNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_readBuffers.getIdToData(pass).data()
                    , (uint32_t)(sizeof(PassResInteract) * getResInteractNum(m_readBuffers, pass))
                    , nullptr
                );
            }

            // wirte res forced alias
            if (0 != passMeta.writeImgAliasNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_writeForcedImageAliases.getIdToData(pass).data()
                    , (uint32_t)(sizeof(WriteOperationAlias) * passMeta.writeImgAliasNum)
                    , nullptr
                );
            }

            if (0 != passMeta.writeBufAliasNum)
            {
                bx::write(
                    m_fgMemWriter
                    , (const void*)m_writeForcedBufferAliases.getIdToData(pass).data()
                    , (uint32_t)(sizeof(WriteOperationAlias) * passMeta.writeBufAliasNum)
                    , nullptr
                );
            }

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeBufferData()
    {
        uint16_t bufCount = m_bufferHandles.getNumHandles();

        for (uint16_t ii = 0; ii < bufCount; ++ii)
        {
            BufferHandle buf { (m_bufferHandles.getHandleAt(ii)) };
            const BufferMetaData& meta = m_bufferMetas.getIdToData(buf);

            // frame graph data
            MagicTag magic{ MagicTag::register_buffer };
            bx::write(m_fgMemWriter, magic, nullptr);

            FGBufferCreateInfo info;
            info.bufId = meta.bufId;
            info.size = meta.size;
            info.pData = meta.pData;
            info.usage = meta.usage;
            info.memFlags = meta.memFlags;
            info.lifetime = meta.lifetime;

            info.initialState.access = AccessFlagBits::none;
            info.initialState.layout = ImageLayout::undefined;
            info.initialState.stage = PipelineStageFlagBits::none;

            bx::write(m_fgMemWriter, info, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeImageData()
    {
        uint16_t imgCount = m_imageHandles.getNumHandles();

        for (uint16_t ii = 0; ii < imgCount; ++ii)
        {
            ImageHandle img { (m_imageHandles.getHandleAt(ii)) };
            const ImageMetaData& meta = m_imageMetas.getIdToData(img);

            // frame graph data
            MagicTag magic{ MagicTag::register_image };
            bx::write(m_fgMemWriter, magic, nullptr);

            FGImageCreateInfo info;
            info.imgId = meta.imgId;
            info.width = meta.width;
            info.height = meta.height;
            info.depth = meta.depth;
            info.numMips = meta.numMips;
            info.size = meta.size;
            info.pData = meta.pData;

            info.type = meta.type;
            info.viewType = meta.viewType;
            info.format = meta.format;
            info.layout = meta.layout;
            info.numLayers = meta.numLayers;
            info.usage = meta.usage;
            info.lifetime = meta.lifetime;
            info.bpp = meta.bpp;
            info.aspectFlags = meta.aspectFlags;
            info.viewCount = meta.viewCount;

            info.initialState.access = AccessFlagBits::none;
            info.initialState.layout = ImageLayout::undefined;
            info.initialState.stage = PipelineStageFlagBits::none;

            for (uint32_t mipIdx = 0; mipIdx < kMaxNumOfImageMipLevel; ++ mipIdx)
            {
                info.mipViews[mipIdx] = mipIdx < info.viewCount ? meta.mipViews[mipIdx] : ImageViewHandle{kInvalidHandle};
            }

            bx::write(m_fgMemWriter, info, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }

        uint16_t imgViewCount = m_imageViewHandles.getNumHandles();

        for (uint16_t ii = 0; ii < imgViewCount; ++ ii)
        {
            ImageViewHandle imgView { (m_imageViewHandles.getHandleAt(ii)) };
            const ImageViewDesc& desc = m_imageViewDesc.getIdToData(imgView);

            // frame graph data
            bx::write(m_fgMemWriter, MagicTag::register_image_view, nullptr);

            bx::write(m_fgMemWriter, desc, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeAliasData()
    {
        // buffers
        for (uint16_t ii = 0; ii < m_aliasBuffers.size(); ++ii)
        {
            BufferHandle buf = m_aliasBuffers.getIdAt(ii);
            const stl::vector<BufferHandle>& aliasVec = m_aliasBuffers.getIdToData(buf);

            if (aliasVec.empty())
            {
                continue;
            }

            // magic tag
            MagicTag magic{ MagicTag::force_alias_buffer };
            bx::write(m_fgMemWriter, magic, nullptr);

            // info
            ResAliasInfo info;
            info.aliasNum = (uint16_t)aliasVec.size();
            info.resBase = buf.id;
            bx::write(m_fgMemWriter, info, nullptr);

            // actual alias indexes
            bx::write(m_fgMemWriter, aliasVec.data(), uint32_t(sizeof(uint16_t) * aliasVec.size()), nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
            
        }

        // images
        for (uint16_t ii = 0; ii < m_aliasImages.size(); ++ii)
        {
            ImageHandle img  = m_aliasImages.getIdAt(ii);
            const stl::vector<ImageHandle>& aliasVec = m_aliasImages.getIdToData(img);

            if (aliasVec.empty())
            {
                continue;
            }


            const ImageMetaData& meta = m_imageMetas.getIdToData(img);
            MagicTag magic{ MagicTag::force_alias_image };
            bx::write(m_fgMemWriter, magic, nullptr);

            // info
            ResAliasInfo info;
            info.aliasNum = (uint16_t)aliasVec.size();
            info.resBase = img.id;
            bx::write(m_fgMemWriter, info, nullptr);

            // actual alias indexes
            bx::write(m_fgMemWriter, aliasVec.data(), uint32_t(sizeof(uint16_t) * aliasVec.size()), nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeSamplerData()
    {
        for (uint16_t ii = 0; ii < m_samplerHandles.getNumHandles(); ++ii )
        {
            uint16_t samplerId = m_samplerHandles.getHandleAt(ii);
            const SamplerDesc& desc = m_samplerDescs.getIdToData({ samplerId });

            // magic tag
            bx::write(m_fgMemWriter, MagicTag::register_sampler, nullptr);

            SamplerMetaData meta{desc};
            meta.samplerId = samplerId;

            bx::write(m_fgMemWriter, meta, nullptr);

            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }
    }

    void Context::storeBackBufferData()
    {
        for (ImageHandle hImg : m_backBuffers)
        {
            MagicTag magic{ MagicTag::store_back_buffer };
            bx::write(m_fgMemWriter, magic, nullptr);
            bx::write(m_fgMemWriter, hImg.id, nullptr);
            bx::write(m_fgMemWriter, MagicTag::magic_body_end, nullptr);
        }

    }

    BufferHandle Context::aliasBuffer(const BufferHandle _baseBuf)
    {
        uint16_t aliasId = m_bufferHandles.alloc();

        auto actualBaseMap = m_bufferAliasMapToBase.find(_baseBuf.id);

        BufferHandle actualBase = _baseBuf;
        // check if the base buffer is an alias
        if (m_bufferAliasMapToBase.end() != actualBaseMap)
        {
            actualBase = actualBaseMap->second;
        }

        m_bufferAliasMapToBase.insert({ aliasId, actualBase });

        if (m_aliasBuffers.exist(actualBase))
        {
            stl::vector<BufferHandle>& aliasVecRef = m_aliasBuffers.getDataRef(actualBase);
            aliasVecRef.emplace_back(BufferHandle{ aliasId });
        }
        else 
        {
            stl::vector<BufferHandle> aliasVec{};
            aliasVec.emplace_back(BufferHandle{ aliasId });

            m_aliasBuffers.addOrUpdate({ actualBase }, aliasVec);
        }

        BufferMetaData meta = { m_bufferMetas.getIdToData(actualBase) };
        meta.bufId = aliasId;
        m_bufferMetas.addOrUpdate({ aliasId }, meta);

        bx::StringView baseName = getName(actualBase);
        setName(getNameManager(), baseName.getPtr(), baseName.getLength(), BufferHandle{ aliasId }, true);

        setRenderGraphDataDirty();

        return { aliasId };
    }

    ImageHandle Context::aliasImage(const ImageHandle _baseImg)
    {
        uint16_t aliasId = m_imageHandles.alloc();

        auto actualBaseMap = m_imageAliasMapToBase.find(_baseImg.id);

        ImageHandle actualBase = _baseImg;
        // check if the base buffer is an alias
        if (m_imageAliasMapToBase.end() != actualBaseMap)
        {
            actualBase = actualBaseMap->second;
        }

        m_imageAliasMapToBase.insert({ {aliasId}, actualBase });

        if (m_aliasImages.exist(actualBase))
        {
            stl::vector<ImageHandle>& aliasVecRef = m_aliasImages.getDataRef(actualBase);
            aliasVecRef.emplace_back(ImageHandle{ aliasId });
        }
        else
        {
            stl::vector<ImageHandle> aliasVec{};
            aliasVec.emplace_back(ImageHandle{ aliasId });

            m_aliasImages.addOrUpdate({ actualBase }, aliasVec);
        }

        ImageMetaData meta = { m_imageMetas.getIdToData(actualBase) };
        meta.imgId = aliasId;
        m_imageMetas.addOrUpdate({ aliasId }, meta);

        bx::StringView baseName = getName(actualBase);
        setName(getNameManager(), baseName.getPtr(), baseName.getLength(), ImageHandle{ aliasId }, true);

        setRenderGraphDataDirty();

        return { aliasId };
    }

    uint32_t availableBinding(ContinuousMap<PassHandle, stl::vector<uint32_t>>& _container, const PassHandle _hPass, const uint32_t _binding)
    {
        bool conflict = false;
        if (_container.exist(_hPass))
        {
            const stl::vector<uint32_t>& bindingVec = _container.getDataRef(_hPass);
            for (const uint32_t& binding : bindingVec)
            {
                if (binding == _binding)
                {
                    conflict = true;
                }
            }
        }
        else
        {
            stl::vector<uint32_t> bindingVec{};
            bindingVec.emplace_back(_binding);
            _container.addOrUpdate({ _hPass }, bindingVec);
        }

        return !conflict;
    }

    uint32_t insertResInteract(ContinuousMap<PassHandle, stl::vector<PassResInteract>>& _container
        , const PassHandle _hPass, const uint16_t _resId, const SamplerHandle _hSampler, const ResInteractDesc& _interact
    )
    {
        uint32_t vecSize = 0u;

        PassResInteract pri;
        pri.passId = _hPass.id;
        pri.resId = _resId;
        pri.interact = _interact;
        pri.samplerId = _hSampler.id;

        if (_container.exist(_hPass))
        {
            stl::vector<PassResInteract>& prInteractVec = _container.getDataRef(_hPass);
            prInteractVec.emplace_back(pri);

            vecSize = (uint32_t)prInteractVec.size();
        }
        else
        {
            stl::vector<PassResInteract> prInteractVec{};
            prInteractVec.emplace_back(pri);
            _container.addOrUpdate({ _hPass }, prInteractVec);

            vecSize = (uint32_t)prInteractVec.size();
        }

        return vecSize;
    }
    
    /*
    uint32_t insertResInteract(UniDataContainer<PassHandle, stl::vector<PassResInteract>>& _container
        , const PassHandle _hPass, const uint16_t _resId, const SamplerHandle _hSampler, const ResInteractDesc& _interact
        )
    {
        return insertResInteract(_container, _hPass, _resId, _hSampler, _interact);
    }
    */

    uint32_t insertWriteResAlias(ContinuousMap<PassHandle, stl::vector<WriteOperationAlias>>& _container, const PassHandle _hPass, const uint16_t _resIn, const uint16_t _resOut)
    {
        uint32_t vecSize = 0u;

        if (_container.exist(_hPass))
        {
            stl::vector<WriteOperationAlias>& aliasVec = _container.getDataRef(_hPass);
            aliasVec.emplace_back(WriteOperationAlias{ _resIn, _resOut });

            vecSize = (uint32_t)aliasVec.size();
        }
        else
        {
            stl::vector<WriteOperationAlias> aliasVec{};
            aliasVec.emplace_back(WriteOperationAlias{ _resIn, _resOut });
            _container.addOrUpdate({ _hPass }, aliasVec);

            vecSize = (uint32_t)aliasVec.size();
        }

        return vecSize;
    }

    void Context::bindVertexBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _vtxCount)
    {
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.vertexBufferId = _hBuf.id;

        passMeta.vertexCount = _vtxCount;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::none;
        interact.access = AccessFlagBits::none;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);

        setRenderGraphDataDirty();
    }

    void Context::bindIndexBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _idxCount)
    {
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.indexBufferId = _hBuf.id;

        passMeta.indexCount = _idxCount;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::none;
        interact.access = AccessFlagBits::none;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);

        setRenderGraphDataDirty();
    }

    void Context::setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _defaultMaxCount)
    {
        assert(isGraphics(_hPass));

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.indirectBufferId = _hBuf.id;
        passMeta.indirectBufOffset = _offset;
        passMeta.indirectBufStride = _stride;
        passMeta.indirectMaxDrawCount = _defaultMaxCount;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::draw_indirect;
        interact.access = AccessFlagBits::indirect_command_read;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);

        setRenderGraphDataDirty();
    }

    void Context::setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset)
    {
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.indirectCountBufferId = _hBuf.id;
        passMeta.indirectCountBufOffset = _offset;

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::draw_indirect;
        interact.access = AccessFlagBits::indirect_command_read;

        passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);

        setRenderGraphDataDirty();
    }

    void Context::bindBuffer(PassHandle _hPass, BufferHandle _buf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias)
    {
        if (!availableBinding(m_usedBindPoints, _hPass, _binding))
        {
            message(DebugMessageType::error, "trying to binding the same point: %d", _binding);
            return;
        }

        ResInteractDesc interact{};
        interact.binding = _binding;
        interact.stage = _stage;
        interact.access = _access;

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        if (isRead(_access))
        {
            passMeta.readBufferNum = insertResInteract(m_readBuffers, _hPass, _buf.id, { kInvalidHandle }, interact);
            setRenderGraphDataDirty();
        }

        if (isWrite(_access, _stage))
        {
            if (!isValid(_outAlias))
            {
                message(DebugMessageType::error, "the _outAlias must be valid if trying to write to resource in pass");
                return;
            }

            passMeta.writeBufferNum = insertResInteract(m_writeBuffers, _hPass, _buf.id, {kInvalidHandle}, interact);
            passMeta.writeBufAliasNum = insertWriteResAlias(m_writeForcedBufferAliases, _hPass, _buf.id, _outAlias.id);

            assert(passMeta.writeBufferNum == passMeta.writeBufAliasNum);
            setRenderGraphDataDirty();
        }
    }

    SamplerHandle Context::sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, ImageLayout _layout, SamplerReductionMode _reductionMode)
    {
        if (!availableBinding(m_usedBindPoints, _hPass, _binding))
        {
            message(DebugMessageType::error, "trying to binding the same point: %d", _binding);
            return { kInvalidHandle };
        }

        // get the sampler id
        SamplerDesc desc;
        desc.reductionMode = _reductionMode;

        SamplerHandle sampler = requestSampler(desc);

        ResInteractDesc interact{};
        interact.binding = _binding;
        interact.stage = _stage;
        interact.access = AccessFlagBits::shader_read;
        interact.layout = _layout;

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.readImageNum = insertResInteract(m_readImages, _hPass, _hImg.id, sampler, interact);
        passMeta.sampleImageNum++; // Note: is this the only way to read texture?

        setRenderGraphDataDirty();

        return sampler;
    }

    void Context::bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
        , const ImageHandle _outAlias, const ImageViewHandle _hImgView)
    {
        if (!availableBinding(m_usedBindPoints, _hPass, _binding))
        {
            message(DebugMessageType::error, "trying to binding the same point: %d", _binding);
            return;
        }

        ImageMetaData& meta = m_imageMetas.getDataRef(_hImg);
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);

        ResInteractDesc interact{};
        interact.binding = _binding; 
        interact.stage = _stage;
        interact.access = _access;
        interact.layout = _layout;


        if (isRead(_access))
        {
            if (isGraphics(_hPass) && isDepthStencil(_hImg))
            {
                assert((meta.aspectFlags & ImageAspectFlagBits::depth) > 0);
                interact.access |= AccessFlagBits::depth_stencil_attachment_read;
            }
            else if (isGraphics(_hPass) && isColorAttachment(_hImg))
            {
                assert((meta.aspectFlags & ImageAspectFlagBits::color) > 0);
                interact.access |= AccessFlagBits::color_attachment_read;
            }

            passMeta.readImageNum = insertResInteract(m_readImages, _hPass, _hImg.id, { kInvalidHandle }, interact);
            setRenderGraphDataDirty();
        }

        if (isWrite(_access, _stage))
        {
            if (!isValid(_outAlias))
            {
                message(DebugMessageType::error, "the _outAlias must be valid if trying to write to resource in pass");
            }

            if (isGraphics(_hPass))
            {
                message(DebugMessageType::error, "Graphics pass not allowed to write to texture via bind! try to use setAttachmentOutput instead.");
            }
            else if (isCompute(_hPass) && isNormalImage(_hImg))
            {
                const ImageViewDesc& imgViewDesc = isValid(_hImgView) ? m_imageViewDesc.getIdToData(_hImgView) : defaultImageView(_hImg);

                passMeta.writeImageNum = insertResInteract(m_writeImages, _hPass, _hImg.id, { kInvalidHandle }, interact);
                passMeta.writeImgAliasNum = insertWriteResAlias(m_writeForcedImageAliases, _hPass, _hImg.id, _outAlias.id);

                assert(passMeta.writeImageNum == passMeta.writeImgAliasNum);

                setRenderGraphDataDirty();
            }
            else
            {
                message(DebugMessageType::error, "write to an attachment via binding? nope!");
                assert(0);
            }
        }
    }

    void Context::setCustomRenderFunc(const PassHandle _hPass, RenderFuncPtr _func, const Memory* _dataMem)
    {
        assert(_func != nullptr);

        void * mem = alloc(getBxAllocator(), _dataMem->size);
        memcpy(mem, _dataMem->data, _dataMem->size);

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);
        passMeta.renderFunc = _func;
        passMeta.renderFuncDataPtr = mem;
        passMeta.renderFuncDataSize = _dataMem->size;

        release(_dataMem);
    }

    void Context::setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias)
    {
        if (isColorAttachment(_hImg) && !availableBinding(m_usedAttchBindPoints, _hPass, _attachmentIdx))
        {
            message(DebugMessageType::error, "trying to binding the same point: %d", _attachmentIdx);
            return;
        }

        assert(isGraphics(_hPass));
        assert(isValid(_outAlias));

        ImageMetaData& meta = m_imageMetas.getDataRef(_hImg);
        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);

        ResInteractDesc interact{};
        interact.binding = _attachmentIdx;
        interact.stage = PipelineStageFlagBits::color_attachment_output;
        interact.access = AccessFlagBits::none;

        if (isDepthStencil(_hImg))
        {
            assert((meta.aspectFlags & ImageAspectFlagBits::depth) > 0);
            assert(kInvalidHandle == passMeta.writeDepthId);

            passMeta.writeDepthId = _hImg.id;
            interact.layout = ImageLayout::depth_stencil_attachment_optimal;
        }
        else if (isColorAttachment(_hImg))
        {
            assert((meta.aspectFlags & ImageAspectFlagBits::color) > 0);
            interact.layout = ImageLayout::color_attachment_optimal;
        }
        else
        {
            assert(0);
        }

        passMeta.writeImageNum = insertResInteract(m_writeImages, _hPass, _hImg.id, { kInvalidHandle }, interact);
        passMeta.writeImgAliasNum = insertWriteResAlias(m_writeForcedImageAliases, _hPass, _hImg.id, _outAlias.id);
        assert(passMeta.writeImageNum == passMeta.writeImgAliasNum);

        setRenderGraphDataDirty();
    }

    void Context::resizeTexture(ImageHandle _hImg, uint32_t _width, uint32_t _height)
    {

    }

    void Context::fillBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _offset, const uint32_t _size, const uint32_t _value, const BufferHandle _outAlias)
    {

        if (!isFillBuffer(_hPass))
        {
            message(DebugMessageType::error, "pass type is not match to operation");
            return;
        }

        if (m_oneOpPassTouched.getIdToData(_hPass) == kTouched)
        {
            message(DebugMessageType::error, "one op pass already touched!");
            return;
        }

        BufferMetaData& bufMeta = m_bufferMetas.getDataRef(_hBuf);
        bufMeta.fillVal = _value;

        PassMetaData& passMeta = m_passMetas.getDataRef(_hPass);

        ResInteractDesc interact{};
        interact.binding = kDescriptorSetBindingDontCare;
        interact.stage = PipelineStageFlagBits::transfer;
        interact.access = AccessFlagBits::transfer_write;

        passMeta.writeBufferNum = insertResInteract(m_writeBuffers, _hPass, _hBuf.id, { kInvalidHandle }, interact);
        passMeta.writeBufAliasNum = insertWriteResAlias(m_writeForcedBufferAliases, _hPass, _hBuf.id, _outAlias.id);
        assert(passMeta.writeBufferNum == passMeta.writeBufAliasNum);

        m_oneOpPassTouched.update(_hPass, kTouched);
        setRenderGraphDataDirty();
    }

    void Context::setMultiFrame(ImageHandle _img)
    {
        ImageMetaData meta = m_imageMetas.getIdToData(_img);
        meta.lifetime = ResourceLifetime::non_transition;

        m_imageMetas.update(_img, meta);

        setRenderGraphDataDirty();
    }

    void Context::setMultiFrame(BufferHandle _buf)
    {
        BufferMetaData meta = m_bufferMetas.getIdToData(_buf);
        meta.lifetime = ResourceLifetime::non_transition;

        m_bufferMetas.update(_buf, meta);

        setRenderGraphDataDirty();
    }

    void Context::setPresentImage(ImageHandle _rt, uint32_t _mipLv)
    {
        m_presentImage = _rt;
        m_presentMipLevel = _mipLv;

        setRenderGraphDataDirty();
    }

    void Context::updateCustomRenderFuncData(const PassHandle _hPass, const Memory* _dataMem)
    {
        m_rhiContext->updateCustomFuncData(_hPass, _dataMem->data, _dataMem->size);
        release(_dataMem);
    }

    void Context::updateBuffer(const BufferHandle _hbuf, const Memory* _mem)
    {
        if (_mem == nullptr)
        {
            return;
        }

        m_rhiContext->updateBuffer(_hbuf, _mem->data, _mem->size);
        release(_mem);
    }

    void Context::updatePushConstants(const PassHandle _hPass, const Memory* _mem)
    {
        if (isRenderGraphDataDirty())
        {
            message(warning, "render graph is not baked yet!");
            return;
        }

        m_rhiContext->updatePushConstants(_hPass, _mem->data, _mem->size);
        release(_mem);
    }

    void Context::updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ)
    {
        if (1 > _threadCountX || 1 > _threadCountY || 1 > _threadCountZ)
        {
            message(warning, "thread count in any dimension should not less than 1! update failed");
            return;
        }

        m_rhiContext->updateThreadCount(_hPass, _threadCountX, _threadCountY, _threadCountZ);
    }

    static void* glfwNativeWindowHandle(GLFWwindow* _window)
    {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        return glfwGetWin32Window(_window);
#else
        #error unsupport platform
#endif
    }

    void Context::init()
    {
        m_pAllocator = getBxAllocator();
        m_pNameManager = getNameManager();

        // init glfw
        m_glfwManager.init(m_renderWidth, m_renderHeight, m_windowTitle);
        
        void* wnd = glfwNativeWindowHandle(m_glfwManager.getWindow());

        RHI_Config rhiConfig{};
        rhiConfig.windowWidth = m_renderWidth;
        rhiConfig.windowHeight = m_renderHeight;
        m_rhiContext = BX_NEW(getBxAllocator(), RHIContext_vk)(getBxAllocator(), rhiConfig, wnd);

        m_frameGraph = BX_NEW(getBxAllocator(), Framegraph2)(getBxAllocator(), m_rhiContext->memoryBlock());

        m_pFgMemBlock = m_frameGraph->getMemoryBlock();
        m_fgMemWriter = BX_NEW(getBxAllocator(), bx::MemoryWriter)(m_pFgMemBlock);

        setRenderGraphDataDirty();
    }

    bool Context::shouldClose()
    {
        return m_glfwManager.shouldClose();
    }

    void Context::run()
    {
        m_glfwManager.update();

        if (kInvalidHandle == m_presentImage.id)
        {
            message(error, "result render target is not set!");
            return;
        }

        if (isRenderGraphDataDirty())
        {
            bake();
        }

        render();
    }

    void Context::bake()
    {
        // store all resources
        storeBrief();
        storeShaderData();
        storeProgramData();
        storeImageData();
        storeBufferData();
        storeSamplerData();
        storePassData();
        storeAliasData();

        // write the finish tag
        MagicTag magic{ MagicTag::end };
        bx::write(m_fgMemWriter, magic, nullptr);

        m_frameGraph->setMemoryBlock(m_pFgMemBlock);
        m_frameGraph->bake();

        m_isRenderGraphDataDirty = false;

        m_rhiContext->bake();
    }

    void Context::getWindowSize(uint32_t& _width, uint32_t _height)
    {
        m_glfwManager.getWindowSize(_width, _height);
    }

    void Context::resizeBackBuffer(uint32_t _windth, uint32_t _height)
    {

    }

    void Context::render()
    {
        m_rhiContext->render();
    }

    void Context::shutdown()
    {
        deleteObject(m_pAllocator, m_frameGraph);
        deleteObject(m_pAllocator, m_rhiContext);
        deleteObject(m_pAllocator, m_fgMemWriter);

        // free memorys
        for (const Memory* pMem : m_inputMemories)
        {
            release(pMem);
        }

        m_pFgMemBlock = nullptr;
        m_pAllocator = nullptr;
    }

    void Context::setKeyCallback(KeyCallbackFunc _func)
    {
        if (!_func)
        {
            return;
        }
        s_keyCallback = _func;
    }

    void Context::setMouseMoveCallback(MouseMoveCallbackFunc _func)
    {
        if (!_func)
        {
            return;
        }
        s_mouseMoveCallback = _func;
    }

    void Context::setMouseButtonCallback(MouseButtonCallbackFunc _func)
    {
        if (!_func)
        {
            return;
        }
        s_mouseButtonCallback = _func;
    }

    double Context::getPassTime(const PassHandle _hPass)
    {
        return m_rhiContext->getPassTime(_hPass);
    }

    // ================================================
    static Context* s_ctx = nullptr;


    bool checkSupports(VulkanSupportExtension _ext)
    {
        return s_ctx->checkSupports(_ext);
    }


    ShaderHandle registShader(const char* _name, const char* _path)
    {
        return s_ctx->registShader(_name, _path);
    }

    ProgramHandle registProgram(const char* _name, ShaderHandleList _shaders, const uint32_t _sizePushConstants /*= 0*/)
    {
        const uint16_t shaderNum = static_cast<const uint16_t>(_shaders.size());
        const Memory* mem = alloc(shaderNum * sizeof(uint16_t));

        bx::StaticMemoryBlockWriter writer(mem->data, mem->size);
        for (uint16_t ii = 0; ii < shaderNum; ++ii)
        {
            const ShaderHandle& handle = begin(_shaders)[ii];
            bx::write(&writer, handle.id, nullptr);
        }

        return s_ctx->registProgram(_name, mem, shaderNum, _sizePushConstants);
    }

    BufferHandle registBuffer(const char* _name, const BufferDesc& _desc, const Memory* _mem /* = nullptr */, const ResourceLifetime _lifetime /*= ResourceLifetime::transision*/)
    {
        return s_ctx->registBuffer(_name, _desc, _mem, _lifetime);
    }

    ImageHandle registTexture(const char* _name, const ImageDesc& _desc, const Memory* _mem /* = nullptr */, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registTexture(_name, _desc, _mem, _lifetime);
    }

    ImageHandle registRenderTarget(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registRenderTarget(_name, _desc, _lifetime);
    }

    ImageHandle registDepthStencil(const char* _name, const ImageDesc& _desc, const ResourceLifetime _lifetime /*= ResourceLifetime::single_frame*/)
    {
        return s_ctx->registDepthStencil(_name, _desc, _lifetime);
    }

    vkz::ImageViewHandle registImageView(const char* _name, const ImageHandle _hImg, const uint32_t _baseMip, const uint32_t _mipLevel)
    {
        return s_ctx->registImageView(_name, _hImg, _baseMip, _mipLevel);
    }

    PassHandle registPass(const char* _name, PassDesc _desc)
    {
        return s_ctx->registPass(_name, _desc);
    }

    BufferHandle alias(const BufferHandle _baseBuf)
    {
        return s_ctx->aliasBuffer(_baseBuf);
    }

    ImageHandle alias(const ImageHandle _baseImg)
    {
        return s_ctx->aliasImage(_baseImg);
    }

    void bindVertexBuffer(PassHandle _hPass, BufferHandle _hBuf, const uint32_t _vtxCount /*= 0*/)
    {
        s_ctx->bindVertexBuffer(_hPass, _hBuf, _vtxCount);
    }

    void bindIndexBuffer(PassHandle _hPass, BufferHandle _hBuf, const uint32_t _idxCount /*= 0*/)
    {
        s_ctx->bindIndexBuffer(_hPass, _hBuf, _idxCount);
    }

    void setIndirectBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset, uint32_t _stride, uint32_t _maxCount)
    {
        s_ctx->setIndirectBuffer(_hPass, _hBuf, _offset, _stride, _maxCount);
    }

    void setIndirectCountBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _offset)
    {
        s_ctx->setIndirectCountBuffer(_hPass, _hBuf, _offset);
    }

    void bindBuffer(PassHandle _hPass, BufferHandle _hBuf, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, const BufferHandle _outAlias /*= {kInvalidHandle}*/)
    {
        s_ctx->bindBuffer(_hPass, _hBuf, _binding, _stage, _access, _outAlias);
    }

    SamplerHandle sampleImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, ImageLayout _layout, SamplerReductionMode _reductionMode)
    {
        return s_ctx->sampleImage(_hPass, _hImg, _binding, _stage, _layout, _reductionMode);
    }

    void bindImage(PassHandle _hPass, ImageHandle _hImg, uint32_t _binding, PipelineStageFlags _stage, AccessFlags _access, ImageLayout _layout
        , const ImageHandle _outAlias /*= {kInvalidHandle}*/, const ImageViewHandle _hImgView/* = {kInvalidHandle}*/)
    {
        s_ctx->bindImage(_hPass, _hImg, _binding, _stage, _access, _layout, _outAlias, _hImgView);
    }

    void setAttachmentOutput(const PassHandle _hPass, const ImageHandle _hImg, const uint32_t _attachmentIdx, const ImageHandle _outAlias)
    {
        s_ctx->setAttachmentOutput(_hPass, _hImg, _attachmentIdx, _outAlias);
    }

    void resizeTexture(ImageHandle _hImg, uint32_t _width, uint32_t _height)
    {
        s_ctx->resizeTexture(_hImg, _width, _height);
    }

    void fillBuffer(const PassHandle _hPass, const BufferHandle _hBuf, const uint32_t _offset, const uint32_t _size, const uint32_t _value, const BufferHandle _outAlias)
    {
        s_ctx->fillBuffer(_hPass, _hBuf, _offset, _size, _value, _outAlias);
    }

    void copyBuffer(const PassHandle _hPass, const BufferHandle _hSrc, const BufferHandle _hDst, const uint32_t _size)
    {
        ;
    }

    void blitImage(const PassHandle _hPass, const ImageHandle _hSrc, const ImageHandle _hDst)
    {
        ;
    }

    void setCustomRenderFunc(const PassHandle _hPass, RenderFuncPtr _func, const Memory* _dataMem)
    {
        s_ctx->setCustomRenderFunc(_hPass, _func, _dataMem);
    }

    void setPresentImage(ImageHandle _rt, uint32_t _mipLv /*= 0*/)
    {
        s_ctx->setPresentImage(_rt, _mipLv);
    }
    
    void updatePushConstants(const PassHandle _hPass, const Memory* _mem)
    {
        s_ctx->updatePushConstants(_hPass, _mem);
    }

    void updateThreadCount(const PassHandle _hPass, const uint32_t _threadCountX, const uint32_t _threadCountY, const uint32_t _threadCountZ)
    {
        s_ctx->updateThreadCount(_hPass, _threadCountX, _threadCountY, _threadCountZ);
    }

    void updateBuffer(const BufferHandle _hbuf, const Memory* _mem)
    {
        s_ctx->updateBuffer(_hbuf, _mem);
    }

    void updateCustomRenderFuncData(const PassHandle _hPass, const Memory* _dataMem)
    {
        s_ctx->updateCustomRenderFuncData(_hPass, _dataMem);
    }

    const Memory* alloc(uint32_t _sz)
    {
        if (_sz < 0)
        {
            message(error, "_sz < 0");
        }

        Memory* mem = (Memory*)alloc(getBxAllocator(), sizeof(Memory) + _sz);
        mem->size = _sz;
        mem->data = (uint8_t*)mem + sizeof(Memory);
        return mem;
    }

    const Memory* copy(const void* _data, uint32_t _sz)
    {
        if (_sz < 0)
        {
            message(error, "_sz < 0");
        }

        const Memory* mem = alloc(_sz);
        ::memcpy(mem->data, _data, _sz);
        return mem;
    }

    const Memory* copy(const Memory* _mem)
    {
        return copy(_mem->data, _mem->size);
    }

    const Memory* makeRef(const void* _data, uint32_t _sz, ReleaseFn _releaseFn /*= nullptr*/, void* _userData /*= nullptr*/)
    {
        MemoryRef* memRef = (MemoryRef*)alloc(getBxAllocator(), sizeof(MemoryRef));
        memRef->mem.size = _sz;
        memRef->mem.data = (uint8_t*)_data;
        memRef->releaseFn = _releaseFn;
        memRef->userData = _userData;
        return &memRef->mem;
    }

    // main data here
    bool init(VKZInitConfig _config /*= {}*/)
    {
        s_ctx = BX_NEW(getBxAllocator(), Context);
        s_ctx->setRenderSize(_config.windowWidth, _config.windowHeight);
        s_ctx->setWindowName(_config.name);
        s_ctx->init();

        return true;
    }

    bool shouldClose()
    {
        return s_ctx->shouldClose();
    }

    void bake()
    {
        s_ctx->bake();
    }

    void getWindowSize(uint32_t& _width, uint32_t _height)
    {
        s_ctx->getWindowSize(_width, _height);
    }

    void resizeBackBuffer(uint32_t _width, uint32_t _height)
    {
        s_ctx->resizeBackBuffer(_width, _height);
    }

    void run()
    {
        s_ctx->run();
    }

    void shutdown()
    {
        deleteObject(getBxAllocator(), s_ctx);
        shutdownAllocator();
    }

    const char* getName(ShaderHandle _hShader)
    {
        return getNameManager()->getName(_hShader);
    }

    const char* getName(ProgramHandle _hProg)
    {
        return getNameManager()->getName(_hProg);
    }
    
    const char* getName(PassHandle _hPass)
    {
        return getNameManager()->getName(_hPass);
    }

    const char* getName(BufferHandle _hBuf)
    {
        return getNameManager()->getName(_hBuf);
    }

    const char* getName(ImageHandle _hImg)
    {
        return getNameManager()->getName(_hImg);
    }

    const char* getName(ImageViewHandle _hImgView)
    {
        return getNameManager()->getName(_hImgView);
    }
    const char* getName(SamplerHandle _hSampler)
    {
        return getNameManager()->getName(_hSampler);
    }

    void setKeyCallback(KeyCallbackFunc _func)
    {
        s_ctx->setKeyCallback(_func);
    }

    void setMouseMoveCallback(MouseMoveCallbackFunc _func)
    {
        s_ctx->setMouseMoveCallback(_func);
    }

    void setMouseButtonCallback(MouseButtonCallbackFunc _func)
    {
        s_ctx->setMouseButtonCallback(_func);
    }

    double getPassTime(const PassHandle _hPass)
    {
        return s_ctx->getPassTime(_hPass);
    }

} // namespace vkz