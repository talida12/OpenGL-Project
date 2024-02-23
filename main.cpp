#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>
#include <random> //for snowing effect

#include <cstdlib> // for std::rand
#include <ctime>   // for std::time
#include <Windows.h>

#pragma warning (disable: 4996)

#define FIREWORKS_COUNT 18 // vor fi 18 pozitii pentru artificii
#define MAX_FIREWORK_QUARTER_SPLIT 15 // per sfert de cerc se vor imparti in 10 directii
#define MAX_FIREWORK_DIRECTIONS MAX_FIREWORK_QUARTER_SPLIT * 4


// window
gps::Window myWindow;

// matrices1
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat3 sleighNormalMatrix;

//models
glm::mat4 model;
glm::mat4 snowflakeModel;
glm::mat4 sleighModel;
glm::mat4 fireworksModels[FIREWORKS_COUNT * MAX_FIREWORK_DIRECTIONS];

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

glm::mat4 lightRotation;
GLfloat constant = 1.0f;
GLfloat linear = 0.1f;
GLfloat quadratic = 0.1f;

//TUR AUTOMAT AL SCENEI
FILE* recording;
bool isRecording = false;
bool isPlaying = false;
unsigned int lastTime = 0;

glm::vec3 lamps_positions[] = { glm::vec3(-1.0f, 2.9f, 0.0f), glm::vec3(-0.1f, 2.9f, 0.0f),
            glm::vec3(3.0f, 2.9f, 0.0f) };

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint showLightDirLoc; //global light

GLint lampColorLoc;
GLint constantLoc;
GLint linearLoc;
GLint quadraticLoc;

GLuint positionLoc;
GLuint positionsLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.4f, 5.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.25f;

GLboolean pressedKeys[1000];

// models
gps::Model3D snowflake;
gps::Model3D myScene;
gps::Model3D sleigh;
gps::Model3D fireworks3D[6];

struct Snowflake {
    glm::vec3 position;
    glm::vec3 velocity;
};

struct Sleigh {
    glm::vec3 position;
    glm::vec3 velocity;
};

struct Fireworks {
    glm::vec3 position;
    glm::vec3 velocity;
};

struct Firework {
    glm::vec3 initPosition;
    glm::vec3 currentPositions[MAX_FIREWORK_DIRECTIONS];
    glm::vec3 directions[MAX_FIREWORK_DIRECTIONS];
    float radius;
    bool active;
    float speed;
    int directionsCount;
};


Firework initFirework(glm::vec3 initPosition, int quarterSplit, float radius, float speed) {
    Firework firework;
    firework.initPosition = initPosition;
    glm::vec3 anchors[] = { glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0) };
    float floatSplit = quarterSplit * 1.0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < quarterSplit; j++) {
            firework.currentPositions[i * quarterSplit + j] = initPosition;
            float floatJ = j * 1.0;
            glm::vec3 dir = (anchors[i] * floatJ + anchors[(i + 1) % 4] * (floatSplit - floatJ)) / floatSplit;
            // normalize
            dir /= sqrtf(glm::dot(dir, dir));
            firework.directions[i * quarterSplit + j] = dir;
        }
    }
    firework.active = false;
    firework.radius = radius;
    firework.speed = speed;
    firework.directionsCount = 4 * quarterSplit;
    return firework;
}

Firework fireworks[FIREWORKS_COUNT];
int fireworksCursor = 0;

void initFireworks()
{
    float fireworksY = 3.0f;
    float values[] = { 1.0f, 1.5f, 3.0f };
    glm::vec3 fireworksPositions[FIREWORKS_COUNT] = {
        glm::vec3(values[0], fireworksY, values[0]), glm::vec3(-values[0], fireworksY, -values[0]), glm::vec3(values[1], fireworksY, values[1]),
        glm::vec3(-values[1], fireworksY, -values[1]), glm::vec3(values[1], fireworksY, values[0]), glm::vec3(-values[1], fireworksY, values[0]),
        glm::vec3(values[1], fireworksY, -values[0]), glm::vec3(-values[1], fireworksY, -values[0]), glm::vec3(values[2], fireworksY, values[2]),
        glm::vec3(-values[2], fireworksY, -values[2]), glm::vec3(values[2], fireworksY, values[1]), glm::vec3(values[2], fireworksY, -values[1]),
        glm::vec3(-values[2], fireworksY, values[1]), glm::vec3(-values[2], fireworksY, -values[1]), glm::vec3(values[2], fireworksY, values[0]),
        glm::vec3(values[2], fireworksY, -values[0]), glm::vec3(-values[2], fireworksY, values[0]), glm::vec3(-values[2], fireworksY, -values[0]),
    };
    for (int i = 0; i < FIREWORKS_COUNT; i++) {
        int quarterSplit = 4 + rand() % (MAX_FIREWORK_QUARTER_SPLIT - 6 + 1); // 4 - MAX_FIREWORK_QUARTER_SPLIT
        float radius = (15 + rand() % 11) / 10.0; // 1.5 - 2.5
        float speed = (15 + rand() % 11) / 100.0; // 0.15 - 0.25
        fireworks[i] = initFirework(fireworksPositions[i], quarterSplit, radius, speed);
    }
}


void renderFireworks(gps::Shader shader)
{
    shader.useShaderProgram();
    for (int i = 0; i < FIREWORKS_COUNT; i++) {
        if (!fireworks[i].active) {
            continue;
        }
        int directions = fireworks[i].directionsCount;
        for (int j = 0; j < directions; j++) {
            int modelIdx = i * MAX_FIREWORK_DIRECTIONS + j;
            fireworksModels[modelIdx] = glm::translate(glm::mat4(1.0f), fireworks[i].currentPositions[j]);
            fireworksModels[modelIdx] = glm::scale(fireworksModels[modelIdx], glm::vec3(0.01f, 0.01f, 0.01f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(fireworksModels[modelIdx]));
            fireworks3D[i % 6].Draw(shader);
        }
    }
}

//FIREWORKS ANIMATION
void updateFireworksAnimation() {
    for (int i = 0; i < FIREWORKS_COUNT; i++) {
        if (!fireworks[i].active) {
            continue;
        }
        int directions = fireworks[i].directionsCount;
        for (int j = 0; j < directions; j++) {
            fireworks[i].currentPositions[j] += fireworks[i].directions[j] * fireworks[i].speed;
        }
        glm::vec3 radiusVec = fireworks[i].currentPositions[0] - fireworks[i].initPosition;
        //vectorul de deplasare; cand atinge lungimea razei cercului, se dezactiveaza si se reseteaza la pozitiile initiale
        float radiusVecLength = sqrtf(glm::dot(radiusVec, radiusVec));
        if (radiusVecLength > fireworks[i].radius) {
            fireworks[i].active = false;
            for (int j = 0; j < directions; j++) {
                fireworks[i].currentPositions[j] = fireworks[i].initPosition;
            }
        }
    }
}



const int numSnowflakes = 1000;
Snowflake snowflakes[numSnowflakes];
Sleigh mySleigh;

// const int numFireworks = 1000;
// Fireworks fireworks[numFireworks];


GLfloat angle = -90.0f;
GLfloat pitch = 0.0f; //the pitch is the rotation around the lateral axis;
//it represents tilting the camera up or down
GLfloat yaw = -45.0f; //the rotation around the vertical axis
//it represents turning the camera left or right
GLfloat lightAngle = 0.0f;

// shaders
gps::Shader myBasicShader;
gps::Shader depthMapShader;

bool isSnowing = false; //snowing effect
bool sleighAnimation = false; //sleigh animation 
bool fireworksAnimation = false; //fireworks animation

bool isFog = false; //fog effect
bool enableMouseCallback = false;
int enableGlobalLight = 0;

int windowWidth = 700;
int windowHeight = 700;
double last_x = windowWidth / 2.0;
double last_y = windowHeight / 2.0;

//shadowing
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 9900;
const unsigned int SHADOW_HEIGHT = 9900;


GLenum glCheckError_(const char* file, int line)
{
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

//this ensures that when the window is resized, the rendering setup is adjusted acoordingly to maintain correct proportions and aspect ratios.

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) {
        return;
    }
    // Set the new dimensions of the window
    glViewport(0, 0, width, height);

    // Update the projection matrix
    projection = glm::perspective(glm::radians(45.0f),
        (float)width / (float)height,
        0.1f, 20.0f);

    // Send the updated projection matrix to the shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    windowWidth = width;
    windowHeight = height;
}


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

bool firstMouseCallback = true;
//this function gets triggered whenever the mouse is moved
//takes the current window and the new mouse position coordinates as params
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!enableMouseCallback) {
        return;
    }

    if (firstMouseCallback) {
        last_x = xpos;
        last_y = ypos;
        firstMouseCallback = false;
    }

    double xoffset = xpos - last_x;
    double yoffset = last_y - ypos; // reversed since y-coordinates range from bottom to top

    last_x = xpos;
    last_y = ypos;

    const float sensitivity = 0.2f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
}




void processMovement() {
    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle -= 1.0f;
        // update model matrix for teapot
        model = glm::rotate(model, glm::radians(-1.0f), glm::vec3(0, 1, 0));
        sleighModel = glm::rotate(sleighModel, glm::radians(-1.0f), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        angle += 1.0f;
        // update model matrix for teapot
        model = glm::rotate(model, glm::radians(1.0f), glm::vec3(0, 1, 0));
        sleighModel = glm::rotate(sleighModel, glm::radians(1.0f), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    //MOD DE VIZUALIZARE WIREFRAME = 1
    if (pressedKeys[GLFW_KEY_1]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    }
    //MOD DE VIZUALIZARE SOLID = 2
    if (pressedKeys[GLFW_KEY_2]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    //MOD DE VIZUALIZARE POLIGONAL = 3
    if (pressedKeys[GLFW_KEY_3]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }

    //ENABLE MOUSE CALLBACK = CLICK STANGA
    if (pressedKeys[GLFW_KEY_M]) {
        enableMouseCallback = !enableMouseCallback;
        printf("Toggled enableMouseCallback to %d\n", enableMouseCallback);
    }


    //EFECT DE NINSOARE = J
    if (pressedKeys[GLFW_KEY_J]) {
        isSnowing = !isSnowing; //toggle snowing effect 
        printf("Toggled isSnowing to %d\n", isSnowing);
    }

    //ANIMATIE SANIE = Y
    if (pressedKeys[GLFW_KEY_Y]) {
        sleighAnimation = !sleighAnimation; //toggle animation 
        printf("Toggled sleigh animation to %d\n", sleighAnimation);
    }

    //APARITIE CEATA = F
    if (pressedKeys[GLFW_KEY_F]) {
        isFog = !isFog; //toggle fog effect
        printf("Toggled isFog to %d\n", isFog);
        myBasicShader.useShaderProgram();
        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "showFog"), isFog);
    }

    //LUMINA GLOBALA = L
    if (pressedKeys[GLFW_KEY_L]) {
        enableGlobalLight = 1 - enableGlobalLight;
        printf("Toggled enabled global light to %d\n", enableGlobalLight);
        myBasicShader.useShaderProgram();
        showLightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "showDirLight");
        glUniform1i(showLightDirLoc, enableGlobalLight);
    }

    //APARITIE ARTIFICII = P
    if (pressedKeys[GLFW_KEY_P]) {
        fireworks[fireworksCursor].active = true;
        printf("Activated fireworks idx %d\n", fireworksCursor);
        fireworksCursor = (fireworksCursor + 1) % FIREWORKS_COUNT;
    }

    if (pressedKeys[GLFW_KEY_H]) {
        if (!isRecording) {
            recording = fopen("tour_directions.txt", "w");
            isRecording = true;
            printf("Started recording!\n");
        }
        else {
            fclose(recording);
            recording = NULL;
            isRecording = false;
            printf("Stopped recording!\n");
        }
    }

    if (pressedKeys[GLFW_KEY_K]) {
        if (!isPlaying) {
            recording = fopen("tour_directions.txt", "r");
            isPlaying = true;
            printf("Started playing!\n");
        }
        else {
            fclose(recording);
            recording = NULL;
            isPlaying = false;
            printf("Stopped playing!");
        }
    }

}

void recordRelatedStuff() {
    LPFILETIME currentTime = new FILETIME();
    GetSystemTimeAsFileTime(currentTime);
    if (currentTime->dwLowDateTime - lastTime < 200) {
        return;
    }
    if (isRecording) {
        fprintf(
            recording,
            "%f %f %f %f %f %f %f %f %f\n",
            myCamera.cameraPosition.x, myCamera.cameraPosition.y, myCamera.cameraPosition.z,
            myCamera.cameraFrontDirection.x, myCamera.cameraFrontDirection.y, myCamera.cameraFrontDirection.z,
            myCamera.cameraRightDirection.x, myCamera.cameraRightDirection.y, myCamera.cameraRightDirection.z
        );
        lastTime = currentTime->dwLowDateTime;
    }
    else if (isPlaying) {
        if (!feof(recording)) {
            fscanf(
                recording,
                "%f%f%f%f%f%f%f%f%f",
                &myCamera.cameraPosition.x, &myCamera.cameraPosition.y, &myCamera.cameraPosition.z,
                &myCamera.cameraFrontDirection.x, &myCamera.cameraFrontDirection.y, &myCamera.cameraFrontDirection.z,
                &myCamera.cameraRightDirection.x, &myCamera.cameraRightDirection.y, &myCamera.cameraRightDirection.z
            );
            lastTime = currentTime->dwLowDateTime;
        }
        else {
            printf("Finished tour!\n");
            fclose(recording);
            recording = NULL;
            isPlaying = false;
        }
    }
    delete currentTime;
}



void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
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
    myScene.LoadModel("models/scene-final.obj");
    snowflake.LoadModel("models/snowflake/snowflake.obj");
    sleigh.LoadModel("models/sleigh/sleigh.obj");
    fireworks3D[0].LoadModel("models/spheres/sphere1.obj");
    fireworks3D[1].LoadModel("models/spheres/sphere2.obj");
    fireworks3D[2].LoadModel("models/spheres/sphere3.obj");
    fireworks3D[3].LoadModel("models/spheres/sphere4.obj");
    fireworks3D[4].LoadModel("models/spheres/sphere5.obj");
    fireworks3D[5].LoadModel("models/spheres/sphere6.obj");
}

void initShaders() {
    myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    myBasicShader.useShaderProgram();
    depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
    depthMapShader.useShaderProgram();
}


void initUniforms() {
    myBasicShader.useShaderProgram();

    // model matrix for the scene
    model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

    // get view matrix for current camera
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    // send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    sleighNormalMatrix = glm::mat3(glm::inverseTranspose(view * sleighModel));
    //normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    // create projection matrix
    projection = glm::perspective(glm::radians(45.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 20.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    // send projection matrix to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //set the light direction (direction towards the light)
    lightDir = glm::vec3(5.0f, 5.0f, 5.0f);
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    // send light dir to shader
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    //set light color
    lightColor = glm::vec3(1.0f, 1.0f, 0.43f); //light yellow light
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    // send light color to shader
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));


    constantLoc = glGetUniformLocation(myBasicShader.shaderProgram, "constant");
    glUniform1f(constantLoc, constant); 
    linearLoc = glGetUniformLocation(myBasicShader.shaderProgram, "linear");
    glUniform1f(linearLoc, linear); 
    quadraticLoc = glGetUniformLocation(myBasicShader.shaderProgram, "quadratic");
    glUniform1f(quadraticLoc, quadratic); 

    //global light 
    showLightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "showDirLight");
    glUniform1i(showLightDirLoc, enableGlobalLight);

    positionsLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lamps_positions");//vector
    glUniform3fv(positionsLoc, 3, glm::value_ptr(lamps_positions[0]));

}

void initFBO() {
    //generate FBO ID 
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    //attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
    glm::mat4 lightView = glm::lookAt(glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = -50.0f, far_plane = 50.0f;
    glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

    return lightSpaceTrMatrix;
}

void drawObjects(gps::Shader shader, bool depthPass)
{
    shader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        constantLoc = glGetUniformLocation(shader.shaderProgram, "constant");
        glUniform1f(constantLoc, constant); //constant
        linearLoc = glGetUniformLocation(shader.shaderProgram, "linear");
        glUniform1f(linearLoc, linear);//linear
        quadraticLoc = glGetUniformLocation(shader.shaderProgram, "quadratic");
        glUniform1f(quadraticLoc, quadratic);//quadratic
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    myScene.Draw(shader);
}


//SNOWING EFFECT 
void updateSnowfall() {
    for (int i = 0; i < numSnowflakes; i++) {
        snowflakes[i].position += snowflakes[i].velocity;
        if (snowflakes[i].position.y < -1.0f) {
            snowflakes[i].position.y = 4.0f;
        }
    }
}

//SLEIGH ANIMATION 
void updateSleighAnimation() {
    if (mySleigh.position.x < 8.0f) {
        mySleigh.position += mySleigh.velocity;
        //printf("Sleigh Position: (%f, %f, %f)\n", mySleigh.position.x, mySleigh.position.y, mySleigh.position.z);
    }
    if (mySleigh.position.x >= 6.0f) {
        mySleigh.position = glm::vec3(-5.0f, 0.12f, 2.8f);
    }
}




void initSnow() {
    //SNOWING EFFECT 
    float next_x, next_z;
    for (int i = 0; i < numSnowflakes; i++) {
        next_x = -8.0f + (i / 20) * 0.32f; // -8 -> 8 pt x, 50 vals  16 / 50 = 0.32
        next_z = -5.0f + (i % 20) * 0.5f;  // -5 -> 5 pt z, 20 vals,  10 / 20 = 0.5
        float randomValue = static_cast<float>(std::rand()) / RAND_MAX;
        float randomInRange = 4.0f + randomValue * 4.0f; //random y value between 4 and 8
        snowflakes[i].position = glm::vec3(next_x, randomInRange, next_z);
        snowflakes[i].velocity = glm::vec3(0.0f, -0.05f, 0.0f);
    }
}

void initSleigh() {
    //SLEIGH ANIMATION 
    mySleigh.position = glm::vec3(-5.0f, 0.12f, 2.8f); //initial position
    mySleigh.velocity = glm::vec3(0.1f, 0.0f, 0.0f); //"speed" on x axis 
}



void renderSnowfall(gps::Shader shader) {
    shader.useShaderProgram();
    for (int i = 0; i < numSnowflakes; i++) {
        snowflakeModel = glm::mat4(1.0f); //Set an initial matrix for the snowflake
        snowflakeModel = glm::translate(snowflakeModel, snowflakes[i].position);
        snowflakeModel = glm::scale(snowflakeModel, glm::vec3(0.0015f, 0.0015f, 0.0015f));
        snowflakeModel = glm::rotate(snowflakeModel, glm::radians(angle + 90), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(snowflakeModel));
        snowflake.Draw(shader);
    }
}

void renderSleigh(gps::Shader shader)
{
    shader.useShaderProgram();
    sleighModel = glm::mat4(1.0f);
    sleighModel = glm::translate(sleighModel, mySleigh.position);
    sleighModel = glm::scale(sleighModel, glm::vec3(0.0025f, 0.0025f, 0.004f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sleighModel));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(sleighNormalMatrix));
    sleigh.Draw(shader);
}



void renderScene() {
    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    drawObjects(depthMapShader, true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    //for windowResizeCallBack()


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    //render the scene
    myBasicShader.useShaderProgram();

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));

    //SLEIGH ANIMATION 
    if (sleighAnimation) {
        updateSleighAnimation();
        renderSleigh(myBasicShader);
    }
    myBasicShader.useShaderProgram();

    //SNOWING EFFECT
    if (isSnowing) {
        updateSnowfall();
        renderSnowfall(myBasicShader);
    }

    //FIREWORKS ANIMATION
    updateFireworksAnimation();
    renderFireworks(myBasicShader);
    myBasicShader.useShaderProgram();

    drawObjects(myBasicShader, false);
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char* argv[]) {

    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initModels();
    initShaders();
    initUniforms();
    setWindowCallbacks();

    initFBO();

    glCheckError();

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    initSnow();
    initSleigh();
    initFireworks();

    windowHeight = myWindow.getWindowDimensions().height;
    windowWidth = myWindow.getWindowDimensions().width;

    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
        renderScene();
        recordRelatedStuff();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}