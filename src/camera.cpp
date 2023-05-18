#include "camera.h"
#include "GLFW/glfw3.h"

static bool recordingMouse = false;

void freeCameraInit(FreeCamera& camera, vec3 pos, vec3 up, float yaw, float pitch)
{
    camera.pos = pos;
    camera.worldUp = up;
    camera.yaw = yaw;
    camera.pitch = pitch;
    camera.front = vec3(0.f, 0.f, 1.f);
    camera.moveSpeed = .2f;
    camera.mouseSensitivity = .2f;
    freeCameraUpdateVectors(camera);
}

// the freeCamera use left handed coordinate
// pay attention to `perspective`, `yaw`, `front`
glm::mat4 freeCameraGetViewMatrix(const FreeCamera& camera)
{
    return glm::lookAt(camera.pos, camera.pos - camera.front, camera.up);
}

void freeCameraProcessKeyboard(FreeCamera& camera, int key, float deltaTime)
{
    float velocity = camera.moveSpeed * deltaTime;
    if (key == GLFW_KEY_W)
        camera.pos += camera.front * velocity;
    if (key == GLFW_KEY_S)
        camera.pos -= camera.front * velocity;
    if (key == GLFW_KEY_A)
        camera.pos -= camera.right * velocity;
    if (key == GLFW_KEY_D)
        camera.pos += camera.right * velocity;
    if (key == GLFW_KEY_R)
        freeCameraInit(camera, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, 90.f, 0.f);
    if (key == GLFW_KEY_F)
        recordingMouse = !recordingMouse;
}

void freeCameraProcessMouseKey(FreeCamera& camera, int key, int action, int mods)
{

}



void freeCameraProcessMouseMovement(FreeCamera& camera, float xposIn, float yposIn, bool constrainPitch /* = true*/)
{
    static float xpos{ 0.f }, ypos{ 0.f };
    
    float xoffset = xposIn - xpos;
    float yoffset = yposIn - ypos;

    xpos = xposIn;
    ypos = yposIn;

    if (!recordingMouse)
        return;

    xoffset *= camera.mouseSensitivity;
    yoffset *= camera.mouseSensitivity;

    camera.yaw -= xoffset;
    camera.pitch -= yoffset;



    if (constrainPitch)
    {
        if (camera.pitch > 89.f)
            camera.pitch = 89.f;
        if (camera.pitch < -89.f)
            camera.pitch = -89.f;
    }
    freeCameraUpdateVectors(camera);
}

void freeCameraProcessMouseScroll(FreeCamera& camera, float yoffset)
{
    if (camera.moveSpeed >= 1.f && camera.moveSpeed <= 10.f)
        camera.moveSpeed -= yoffset;
    if (camera.moveSpeed <= 1.f)
        camera.moveSpeed = 1.f;
    if (camera.moveSpeed >= 10.f)
        camera.moveSpeed = 10.f;
}

void freeCameraUpdateVectors(FreeCamera& camera)
{
    vec3 front;
    front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front.y = sin(glm::radians(camera.pitch));
    front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    camera.front = glm::normalize(front);

    // using left handed coordinate system
    camera.right = glm::normalize(glm::cross(camera.worldUp, camera.front));
    camera.up = glm::normalize(glm::cross(camera.front, camera.right));
}