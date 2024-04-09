#include "camera.h"

#include "kage_structs.h"
#include "entry/cmd.h"
#include "bx/string.h"
#include "entry/input.h"
#include "debug.h"

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
        update(0.0f, mouseState, true);

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
        m_direction.x = bx::cos(bx::toRad(m_pitch)) * bx::cos(bx::toRad(m_yaw));
        m_direction.y = bx::sin(bx::toRad(m_pitch));
        m_direction.z = bx::cos(bx::toRad(m_pitch)) * bx::sin(bx::toRad(m_yaw));
        
        m_front = bx::normalize(m_direction);

        m_right = bx::normalize(bx::cross(m_up, m_direction));
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
    }

    void processMouse(float _xpos, float _ypos, bool _constrainPitch = true)
    {
        float xoffset = _xpos - m_mousePrev.m_mx;
        float yoffset = _ypos - m_mousePrev.m_my;

        m_mousePrev.m_mx = m_mouseNow.m_mx;
        m_mousePrev.m_my = m_mouseNow.m_my;

        m_mouseNow.m_mx = int32_t(_xpos);
        m_mouseNow.m_my = int32_t(_ypos);

        xoffset *= m_mouseSpeed;
        yoffset *= m_mouseSpeed;

        m_yaw -= xoffset;
        m_pitch -= yoffset;

        if (_constrainPitch)
        {
            if (m_pitch > 89.f)
                m_pitch = 89.f;
            if (m_pitch < -89.f)
                m_pitch = -89.f;
        }
    }

    void update(float _delta, const entry::MouseState& _mouseState, bool _reset) 
    {
        processMouse(float(_mouseState.m_mx), float(_mouseState.m_my), true);
        updateVecs();
        processKey(_delta);

        m_at = bx::add(m_pos, m_direction);
    }

    void getViewMat(float* _mat)
    {
        bx::mtxLookAt(_mat, bx::load<bx::Vec3>(&m_pos.x), bx::load<bx::Vec3>(&m_at.x), bx::load<bx::Vec3>(&m_up.x));
    }

    void reset()
    {
        m_mouseNow.m_mx = 0;
        m_mouseNow.m_my = 0;
        m_mouseNow.m_mz = 0;
        m_mousePrev.m_mx = 0;
        m_mousePrev.m_my = 0;
        m_mousePrev.m_mz = 0;

        m_pos.x = 0.0f;
        m_pos.y = 0.f;
        m_pos.z = 20.0f;

        m_up.x = 0.0f;
        m_up.y = 1.0f;
        m_up.z = 0.0f;

        m_front.x = 0.0f;
        m_front.y = 0.0f;
        m_front.z = 1.0f;

        m_right.x = 1.0f;
        m_right.y = 0.0f;
        m_right.z = 0.0f;

        m_direction = m_front;
        m_at = bx::add(m_pos, m_direction);

        m_yaw = 0.01f;
        m_pitch = 0.01f;

        m_mouseSpeed = 0.01f;
        m_gamepadSpeed = 0.01f;
        m_moveSpeed = 0.01f;
        m_keys = 0;
        m_mouseDown = false;
    }

    void setPos(const bx::Vec3& _pos)
    {
        m_pos = _pos;
    }

    MouseCoords m_mousePrev;
    MouseCoords m_mouseNow;

    bx::Vec3 m_pos = bx::InitZero;
    bx::Vec3 m_up = bx::InitZero;
    bx::Vec3 m_front = bx::InitZero;
    bx::Vec3 m_direction = bx::InitZero;
    bx::Vec3 m_right = bx::InitZero;
    bx::Vec3 m_at = bx::InitZero;

    float m_yaw;
    float m_pitch;

    float m_mouseSpeed;
    float m_gamepadSpeed;
    float m_moveSpeed;

    uint8_t m_keys;
    bool m_mouseDown;

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

void freeCameraUpdate(float _delta, const entry::MouseState& _mouseState, bool _reset)
{
    s_freeCamera->update(_delta, _mouseState, _reset);
}

void freeCameraSetKeyState(uint8_t _key, bool _down)
{
    s_freeCamera->setKeyState(_key, _down);
}

bx::Vec3 freeCameraGetPos()
{
    return s_freeCamera->m_pos;
}
