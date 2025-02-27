#pragma once

#include "entry/entry.h"
#include "kage_math.h"

#define CAMERA_KEY_FORWARD   UINT8_C(0x01)
#define CAMERA_KEY_BACKWARD  UINT8_C(0x02)
#define CAMERA_KEY_LEFT      UINT8_C(0x04)
#define CAMERA_KEY_RIGHT     UINT8_C(0x08)
#define CAMERA_KEY_UP        UINT8_C(0x10)
#define CAMERA_KEY_DOWN      UINT8_C(0x20)
#define CAMERA_KEY_RESET     UINT8_C(0x40)
#define CAMERA_KEY_TOGGLE_LOCK     UINT8_C(0x80)


void freeCameraInit(const vec3& _pos, const quat& _orit, const float _fov);
void freeCameraDestroy();
mat4 freeCameraGetOrthoProjMatrix(float _halfWidth, float _halfHeight, float _halfDepth);
mat4 freeCameraGetPerpProjMatrix(float _aspect, float _znear);
mat4 freeCameraGetViewMatrix();
vec3 freeCameraGetPos();
vec3 freeCameraGetFront();
float freeCameraGetFov();

bool freeCameraUpdate(float _delta, const entry::MouseState& _mouseState);

