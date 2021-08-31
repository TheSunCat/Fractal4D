#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "include/glad/glad.h"
#include <GLFW/glfw3.h>

#include "Constants.h"
#include "Shader.h"
#include "Util.h"

struct Controller
{
    float forward;
    float right;

    bool jump;
    bool sneak;

    glm::vec2 lastMousePos;

    bool firstMouse = true;

    void reset()
    {
        forward = 0.0f;
        right = 0.0f;
        jump = false;
        sneak = false;
    }
};

Controller controller{};

bool needsResUpdate = true;

int SCR_DETAIL = 3;

constexpr glm::vec2 defaultRes(214, 120);

glm::vec2 SCR_RES = defaultRes * float(1 << SCR_DETAIL);

Shader raytraceShader;
Shader screenShader;
GLuint buffer;
GLuint quadVAO;

GLuint framebuffer;
GLuint textureColorbuffer;

float deltaTime = 16.666f; // 16.66 = 60fps

// spawn player at world center
glm::vec3 cameraPos = glm::vec3(-1.5, 0, -1.5);

float cameraYaw = PI / 4.f; // PI/4
float cameraPitch = -2.0f * PI;                                                                                 
float FOV = 90.0f;
glm::vec2 frustumDiv = (SCR_RES * FOV);

glm::vec3 cameraFront;
glm::vec3 cameraRight;
glm::vec3 cameraUp;

float moveSpeed = 1.0f;

glm::vec3 worldUp = glm::vec3(0, 1, 0);

float sinYaw, sinPitch;
float cosYaw, cosPitch;

void initFramebuffer(const int width, const int height);

void updateScreenResolution(GLFWwindow* window)
{
    if (SCR_DETAIL < -4)
        SCR_DETAIL = -4;
    if (SCR_DETAIL > 6)
        SCR_DETAIL = 6;

    SCR_RES.x = 107 * pow(2, SCR_DETAIL);
    SCR_RES.y = 60 * pow(2, SCR_DETAIL);


    std::string title = "Fractal4D";

    switch (SCR_DETAIL) {
    case -4:
        title += " on battery-saving mode";
        break;
    case -3:
        title += " on a potato";
        break;
    case -2:
        title += " on an undocked switch";
        break;
    case -1:
        title += " on a TI-84";
        break;
    case 0:
        title += " on an Atari 2600";
        break;
    case 2:
        title += " at SD";
        break;
    case 3:
        title += " at HD";
        break;
    case 4:
        title += " at Full HD";
        break;
    case 5:
        title += " at 4K";
        break;
    case 6:
        title += " on a NASA supercomputer";
        break;
    }

    glfwSetWindowTitle(window, title.c_str());
    
    initFramebuffer(int(SCR_RES.x), int(SCR_RES.y));

    raytraceShader.use();
    //glViewport(0, 0, SCR_RES.x, SCR_RES.y);
    glUseProgram(0);

    needsResUpdate = false;
}

void pollInputs(GLFWwindow* window);

void run(GLFWwindow* window) {
    auto lastUpdateTime = currentTime();
    float lastFrameTime = lastUpdateTime - 16;

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window)) {
        const float frameTime = currentTime();
        deltaTime = frameTime - lastFrameTime;
        lastFrameTime = frameTime;

        pollInputs(window);

        if (needsResUpdate) {
            updateScreenResolution(window);
        }
    	
        cosYaw = cos(cameraYaw);
        cosPitch = cos(cameraPitch);
        sinYaw = sin(cameraYaw);
        sinPitch = sin(cameraPitch);

    	// """"physics""""
        cameraPos.x += (sinYaw * controller.forward + cosYaw * controller.right) / 100. * moveSpeed;
        cameraPos.z += (cosYaw * controller.forward - sinYaw * controller.right) / 100. * moveSpeed;

    	
        cameraPos += (controller.jump * -worldUp) / 100.F * moveSpeed;
        cameraPos += (controller.sneak * worldUp) / 100.F * moveSpeed;

        // Compute the raytracing!
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        frustumDiv = (SCR_RES * FOV) / defaultRes;
        
        // render the screen texture
        raytraceShader.use();
        raytraceShader.setVec2("screenSize", SCR_RES.x, SCR_RES.y);

        raytraceShader.setFloat("camera.cosYaw", cosYaw);
        raytraceShader.setFloat("camera.cosPitch", cosPitch);
        raytraceShader.setFloat("camera.sinYaw", sinYaw);
        raytraceShader.setFloat("camera.sinPitch", sinPitch);
        raytraceShader.setVec2("camera.frustumDiv", frustumDiv);
        raytraceShader.setVec3("camera.pos", cameraPos);

        raytraceShader.setVec3("color", glm::vec3(0.592, 0.835, 0.996));
        
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        

        // render the screen texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screenShader.use();

        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwTerminate();
}

void mouse_callback(GLFWwindow*, const double xPosD, const double yPosD)
{
    const auto xPos = float(xPosD);
    const auto yPos = float(yPosD);

    if (controller.firstMouse) {
        controller.lastMousePos.x = xPos;
        controller.lastMousePos.y = yPos;
        controller.firstMouse = false;
        return; // nothing to calculate because we technically didn't move the mouse
    }

    const float xOffset = xPos - controller.lastMousePos.x;
    const float yOffset = controller.lastMousePos.y - yPos;

    controller.lastMousePos.x = xPos;
    controller.lastMousePos.y = yPos;

    cameraYaw += xOffset / 500.0f;
    cameraPitch += yOffset / 500.0f;

    if(fabs(cameraYaw) > PI)
    {
        if (cameraYaw > 0)
            cameraYaw = -PI - (cameraYaw - PI);
        else
            cameraYaw = PI + (cameraYaw + PI);
    }
    cameraPitch = clamp(cameraPitch, -PI / 2.0f, PI / 2.0f);
}

void scroll_callback(GLFWwindow*, double xoffset, double yoffset)
{
    moveSpeed += (yoffset);
}

bool keyDown(GLFWwindow* window, int key)
{
    return glfwGetKey(window, key) == GLFW_PRESS;
}


std::unordered_map<int, float> keyHistory;
bool keyPress(GLFWwindow* window, int key)
{
    int pressState = glfwGetKey(window, key);

    auto loc = keyHistory.find(key);

    if(pressState == GLFW_PRESS)
    {
        if (loc == keyHistory.end()) { // not in history, this is first key press
            keyHistory.insert(std::make_pair(key, currentTime()));

            return true;
        }

        auto time = currentTime();
        if (time - loc->second > 500) // repeat key press after 500ms
            return true;
        else
            return false;
    }

    if (loc != keyHistory.end()) // key released, so clear from history
        keyHistory.erase(loc);

    return false;
}

void pollInputs(GLFWwindow* window)
{
    controller.reset();

    if (keyDown(window, GLFW_KEY_W))
        controller.forward += 1.0f;
    if (keyDown(window, GLFW_KEY_A))
        controller.right -= 1.0f;
    if (keyDown(window, GLFW_KEY_S))
        controller.forward -= 1.0f;
    if (keyDown(window, GLFW_KEY_D))
        controller.right += 1.0f;
    if (keyDown(window, GLFW_KEY_SPACE))
        controller.jump = true;
	if (keyDown(window, GLFW_KEY_LEFT_SHIFT))
        controller.sneak = true;

    if (keyPress(window, GLFW_KEY_COMMA)) {
        SCR_DETAIL--;
        needsResUpdate = true;
    }
    if (keyPress(window, GLFW_KEY_PERIOD)) {
        SCR_DETAIL++;
        needsResUpdate = true;
    }

    controller.forward = clamp(controller.forward, -1.0f, 1.0f);
    controller.right = clamp(controller.right, -1.0f, 1.0f);
}

void initBuffers() {
    GLfloat vertices[] = {
        -1.f, -1.f,
        0.f, 1.f,
        -1.f, 1.f,
        0.f, 0.f,
        1.f, -1.f,
        1.f, 1.f,
        -1.f, 1.f,
        0.f, 0.f,
        1.f, -1.f,
        1.f, 1.f,
        1.f, 1.f,
        1.f, 0.f
    };

    glGenVertexArrays(1, &quadVAO);

    glGenBuffers(1, &buffer);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initFramebuffer(const int width, const int height) {
    // framebuffer configuration
    // -------------------------
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a color attachment texture
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    //glViewport(0, 0, width, height);
}

int main(const int argc, const char** argv)
{
    std::cout << "Initializing GLFW... ";

    if (!glfwInit())
    {
        // Initialization failed
        std::cout << "Failed to init GLFW!\n";
        return -1;
    }

    std::cout << "Done!\n";

    std::cout << "Creating window... ";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // add on Mac bc Apple is big dumb :(
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Fractal4D", nullptr, nullptr);
    if (!window)
    {
        // Window or OpenGL context creation failed
        std::cout << "Failed to create window!\n";
        return -1;
    }
    std::cout << "Done!\n";

    std::cout << "Setting OpenGL context... ";
    glfwMakeContextCurrent(window);
    std::cout << "Done!\n";

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // turn off VSync so we run at about a kjghpillion fps
    glfwSwapInterval(0);

    std::cout << "Loading OpenGL functions... ";
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cout << "Failed to initialize GLAD!\n" << std::endl;
        return -1;
    }
    std::cout << "Done!\n";

    std::cout << "Configuring OpenGL... ";
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(error_callback, nullptr);
    
   // glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glClearDepth(1);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);	
    glCullFace(GL_FRONT_AND_BACK);
    glClearColor(0, 0, 0, 1);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    std::cout << "Done!\n";

    std::cout << "Building shaders... ";
    std::stringstream defines;
    defines << "#define RENDER_DIST " << RENDER_DIST << "\n";

    defines << "layout(local_size_x = " << WORK_GROUP_SIZE << ", local_size_y = " << WORK_GROUP_SIZE << ") in;";

    const std::string definesStr = defines.str();

    screenShader = Shader("screen", "screen");
    raytraceShader = Shader("screen", "raytrace");

    std::cout << "Done!\n";
    
    glActiveTexture(GL_TEXTURE0);

    std::cout << "Building buffers... ";
    initBuffers();
    std::cout << "Done!\n";

    std::cout << "Building render texture... ";
    initFramebuffer(int(SCR_RES.x), int(SCR_RES.y));
    std::cout << "Done!\n";

    std::cout << "Fractalizing the renderer...\n";

    run(window);
}