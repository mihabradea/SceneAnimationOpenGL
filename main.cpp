#define GLEW_STATIC
#include <GL/glew.h>
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

#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024

enum shaders
{
    MAIN,
    depthMapShader
};
// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc[2];
GLint viewLoc[2];
GLint projectionLoc[2];
GLint normalMatrixLoc[2];
GLint lightDirLoc[2];
GLint lightColorLoc[2];

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.15f;

GLboolean pressedKeys[1024];

// models
gps::Model3D ground;
gps::Model3D object;
GLfloat angle;

// shaders
gps::Shader shader[2];

GLuint shadowMapFBO;
GLuint depthMapTexture;


GLenum glCheckError_(const char *file, int line)
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
    fprintf(stdout, "Window resized! New width: %d, and height: %d\n", width, height);

    // Toggle fullscreen if a specific key is pressed (e.g., F key)
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        // Toggle fullscreen mode
        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, width, height, GLFW_DONT_CARE);
    }

    // Update the viewport to match the new window size
    glViewport(0, 0, width, height);

    // Update the projection matrix to match the new aspect ratio
    projection = glm::perspective(glm::radians(45.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f, 20.0f);

    // Update the projection matrix in the shader
    for (int i = 0; i <= 1; i++) {
        shader[i].useShaderProgram();
        glUniformMatrix4fv(projectionLoc[i], 1, GL_FALSE, glm::value_ptr(projection));
    }
}


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

float lastX, lastY;
bool firstMouse = true;

void cameraUpdate() {
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc[MAIN], 1, GL_FALSE, glm::value_ptr(view));
    // compute normal matrix for ground
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //TODO
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.03f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    myCamera.rotate(yoffset, xoffset);
    cameraUpdate();
}


void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
        cameraUpdate();
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        cameraUpdate();
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        cameraUpdate();
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        cameraUpdate();
	}

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        // update model matrix for ground
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for ground
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        // update model matrix for ground
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for ground
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }
}

void initOpenGLWindow() {
    myWindow.Create(2048, 1024, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
    ground.LoadModel("models/ground1/ground1.obj");
    object.LoadModel("models/hotbaloon/hotbaloon.obj");
}

void initShaders() {
	shader[MAIN].loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    shader[depthMapShader].loadShader(
        "shaders/depthMapShader.vert",
        "shaders/depthMapShader.frag");
}

glm::mat4 computeLightSpaceTrMatrix() {
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = -600, far_plane = 20.0f;
    glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
    return lightSpaceTrMatrix;
}

void initUniforms() {
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
        0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    for (int i = 0; i <= 0; i++) {
        shader[i].useShaderProgram();

        // create model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        modelLoc[i] = glGetUniformLocation(shader[i].shaderProgram, "model");
        // get view matrix for current camera
        view = myCamera.getViewMatrix();
        viewLoc[i] = glGetUniformLocation(shader[i].shaderProgram, "view");
        // send view matrix to shader
        glUniformMatrix4fv(viewLoc[i], 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        normalMatrixLoc[i] = glGetUniformLocation(shader[i].shaderProgram, "normalMatrix");
        // create projection matrix
        projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 100.0f);

        projectionLoc[i] = glGetUniformLocation(shader[i].shaderProgram, "projection");

        // send projection matrix to shader
        glUniformMatrix4fv(projectionLoc[i], 1, GL_FALSE, glm::value_ptr(projection));

        //set the light direction (direction towards the light)
        lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
        lightDirLoc[i] = glGetUniformLocation(shader[i].shaderProgram, "lightDir");

        // send light dir to shader
        glUniform3fv(lightDirLoc[i], 1, glm::value_ptr(lightDir));

        //set light color
        lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
        lightColorLoc[i] = glGetUniformLocation(shader[i].shaderProgram, "lightColor");
        // send light color to shader
        glUniform3fv(lightColorLoc[i], 1, glm::value_ptr(lightColor));
    }
    
}

void initFBO() {
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
        0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderGround(int index) {
    // select active shader program

    shader[index].useShaderProgram();

    //send teapot model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader[index].shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    if (index != 1) {
        glUniformMatrix3fv(normalMatrixLoc[index], 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    // draw teapot
    ground.Draw(shader[index]);
}

double elapsedTime1 = 0.0;

void renderObject(int index) {
    // Select active shader program
    shader[index].useShaderProgram();

    // Update the model matrix based on elapsed time (up-and-down motion)

    float yOffset = 1.0f * sin(elapsedTime1);  // Adjust the amplitude and frequency
    float heightOffset = 5.0f;  // Adjust the height offset
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, yOffset + heightOffset, 0.0f));
    glCheckError();
    glUniformMatrix4fv(glGetUniformLocation(shader[index].shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glCheckError();
    // Send object normal matrix data to shader
    if (index != 1) {
        glUniformMatrix3fv(normalMatrixLoc[index], 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    glCheckError();

    // Draw the object
    object.Draw(shader[index]);
}


void renderScene() {
    shader[depthMapShader].useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shader[depthMapShader].shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glCheckError();
    glClear(GL_DEPTH_BUFFER_BIT);
    renderObject(depthMapShader);
    renderGround(depthMapShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//render the scene
    glViewport(0, 0, 2000, 1024);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader[MAIN].useShaderProgram();

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc[MAIN], 1, GL_FALSE, glm::value_ptr(view));

    glUniform3fv(lightDirLoc[MAIN], 1, glm::value_ptr(lightDir));

    //bind the shadow map
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(shader[MAIN].shaderProgram, "shadowMap"), 3);

    glUniformMatrix4fv(glGetUniformLocation(shader[MAIN].shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));

    renderObject(MAIN);

	// render the teapot
	renderGround(MAIN);
 
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
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

	glCheckError();
    double startTime = glfwGetTime();
    double animationDuration = 2.0;  // 2 seconds

    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();

        // Calculate elapsed time
        double currentTime = glfwGetTime();
        double elapsedTime = currentTime - startTime;

        // Time-based animation
        if (elapsedTime < animationDuration) {
            // Animation logic
            float distanceToMove = 0.1f;  // Adjust this value as needed
            myCamera.move(gps::MOVE_FORWARD, distanceToMove * static_cast<float>(1.0 - elapsedTime / animationDuration));
            cameraUpdate();
        }

        double currentTime1 = glfwGetTime();
        double deltaTime = currentTime1 - startTime;
        startTime = currentTime1;

        // Increment the total elapsed time
        elapsedTime1 += deltaTime;

        // Time-based animation
        if (elapsedTime1 < animationDuration) {
            // Animation logic
            float distanceToMove = 0.01f;  // Adjust this value as needed
            myCamera.move(gps::MOVE_FORWARD, distanceToMove * static_cast<float>(1.0 - elapsedTime1 / animationDuration));
            cameraUpdate();
        }

        // Render the scene
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

	cleanup();

    return EXIT_SUCCESS;
}
