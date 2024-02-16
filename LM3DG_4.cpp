// --- Quaternions --- \\

#include "libraries/glew-2.1.0/include/GL/glew.h"
#include "libraries/glfw-master/include/GLFW/glfw3.h"
#include "libraries/glm-master/glm/glm.hpp"
#include "libraries/glm-master/glm/ext.hpp"
#include "xmlparser.cpp"
#include <cmath>
#include <iostream>
#include <vector>
#include <random>

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

GLuint g_GlobalMatricesUBO;
static const int g_iGlobalMatricesBindingIndex = 0;

glm::fquat g_orientation(1.0f, 0.0f, 0.0f, 0.0f);

#define SMALL_ANGLE_INCREMENT 15.0f

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
    GLuint shipVBO{};
    GLuint shipVAO{};
    std::vector<GLfloat> shipVertexData;
    std::vector<std::vector<GLuint>> shipVertexIndicesTri;
    glm::mat4 modelMatrix{};

    static bool g_bRightMultiply;

    Renderer() {
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
    }

    void createShip() {
        shipVertexData = {
                0.0f, 0.0f, 5.486261f, 0.6705883f, 0.7607843f, 0.7607843f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 0.0f, 5.486261f, 0.6705883f, 0.7607843f, 0.7607843f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.8f, 0.0f, 5.091465f, 0.682353f, 0.772549f, 0.7411765f, 0.0f, 0.0f, 5.486261f, 0.6705883f, 0.7607843f, 0.7607843f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 0.0f, 5.486261f, 0.6705883f, 0.7607843f, 0.7607843f, -0.8f, 0.0f, 5.091465f, 0.682353f, 0.772549f, 0.7411765f, 1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.8f, 0.0f, 5.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.8f, 0.0f, 5.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, 0.8f, 0.0f, 5.091465f, 0.4117647f, 0.4666666f, 0.4705882f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -0.8f, 0.0f, 5.091465f, 0.4117647f, 0.4666666f, 0.4705882f, 0.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, 0.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, 0.0f, 0.0f, 5.486261f, 0.3843137f, 0.4352941f, 0.4352941f, 0.8f, 0.0f, 5.091465f, 0.4117647f, 0.4666666f, 0.4705882f, -0.8f, 0.0f, 5.091465f, 0.4117647f, 0.4666666f, 0.4705882f, 0.0f, 0.0f, 5.486261f, 0.3843137f, 0.4352941f, 0.4352941f, 0.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, -0.7365548f, 0.7365548f, 4.091465f, 1.0f, 1.0f, 1.0f, -0.7365548f, 0.7365548f, 4.091465f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 4.091465f, 1.0f, 1.0f, 1.0f, -0.7365548f, 0.7365548f, 4.091465f, 1.0f, 1.0f, 1.0f, -0.7365548f, 0.7365548f, 4.091465f, 1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 4.091465f, 1.0f, 1.0f, 1.0f, 0.0f, 1.845113f, -1.908402f, 0.6352941f, 0.7215687f, 0.6941177f, 1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, 0.0f, 1.845113f, -1.908402f, 0.6352941f, 0.7215687f, 0.6941177f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, -1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, -1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 1.516663f, 0.2699136f, -1.908402f, 0.4627451f, 0.5411765f, 0.5215687f, -1.516663f, 0.2699136f, -1.908402f, 0.4627451f, 0.5411765f, 0.5215687f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, -1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, 2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -1.094106f, 0.5941064f, 1.091532f, 0.6235294f, 0.7137255f, 0.7137255f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 3.05465f, 0.1929539f, -1.733622f, 0.5647059f, 0.6627451f, 0.6196079f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -3.05465f, 0.1929539f, -1.733622f, 0.5647059f, 0.6627451f, 0.6196079f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.516663f, 0.2699136f, -1.908402f, 0.372549f, 0.4156863f, 0.3803921f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.516663f, 0.2699136f, -1.908402f, 0.372549f, 0.4156863f, 0.3803921f, 1.516663f, 0.2699136f, -1.908402f, 0.372549f, 0.4156863f, 0.3803921f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, 3.05465f, 0.1929539f, -1.733622f, 0.8784314f, 0.8627451f, 0.6784314f, -3.05465f, 0.1929539f, -1.733622f, 0.8784314f, 0.8627451f, 0.6784314f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -1.516663f, 0.2699136f, -1.908402f, 0.372549f, 0.4156863f, 0.3803921f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 2.750729f, -0.2587609f, 1.277193f, 0.8901961f, 0.8784314f, 0.6392157f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -2.750729f, -0.2587609f, 1.277193f, 0.8901961f, 0.8784314f, 0.6392157f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 1.422557f, -0.3241928f, 1.091532f, 0.6156863f, 0.7019608f, 0.7058824f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 2.753033f, 0.2287188f, 1.265094f, 0.8392157f, 0.8431373f, 0.6941177f, -2.753033f, 0.2287188f, 1.265094f, 0.8392157f, 0.8431373f, 0.6941177f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, -1.422557f, -0.3241928f, 1.091532f, 0.6156863f, 0.7019608f, 0.7058824f, 1.422557f, -0.3241928f, 1.091532f, 0.6156863f, 0.7019608f, 0.7058824f, 2.753033f, 0.2287188f, 1.265094f, 0.827451f, 0.8392157f, 0.6980392f, 2.750729f, -0.2587609f, 1.277193f, 0.7529412f, 0.8196079f, 0.6470589f, -2.750729f, -0.2587609f, 1.277193f, 0.7529412f, 0.8196079f, 0.6470589f, -2.753033f, 0.2287188f, 1.265094f, 0.827451f, 0.8392157f, 0.6980392f, -1.422557f, -0.3241928f, 1.091532f, 0.6156863f, 0.7019608f, 0.7058824f, 1.094106f, 0.5941064f, 1.091532f, 0.6196079f, 0.7098039f, 0.7137255f, 2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, 2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, -2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, -2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -1.094106f, 0.5941064f, 1.091532f, 0.6196079f, 0.7098039f, 0.7137255f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, 2.750729f, -0.2587609f, 1.277193f, 0.8627451f, 0.854902f, 0.6745098f, 2.753033f, 0.2287188f, 1.265094f, 0.8470588f, 0.8431373f, 0.6862745f, -2.753033f, 0.2287188f, 1.265094f, 0.8470588f, 0.8431373f, 0.6862745f, -2.750729f, -0.2587609f, 1.277193f, 0.8627451f, 0.854902f, 0.6745098f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, 2.904993f, 0.4069349f, -0.2403131f, 0.8431373f, 0.8431373f, 0.6588236f, 2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, 2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -2.755336f, 0.5183109f, 1.252996f, 0.8666667f, 0.854902f, 0.682353f, -2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, -2.904993f, 0.4069349f, -0.2403131f, 0.8431373f, 0.8431373f, 0.6588236f, 3.05465f, 0.1929539f, -1.733622f, 0.5568628f, 0.6588236f, 0.6392157f, 2.902689f, -0.03290349f, -0.2282143f, 0.8745098f, 0.8588235f, 0.6784314f, 2.904993f, 0.4069349f, -0.2403131f, 0.8431373f, 0.8470588f, 0.6627451f, -2.904993f, 0.4069349f, -0.2403131f, 0.8431373f, 0.8470588f, 0.6627451f, -2.902689f, -0.03290349f, -0.2282143f, 0.8745098f, 0.8588235f, 0.6784314f, -3.05465f, 0.1929539f, -1.733622f, 0.5568628f, 0.6588236f, 0.6392157f, 4.670839f, 0.2244458f, 1.458042f, 0.6078432f, 0.6431373f, 0.6470589f, 4.822799f, 0.4503033f, -0.04736506f, 0.6196079f, 0.654902f, 0.6588236f, 4.820496f, -0.03717672f, -0.03526628f, 0.6156863f, 0.6509804f, 0.654902f, -4.820496f, -0.03717672f, -0.03526628f, 0.6156863f, 0.6509804f, 0.654902f, -4.822799f, 0.4503033f, -0.04736506f, 0.6196079f, 0.654902f, 0.6588236f, -4.670839f, 0.2244458f, 1.458042f, 0.6078432f, 0.6431373f, 0.6470589f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 0.0f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 1.845113f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.845113f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 1.188213f, 1.188213f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, 1.845113f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, 1.845113f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.188213f, 1.188213f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 0.0f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 0.0f, 1.845113f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, 1.845113f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, -1.516663f, 0.2699136f, -1.908402f, 0.7411765f, 0.8f, 0.4941176f, 0.0f, -0.6483857f, -1.908402f, 0.9960784f, 1.0f, 0.317647f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.9623838f, 0.9623838f, 1.091532f, 0.6235294f, 0.7137255f, 0.7176471f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -0.9623838f, 0.9623838f, 1.091532f, 0.6235294f, 0.7137255f, 0.7176471f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, 0.9623838f, 0.9623838f, 1.091532f, 0.6235294f, 0.7137255f, 0.7176471f, -0.9623838f, 0.9623838f, 1.091532f, 0.6235294f, 0.7137255f, 0.7176471f, -1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, 0.0f, 1.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 0.9623838f, 0.9623838f, 1.091532f, 0.7137255f, 0.7647059f, 0.7019608f, 1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, -1.188213f, 1.188213f, -1.908402f, 0.7019608f, 0.772549f, 0.627451f, -0.9623838f, 0.9623838f, 1.091532f, 0.7137255f, 0.7647059f, 0.7019608f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, 0.9623838f, 0.9623838f, 1.091532f, 0.7137255f, 0.7647059f, 0.7019608f, -0.9623838f, 0.9623838f, 1.091532f, 0.7137255f, 0.7647059f, 0.7019608f, -0.7365548f, 0.7365548f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, -1.845113f, -0.6483857f, -1.908402f, 0.3686274f, 0.4156863f, 0.4196078f, 1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -1.422557f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, -0.7112783f, -0.4862892f, -0.4084351f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.211278f, -0.1620964f, 2.591498f, 0.6039216f, 0.6901961f, 0.6941177f, 1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.0f, 0.0f, 4.091465f, 0.6666667f, 0.7607843f, 0.7647059f, -1.211278f, -0.1620964f, 2.591498f, 0.6039216f, 0.6901961f, 0.6941177f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 1.422557f, -0.3241928f, 1.091532f, 0.6980392f, 0.7529412f, 0.6980392f, 1.211278f, -0.1620964f, 2.591498f, 0.6039216f, 0.6901961f, 0.6941177f, -1.211278f, -0.1620964f, 2.591498f, 0.6039216f, 0.6901961f, 0.6941177f, -1.422557f, -0.3241928f, 1.091532f, 0.6980392f, 0.7529412f, 0.6980392f, -1.094106f, 0.5941064f, 1.091532f, 0.6392157f, 0.7215687f, 0.7098039f, 0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, 0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.75f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -1.211278f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, 0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, -0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, -0.5f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, -0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.6483857f, -1.908402f, 0.3215686f, 0.3647059f, 0.3686274f, -0.125f, -0.405241f, 0.3415482f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -2.753033f, 0.2287188f, 1.265094f, 0.7568628f, 0.8039216f, 0.6235294f, -2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, -4.820496f, -0.03717672f, -0.03526628f, 0.7058824f, 0.7450981f, 0.7450981f, -4.670839f, 0.2244458f, 1.458042f, 0.6313726f, 0.6666667f, 0.6705883f, -2.753033f, 0.2287188f, 1.265094f, 0.7568628f, 0.8039216f, 0.6235294f, -4.820496f, -0.03717672f, -0.03526628f, 0.7058824f, 0.7450981f, 0.7450981f, 4.820496f, -0.03717672f, -0.03526628f, 0.7058824f, 0.7450981f, 0.7450981f, 2.902689f, -0.03290349f, -0.2282143f, 0.882353f, 0.8705883f, 0.6588236f, 2.753033f, 0.2287188f, 1.265094f, 0.7568628f, 0.8039216f, 0.6235294f, 4.670839f, 0.2244458f, 1.458042f, 0.6313726f, 0.6666667f, 0.6705883f, 4.820496f, -0.03717672f, -0.03526628f, 0.7058824f, 0.7450981f, 0.7450981f, 2.753033f, 0.2287188f, 1.265094f, 0.7568628f, 0.8039216f, 0.6235294f, -4.820496f, -0.03717672f, -0.03526628f, 0.6352941f, 0.6627451f, 0.6470589f, -2.904993f, 0.4069349f, -0.2403131f, 0.8f, 0.827451f, 0.6352941f, -4.822799f, 0.4503033f, -0.04736506f, 0.6313726f, 0.6666667f, 0.6509804f, -4.820496f, -0.03717672f, -0.03526628f, 0.6352941f, 0.6627451f, 0.6470589f, -2.902689f, -0.03290349f, -0.2282143f, 0.8705883f, 0.8588235f, 0.6784314f, -2.904993f, 0.4069349f, -0.2403131f, 0.8f, 0.827451f, 0.6352941f, 4.820496f, -0.03717672f, -0.03526628f, 0.6352941f, 0.6627451f, 0.6470589f, 2.904993f, 0.4069349f, -0.2403131f, 0.8f, 0.827451f, 0.6352941f, 2.902689f, -0.03290349f, -0.2282143f, 0.8705883f, 0.8588235f, 0.6784314f, 4.820496f, -0.03717672f, -0.03526628f, 0.6352941f, 0.6627451f, 0.6470589f, 4.822799f, 0.4503033f, -0.04736506f, 0.6313726f, 0.6666667f, 0.6509804f, 2.904993f, 0.4069349f, -0.2403131f, 0.8f, 0.827451f, 0.6352941f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, -4.670839f, 0.2244458f, 1.458042f, 0.6117647f, 0.6509804f, 0.6509804f, -4.822799f, 0.4503033f, -0.04736506f, 0.6313726f, 0.6705883f, 0.6745098f, -2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, -4.670839f, 0.2244458f, 1.458042f, 0.6117647f, 0.6509804f, 0.6509804f, 4.670839f, 0.2244458f, 1.458042f, 0.6117647f, 0.6509804f, 0.6509804f, 2.753033f, 0.2287188f, 1.265094f, 0.8431373f, 0.8431373f, 0.6941177f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, 4.822799f, 0.4503033f, -0.04736506f, 0.6313726f, 0.6705883f, 0.6745098f, 4.670839f, 0.2244458f, 1.458042f, 0.6117647f, 0.6509804f, 0.6509804f, 2.904993f, 0.4069349f, -0.2403131f, 0.8666667f, 0.854902f, 0.6745098f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, -1.0f, 0.0f, 4.091465f, 0.3843137f, 0.4392157f, 0.4431372f, 0.0f, 0.0f, 4.091465f, 0.3529412f, 0.4f, 0.4039216f, 0.0f, 0.0f, 4.091465f, 0.3529412f, 0.4f, 0.4039216f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, 0.0f, 4.091465f, 0.3529412f, 0.4f, 0.4039216f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3529412f, 0.4f, 0.4039216f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.1620964f, 2.591498f, 0.3529412f, 0.4f, 0.4039216f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, 0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, 0.0f, -0.3241928f, 1.091532f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f, -1.0f, 0.0f, 4.091465f, 0.3215686f, 0.3647059f, 0.3686274f, -0.25f, -0.1620964f, 2.591498f, 0.3215686f, 0.3647059f, 0.3686274f
        };

        glGenVertexArrays(1, &shipVAO);
        glGenBuffers(1, &shipVBO);
        // glGenBuffers(1, &shipEBO);

        glBindVertexArray(shipVAO);
        glBindBuffer(GL_ARRAY_BUFFER, shipVBO);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shipEBO);

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

    static void OffsetOrientation(const glm::vec3 &_axis, float fAngDeg)
    {
        float fAngRad = glm::radians(fAngDeg);

        glm::vec3 axis = glm::normalize(_axis);

        axis = axis * sinf(fAngRad / 2.0f);
        float scalar = cosf(fAngRad / 2.0f);

        glm::fquat offset(scalar, axis.x, axis.y, axis.z);

        if (g_bRightMultiply)
            g_orientation = g_orientation * offset;
        else
            g_orientation = offset * g_orientation;

        g_orientation = glm::normalize(g_orientation);
    }

    void perform_render_sequence(GLFWwindow* window) {
        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(data.shaderProgram);

        // Define camera parameters
        glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 300.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

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

        MatrixStack modelMatrixStack;
        modelMatrixStack.Translate(glm::vec3(0.0f, 0.0f, -200.0f));
        modelMatrixStack.ApplyMatrix(glm::mat4_cast(g_orientation));

        modelMatrixStack.Scale(glm::vec3(20.0, 20.0, 20.0));
        modelMatrixStack.RotateX(-90);

        glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrixStack.Top()));

        // Draw ship
        drawShip();

        glUseProgram(0);
    }

    void drawShip() const {
        glBindVertexArray(shipVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(shipVertexData.size()));
    }

    // GLFW key callback function
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {

            switch (key) {
                case GLFW_KEY_W: {
                    wKeyPressed = true;
                    OffsetOrientation(glm::vec3(1.0f, 0.0f, 0.0f), SMALL_ANGLE_INCREMENT);
                    break;
                }
                case GLFW_KEY_S: {
                    sKeyPressed = true;
                    OffsetOrientation(glm::vec3(1.0f, 0.0f, 0.0f), -SMALL_ANGLE_INCREMENT);
                    break;
                }
                case GLFW_KEY_A: {
                    aKeyPressed = true;
                    OffsetOrientation(glm::vec3(0.0f, 0.0f, 1.0f), SMALL_ANGLE_INCREMENT);
                    break;
                }
                case GLFW_KEY_D: {
                    dKeyPressed = true;
                    OffsetOrientation(glm::vec3(0.0f, 0.0f, 1.0f), -SMALL_ANGLE_INCREMENT);
                    break;
                }
                case GLFW_KEY_Q: {
                    qKeyPressed = true;
                    OffsetOrientation(glm::vec3(0.0f, 1.0f, 0.0f), SMALL_ANGLE_INCREMENT);
                    break;
                }
                case GLFW_KEY_E: {
                    eKeyPressed = true;
                    OffsetOrientation(glm::vec3(0.0f, 1.0f, 0.0f), -SMALL_ANGLE_INCREMENT);
                    break;
                }
                case GLFW_KEY_SPACE: {
                    g_bRightMultiply = !g_bRightMultiply;
                    printf(g_bRightMultiply ? "Right-multiply\n" : "Left-multiply\n");
                    break;
                }
                default:
                    break;
            }

            if (action == GLFW_RELEASE) {
                wKeyPressed = false;
                sKeyPressed = false;
                dKeyPressed = false;
                aKeyPressed = false;
                eKeyPressed = false;
                qKeyPressed = false;
            }
        }
    }

private:
    std::vector<GLuint> shaders;
    const char* vertexShaderSource{};
    const char* fragmentShaderSource{};
};

bool Renderer::g_bRightMultiply = true;

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

    // Create plane and gimbals
    std::cout << "Creating ship..." << "\n";
    renderer.createShip();

    // Set initial positions
    renderer.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // Set the user-defined pointer to the Renderer instance
    glfwSetWindowUserPointer(window, &renderer);

    // Set the key callback
    glfwSetKeyCallback(window, Renderer::keyCallback);

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
            renderer.perform_render_sequence(window);
            renderer.resizeFlag = false; // Reset the flag
        }

        renderer.perform_render_sequence(window);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();

    return 0;
}