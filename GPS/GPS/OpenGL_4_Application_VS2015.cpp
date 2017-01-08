//
//  main.cpp
//  OpenGL Shadows
//
//  Created by CGIS on 05/12/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#define GLEW_STATIC
#define TINYOBJLOADER_IMPLEMENTATION
#include <iostream>
#include "glm/glm.hpp"//core glm functionality
#include "glm/gtc/matrix_transform.hpp"//glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include <unistd.h>
#include "Shader.hpp"
#include "Camera.hpp"

#include "Model3D.hpp"
#include "Mesh.hpp"
#include <dirent.h>
#include <stdio.h>
#include <string.h>



#define MODEL_COUNT 100
#define LIGHT_COUNT 2


int glWindowWidth = 1280;
int glWindowHeight = 800;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;


//matrici si prostii de genu
glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat3 lightDirMatrix;
GLuint lightDirMatrixLoc;


//directia luminii globale (cred)
glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;


//point lights
glm::vec3 lightPos[LIGHT_COUNT];
GLuint lightPosLoc;



//Camera si alte prostii pt miscare la mouse
gps::Camera myCamera(glm::vec3(0.0f, 0.5f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f));
GLfloat cameraSpeed = 0.05f;
float cameraYaw= -90.0f;
float cameraPitch = 0.0f;
float lastY = glWindowHeight/2;
float lastX = glWindowWidth/2;
bool firstMouse = true;
bool pressedKeys[1024];

//niste floats sa pot misca prostii
GLfloat angle;
GLfloat lightAngle;

float light_z = -4.0f;

//modele 3d
gps::Model3D myModel;
gps::Model3D ground;
gps::Model3D lightCube;
gps::Model3D gun;

//modele incarcate automat
int modelCount;
gps::Model3D models[MODEL_COUNT]; //modele incarcate automat

//shaders
gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader depthMapShader;

//pt umbre (shadow map object si textura)
GLuint shadowMapFBO;
GLuint depthMapTexture;


//jump
bool jump = false;
float ax = 0.0f;

//ia din foldere si subfoldere toate modelele si le randeaza
void importModels(const char * path){
    DIR           *d;
    struct dirent *dir;
    d = opendir(path);
    char p[256];
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if(strcmp(dir->d_name,".")&&strcmp(dir->d_name,"..")){
                if(strlen(dir->d_name) > 4 && !strcmp(dir->d_name + strlen(dir->d_name) - 4, ".obj")){
                    //printf("%s\n", dir->d_name);
                    sprintf(p,"%s/%s",path,dir->d_name);
                    printf("%s\n",p); //path complet
                    
                    models[modelCount++] = gps::Model3D(p,path);
                }else if (dir->d_type==DT_DIR){
                    sprintf(p,"%s/%s/", path,dir->d_name);
                    importModels(p);
                }
                
            }
        }
        
        closedir(d);
    }
}

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO
	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	myCustomShader.useShaderProgram();

	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	
	lightShader.useShaderProgram();
	
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    
    if(firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    
    GLfloat sensitivity = 0.05;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    cameraYaw   += xoffset;
    cameraPitch += yoffset;
    
    if(cameraPitch > 89.0f)
        cameraPitch = 89.0f;
    if(cameraPitch < -89.0f)
        cameraPitch = -89.0f;
    myCamera.rotate(cameraPitch,cameraYaw);
}


void processMovement()
{

	if (pressedKeys[GLFW_KEY_Q]) {
		angle += 0.1f;
		if (angle > 360.0f)
			angle -= 360.0f;
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angle -= 0.1f;
		if (angle < 0.0f)
			angle += 360.0f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}
    if (pressedKeys[GLFW_KEY_X]){
        light_z -= 0.2f;
    }
    if (pressedKeys[GLFW_KEY_Z]){
        light_z += 0.2f;
    }
    if (pressedKeys[GLFW_KEY_SPACE]) {
        jump = true;
    }

	if (pressedKeys[GLFW_KEY_J]) {

		lightAngle += 0.3f;
		if (lightAngle > 360.0f)
			lightAngle -= 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle -= 0.3f; 
		if (lightAngle < 0.0f)
			lightAngle += 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}	
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	//for Mac OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);

	glfwWindowHint(GLFW_SAMPLES, 4);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	//glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}


void initFBOs()
{
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix()
{
	const GLfloat near_plane = 1.0f, far_plane = 10.0f;
	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);

	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

void initModels()
{
    //auto load
    importModels("./objects/auto-load/");
	//myModel = gps::Model3D("objects/nanosuit/nanosuit.obj", "objects/nanosuit/");
	ground = gps::Model3D("objects/ground/ground.obj", "objects/ground/");
	lightCube = gps::Model3D("objects/cube/cube.obj", "objects/cube/");
    gun = gps::Model3D("objects/weapon/gun_and_arms.obj", "objects/weapon/");
}

void initShaders()
{
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");
}

void initUniforms()
{
	myCustomShader.useShaderProgram();

	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");

	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	
	lightDirMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDirMatrix");

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 2.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    
    //lumina 1
    lightPos[0] = glm::vec3(0.0f,1.0f,4.0f);
    lightPosLoc = glGetUniformLocation(myCustomShader.shaderProgram,"light_pos[0]");
    glUniform3fv(lightPosLoc,1,glm::value_ptr(lightPos[0]));
   
    
    //lumina 2
    lightPos[1] = glm::vec3(0.0f,1.0f,light_z);
    lightPosLoc = glGetUniformLocation(myCustomShader.shaderProgram,"light_pos[1]");
    glUniform3fv(lightPosLoc,1,glm::value_ptr(lightPos[1]));
  
    
	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

//this function draws the auto loaded models
void drawAutoModels(gps::Shader shader)
{
    //printf("Drawing auto loaded models.\n");
    
    for(int i =0;i<modelCount;i++){
        models[i].Draw(shader);
    }
}




void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	processMovement();	

    
    // ====================================  FIRST PASS ========================================
	//render the scene to the depth buffer (first pass)

	depthMapShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
		
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	
    
    //===================================== MODELS DRAW =======================================
    
    //create model matrix for ground
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 
						1, 
						GL_FALSE, 
						glm::value_ptr(model));

	ground.Draw(depthMapShader);
    
    //=======================AUTO MODELS ======================================================
    
    //create model matrix for nanosuit
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    //send model matrix to shader
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
                       1,
                       GL_FALSE,
                       glm::value_ptr(model));
    
    drawAutoModels(depthMapShader);
    
    
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    
    //===================================== SECOND PASS =======================================
	//render the scene (second pass)

	myCustomShader.useShaderProgram();

	//send lightSpace matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	//send view matrix to shader
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"),
		1,
		GL_FALSE,
		glm::value_ptr(view));	

	//compute light direction transformation matrix
	lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
	//send lightDir matrix data to shader
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));

	glViewport(0, 0, retina_width, retina_height);
	myCustomShader.useShaderProgram();
    
    //===================================== LIGHTS ============================================
    //lumina 1
    lightPos[1] = glm::vec3(0.0f,1.0f,light_z);
    lightPosLoc = glGetUniformLocation(myCustomShader.shaderProgram,"light_pos[1]");
    glUniform3fv(lightPosLoc,1,glm::value_ptr(lightPos[1]));
    
    
	//bind the depth map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
	
    //====================================== MODELS DRAW =======================================

		
	//create model matrix for ground
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//create normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	ground.Draw(myCustomShader);

    //======================================== AUTO MODELS DRAW ================================
    //create model matrix for nanosuit
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    //send model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    
    //compute normal matrix
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    //send normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    drawAutoModels(myCustomShader);
    
    //======================================== Draw GUN ========================================
    
    // JUMP CODE
    if(jump == true){
        ax+=10.0f;
        if (ax>360.0f ){
            ax = 0.0f;
            jump = false;
        }
        glm::vec3 camPos = myCamera.cameraPosition;
        camPos.y+=glm::sin(glm::radians(ax))*0.049f;
        printf("Sinus: %f\n",camPos.y);
        myCamera.cameraPosition= camPos;
    }
    
    
    //glm::vec3(0.0f,-1.7f,2.5f)
    glm::mat4 model1 = glm::translate(glm::mat4(1.0f), myCamera.cameraPosition);
    model1 = glm::rotate(model1,glm::radians(-cameraYaw+98.0f),glm::vec3(0.0f,1.0f,0.0f));
    model1 = glm::rotate(model1,glm::radians(-cameraPitch),glm::vec3(1.0f,0.0f,0.0f));
    model1 = glm::translate(model1, glm::vec3(-0.02f,-1.739999,0.030002));
    
    glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram,"model"), 1, GL_FALSE, glm::value_ptr(model1));
    
    //compute normal matrix
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model1));
    //send normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    gun.Draw(myCustomShader);
    
    
	//draw a white cube around the light

//	lightShader.useShaderProgram();
//
//	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
//
//	model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
//	model = glm::translate(model, lightDir);
//	model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
//	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
//
//	lightCube.Draw(lightShader);
}

int main(int argc, const char * argv[]) {

    int dir = chdir("/Users/macbookpro/home stuff/xcode projects/git/Grafica/GPS/GPS");
    if(dir != 0){
        printf("Could not change working dir.\n");
        return 0;
    }
    
	initOpenGLWindow();
	initOpenGLState();
	initFBOs();
	initModels();
	initShaders();
	initUniforms();	
	glCheckError();
    
    
	while (!glfwWindowShouldClose(glWindow)) {
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	//close GL context and any other GLFW resources
	glfwTerminate();

	return 0;
}
