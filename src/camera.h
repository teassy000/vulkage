#pragma once

#include "math.h"
#include "vkz_structs.h"

struct FreeCamera
{
    vec3 pos;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 worldUp;

    float yaw;
    float pitch;
    float moveSpeed;
    float mouseSensitivity;
};

void freeCameraInit(FreeCamera& camera, vec3 position, vec3 up, float yaw, float pitch);
mat4 freeCameraGetViewMatrix(const FreeCamera& camera);
void freeCameraProcessKeyboard(FreeCamera& camera, int key, float deltaTime);
void freeCameraProcessKeyboard(FreeCamera& camera, vkz::KeyEnum _key, float deltaTime);
void freeCameraProcessMouseKey(FreeCamera& camera, int key, int action, int mods);
void freeCameraProcessMouseMovement(FreeCamera& camera, float xoffset, float yoffset, bool constrainPitch = true);
void freeCameraProcessMouseScroll(FreeCamera& camera, float yoffset);
void freeCameraUpdateVectors(FreeCamera& camera);

