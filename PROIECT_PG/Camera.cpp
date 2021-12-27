#include "Camera.hpp"

namespace gps {
    // Camera constructors
    Camera::Camera() {}
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
        cameraFrontDirection = glm::vec3(0.0f, 0.0f, -3.0f);
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        glm::mat4 view = glm::lookAt(cameraPosition, cameraFrontDirection + cameraPosition, cameraUpDirection);
        return view;
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        const float camSpeed = speed;
        switch (direction) {
            case MOVE_FORWARD: cameraPosition += camSpeed * cameraFrontDirection; break;
            case MOVE_BACKWARD: cameraPosition -= camSpeed * cameraFrontDirection; break;
            case MOVE_LEFT: cameraPosition -= glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection)) * camSpeed; break;
            case MOVE_RIGHT: cameraPosition += glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection)) * camSpeed; break;
            default:
                break;
        }
    }

    void Camera::setCameraFrontDirection(glm::vec3 vec) {
        cameraFrontDirection = vec;
    }
}
