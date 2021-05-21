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

    glm::vec2 lastMousePos;

    bool firstMouse = true;

    void reset()
    {
        forward = 0.0f;
        right = 0.0f;
        jump = false;
    }
};

Controller controller{};

bool needsResUpdate = true;

int SCR_DETAIL = 2;

constexpr glm::vec2 defaultRes(214, 120);

glm::vec2 SCR_RES = defaultRes * float(1 << SCR_DETAIL);

Shader screenShader;
Shader computeShader;
GLuint buffer;
GLuint vao;

GLuint screenTexture;

float deltaTime = 16.666f; // 16.66 = 60fps

// spawn player at world center
glm::vec3 cameraPos = glm::vec3(0, 20, 0);

float cameraYaw = 0.01f; // make it not axis aligned by default to avoid raymarching error
float cameraPitch = -2.0f * PI;                                                                                 
float FOV = 90.0f;
glm::vec2 frustumDiv = (SCR_RES * FOV);

float sinYaw, sinPitch;
float cosYaw, cosPitch;

static glm::vec3 lerp(const glm::vec3& start, const glm::vec3& end, const float t)
{
    return start + (end - start) * t;
}

void initTexture(GLuint* texture, const int width, const int height);

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

    glDeleteTextures(1, &screenTexture);
    initTexture(&screenTexture, int(SCR_RES.x), int(SCR_RES.y));

    needsResUpdate = false;
}

void init()
{
    
}

void pollInputs(GLFWwindow* window);

void run(GLFWwindow* window) {
    auto lastUpdateTime = currentTime();
    float lastFrameTime = lastUpdateTime - 16;

    while (!glfwWindowShouldClose(window)) {
        const float frameTime = currentTime();
        deltaTime = frameTime - lastFrameTime;
        lastFrameTime = frameTime;

        pollInputs(window);

        if (needsResUpdate) {
            updateScreenResolution(window);
        }


        // Compute the raytracing!
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        frustumDiv = (SCR_RES * FOV) / defaultRes;

        computeShader.use();

        computeShader.setVec2("screenSize", SCR_RES.x, SCR_RES.y);


        computeShader.setFloat("camera.cosYaw", cos(cameraYaw));
        computeShader.setFloat("camera.cosPitch", cos(cameraPitch));
        computeShader.setFloat("camera.sinYaw", sin(cameraYaw));
        computeShader.setFloat("camera.sinPitch", sin(cameraPitch));
        computeShader.setVec2("camera.frustumDiv", frustumDiv);
        computeShader.setVec3("camera.pos", cameraPos);


        glInvalidateTexImage(screenTexture, 0);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glDispatchCompute(GLuint((SCR_RES.x + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE), GLuint((SCR_RES.y + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE), 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        glUseProgram(0);

        // render the screen texture
        screenShader.use();
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(2);

        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(0);

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

    if(keyDown(window, GLFW_KEY_W))
        controller.forward += 1.0f;
    if (keyDown(window, GLFW_KEY_S))
        controller.forward -= 1.0f;
    if (keyDown(window, GLFW_KEY_D))
        controller.right += 1.0f;
    if (keyDown(window, GLFW_KEY_A))
        controller.right -= 1.0f;
    if (keyDown(window, GLFW_KEY_SPACE))
        controller.jump = true;

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

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initTexture(GLuint* texture, const int width, const int height) {
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindImageTexture(0, *texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
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
    
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Minecraft4k", nullptr, nullptr);
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

    // turn on VSync so we don't run at about a kjghpillion fps
    glfwSwapInterval(1);

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
    
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glClearDepth(1);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);	
    glCullFace(GL_FRONT_AND_BACK);
    glClearColor(0, 0, 0, 1);

    glfwSetCursorPosCallback(window, mouse_callback);

    std::cout << "Done!\n";

    std::cout << "Building shaders... ";
    std::stringstream defines;
    defines << "#define WORLD_SIZE " << WORLD_SIZE << "\n"
            << "#define WORLD_HEIGHT " << WORLD_HEIGHT << "\n"
            << "#define TEXTURE_RES " << TEXTURE_RES << "\n"
            << "#define RENDER_DIST " << RENDER_DIST << "\n";
#ifdef CLASSIC
    defines << "#define CLASSIC\n";
#endif

    defines << "layout(local_size_x = " << WORK_GROUP_SIZE << ", local_size_y = " << WORK_GROUP_SIZE << ") in;";

    const std::string definesStr = defines.str();

    screenShader = Shader("screen", "screen");
    computeShader = Shader("raytrace", HasExtra::Yes, definesStr.c_str());

    std::cout << "Done!\n";
    
    glActiveTexture(GL_TEXTURE0);

    std::cout << "Building buffers... ";
    initBuffers();
    std::cout << "Done!\n";

    std::cout << "Building render texture... ";
    initTexture(&screenTexture, int(SCR_RES.x), int(SCR_RES.y));
    std::cout << "Done!\n";

    std::cout << "Initializing engine...\n";
    init();
    std::cout << "Finished initializing engine! Running the game...\n";

    run(window);
}