#include "camera.h"

#include "kage_structs.h"
#include "entry/cmd.h"
#include "bx/string.h"
#include "entry/input.h"
#include "debug.h"

void freeCameraSetKeyState(uint8_t _key, bool _down);

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
    { entry::Key::KeyQ,             entry::Modifier::None, 0, cmd, "move up"      },
    { entry::Key::GamepadShoulderL, entry::Modifier::None, 0, cmd, "move up"      },
    { entry::Key::KeyE,             entry::Modifier::None, 0, cmd, "move down"        },
    { entry::Key::GamepadShoulderR, entry::Modifier::None, 0, cmd, "move down"        },

    { entry::Key::KeyR,             entry::Modifier::None, 0, cmd, "op reset"       },
    
    { entry::Key::Esc,              entry::Modifier::None, 0, cmd, "exit"        },

    INPUT_BINDING_END
};


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
        reset();

        entry::MouseState mouseState;
        update(0.0f, mouseState);

        cmdAdd("move", cmdMove);
        cmdAdd("op", cmdOp);
        inputAddBindings("camBindings", s_camBindings);
    }

    ~FreeCamera()
    {
        cmdRemove("move");
        inputRemoveBindings("camBindings");
    }

    void setKeyState(uint8_t _key, bool _down)
    {
        m_keys &= ~_key;
        m_keys |= _down ? _key : 0;
    }

    void updateVecs()
    {
        m_front.x = bx::sin(bx::toRad(m_yaw));
        m_front.y = bx::sin(bx::toRad(m_pitch));
        m_front.z = bx::cos(bx::toRad(m_yaw)) * bx::cos(bx::toRad(m_pitch));

        m_right = bx::normalize(bx::cross(m_up, m_front));
    }

    void processKey(float _deltaTime)
    {
        if (m_keys & CAMERA_KEY_FORWARD)
        {
            m_pos = bx::mad(m_front, _deltaTime * m_moveSpeed, m_pos);
            setKeyState(CAMERA_KEY_FORWARD, false);
        }

        if (m_keys & CAMERA_KEY_BACKWARD)
        {
            m_pos = bx::mad(m_front, -_deltaTime * m_moveSpeed, m_pos);
            setKeyState(CAMERA_KEY_BACKWARD, false);
        }

        if (m_keys & CAMERA_KEY_LEFT)
        {
            m_pos = bx::mad(m_right, -_deltaTime * m_moveSpeed, m_pos);
            setKeyState(CAMERA_KEY_LEFT, false);
        }

        if (m_keys & CAMERA_KEY_RIGHT)
        {
            m_pos = bx::mad(m_right, _deltaTime * m_moveSpeed, m_pos);
            setKeyState(CAMERA_KEY_RIGHT, false);
        }

        if (m_keys & CAMERA_KEY_UP)
        {
            m_pos = bx::mad(m_up, _deltaTime * m_moveSpeed, m_pos);
            setKeyState(CAMERA_KEY_UP, false);
        }

        if (m_keys & CAMERA_KEY_DOWN)
        {
            m_pos = bx::mad(m_up, -_deltaTime * m_moveSpeed, m_pos);
            setKeyState(CAMERA_KEY_DOWN, false);
        }

        if (m_keys & CAMERA_KEY_RESET)
        {
            reset();
            setKeyState(CAMERA_KEY_RESET, false);
        }

        m_at = bx::add(m_pos, m_front);
    }

    void processMouse(float _xpos, float _ypos, bool _constrainPitch = true)
    {
        float xdelta = _xpos - cursorPos.x;
        float ydelta = _ypos - cursorPos.y;

        cursorPos.x = _xpos;
        cursorPos.y = _ypos;

        xdelta *= m_mouseSpeed;
        ydelta *= m_mouseSpeed;

        m_yaw += xdelta; 
        m_pitch -= ydelta; // reversed since the y-coordinates go from bottom to top

        if (_constrainPitch)
        {
            m_pitch = bx::clamp(m_pitch, -89.5f, 89.5f);
        }

        updateVecs();
    }

    void update(float _delta, const entry::MouseState& _mouseState) 
    {
        processMouse(float(_mouseState.m_mx), float(_mouseState.m_my), true);
        processKey(_delta);
    }

    void getViewMat(float* _mat)
    {
        bx::mtxLookAt(
            _mat
            , bx::load<bx::Vec3>(&m_pos.x)
            , bx::load<bx::Vec3>(&m_at.x)
            , bx::load<bx::Vec3>(&m_up.x)
        );
    }

    void reset()
    {
        m_pos.x = 0.0f;
        m_pos.y = 0.0f;
        m_pos.z = -10.0f;

        m_yaw = 0.001f;
        m_pitch = 0.001f;

        m_up = { 0.0f, 1.0f, 0.0f };

        m_mouseSpeed = 0.01f;
        m_gamepadSpeed = 0.01f;
        m_moveSpeed = 0.01f;
        m_keys = 0;

        updateVecs();

        m_at = bx::add(m_pos, m_front);
    }

    bx::Vec3 m_mousePrev{ bx::InitZero };
    bx::Vec3 cursorPos{ bx::InitZero };

    bx::Vec3 m_pos{ bx::InitZero };
    bx::Vec3 m_up{ bx::InitZero };
    bx::Vec3 m_front{ bx::InitZero };
    bx::Vec3 m_right{ bx::InitZero };
    bx::Vec3 m_at{ bx::InitZero };

    float m_yaw{ 0.f };
    float m_pitch{ 0.f };

    float m_mouseSpeed{ 0.f };
    float m_gamepadSpeed{ 0.f };
    float m_moveSpeed{ 0.f };

    uint8_t m_keys;

    bool m_exit;
};

static FreeCamera* s_freeCamera = nullptr;

void freeCameraInit()
{
    s_freeCamera = BX_NEW(entry::getAllocator(), FreeCamera);
}


void freeCameraDestroy()
{
    bx::deleteObject(entry::getAllocator(), s_freeCamera);
    s_freeCamera = nullptr;
}

void freeCameraGetViewMatrix(float* _mat)
{
    s_freeCamera->getViewMat(_mat);
}

void freeCameraUpdate(float _delta, const entry::MouseState& _mouseState)
{
    s_freeCamera->update(_delta, _mouseState);
}

void freeCameraSetKeyState(uint8_t _key, bool _down)
{
    s_freeCamera->setKeyState(_key, _down);
}

bx::Vec3 freeCameraGetPos()
{
    return s_freeCamera->m_pos;
}
