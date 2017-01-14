//
//  Camera.cpp
//  lab5_grafica
//
//  Created by Paul Satmarean on 11/4/16.
//  Copyright © 2016 MacBook Pro. All rights reserved.
//

#include "Camera.hpp"
#define GODMODE


glm::mat4 viewMatrix;

namespace gps{
    
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget){
        this->cameraTarget = cameraTarget;
        this->cameraPosition = cameraPosition;
        //vectorul de directie normalizat
        this->cameraDirection = glm::normalize(cameraTarget-cameraPosition);
        
        //cross intre vectorul de directie si axa oy deci practic arata inspre dreapta
        //relativ cu vectorul de directie
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraDirection,glm::vec3(0.0f,1.0f,0.0f)));
        
    }
    
    glm::mat4 Camera::getViewMatrix(){
        return glm::lookAt(cameraPosition,cameraPosition+cameraDirection,glm::vec3(0.0f,1.0f,0.0f));
    }
    
    glm::vec3 Camera::getCameraTarget(){
        return this->cameraTarget;
    }
    
    void Camera::move(gps::MOVE_DIRECTION direction, float speed){
        glm::vec3 modDir = cameraDirection;
#ifndef GODMODE
        modDir.y = 0.0f;
#endif
        glm::vec3 modRightDir = cameraRightDirection;
#ifndef GODMODE
        modRightDir.y = 0.0f;
#endif
        switch (direction) {
            case MOVE_FORWARD:
                cameraPosition += modDir * speed;
                break;
            case MOVE_BACKWARD:
                cameraPosition -= modDir * speed;
                break;
            case MOVE_LEFT:
                cameraPosition -= modRightDir * speed;
                break;
            case MOVE_RIGHT:
                cameraPosition +=modRightDir * speed;
                
            default:
                break;
        }
    }
    
    void Camera::rotate(float pitch, float yaw){
        glm::vec3 direction(1.0f);
        direction.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
        direction.y = sin(glm::radians(pitch));
        direction.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
        cameraDirection = glm::normalize(direction);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraDirection,glm::vec3(0.0f,1.0f,0.0f)));
    }
    
    
}
