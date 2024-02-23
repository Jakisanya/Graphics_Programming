// --- Quaternions: Orientation --- \\

#include "libraries/glew-2.1.0/include/GL/glew.h"
#include "libraries/glfw-master/include/GLFW/glfw3.h"
#include "libraries/glm-master/glm/glm.hpp"
#include "libraries/glm-master/glm/ext.hpp"
#include "xmlparser.cpp"
#include <cmath>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct ProgramData
{
    GLuint shaderProgram;
    GLint globalUniformBlockIndex;
    GLint modelMatrixLocation;
};

// Define global variables to track key presses
bool wKeyPressed = false;
bool sKeyPressed = false;
bool dKeyPressed = false;
bool aKeyPressed = false;
bool eKeyPressed = false;
bool qKeyPressed = false;
bool iKeyPressed = false;
bool kKeyPressed = false;
bool lKeyPressed = false;
bool jKeyPressed = false;

class Timer {
public:
    enum TimerType {
        SINGLE, // Timer runs once
        REPEATING // Timer repeats
    };

    Timer(TimerType type, std::chrono::seconds duration) : m_type(type), m_duration(duration) {}

    // Start the timer
    void Start() {
        m_running = true;
        m_startTime = std::chrono::steady_clock::now();
    }

    // Check if the timer has elapsed
    bool isElapsed() {
        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime < m_endTime) {
            m_running = true;
            return false;
        }
        else {
            m_running = false;
            TimerUpdateTime();
            return true;
        }
    }

    void TimerUpdateTime() {
        if (m_type == REPEATING) {
            m_running = true; // Stop if single timer
            m_startTime = std::chrono::steady_clock::now(); // Restart if repeating timer
        }
    }

    // Interpolation function to calculate alpha value
    float getAlpha() {
        // Ensure startTime < endTime to avoid division by zero or negative results
        if (isElapsed()) {
            return 1.0f; // Return full alpha if startTime equals or exceeds endTime
        }

        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point endTime = m_startTime + m_duration;
        // Calculate the normalized time position between startTime and endTime
        float normalisedTime = static_cast<float>((currentTime - m_startTime) / (endTime - m_startTime));
        std::cout << "normalisedTime: " << normalisedTime << "\n";
        // Clamp normalizedTime to the range [0, 1]
        normalisedTime = std::fminf(std::fmaxf(normalisedTime, 0.0f), 1.0f);

        // Return the alpha value based on the interpolation function
        return normalisedTime;
    }

    // Stop the timer
    void Stop() {
        m_running = false;
    }

private:
    TimerType m_type;
    std::chrono::seconds m_duration; // Duration of the timer in seconds
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_endTime;
    bool m_running = false;
};

static glm::fquat Orients[] = {
                glm::fquat(0.7071f, 0.7071f, 0.0f, 0.0f),
                glm::fquat(0.5f, 0.5f, -0.5f, 0.5f),
                glm::fquat(-0.4895f, -0.7892f, -0.3700f, -0.02514f),
                glm::fquat(0.4895f, 0.7892f, 0.3700f, 0.02514f),

                glm::fquat(0.3840f, -0.1591f, -0.7991f, -0.4344f),
                glm::fquat(0.5537f, 0.5208f, 0.6483f, 0.0410f),
                glm::fquat(0.0f, 0.0f, 1.0f, 0.0f)
};

static std::vector<int> OrientKeys = {
                GLFW_KEY_Q,
                GLFW_KEY_W,
                GLFW_KEY_E,
                GLFW_KEY_R,
                GLFW_KEY_T,
                GLFW_KEY_Y,
                GLFW_KEY_U
};

static glm::vec4 Vectorize(const glm::fquat theQuat) {
    glm::vec4 ret;

    ret.x = theQuat.x;
    ret.y = theQuat.y;
    ret.z = theQuat.z;
    ret.w = theQuat.w;

    return ret;
}

static glm::fquat Lerp(const glm::fquat &v0, const glm::fquat &v1, float alpha) {
    glm::vec4 start = Vectorize(v0);
    glm::vec4 end = Vectorize(v1);
    glm::vec4 interp = glm::mix(start, end, alpha);

    printf("alpha: %f, (%f, %f, %f, %f)\n", alpha, interp.w, interp.x, interp.y, interp.z);

    interp = glm::normalize(interp);
    return {interp.w, interp.x, interp.y, interp.z};
}

static glm::fquat Slerp(const glm::fquat &v0, const glm::fquat &v1, float alpha) {
    std::cout << "Slerp alpha: " << alpha << "\n";
    float dot = glm::dot(v0, v1);

    const float DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD)
        return Lerp(v0, v1, alpha);

    glm::clamp(dot, -1.0f, 1.0f);
    float theta_0 = acosf(dot);
    float theta = theta_0 * alpha;

    glm::fquat v2 = v1 + -(v0 * dot);
    v2 = glm::normalize(v2);

    return (v0 * glm::cos(theta)) + (v2 * glm::sin(theta));
}

class Orientation {
public:
    static bool m_bSlerp;

    void ToggleSlerp() {
        m_bSlerp = !m_bSlerp;
    }

    [[nodiscard]] glm::fquat OrientationGetOrient() {
        if(m_bIsAnimating) {
            std::cout << "Am I ever animating?" << "\n";
            return m_anim.AnimationGetOrient(Orients[m_ixCurrOrient], m_bSlerp);
        }
        else
            return Orients[m_ixCurrOrient];
    }

    [[nodiscard]] static bool IsAnimating() {return m_bIsAnimating;}

    void OrientationUpdateTime() {
        if (m_bIsAnimating) {
            bool bIsFinished = m_anim.AnimationUpdateTime();
            if (bIsFinished) {
                std::cout << "Orientation finished." << "\n";
                m_bIsAnimating = false;
                m_ixCurrOrient = m_anim.GetFinalIx();
            }
        }
    }

    void AnimateToOrient(int ixDestination) {
        std::cout << "Current orient index = " << m_ixCurrOrient << "\n";
        std::cout << "Destination orient index = " << ixDestination << "\n";
        if(m_ixCurrOrient == ixDestination)
            return;
        m_anim.StartAnimation(ixDestination);
        m_bIsAnimating = true;
    }

private:
    class Animation {
    public:
        //Returns true if the animation is over.
        bool AnimationUpdateTime() {
            return m_currTimer.isElapsed();
        }

        [[nodiscard]] glm::fquat AnimationGetOrient(const glm::fquat &initial, bool bSlerp) {
            if (bSlerp) {
                return Slerp(initial, Orients[m_ixFinalOrient], m_currTimer.getAlpha());
            }
            else {
                return Lerp(initial, Orients[m_ixFinalOrient], m_currTimer.getAlpha());
            }
        }

        void StartAnimation(int ixDestination) {
            m_ixFinalOrient = ixDestination;
            m_currTimer.Start();
            std::cout << "Timer started." << "\n";
        }

        [[nodiscard]] int GetFinalIx() const {return m_ixFinalOrient;}

    private:
        int m_ixFinalOrient{};
        Timer m_currTimer{Timer::TimerType::SINGLE, std::chrono::seconds (5)};
    };

    static bool m_bIsAnimating;
    static int m_ixCurrOrient;
    static Animation m_anim;
};

bool Orientation::m_bSlerp = false;
bool Orientation::m_bIsAnimating = false;
int Orientation::m_ixCurrOrient{};
Orientation::Animation Orientation::m_anim;

GLuint g_GlobalMatricesUBO;
static const int g_iGlobalMatricesBindingIndex = 0;

GLFWwindow* initializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Set GLFW to use OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT),
                                          "Getting the hang...", nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Set the initial viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    return window;
}

void initializeGLEW() {
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(EXIT_FAILURE);
    }
}

class MatrixStack {
public:
    MatrixStack() {
        // Initialize with the identity matrix
        stack.emplace_back(glm::mat4(1.0f));
    }

    void Push() {
        // Duplicate the current top matrix and push it onto the stack
        stack.push_back(stack.back());
    }

    void Pop() {
        // Remove the last matrix from the stack
        stack.pop_back();
    }

    void Translate(glm::vec3 xyz) {
        // Apply translation to the current top matrix
        stack.back() = glm::translate(stack.back(), xyz);
    }

    void RotateX(float angle) {
        // Apply rotation around the Y-axis to the current top matrix
        stack.back() = glm::rotate(stack.back(), glm::radians(angle), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    void RotateY(float angle) {
        // Apply rotation around the Y-axis to the current top matrix
        stack.back() = glm::rotate(stack.back(), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void RotateZ(float angle) {
        // Apply rotation around the Y-axis to the current top matrix
        stack.back() = glm::rotate(stack.back(), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    void Scale(glm::vec3 xyz) {
        // Apply a scaling to the current top matrix
        stack.back() = glm::scale(stack.back(), xyz);
    }

    void ApplyMatrix(glm::mat4 mat) {
        // Apply a matrix by multiplying current by given matrix
        stack.back() = stack.back() * mat;
    }

    [[nodiscard]] glm::mat4 Top() const {
        // Get the current top matrix
        return stack.back();
    }

    void SetMatrix(glm::mat4 matrix) {
        stack.back() = matrix;
    }

private:
    std::vector<glm::mat4> stack;
};

class Renderer {
public:
    ProgramData data{};
    GLuint unitPlaneVBO{}, shipVBO{};
    GLuint unitPlaneVAO{}, shipVAO{};
    GLuint unitPlaneEBO{};
    std::vector<GLfloat> shipVertexData;
    std::vector<GLuint> unitPlaneVertexIndicesTri;
    std::vector<std::vector<GLuint>> shipVertexIndicesTri;
    glm::mat4 modelMatrix{};

    static glm::fquat orientation;
    static glm::vec3 cameraTarget;
    static glm::vec3 upVector;
    static glm::vec3 sphereCameraRelativePosition;

    Renderer() {
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
    }

    void createUnitPlane() {
        // Vertex data for the plan
        std::vector<GLfloat> unitPlaneVertexData = {
                0.5f, 0.0f, -0.5f, 0.0f, 0.65098f, 0.09804f,
                0.5f, 0.0f, 0.5f, 0.0f, 0.65098f, 0.09804f,
                -0.5f, 0.0f, 0.5f, 0.0f, 0.65098f, 0.09804f,
                -0.5f, 0.0f, -0.5f, 0.0f, 0.65098f, 0.09804f
        };

        unitPlaneVertexIndicesTri = {
                0, 1, 2,
                0, 2, 1,
                2, 3, 0,
                2, 0, 3
        };

        glGenVertexArrays(1, &unitPlaneVAO);
        glGenBuffers(1, &unitPlaneVBO);
        glGenBuffers(1, &unitPlaneEBO);

        glBindVertexArray(unitPlaneVAO);
        glBindBuffer(GL_ARRAY_BUFFER, unitPlaneVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitPlaneEBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // Add data to VBO and EBO
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(unitPlaneVertexData.size() * sizeof(GLfloat)),
                     unitPlaneVertexData.data(), GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(unitPlaneVertexIndicesTri.size() * sizeof(GLint)),
                     unitPlaneVertexIndicesTri.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    };

    void createShip() {
        shipVertexData = {
                0.0f, 0.0f, 5.486261f, 0.6705883f, 0.7607843f, 0.7607843f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 0.0f, 5.486261f, 0.6705883f, 0.7607843f, 0.7607843f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.8f, 0.0f, 5.091465f, 0.682353f, 0.772549f, 0.7411765f, 0.0f, 0.0f, 5.486261f, 0.6705883f, 0.7607843f, 0.7607843f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 0.0f, 5.486261f, 0.6705883f, 0.7607843f, 0.7607843f, -0.8f, 0.0f, 5.091465f, 0.682353f, 0.772549f, 0.7411765f, 1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.8f, 0.0f, 5.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.8f, 0.0f, 5.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, 0.8f, 0.0f, 5.091465f, 0.4117647f, 0.4666666f, 0.4705882f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -0.8f, 0.0f, 5.091465f, 0.4117647f, 0.4666666f, 0.4705882f, 0.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, 0.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, 0.0f, 0.0f, 5.486261f, 0.3843137f, 0.4352941f, 0.4352941f, 0.8f, 0.0f, 5.091465f, 0.4117647f, 0.4666666f, 0.4705882f, -0.8f, 0.0f, 5.091465f, 0.4117647f, 0.4666666f, 0.4705882f, 0.0f, 0.0f, 5.486261f, 0.3843137f, 0.4352941f, 0.4352941f, 0.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, -0.7365548f, 0.7365548f, 4.091465f, 1.0f, 1.0f, 1.0f, -0.7365548f, 0.7365548f, 4.091465f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 4.091465f, 1.0f, 1.0f, 1.0f, -0.7365548f, 0.7365548f, 4.091465f, 1.0f, 1.0f, 1.0f, -0.7365548f, 0.7365548f, 4.091465f, 1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 4.091465f, 1.0f, 1.0f, 1.0f, 0.0f, 1.845113f, -1.908402f, 0.6352941f, 0.7215687f, 0.6941177f, 1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, 0.0f, 1.845113f, -1.908402f, 0.6352941f, 0.7215687f, 0.6941177f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, -1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, -1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 1.516663f, 0.2699136f, -1.908402f, 0.4627451f, 0.5411765f, 0.5215687f, -1.516663f, 0.2699136f, -1.908402f, 0.4627451f, 0.5411765f, 0.5215687f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, -1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, 2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 3.05465f, 0.1929539f, -1.733622f, 0.5647059f, 0.6627451f, 0.6196079f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -3.05465f, 0.1929539f, -1.733622f, 0.5647059f, 0.6627451f, 0.6196079f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.516663f, 0.2699136f, -1.908402f, 0.372549f, 0.4156863f, 0.3803921f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.516663f, 0.2699136f, -1.908402f, 0.372549f, 0.4156863f, 0.3803921f, 1.516663f, 0.2699136f, -1.908402f, 0.372549f, 0.4156863f, 0.3803921f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, 3.05465f, 0.1929539f, -1.733622f, 0.8784314f, 0.8627451f, 0.6784314f, -3.05465f, 0.1929539f, -1.733622f, 0.8784314f, 0.8627451f, 0.6784314f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -1.516663f, 0.2699136f, -1.908402f, 0.372549f, 0.4156863f, 0.3803921f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 2.750729f, -0.2587609f, 1.277193f, 0.8901961f, 0.8784314f, 0.6392157f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -2.750729f, -0.2587609f, 1.277193f, 0.8901961f, 0.8784314f, 0.6392157f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 1.422557f, -0.3241928f, 1.091532f, 0.6156863f, 0.7019608f, 0.7058824f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 2.753033f, 0.2287188f, 1.265094f, 0.8392157f, 0.8431373f, 0.6941177f, -2.753033f, 0.2287188f, 1.265094f, 0.8392157f, 0.8431373f, 0.6941177f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, -1.422557f, -0.3241928f, 1.091532f, 0.6156863f, 0.7019608f, 0.7058824f, 1.422557f, -0.3241928f, 1.091532f, 0.6156863f, 0.7019608f, 0.7058824f, 2.753033f, 0.2287188f, 1.265094f, 0.827451f, 0.8392157f, 0.6980392f, 2.750729f, -0.2587609f, 1.277193f, 0.7529412f, 0.8196079f, 0.6470589f, -2.750729f, -0.2587609f, 1.277193f, 0.7529412f, 0.8196079f, 0.6470589f, -2.753033f, 0.2287188f, 1.265094f, 0.827451f, 0.8392157f, 0.6980392f, -1.422557f, -0.3241928f, 1.091532f, 0.6156863f, 0.7019608f, 0.7058824f, 1.094106f, 0.5941064f, 1.091532f, 0.6196079f, 0.7098039f, 0.7137255f, 2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, 2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, -2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, -2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -1.094106f, 0.5941064f, 1.091532f, 0.6196079f, 0.7098039f, 0.7137255f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, 2.750729f, -0.2587609f, 1.277193f, 0.8627451f, 0.854902f, 0.6745098f, 2.753033f, 0.2287188f, 1.265094f, 0.8470588f, 0.8431373f, 0.6862745f, -2.753033f, 0.2287188f, 1.265094f, 0.8470588f, 0.8431373f, 0.6862745f, -2.750729f, -0.2587609f, 1.277193f, 0.8627451f, 0.854902f, 0.6745098f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, 2.904993f, 0.4069349f, -0.2403131f, 0.8431373f, 0.8431373f, 0.6588236f, 2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, 2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, -2.904993f, 0.4069349f, -0.2403131f, 0.8431373f, 0.8431373f, 0.6588236f, 3.05465f, 0.1929539f, -1.733622f, 0.5568628f, 0.6588236f, 0.6392157f, 2.902689f, -0.03290349f, -0.2282143f, 0.8745098f, 0.8588235f, 0.6784314f, 2.904993f, 0.4069349f, -0.2403131f, 0.8431373f, 0.8470588f, 0.6627451f, -2.904993f, 0.4069349f, -0.2403131f, 0.8431373f, 0.8470588f, 0.6627451f, -2.902689f, -0.03290349f, -0.2282143f, 0.8745098f, 0.8588235f, 0.6784314f, -3.05465f, 0.1929539f, -1.733622f, 0.5568628f, 0.6588236f, 0.6392157f, 4.670839f, 0.2244458f, 1.458042f, 0.6078432f, 0.6431373f, 0.6470589f, 4.822799f, 0.4503033f, -0.04736506f, 0.6196079f, 0.654902f, 0.6588236f, 4.820496f, -0.03717672f, -0.03526628f, 0.6156863f, 0.6509804f, 0.654902f, -4.820496f, -0.03717672f, -0.03526628f, 0.6156863f, 0.6509804f, 0.654902f, -4.822799f, 0.4503033f, -0.04736506f, 0.6196079f, 0.654902f, 0.6588236f, -4.670839f, 0.2244458f, 1.458042f, 0.6078432f, 0.6431373f, 0.6470589f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 0.0f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 1.845113f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.845113f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.188213f, 1.188213f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, 1.845113f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, 1.845113f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.188213f, 1.188213f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 0.0f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 0.0f, 1.845113f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, 1.845113f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 0.0f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.9623838f, 0.9623838f, 1.091532f, 0.6235294f, 0.7137255f, 0.7176471f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.9623838f, 0.9623838f, 1.091532f, 0.6235294f, 0.7137255f, 0.7176471f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, 0.9623838f, 0.9623838f, 1.091532f, 0.6235294f, 0.7137255f, 0.7176471f, -0.9623838f, 0.9623838f, 1.091532f, 0.6235294f, 0.7137255f, 0.7176471f, -1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 0.9623838f, 0.9623838f, 1.091532f, 0.7137255f, 0.7647059f, 0.7019608f, 1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, -1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, -0.9623838f, 0.9623838f, 1.091532f, 0.7137255f, 0.7647059f, 0.7019608f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.9623838f, 0.9623838f, 1.091532f, 0.7137255f, 0.7647059f, 0.7019608f, -0.9623838f, 0.9623838f, 1.091532f, 0.7137255f, 0.7647059f, 0.7019608f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, -1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.211278f, -0.1620964f, 2.591498f, 0.6039216f, 0.6901961f, 0.6941177f, 1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.211278f, -0.1620964f, 2.591498f, 0.6039216f, 0.6901961f, 0.6941177f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.422557f, -0.3241928f, 1.091532f, 0.6980392f, 0.7529412f, 0.6980392f, 1.211278f, -0.1620964f, 2.591498f, 0.6039216f, 0.6901961f, 0.6941177f, -1.211278f, -0.1620964f, 2.591498f, 0.6039216f, 0.6901961f, 0.6941177f, -1.422557f, -0.3241928f, 1.091532f, 0.6980392f, 0.7529412f, 0.6980392f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, 0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, 0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, -0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, -0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, -0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -2.753033f, 0.2287188f, 1.265094f, 0.7568628f, 0.8039216f, 0.6235294f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -4.820496f, -0.03717672f, -0.03526628f, 0.7058824f, 0.7450981f, 0.7450981f, -4.670839f, 0.2244458f, 1.458042f, 0.6313726f, 0.6666667f, 0.6705883f, -2.753033f, 0.2287188f, 1.265094f, 0.7568628f, 0.8039216f, 0.6235294f, -4.820496f, -0.03717672f, -0.03526628f, 0.7058824f, 0.7450981f, 0.7450981f, 4.820496f, -0.03717672f, -0.03526628f, 0.7058824f, 0.7450981f, 0.7450981f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, 2.753033f, 0.2287188f, 1.265094f, 0.7568628f, 0.8039216f, 0.6235294f, 4.670839f, 0.2244458f, 1.458042f, 0.6313726f, 0.6666667f, 0.6705883f, 4.820496f, -0.03717672f, -0.03526628f, 0.7058824f, 0.7450981f, 0.7450981f, 2.753033f, 0.2287188f, 1.265094f, 0.7568628f, 0.8039216f, 0.6235294f, -4.820496f, -0.03717672f, -0.03526628f, 0.6352941f, 0.6627451f, 0.6470589f, -2.904993f, 0.4069349f, -0.2403131f, 0.8f, 0.827451f, 0.6352941f, -4.822799f, 0.4503033f, -0.04736506f, 0.6313726f, 0.6666667f, 0.6509804f, -4.820496f, -0.03717672f, -0.03526628f, 0.6352941f, 0.6627451f, 0.6470589f, -2.902689f, -0.03290349f, -0.2282143f, 0.8705883f, 0.8588235f, 0.6784314f, -2.904993f, 0.4069349f, -0.2403131f, 0.8f, 0.827451f, 0.6352941f, 4.820496f, -0.03717672f, -0.03526628f, 0.6352941f, 0.6627451f, 0.6470589f, 2.904993f, 0.4069349f, -0.2403131f, 0.8f, 0.827451f, 0.6352941f, 2.902689f, -0.03290349f, -0.2282143f, 0.8705883f, 0.8588235f, 0.6784314f, 4.820496f, -0.03717672f, -0.03526628f, 0.6352941f, 0.6627451f, 0.6470589f, 4.822799f, 0.4503033f, -0.04736506f, 0.6313726f, 0.6666667f, 0.6509804f, 2.904993f, 0.4069349f, -0.2403131f, 0.8f, 0.827451f, 0.6352941f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, -4.670839f, 0.2244458f, 1.458042f, 0.6117647f, 0.6509804f, 0.6509804f, -4.822799f, 0.4503033f, -0.04736506f, 0.6313726f, 0.6705883f, 0.6745098f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -4.670839f, 0.2244458f, 1.458042f, 0.6117647f, 0.6509804f, 0.6509804f, 4.670839f, 0.2244458f, 1.458042f, 0.6117647f, 0.6509804f, 0.6509804f, 2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, 4.822799f, 0.4503033f, -0.04736506f, 0.6313726f, 0.6705883f, 0.6745098f, 4.670839f, 0.2244458f, 1.458042f, 0.6117647f, 0.6509804f, 0.6509804f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, -1.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, 0.0f, 0.0f, 4.091465f, 0.3529412f, 0.4f, 0.4039216f, 0.0f, 0.0f, 4.091465f, 0.3529412f, 0.4f, 0.4039216f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, 0.0f, 4.091465f, 0.3529412f, 0.4f, 0.4039216f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3529412f, 0.4f, 0.4039216f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f
        };

        glGenVertexArrays(1, &shipVAO);
        glGenBuffers(1, &shipVBO);

        glBindVertexArray(shipVAO);
        glBindBuffer(GL_ARRAY_BUFFER, shipVBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(shipVertexData.size() * sizeof(GLfloat)),
                     shipVertexData.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    bool resizeFlag = false;
    void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        // Update viewport dimensions on window resize
        glUseProgram(data.shaderProgram);
        int windowWidthLocation = glGetUniformLocation(data.shaderProgram, "windowWidth");
        int windowHeightLocation = glGetUniformLocation(data.shaderProgram, "windowHeight");
        glViewport(0, 0, width, height);
        glUniform1i(windowWidthLocation, width);
        glUniform1i(windowHeightLocation, height);
        resizeFlag = true;
    }

    void set_shader_sources() {
        // The GLSL code for a simple vertex shader;
        // Specified using a string, which is passed as an argument
        // to functions that compile and link shader programs.
        vertexShaderSource = R"(
            #version 330 core
            layout (location = 0) in vec3 vertexPosition;
            layout (location = 1) in vec3 vertexColour;
            out vec3 vertex_colour;
            uniform float fElapsedTime;
            uniform mat4 modelMatrix;
            layout(std140) uniform GlobalMatrices {
                mat4 viewMatrix;
                mat4 projectionMatrix;
            };

            void main() {
                gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0f);
                vertex_colour = vertexColour;
            }
        )";
        fragmentShaderSource = R"(
            #version 330 core
            in vec3 vertex_colour;
            out vec4 FragColour;
            uniform float fElapsedTime;

            void main() {
                FragColour = vec4(vertex_colour, 1.0f);
            }
        )";
    }

    static std::string get_shader_label(GLuint shader) {
        GLint shaderType;
        glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);

        std::string shaderTypeString;

        switch (shaderType) {
            case GL_VERTEX_SHADER:
                shaderTypeString = "Vertex Shader";
                break;
            case GL_FRAGMENT_SHADER:
                shaderTypeString = "Fragment Shader";
                break;
            default:
                // Code for any other values
                break;
        }
        return shaderTypeString;
    }

    static int error_check_shader(GLuint shader) {
        GLint compileStatus;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
        if (compileStatus == GL_FALSE) {
            // Compilation failed, retrieve and log the error information
            GLint infoLogLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

            auto* infoLog = new GLchar[infoLogLength];
            glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog);

            // Output or log the error information
            std::string shader_name = get_shader_label(shader);
            std::cerr << shader_name << " compilation error; deleting shader...:\n" << infoLog << std::endl;

            delete[] infoLog;

            // Optionally, delete the shader object to free resources
            glDeleteShader(shader);

            // Handle compilation failure (e.g., return an error code)
            return -1;
        }
        return 0;
    }

    void compile_and_link_shaders() {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glObjectLabel(GL_SHADER, vertexShader, -1, "Vertex Shader");
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);
        error_check_shader(vertexShader);
        shaders.push_back(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glObjectLabel(GL_SHADER, fragmentShader, -1, "Fragment Shader");
        glCompileShader(fragmentShader);
        error_check_shader(fragmentShader);
        shaders.push_back(fragmentShader);
    }

    void error_check_program() const {
        GLint status;
        glGetProgramiv (data.shaderProgram, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint infoLogLength;
            glGetProgramiv(data.shaderProgram, GL_INFO_LOG_LENGTH, &infoLogLength);

            auto* strInfoLog = new GLchar[infoLogLength + 1];
            glGetProgramInfoLog(data.shaderProgram, infoLogLength, nullptr, strInfoLog);
            std::cerr << "Linker failure: \n" << strInfoLog;
            delete[] strInfoLog;
        }
    }

    void create_and_link_program() {
        data.shaderProgram = glCreateProgram();

        for (GLuint shader : shaders) {
            glAttachShader(data.shaderProgram, shader);
        }

        glLinkProgram(data.shaderProgram);
        error_check_program();

        for (GLuint shader : shaders) {
            glDetachShader(data.shaderProgram, shader);
            glDeleteShader(shader);
        }
    }

    static glm::vec3 ResolveCamPosition() {
        MatrixStack tempMatrix;

        float phi = (sphereCameraRelativePosition.x) * static_cast<float>(M_PI / 180.0);
        float theta = (sphereCameraRelativePosition.y + 90.0f) * static_cast<float>(M_PI / 180.0);

        float fSinTheta = sinf(theta);
        float fCosTheta = cosf(theta);
        float fCosPhi = cosf(phi);
        float fSinPhi = sinf(phi);

        glm::vec3 dirToCamera(fSinTheta * fCosPhi, fCosTheta, fSinTheta * fSinPhi);
        return (dirToCamera * sphereCameraRelativePosition.z) + cameraTarget;
    }

    static glm::mat4 CalcLookAtMatrix(const glm::vec3 &cameraPt, const glm::vec3 &lookPt, const glm::vec3 &upPt) {
        glm::vec3 lookDir = glm::normalize(lookPt - cameraPt);
        glm::vec3 upDir = glm::normalize(upPt);

        glm::vec3 rightDir = glm::normalize(glm::cross(lookDir, upDir));
        glm::vec3 perpUpDir = glm::cross(rightDir, lookDir);

        glm::mat4 rotMat(1.0f);
        rotMat[0] = glm::vec4(rightDir, 0.0f);
        rotMat[1] = glm::vec4(perpUpDir, 0.0f);
        rotMat[2] = glm::vec4(-lookDir, 0.0f);

        rotMat = glm::transpose(rotMat);

        glm::mat4 transMat(1.0f);
        transMat[3] = glm::vec4(-cameraPt, 1.0f);

        return rotMat * transMat;
    }

    void perform_render_sequence(GLFWwindow* window, Orientation& orient) {
        orient.OrientationUpdateTime();

        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(data.shaderProgram);

        MatrixStack modelMatrixStack;

        // Define camera parameters
        const glm::vec3 &cameraPosition = ResolveCamPosition();
        modelMatrixStack.SetMatrix(CalcLookAtMatrix(cameraPosition, cameraTarget, upVector));

        glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f),
                                                      static_cast<GLfloat>(WINDOW_WIDTH) /
                                                      static_cast<GLfloat>(WINDOW_HEIGHT),
                                                      1.0f, 600.0f);

        data.modelMatrixLocation = glGetUniformLocation(data.shaderProgram, "modelMatrix");

        glBindBuffer(GL_UNIFORM_BUFFER, g_GlobalMatricesUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(viewMatrix));
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projectionMatrix));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Draw ship
        drawShip(modelMatrixStack, orient);

        glUseProgram(0);
    }

    void drawShip(MatrixStack modelMatrixStack, Orientation& orient) const {
        modelMatrixStack.Push();
        modelMatrixStack.ApplyMatrix(glm::mat4_cast(orient.OrientationGetOrient()));
        modelMatrixStack.Scale(glm::vec3(3.0, 3.0, 3.0));
        modelMatrixStack.RotateX(-90);

        glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrixStack.Top()));
        glBindVertexArray(shipVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(shipVertexData.size()));
    }


private:
    std::vector<GLuint> shaders;
    const char* vertexShaderSource{};
    const char* fragmentShaderSource{};
};

glm::fquat Renderer::orientation(1.0f, 0.0f, 0.0f, 0.0f);
glm::vec3 Renderer::cameraTarget{0.0f, 0.0f, 0.0f};
glm::vec3 Renderer::upVector{0.0f, 1.0f, 0.0f};
glm::vec3 Renderer::sphereCameraRelativePosition{90.0f, 0.0f, 66.0f};

void ApplyOrientation(int iIndex, Orientation& orient) {
    std::cout << "Do i get here?,.,.," << "\n";
    if(!Orientation::IsAnimating()) {
        std::cout << "IsAnimating = false" << "\n";
        orient.AnimateToOrient(iIndex);
        std::cout << "In ApplyOrientation... destination Orient Index is: " << iIndex << "\n";
    }
    else
        std::cout << "Not animating..." << "\n";
}

// GLFW key callback function
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods, Orientation& orient) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_SPACE: {
                orient.ToggleSlerp();
                printf(Orientation::m_bSlerp ? "Slerp\n" : "Lerp\n");
                break;
            }
        }

        for (int iOrient = 0; iOrient < OrientKeys.size(); iOrient++) {
            if (key == OrientKeys[iOrient]) {
                ApplyOrientation(iOrient, orient);
            }
        }
    }
}

// GLFW requires a static or non-member function for the framebuffer size callback
void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Retrieve the Renderer instance associated with the window
    auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    // Call the member function on the Renderer instance
    if (renderer != nullptr) {
        renderer->framebuffer_size_callback(window, width, height);
    }
}

int main() {
    GLFWwindow* window = initializeGLFW();
    initializeGLEW();

    Renderer renderer = Renderer();
    renderer.set_shader_sources();
    renderer.compile_and_link_shaders();
    renderer.create_and_link_program();

    // Create and bind globalMatrices buffer object to context
    glGenBuffers(1, &g_GlobalMatricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, g_GlobalMatricesUBO);

    // Add this program's global matrices (2) to the globalMatrices uniform block object at position 0
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    renderer.data.globalUniformBlockIndex = glGetUniformBlockIndex(renderer.data.shaderProgram, "GlobalMatrices");
    glUniformBlockBinding(renderer.data.shaderProgram, renderer.data.globalUniformBlockIndex,
                          g_iGlobalMatricesBindingIndex);
    glBindBufferRange(GL_UNIFORM_BUFFER, g_iGlobalMatricesBindingIndex, g_GlobalMatricesUBO,
                      0, sizeof(glm::mat4) * 2);

    // Set uniform variable values in Shader
    GLint windowWidthLocation = glGetUniformLocation(renderer.data.shaderProgram, "windowWidth");
    GLint windowHeightLocation = glGetUniformLocation(renderer.data.shaderProgram, "windowHeight");
    glUseProgram(renderer.data.shaderProgram);
    glUniform1i(windowWidthLocation, WINDOW_WIDTH);
    glUniform1i(windowHeightLocation, WINDOW_HEIGHT);

    // Create Animation and Orientation instance
    Orientation orient;

    // Create plane and gimbals
    std::cout << "Creating unit plane..." << "\n";
    renderer.createUnitPlane();

    std::cout << "Creating ship..." << "\n";
    renderer.createShip();

    // Set initial positions
    renderer.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // Set the user-defined pointer to the Renderer instance
    glfwSetWindowUserPointer(window, &renderer);

    // Set the key callback
    glfwSetKeyCallback(window, reinterpret_cast<GLFWkeyfun>(keyCallback));

    // Set the resize callback
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Process input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Check if re-render is needed
        if (renderer.resizeFlag) {
            renderer.perform_render_sequence(window, orient);
            renderer.resizeFlag = false; // Reset the flag
        }

        renderer.perform_render_sequence(window, orient);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();

    return 0;
}