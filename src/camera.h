#pragma once

#include "bx/math.h"
#include "entry/entry.h"


#define CAMERA_KEY_FORWARD   UINT8_C(0x01)
#define CAMERA_KEY_BACKWARD  UINT8_C(0x02)
#define CAMERA_KEY_LEFT      UINT8_C(0x04)
#define CAMERA_KEY_RIGHT     UINT8_C(0x08)
#define CAMERA_KEY_UP        UINT8_C(0x10)
#define CAMERA_KEY_DOWN      UINT8_C(0x20)
#define CAMERA_KEY_RESET     UINT8_C(0x40)


void freeCameraInit();
void freeCameraDestroy();
void freeCameraGetViewMatrix(float* _mat);
bx::Vec3 freeCameraGetPos();

void freeCameraUpdate(float _delta, const entry::MouseState& _mouseState, bool _reset);

void freeCameraSetKeyState(uint8_t _key, bool _down);
