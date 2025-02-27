#pragma once

#include "entry/entry.h"
#include "kage_math.h"

#define CAMERA_KEY_FORWARD          UINT16_C(0x001)
#define CAMERA_KEY_BACKWARD         UINT16_C(0x002)
#define CAMERA_KEY_LEFT             UINT16_C(0x004)
#define CAMERA_KEY_RIGHT            UINT16_C(0x008)
#define CAMERA_KEY_UP               UINT16_C(0x010)
#define CAMERA_KEY_DOWN             UINT16_C(0x020)
#define CAMERA_KEY_RESET            UINT16_C(0x040)
#define CAMERA_KEY_TOGGLE_LOCK      UINT16_C(0x080)
#define CAMERA_KEY_TOGGLE_FREE      UINT16_C(0x100)


void freeCameraInit(const vec3& _pos, const quat& _orit, const float _fov);
void freeCameraDestroy();
mat4 freeCameraGetOrthoProjMatrix(float _halfWidth, float _halfHeight, float _halfDepth);
mat4 freeCameraGetPerpProjMatrix(float _aspect, float _znear);
mat4 freeCameraGetViewMatrix();
vec3 freeCameraGetPos();
vec3 freeCameraGetFront();
float freeCameraGetFov();

bool freeCameraUpdate(float _delta, const entry::MouseState& _mouseState);

