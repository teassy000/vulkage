#include "camera.h"

#include "kage_structs.h"
#include "entry/cmd.h"
#include "bx/string.h"
#include "entry/input.h"
#include "debug.h"

#include "kage_math.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/rotate_vector.hpp"

void freeCameraSetKeyState(uint16_t _key, bool _down);

int cmdMove(CmdContext* /*_context*/, void* /*_userData*/, int _argc, char const* const* _argv)
{
    if (_argc > 1)
    {
        if (0 == bx::strCmp(_argv[1], "forward"))
        {
            freeCameraSetKeyState(CAMERA_KEY_FORWARD, true);
            return 0;
        }
        else if (0 == bx::strCmp(_argv[1], "left"))
        {
            freeCameraSetKeyState(CAMERA_KEY_LEFT, true);
            return 0;
        }
        else if (0 == bx::strCmp(_argv[1], "right"))
        {
            freeCameraSetKeyState(CAMERA_KEY_RIGHT, true);
            return 0;
        }
        else if (0 == bx::strCmp(_argv[1], "backward"))
        {
            freeCameraSetKeyState(CAMERA_KEY_BACKWARD, true);
            return 0;
        }
        else if (0 == bx::strCmp(_argv[1], "up"))
        {
            freeCameraSetKeyState(CAMERA_KEY_UP, true);
            return 0;
        }
        else if (0 == bx::strCmp(_argv[1], "down"))
        {
            freeCameraSetKeyState(CAMERA_KEY_DOWN, true);
            return 0;
        }
    }

    return 1;
}

int cmdOp(CmdContext* /*_context*/, void* /*_userData*/, int _argc, char const* const* _argv)
{
    if (_argc > 1)
    {
        if (0 == bx::strCmp(_argv[1], "reset"))
        {
            freeCameraSetKeyState(CAMERA_KEY_RESET, true);
            return 0;
        }
        else if (0 == bx::strCmp(_argv[1], "freecam"))
        {
            freeCameraSetKeyState(CAMERA_KEY_TOGGLE_FREE, true);
            return 0;
        }
        else if (0 == bx::strCmp(_argv[1], "lockcam"))
        {
            freeCameraSetKeyState(CAMERA_KEY_TOGGLE_LOCK, true);
            return 0;
        }
    }

    return 1;
}

static void cmd(const void* _userData)
{
    cmdExec((const char*)_userData);
}

static const InputBinding s_camBindings[] =
{
    { entry::Key::KeyW,             entry::Modifier::None, 0, cmd, "move forward"   },
    { entry::Key::GamepadUp,        entry::Modifier::None, 0, cmd, "move forward"   },
    { entry::Key::KeyA,             entry::Modifier::None, 0, cmd, "move left"      },
    { entry::Key::GamepadLeft,      entry::Modifier::None, 0, cmd, "move left"      },
    { entry::Key::KeyS,             entry::Modifier::None, 0, cmd, "move backward"  },
    { entry::Key::GamepadDown,      entry::Modifier::None, 0, cmd, "move backward"  },
    { entry::Key::KeyD,             entry::Modifier::None, 0, cmd, "move right"     },
    { entry::Key::GamepadRight,     entry::Modifier::None, 0, cmd, "move right"     },
    { entry::Key::KeyQ,             entry::Modifier::None, 0, cmd, "move up"        },
    { entry::Key::GamepadShoulderL, entry::Modifier::None, 0, cmd, "move up"        },
    { entry::Key::KeyE,             entry::Modifier::None, 0, cmd, "move down"      },
    { entry::Key::GamepadShoulderR, entry::Modifier::None, 0, cmd, "move down"      },

    { entry::Key::KeyR,             entry::Modifier::None, 0, cmd, "op reset"       },
    { entry::Key::KeyF,             entry::Modifier::None, 0, cmd, "op freecam"},
    { entry::Key::KeyL,             entry::Modifier::None, 0, cmd, "op lockcam"},
    
    { entry::Key::Esc,              entry::Modifier::None, 0, cmd, "exit"        },

    INPUT_BINDING_END
};


// left handed
// infinity far
// z in [0, 1], 0 is near, 1 is far
inline mat4 perspectiveProjection(float fovY, float aspectWbyH, float zNear)
{
    float f = 1.0f / tanf(fovY / 2.0f);

    return mat4(
        f / aspectWbyH, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, zNear, 0.0f);
}

struct FreeCamera
{
    struct MouseCoords
    {
        int32_t m_mx;
        int32_t m_my;
        int32_t m_mz;
    };


    FreeCamera()
    {
    }

    ~FreeCamera()
    {
        cmdRemove("move");
        inputRemoveBindings("camBindings");
    }

    void setKeyState(uint16_t _key, bool _down)
    {
        m_keys &= ~_key;
        m_keys |= _down ? _key : 0;
    }

    void updateVecs()
    {
        m_front = glm::normalize(m_rot * vec3(0.0f, 0.0f, 1.0f));
        m_right = glm::normalize(glm::cross(m_up, m_front));
    }

    void processKey(float _deltaTime)
    {
        if (m_keys & CAMERA_KEY_FORWARD)
        {
            m_pos = m_front * _deltaTime * m_moveSpeed + m_pos;
            setKeyState(CAMERA_KEY_FORWARD, false);
        }

        if (m_keys & CAMERA_KEY_BACKWARD)
        {
            m_pos = -m_front * _deltaTime * m_moveSpeed + m_pos;
            setKeyState(CAMERA_KEY_BACKWARD, false);
        }

        if (m_keys & CAMERA_KEY_LEFT)
        {
            m_pos = -m_right * _deltaTime * m_moveSpeed + m_pos;
            setKeyState(CAMERA_KEY_LEFT, false);
        }

        if (m_keys & CAMERA_KEY_RIGHT)
        {
            m_pos = m_right * _deltaTime * m_moveSpeed + m_pos;
            setKeyState(CAMERA_KEY_RIGHT, false);
        }

        if (m_keys & CAMERA_KEY_UP)
        {
            m_pos = m_up * _deltaTime * m_moveSpeed + m_pos;
            setKeyState(CAMERA_KEY_UP, false);
        }

        if (m_keys & CAMERA_KEY_DOWN)
        {
            m_pos = -m_up * _deltaTime * m_moveSpeed + m_pos;
            setKeyState(CAMERA_KEY_DOWN, false);
        }

        if (m_keys & CAMERA_KEY_RESET)
        {
            reset();
            setKeyState(CAMERA_KEY_RESET, false);
        }

        if (m_keys & CAMERA_KEY_TOGGLE_LOCK)
        {
            m_lock = true;
            setKeyState(CAMERA_KEY_TOGGLE_LOCK, false);
        }

        if (m_keys & CAMERA_KEY_TOGGLE_FREE)
        {
            m_lock = false;
            setKeyState(CAMERA_KEY_TOGGLE_FREE, false);
        }

    }

    void processMouse(float _xpos, float _ypos, bool _constrainPitch = true)
    {
        float xdelta = _xpos - cursorPos.x;
        float ydelta = _ypos - cursorPos.y;

        cursorPos.x = _xpos;
        cursorPos.y = _ypos;

        xdelta *= m_mouseSpeed;
        ydelta *= m_mouseSpeed;

        quat qu = glm::angleAxis(glm::radians(xdelta), vec3(0, 1, 0)); // yaw
        quat qr = glm::angleAxis(glm::radians(ydelta), vec3(1, 0, 0)); // pitch

        m_rot = qu  * m_rot * qr;

        updateVecs();
    }

    bool update(float _delta, const entry::MouseState& _mouseState) 
    {
        processKey(_delta);

        if (m_lock)
            processMouse(float(_mouseState.m_mx), float(_mouseState.m_my), true);

        return m_lock;
    }

    mat4 getViewMat()
    {
        mat4 view = glm::lookAtLH(
            m_pos,
            m_pos + m_front,
            m_up
        );


        return view;
    }

    // left handed
    // limited near/far
    // _width, _height, _depth are the dimensions of the view volume
    // volume is centered at the origin
    mat4 getOrthoProjMat(float _hw, float _hh, float _hd)
    {
        return glm::orthoLH(-_hw, _hw, -_hh, _hh, -_hd, _hd);
    }

    mat4 getPerspProjMat(float _aspect, float _znear)
    {
        return perspectiveProjection(glm::radians(m_fov), _aspect, _znear);
    }

    void reset()
    {
        m_pos.x = 0.0f;
        m_pos.y = 0.0f;
        m_pos.z = -10.0f;

        m_up = { 0.0f, 1.0f, 0.0f }; // y up
        m_front = { 0.0f, 0.0f, 1.0f }; // z forward

        m_mouseSpeed = 0.03f;
        m_gamepadSpeed = 0.01f;
        m_moveSpeed = 0.01f;
        m_keys = 0;

        m_pos = m_pos0;
        m_rot = glm::angleAxis(0.f, vec3(0, 0, 1));
        m_fov = m_fov0;

        updateVecs();
    }

    vec3 m_mousePrev{ 0.f};
    vec3 cursorPos{ 0.f };

    vec3 m_pos{ 0.f};
    quat m_rot{};
    vec3 m_up{ 0.f};
    vec3 m_front{ 0.f};
    vec3 m_right{ 0.f};
    bool m_lock{ false };

    float m_fov{ 0.f };

    float m_mouseSpeed{ 0.f };
    float m_gamepadSpeed{ 0.f };
    float m_moveSpeed{ 0.f };

    uint16_t m_keys;

    bool m_exit;

    // initial data
    vec3 m_pos0{0.f};
    quat m_rot0{};
    float m_fov0{0.f};
};

static FreeCamera* s_freeCamera = nullptr;

void freeCameraInit(const vec3& _pos, const quat& _orit, const float _fov)
{
    s_freeCamera = BX_NEW(entry::getAllocator(), FreeCamera);
    
    s_freeCamera->m_pos0 = _pos;
    s_freeCamera->m_rot0 = _orit;
    s_freeCamera->m_fov0 = _fov;

    s_freeCamera->reset();

    entry::MouseState mouseState;
    s_freeCamera->update(0.0f, mouseState);

    cmdAdd("move", cmdMove);
    cmdAdd("op", cmdOp);
    inputAddBindings("camBindings", s_camBindings);
}

void freeCameraDestroy()
{
    bx::deleteObject(entry::getAllocator(), s_freeCamera);
    s_freeCamera = nullptr;
}

mat4 freeCameraGetOrthoProjMatrix(float _halfWidth, float _halfHeight, float _halfDepth)
{
    return s_freeCamera->getOrthoProjMat(_halfWidth, _halfHeight, _halfDepth);
}

mat4 freeCameraGetPerpProjMatrix(float _aspect, float _znear)
{
    return s_freeCamera->getPerspProjMat(_aspect, _znear);
}

mat4 freeCameraGetViewMatrix()
{
    return s_freeCamera->getViewMat();
}

bool freeCameraUpdate(float _delta, const entry::MouseState& _mouseState)
{
    return s_freeCamera->update(_delta, _mouseState);
}

void freeCameraSetKeyState(uint16_t _key, bool _down)
{
    s_freeCamera->setKeyState(_key, _down);
}

vec3 freeCameraGetPos()
{
    return s_freeCamera->m_pos;
}

vec3 freeCameraGetFront()
{
    return s_freeCamera->m_front;
}

float freeCameraGetFov()
{
    return s_freeCamera->m_fov;
}
