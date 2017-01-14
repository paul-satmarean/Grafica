#version 410 core

#define LIGHT_COUNT 2
#define SPOTLIGHT_COUNT 1

in vec3 normal;
in vec4 fragPosEye;
in vec4 fragPosLightSpace;
in vec2 fragTexCoords;
in vec3 light[LIGHT_COUNT];

out vec4 fColor;

//spotlights
struct Light {
    vec3 position;
    vec3 direction;
    float cutOff; // la conul intern
    float outerCutOff; // la conul extern
    
    float constant;
    float linear;
    float quadratic;
};


//lighting
uniform	mat3 normalMatrix;
uniform mat3 lightDirMatrix;
uniform	vec3 lightColor;
uniform	vec3 lightDir;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;


vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 64.0f;

//pt point lights (daca o sa am chef poate le fac structurate
float constant = 1.0f;
float linear = 0.0045f;
float quadratic = 0.0075f;

in Light spotlights[SPOTLIGHT_COUNT];

void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(normalMatrix * normal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDirMatrix * lightDir);	

	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);
	
	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDirN);
		
	//compute ambient light
    vec3 lambient = ambientStrength * lightColor;
	
	//compute diffuse light
	vec3 ldiffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	float specCoeff = pow(max(dot(halfVector, normalEye), 0.0f), shininess);
	vec3 lspecular = specularStrength * specCoeff * lightColor;
    specular += lspecular;
    ambient += lambient;
    diffuse += ldiffuse;
}



vec3 calcSpotLight(Light light){
    vec3 cameraPosEye = vec3(0.0f);
    
    vec3 lambient = ambientStrength * lightColor * vec3(texture(diffuseTexture, fragTexCoords));
    
    vec3 lightDir = normalize(light.position - fragPosEye.xyz);
    vec3 norm = normalize(normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ldiffuse = diff * vec3(texture(diffuseTexture, fragTexCoords));
    
    // Specular
    vec3 viewDir = normalize(cameraPosEye - fragPosEye.xyz);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 lspecular = spec * vec3(texture(specularTexture, fragTexCoords));
    
    //soft edges
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    ldiffuse  *= intensity;
    lspecular *= intensity;
    
    float distance    = length(light.position - fragPosEye.xyz);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    lambient  *= attenuation;
    ldiffuse  *= attenuation;
    lspecular *= attenuation;
    
    return (lambient +ldiffuse + lspecular);
}



vec3 calcPointLight(vec3 light)
{
    
    vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
    
    //transform normal
    vec3 normalEye = normalize(normalMatrix * normal);
    
    //compute light direction
    vec3 lightDirN = normalize(light - fragPosEye.xyz);
    
    //compute view direction
    vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);
    
    //compute half vector
    vec3 halfVector = normalize(lightDirN + viewDirN);
    
    //compute distance to light
    float dist = length(light - fragPosEye.xyz);
    
    //compute attenuation
    float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));
    
    
    //compute specular light
    vec3 reflection = reflect(-lightDirN, normalEye);
    
    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
    specular = specularStrength * specCoeff * lightColor;
    
    
    //compute ambient light
    vec3 ambient = att * ambientStrength * lightColor;
    vec3 diffuse = att * max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    vec3 specular = att * specularStrength * specCoeff * lightColor;
    
    ambient *= vec3(texture(diffuseTexture, fragTexCoords));
    diffuse *= vec3(texture(diffuseTexture, fragTexCoords));
    specular *= vec3(texture(specularTexture, fragTexCoords));
    
    return (ambient+ diffuse + specular);
    
    //compute ambient light
//    ambient += att * ambientStrength * lightColor;
//    diffuse += att * max(dot(normalEye, lightDirN), 0.0f) * lightColor;
//    specular += att * specularStrength * specCoeff * lightColor;
//    
//    ambient *= vec3(texture(diffuseTexture, fragTexCoords));
//    diffuse *= vec3(texture(diffuseTexture, fragTexCoords));
//    specular *= vec3(texture(specularTexture, fragTexCoords));
//    
    //return (ambient + diffuse + specular);
}


float computeShadow()
{
    
    // perform perspective divide
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    if(normalizedCoords.z > 1.0f)
        return 0.0f;
    // Transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5f + 0.5f;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;    
    // Get depth of current fragment from light's perspective
    float currentDepth = normalizedCoords.z;
    // Check whether current frag pos is in shadow
    float bias = 0.005f;
    float shadow = currentDepth - bias> closestDepth  ? 1.0f : 0.0f;

    return shadow;	
}

void main() 
{
    ambient = vec3(0.0f);
    specular = vec3(0.0f);
    diffuse = vec3(0.0f);
    
	computeLightComponents();
	
	float shadow = computeShadow();
	
	//modulate with diffuse map
	ambient *= vec3(texture(diffuseTexture, fragTexCoords));
	diffuse *= vec3(texture(diffuseTexture, fragTexCoords));
	//modulate woth specular map
	specular *= vec3(texture(specularTexture, fragTexCoords));
	
    
	//modulate with shadow
	vec3 color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);
    
//    for(int i =0;i<SPOTLIGHT_COUNT;i++){
//        color += calcSpotLight(spotlights[i]);
//    }
    
    //point lights
    for(int i =0;i<LIGHT_COUNT;i++){
        color += calcPointLight(light[i]);
    }
    
    //spotlights
    
    
    fColor = vec4(color, 1.0f);
}
