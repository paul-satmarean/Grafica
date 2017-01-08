#version 410 core

#define LIGHT_COUNT 2

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 normal;
out vec4 fragPosEye;
out vec4 fragPosLightSpace;
out vec2 fragTexCoords;
out vec3 light[LIGHT_COUNT];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceTrMatrix;
uniform vec3 light_pos[LIGHT_COUNT];


void main() 
{
    
    for(int i=0;i<LIGHT_COUNT;i++){
        light[i] = vec3(view * vec4(light_pos[i],1.0f));
    }
    
	//compute eye space coordinates
	fragPosEye = view * model * vec4(vPosition, 1.0f);
	normal = vNormal;
	fragTexCoords = vTexCoords;
	fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f);
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);
}
