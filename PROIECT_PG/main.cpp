#define GLEW_STATIC
#define S_WIDTH 800
#define S_HEIGHT 600
#define M_WIDTH 1280
#define M_HEIGHT 720
#define L_WIDTH 1920
#define L_HEIGHT 1080
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

int glWindowWidth = M_WIDTH;
int glWindowHeight = M_HEIGHT;
int retina_width, retina_height;
bool firstMouse = true;
bool goingUp = true;
float editModeYaw = -90.0f, editModePitch = 0.0f;
float viewModeYaw =  90.0f, viewModePitch = 0.0f;
float editModelastX = M_WIDTH / 2, editModelastY = M_HEIGHT / 2;
float viewModelastX = M_WIDTH / 2, viewModelastY = M_HEIGHT / 2;
float levitation = 0.0f;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
const char* glsl_version = "#version 150";

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 d_lightRotation;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
glm::vec3 d_lightDir;
glm::vec3 d_lightColor;
glm::vec3 d_lightSourceColor(1.0f);
glm::vec3 p_lightPos;
glm::vec3 p_lightDir;
glm::vec3 p_lightColor;
glm::vec3 p_lightSourceColor(0.647f, 0.165f, 0.165f);

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLuint lightSourceColorLoc;
GLuint d_lightDirLoc;
GLuint d_lightColorLoc;
GLuint p_lightPosLoc;
GLuint p_lightDirLoc;
GLuint p_lightColorLoc;
GLuint skyboxLoc;

// cameras
gps::Camera editModeCamera(
                     glm::vec3(0.0f, 2.0f, 5.5f),
                     glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f));

gps::Camera viewModeCamera(
                     glm::vec3(0.0f, 1.0f, -2.0f),
                     glm::vec3(0.0f, 1.0f, -3.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f));

gps::Camera *activeCamera = &editModeCamera;

GLfloat cameraSpeed = 0.1f;

GLboolean pressedKeys[1024];
bool toggleLights = false;  // false - directional,           true - point
bool editMode = true;       // false - no GUI viewModeCamera, true - show GUI, editModeCamera
float lightSourceColorPicker[3] = {0.0f, 1.0f, 1.0f};

// first light parameters
GLfloat firstLightAngle;
GLfloat firstLightX = 2.0f;
GLfloat firstLightY = 0.0f;

// second light parameters
GLfloat secondLightX = 1.0f;
GLfloat secondLightY = 1.0f;
GLfloat secondLightZ = 0.0f;

// ship parameters
GLfloat shipX = 0.0f;
GLfloat shipY = 1.0f;
GLfloat shipZ = 0.0f;
GLfloat shipAngleX = 0.0f;
GLfloat shipAngleY = 0.0f;
GLfloat shipAngleZ = 0.0f;
GLfloat shipScale = 0.01;

// skybox
std::vector<const GLchar*> faces;
gps::SkyBox mySkyBox;

// models
gps::Model3D teapot;
gps::Model3D lightCube;
gps::Model3D screenQuad;
gps::Model3D starFighter;
gps::Model3D terrain;
gps::Model3D lightSphere;
GLfloat angle;

// shaders
gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;
gps::Shader skyboxShader;

// shadows
GLuint shadowMapFBO;
GLuint depthMapTexture;
bool showDepthMap;

// GUI variables
ImVec2 prevWindows;

GLenum glCheckError_(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
    // get the new dimensions of the window
    glfwGetFramebufferSize(myWindow.getWindow(), &retina_width, &retina_height);
    
    // recompute the projection matrix and send it to the shader
    myCustomShader.useShaderProgram();
    projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // redraw the window
    glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    else
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
        showDepthMap = !showDepthMap;
    
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        toggleLights = !toggleLights;
    
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        editMode = !editMode;
        if (editMode) {
            // make edit camera active
            activeCamera = &editModeCamera;
            // enable cursor
            glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            // make view camera active
            activeCamera = &viewModeCamera;
            // disable cursor
            glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            pressedKeys[key] = true;
        else if (action == GLFW_RELEASE)
            pressedKeys[key] = false;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        if (editMode) {
            editModelastX = xpos;
            editModelastY = ypos;
        } else {
            viewModelastX = xpos;
            viewModelastY = ypos;
        }
        firstMouse = false;
    }
    
    float xoffset;
    float yoffset;
    if (editMode) {
        xoffset = xpos - editModelastX;
        yoffset = editModelastY - ypos;
        editModelastX = xpos;
        editModelastY = ypos;
    } else {
        xoffset = xpos - viewModelastX;
        yoffset = viewModelastY - ypos;
        viewModelastX = xpos;
        viewModelastY = ypos;
    }
        
        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;
    
        glm::vec3 direction;
        
        if (editMode) {
            editModeYaw   += xoffset;
            editModePitch += yoffset;
            
            if(editModePitch > 89.0f)
                editModePitch = 89.0f;
            if(editModePitch < -89.0f)
                editModePitch = -89.0f;
            
            direction.x = cos(glm::radians(editModeYaw)) * cos(glm::radians(editModePitch));
            direction.y = sin(glm::radians(editModePitch));
            direction.z = sin(glm::radians(editModeYaw)) * cos(glm::radians(editModePitch));
        } else {
            viewModeYaw   += xoffset;
            viewModePitch += yoffset;
            
            printf("YAW: %f, PITCH: %f\n", viewModeYaw, viewModePitch);
            if(viewModePitch > 89.0f)
                viewModePitch = 89.0f;
            if(viewModePitch < -89.0f)
                viewModePitch = -89.0f;
            
            direction.x = cos(glm::radians(viewModeYaw)) * cos(glm::radians(viewModePitch));
            direction.y = sin(glm::radians(viewModePitch));
            direction.z = sin(glm::radians(viewModeYaw)) * cos(glm::radians(viewModePitch));
        }
    
        activeCamera->setCameraFrontDirection(glm::normalize(direction));
        
        view = activeCamera->getViewMatrix();
        myCustomShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void processMovement() {
    // rotate directional light
    if (pressedKeys[GLFW_KEY_U]) {
        firstLightAngle -= 1.0f;
    }
    
    // rotate directional light
    if (pressedKeys[GLFW_KEY_O]) {
        firstLightAngle += 1.0f;
    }
    
    // move selected light up
    if (pressedKeys[GLFW_KEY_I]) {
        if (editMode) {
            if (!toggleLights) {
                firstLightX += 0.01f;
            } else {
                secondLightX += 0.01f;
            }
        } else {
            shipAngleX -= 0.5f;
        }
    }
    
    // move selected light down
    if (pressedKeys[GLFW_KEY_K]) {
        if (editMode) {
            if (!toggleLights) {
                firstLightX -= 0.01f;
                if (firstLightX < 0.0f)
                    firstLightX = 0.0f;
            } else {
                secondLightX -= 0.01f;
                if (secondLightX < 0.0f)
                    secondLightX = 0.0f;
            }
        } else {
            shipAngleX += 0.5f;
        }
    }
    
    // move selected light left
    if (pressedKeys[GLFW_KEY_J]) {
        if (!toggleLights) {
            firstLightY -= 0.01f;
        } else {
            secondLightY -= 0.01f;
        }
    }
    
    // move selected light right
    if (pressedKeys[GLFW_KEY_L]) {
        if (!toggleLights) {
            firstLightY += 0.01f;
        } else {
            secondLightY += 0.01f;
        }
    }
    
    if (pressedKeys[GLFW_KEY_W]) {
        if (!editMode) {
            shipZ += cameraSpeed;
        }
        activeCamera->move(gps::MOVE_FORWARD, cameraSpeed);
        //update view matrix
        view = activeCamera->getViewMatrix();
        myCustomShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }
    
    if (pressedKeys[GLFW_KEY_S]) {
        if (!editMode) {
            shipZ -= cameraSpeed;
        }
        activeCamera->move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = activeCamera->getViewMatrix();
        myCustomShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }
    
    if (pressedKeys[GLFW_KEY_A]) {
        if (!editMode) {
            shipX += cameraSpeed;
            shipAngleZ -= 2.0f;
            if (shipAngleZ < -44.0f)
                shipAngleZ = -44.0f;
        }
        activeCamera->move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = activeCamera->getViewMatrix();
        myCustomShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    } else {
        if (!editMode) {
            if (shipAngleZ < 0.0f)
                shipAngleZ += 2.0f;
        }
    }
    
    if (pressedKeys[GLFW_KEY_D]) {
        if (!editMode) {
            shipX -= cameraSpeed;
            shipAngleZ += 2.0;
            if (shipAngleZ > 45.0f)
                shipAngleZ = 45.0f;
        }
        activeCamera->move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = activeCamera->getViewMatrix();
        myCustomShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    } else {
        if (!editMode) {
            if (shipAngleZ > 0.0f)
                shipAngleZ -= 2.0f;
        }
    }
    
    if (pressedKeys[GLFW_KEY_Q]) {
        if (!editMode) {
            shipAngleY += 0.5f;
        }
    }
    
    if (pressedKeys[GLFW_KEY_E]) {
        if (!editMode) {
            shipAngleY -= 0.5f;
        }
    }
}

bool initOpenGLWindow() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    
    myWindow.Create(glWindowWidth, glWindowHeight, "Proiect PG");
    if (!myWindow.getWindow()) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(myWindow.getWindow());
    glfwSwapInterval(1);
    
    // start GLEW extension handler
    glewExperimental = GL_TRUE;
    glewInit();
    
    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);
    
    //for RETINA display
    glfwGetFramebufferSize(myWindow.getWindow(), &retina_width, &retina_height);
    
    return true;
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void initOpenGLState() {
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face
    glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    //    teapot.LoadModel("models/teapot/teapot20segUT.obj");
    lightCube.LoadModel("models/cube/cube.obj");
    screenQuad.LoadModel("models/quad/quad.obj");
    starFighter.LoadModel("models/star-fighter/star-fighter.obj");
    terrain.LoadModel("models/terrain/terrain.obj");
    lightSphere.LoadModel("models/sphere/wooden_sphere.obj");
    faces.push_back("models/skybox/redplanet/right.tga");
    faces.push_back("models/skybox/redplanet/left.tga");
    faces.push_back("models/skybox/redplanet/top.tga");
    faces.push_back("models/skybox/redplanet/bottom.tga");
    faces.push_back("models/skybox/redplanet/back.tga");
    faces.push_back("models/skybox/redplanet/front.tga");
    mySkyBox.Load(faces);
}

void initShaders() {
    myCustomShader.loadShader(
                              "shaders/shaderStart.vert",
                              "shaders/shaderStart.frag");
    myCustomShader.useShaderProgram();
    lightShader.loadShader(
                           "shaders/lightCube.vert",
                           "shaders/lightCube.frag");
    lightShader.useShaderProgram();
    screenQuadShader.loadShader(
                                "shaders/screenQuad.vert",
                                "shaders/screenQuad.frag");
    screenQuadShader.useShaderProgram();
    depthMapShader.loadShader(
                              "shaders/depthMapShader.vert",
                              "shaders/depthMapShader.frag");
    depthMapShader.useShaderProgram();
    skyboxShader.loadShader(
                            "shaders/skyboxShader.vert",
                            "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();
}

void initUniforms() {
    myCustomShader.useShaderProgram();
    
    editModeCamera.setCameraFrontDirection(glm::vec3(0.0f, 0.0f, -3.0f));
    viewModeCamera.setCameraFrontDirection(glm::vec3(0.0f, 0.0f,  1.0f));
    
    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    
    view = activeCamera->getViewMatrix();
    viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    
    projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    lightSourceColorLoc = glGetUniformLocation(lightShader.shaderProgram, "lightSourceColor");
    
    /// ---------------------------------------------------- DIRECTIONAL LIGHT -----------------------------------------------------------------
    //set the light direction (direction towards the light)
    d_lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    d_lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(firstLightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    d_lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "d_lightDir");
    glUniform3fv(d_lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * d_lightRotation)) * d_lightDir));
    
    //set light color
    d_lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "d_lightColor");
    glUniform3fv(d_lightColorLoc, 1, glm::value_ptr(d_lightSourceColor));
    
    
    /// ------------------------------------------------------- POINT LIGHT -------------------------------------------------------------------
    // set the light position
    p_lightPos = glm::vec3(secondLightY, secondLightX, secondLightZ);
    p_lightPosLoc = glGetUniformLocation(myCustomShader.shaderProgram, "p_lightPos");
    glUniform3fv(p_lightPosLoc, 1, glm::value_ptr(glm::vec3(view * glm::vec4(p_lightPos, 1.0f))));
    
    // set the light direction (direction towards the light)
    p_lightDir = glm::vec3(0.0f, 0.0f, 1.0f);
    p_lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "p_lightDir");
    glUniform3fv(p_lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * p_lightDir));
    
    // set light color
    p_lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "p_lightColor");
    glUniform3fv(p_lightColorLoc, 1, glm::value_ptr(p_lightSourceColor));
    
    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    skyboxShader.useShaderProgram();
    view = activeCamera->getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    
    projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initFBO() {
    // generate FBO ID
    glGenFramebuffers(1, &shadowMapFBO);
    // create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
    const GLfloat nearPlane = 0.1f, farPlane = 10.0f;
    
    d_lightDir = glm::vec3(firstLightY, firstLightX, 1.0f);
    glm::mat4 lightView = glm::lookAt(glm::mat3(d_lightRotation) * d_lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, nearPlane, farPlane);
    
    return lightProjection * lightView;
}

void levitateShip() {
    if (!editMode) {
        if (goingUp && levitation <= 0.1f) {
            shipY += 0.001;
            levitation += 0.001;
            if (levitation > 0.1f)
                goingUp = false;
        } else if (!goingUp && levitation >= 0.0f) {
            shipY -= 0.001;
            levitation -= 0.001;
            if (levitation < 0.0f)
                goingUp = true;
        }
    }
}

void drawObjects(gps::Shader shader, bool depthPass) {
    
    shader.useShaderProgram();
    
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    
    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    
    model = glm::translate(glm::mat4(1.0f), glm::vec3(shipX, shipY, shipZ));
    model = glm::scale(model, glm::vec3(shipScale));
    model = glm::rotate(model, glm::radians(shipAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(shipAngleY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(shipAngleZ), glm::vec3(0.0f, 0.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    starFighter.Draw(shader);
    
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    terrain.Draw(shader);
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    levitateShip();
    
    // depth maps creation pass
    d_lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(firstLightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1,
                       GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    drawObjects(depthMapShader, true);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // render depth map on screen - toggled with the M key
    if (showDepthMap) {
        glViewport(0, 0, retina_width, retina_height);
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        screenQuadShader.useShaderProgram();
        
        //bind the depth map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);
        
        glDisable(GL_DEPTH_TEST);
        screenQuad.Draw(screenQuadShader);
        glEnable(GL_DEPTH_TEST);
    } else {
        // final scene rendering pass (with shadows)
        glViewport(0, 0, retina_width, retina_height);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        myCustomShader.useShaderProgram();
        
        view = activeCamera->getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(p_lightPosLoc, 1, glm::value_ptr(glm::vec3(view * glm::vec4(p_lightPos, 1.0f))));
        
        d_lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(firstLightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniform3fv(d_lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * d_lightRotation)) * d_lightDir));
        
        //bind the shadow map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
        
        glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(computeLightSpaceTrMatrix()));
        
        p_lightPos = glm::vec3(secondLightY, secondLightX, secondLightZ);
        glUniform3fv(p_lightPosLoc, 1, glm::value_ptr(glm::vec3(view * glm::vec4(p_lightPos, 1.0f))));
        
        // point light color
        glUniform3fv(p_lightColorLoc, 1, glm::value_ptr(glm::make_vec3(lightSourceColorPicker)));
        
        drawObjects(myCustomShader, false);
        
        //draw a white cube around the directional light
        lightShader.useShaderProgram();
        
        glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        
        model = d_lightRotation;
        model = glm::translate(model, 1.0f * d_lightDir);
        model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        // cube color
        glUniform3fv(lightSourceColorLoc, 1, glm::value_ptr(d_lightSourceColor));
        if (editMode) {
            lightCube.Draw(lightShader);
        }
        
        // draw a sphere around the point light
        model = glm::mat4(1.0f);
        model = glm::translate(model, 1.0f * p_lightPos);
        model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        // sphere color
        glUniform3fv(lightSourceColorLoc, 1, glm::value_ptr(glm::make_vec3(lightSourceColorPicker)));
        if (editMode) {
            lightSphere.Draw(lightShader);
        }
        
        mySkyBox.Draw(skyboxShader, view, projection);
    }
}

void cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
    myWindow.Delete();
}

void createGUI() {
    // create new ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // create GUI window for point light
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
    ImGui::Begin("Point Light", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Position:");
    ImGui::SliderFloat("posX", &secondLightX,   0.00f, 10.00f);
    ImGui::SliderFloat("posY", &secondLightY, -10.00f, 10.00f);
    ImGui::SliderFloat("posZ", &secondLightZ, -10.00f, 10.00f);
    ImGui::ColorPicker3("Color", lightSourceColorPicker);
    prevWindows = ImGui::GetWindowSize();
    ImGui::End();
    
    // create GUI window for directional light
    ImGui::SetNextWindowPos(ImVec2(10.0f, 20.0f + prevWindows.y));
    ImGui::Begin("Directional Light", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Position:");
    ImGui::SliderFloat("posX", &firstLightX,   0.00f, 10.00f);
    ImGui::SliderFloat("posY", &firstLightY, -10.00f, 10.00f);
    ImGui::Text("Rotation:");
    ImGui::SliderFloat("rotY", &firstLightAngle, 0.0f, 360.0f);
    prevWindows.y += ImGui::GetWindowSize().y;
    ImGui::End();
    
    // create GUI window for ship
    ImGui::SetNextWindowPos(ImVec2(10.0f, 30.0f + prevWindows.y));
    ImGui::Begin("Ship", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Position:");
    ImGui::SliderFloat("posX", &shipX,   0.00f, 10.00f);
    ImGui::SliderFloat("posY", &shipY, -10.00f, 10.00f);
    ImGui::SliderFloat("posZ", &shipZ, -10.00f, 10.00f);
    ImGui::Text("Rotation:");
    ImGui::SliderFloat("rotX", &shipAngleX, 0.0f, 360.0f);
    ImGui::SliderFloat("rotY", &shipAngleY, 0.0f, 360.0f);
    ImGui::SliderFloat("rotZ", &shipAngleZ, 0.0f, 360.0f);
    
    ImGui::Text("Scale:");
    ImGui::SliderFloat("Sc", &shipScale, 0.001f, 0.5f);
    prevWindows.y += ImGui::GetWindowSize().y;
    ImGui::End();
    
    // end ImGui frame
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main(int argc, const char * argv[]) {
    
    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    initOpenGLState();
    initModels();
    initShaders();
    initUniforms();
    initFBO();
    setWindowCallbacks();
    
    if (editMode) {
        glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(myWindow.getWindow(), true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
        renderScene();
        glfwPollEvents();
        if (editMode) {
            createGUI();
        }
        glfwSwapBuffers(myWindow.getWindow());
    }
    cleanup();
    
    return EXIT_SUCCESS;
}
