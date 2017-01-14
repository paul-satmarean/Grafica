#version 410 core

#define LIGHT_COUNT 6
#define SPOTLIGHT_COUNT 1

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

struct Light {
    vec3 position;
    vec3 direction;
    float cutOff; // la conul intern
    float outerCutOff; // la conul extern
    
    float constant;
    float linear;
    float quadratic;
    
};


out vec3 normal;
out vec4 fragPosEye;
out vec4 fragPosLightSpace;
out vec2 fragTexCoords;
out vec3 light[LIGHT_COUNT];
out Light spotlights[SPOTLIGHT_COUNT];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceTrMatrix;
uniform vec3 light_pos[LIGHT_COUNT];
uniform Light spotlight_pos[SPOTLIGHT_COUNT];

void main() 
{
    
    for(int i=0;i<SPOTLIGHT_COUNT;i++){
        spotlights[i].position = vec3(view * vec4(spotlight_pos[i].position,1.0f));
        spotlights[i].direction = vec3(view *vec4(spotlight_pos[i].direction,1.0f));
        spotlights[i].cutOff = spotlight_pos[i].cutOff;
        spotlights[i].outerCutOff = spotlight_pos[i].outerCutOff;
        spotlights[i].constant = spotlight_pos[i].constant;
        spotlights[i].linear = spotlight_pos[i].linear;
        spotlights[i].quadratic = spotlight_pos[i].quadratic;
        
    }
    
    
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
