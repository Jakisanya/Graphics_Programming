#include "libraries/glew-2.1.0/include/GL/glew.h"
#include "libraries/glfw-master/include/GLFW/glfw3.h"
#include "libraries/glm-master/glm/glm.hpp"
#include "libraries/glm-master/glm/ext.hpp"
#include "libraries/rapidxml-master/rapidxml.hpp"
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
    // GLuint baseColorUnif;
    // GLuint thing;
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
    GLuint unitPlaneVBO{}, smallGimbalVBO{}, mediumGimbalVBO{}, largeGimbalVBO{};
    GLuint unitPlaneVAO{}, smallGimbalVAO{}, mediumGimbalVAO{}, largeGimbalVAO{};
    GLuint unitPlaneEBO{};
    std::vector<GLuint> smallGimbalEBOs{}, mediumGimbalEBOs{}, largeGimbalEBOs{};
    std::vector<GLuint> unitPlaneVertexIndicesTri, smallGimbalVertexIndicesTriStrip1, smallGimbalVertexIndicesTriStrip2,
            smallGimbalVertexIndicesTriStrip3, smallGimbalVertexIndicesTriStrip4, smallGimbalVertexIndicesTri,
            mediumGimbalVertexIndicesTriStrip1, mediumGimbalVertexIndicesTriStrip2, mediumGimbalVertexIndicesTriStrip3,
            mediumGimbalVertexIndicesTriStrip4, mediumGimbalVertexIndicesTri, largeGimbalVertexIndicesTriStrip1,
            largeGimbalVertexIndicesTriStrip2, largeGimbalVertexIndicesTriStrip3, largeGimbalVertexIndicesTriStrip4,
            largeGimbalVertexIndicesTri;
    glm::mat4 modelMatrix{};
    GLint viewMatrixLocation{}, projectionMatrixLocation{};

    static bool g_boolDrawLookatPoint;
    static glm::vec3 g_cameraTarget;
    static glm::vec3 g_sphereCameraRelativePosition;

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

    void createSmallGimbal() {
        std::vector<GLfloat> smallGimbalVertexData {

        }
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

        float phi = (g_sphereCameraRelativePosition.x) * static_cast<float>(M_PI / 180.0);
        float theta = (g_sphereCameraRelativePosition.y + 90.0f) * static_cast<float>(M_PI / 180.0);

        float fSinTheta = sinf(theta);
        float fCosTheta = cosf(theta);
        float fCosPhi = cosf(phi);
        float fSinPhi = sinf(phi);

        glm::vec3 dirToCamera(fSinTheta * fCosPhi, fCosTheta, fSinTheta * fSinPhi);
        return (dirToCamera * g_sphereCameraRelativePosition.z) + g_cameraTarget;
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

    void perform_render_sequence(GLFWwindow* window) {
        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Define camera parameters
        glm::vec3 cameraPosition = ResolveCamPosition();
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f) + g_cameraTarget;
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 viewMatrix = CalcLookAtMatrix(cameraPosition, cameraTarget, upVector);
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f),
                                                      static_cast<GLfloat>(WINDOW_WIDTH) /
                                                      static_cast<GLfloat>(WINDOW_HEIGHT),
                                                      0.1f, 200.0f);

        data.modelMatrixLocation = glGetUniformLocation(data.shaderProgram, "modelMatrix");
        glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

        glBindBuffer(GL_UNIFORM_BUFFER, g_GlobalMatricesUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(viewMatrix));
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projectionMatrix));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        drawTerrain(window);
        drawForest(window);
        // Draw the Parthenon.
        {
            MatrixStack modelToCameraStack;
            // Set Parthenon position in scene
            modelToCameraStack.Translate(glm::vec3(20.0f, 0.0f, -10.0f));
            drawParthenon(modelToCameraStack);
        }
        if (g_boolDrawLookatPoint) drawLookAtPoint();
    }

    void drawTerrain(GLFWwindow* window) const {
        MatrixStack modelToCameraStack;

        glUseProgram(data.shaderProgram);
        glBindVertexArray(unitPlaneVAO);

        glm::vec3 sceneScale = {100.0f, 0.0f, 100.0f};
        modelToCameraStack.Scale(sceneScale);
        glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitPlaneEBO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndicesTri.size()),
                       GL_UNSIGNED_INT, nullptr);
    }

    struct TreeData
    {
        float fXPos;
        float fZPos;
        float fTrunkHeight;
        float fConeHeight;
    };

    const std::vector<TreeData> g_forest = {
        {-45.0f, -40.0f, 2.0f, 3.0f},
        {-42.0f, -35.0f, 2.0f, 3.0f},
        {-39.0f, -29.0f, 2.0f, 4.0f},
        {-44.0f, -26.0f, 3.0f, 3.0f},
        {-40.0f, -22.0f, 2.0f, 4.0f},
        {-36.0f, -15.0f, 3.0f, 3.0f},
        {-41.0f, -11.0f, 2.0f, 3.0f},
        {-37.0f, -6.0f, 3.0f, 3.0f},
        {-45.0f, 0.0f, 2.0f, 3.0f},
        {-39.0f, 4.0f, 3.0f, 4.0f},
        {-36.0f, 8.0f, 2.0f, 3.0f},
        {-44.0f, 13.0f, 3.0f, 3.0f},
        {-42.0f, 17.0f, 2.0f, 3.0f},
        {-38.0f, 23.0f, 3.0f, 4.0f},
        {-41.0f, 27.0f, 2.0f, 3.0f},
        {-39.0f, 32.0f, 3.0f, 3.0f},
        {-44.0f, 37.0f, 3.0f, 4.0f},
        {-36.0f, 42.0f, 2.0f, 3.0f},

        {-32.0f, -45.0f, 2.0f, 3.0f},
        {-30.0f, -42.0f, 2.0f, 4.0f},
        {-34.0f, -38.0f, 3.0f, 5.0f},
        {-33.0f, -35.0f, 3.0f, 4.0f},
        {-29.0f, -28.0f, 2.0f, 3.0f},
        {-26.0f, -25.0f, 3.0f, 5.0f},
        {-35.0f, -21.0f, 3.0f, 4.0f},
        {-31.0f, -17.0f, 3.0f, 3.0f},
        {-28.0f, -12.0f, 2.0f, 4.0f},
        {-29.0f, -7.0f, 3.0f, 3.0f},
        {-26.0f, -1.0f, 2.0f, 4.0f},
        {-32.0f, 6.0f, 2.0f, 3.0f},
        {-30.0f, 10.0f, 3.0f, 5.0f},
        {-33.0f, 14.0f, 2.0f, 4.0f},
        {-35.0f, 19.0f, 3.0f, 4.0f},
        {-28.0f, 22.0f, 2.0f, 3.0f},
        {-33.0f, 26.0f, 3.0f, 3.0f},
        {-29.0f, 31.0f, 3.0f, 4.0f},
        {-32.0f, 38.0f, 2.0f, 3.0f},
        {-27.0f, 41.0f, 3.0f, 4.0f},
        {-31.0f, 45.0f, 2.0f, 4.0f},
        {-28.0f, 48.0f, 3.0f, 5.0f},

        {-25.0f, -48.0f, 2.0f, 3.0f},
        {-20.0f, -42.0f, 3.0f, 4.0f},
        {-22.0f, -39.0f, 2.0f, 3.0f},
        {-19.0f, -34.0f, 2.0f, 3.0f},
        {-23.0f, -30.0f, 3.0f, 4.0f},
        {-24.0f, -24.0f, 2.0f, 3.0f},
        {-16.0f, -21.0f, 2.0f, 3.0f},
        {-17.0f, -17.0f, 3.0f, 3.0f},
        {-25.0f, -13.0f, 2.0f, 4.0f},
        {-23.0f, -8.0f, 2.0f, 3.0f},
        {-17.0f, -2.0f, 3.0f, 3.0f},
        {-16.0f, 1.0f, 2.0f, 3.0f},
        {-19.0f, 4.0f, 3.0f, 3.0f},
        {-22.0f, 8.0f, 2.0f, 4.0f},
        {-21.0f, 14.0f, 2.0f, 3.0f},
        {-16.0f, 19.0f, 2.0f, 3.0f},
        {-23.0f, 24.0f, 3.0f, 3.0f},
        {-18.0f, 28.0f, 2.0f, 4.0f},
        {-24.0f, 31.0f, 2.0f, 3.0f},
        {-20.0f, 36.0f, 2.0f, 3.0f},
        {-22.0f, 41.0f, 3.0f, 3.0f},
        {-21.0f, 45.0f, 2.0f, 3.0f},

        {-12.0f, -40.0f, 2.0f, 4.0f},
        {-11.0f, -35.0f, 3.0f, 3.0f},
        {-10.0f, -29.0f, 1.0f, 3.0f},
        {-9.0f, -26.0f, 2.0f, 2.0f},
        {-6.0f, -22.0f, 2.0f, 3.0f},
        {-15.0f, -15.0f, 1.0f, 3.0f},
        {-8.0f, -11.0f, 2.0f, 3.0f},
        {-14.0f, -6.0f, 2.0f, 4.0f},
        {-12.0f, 0.0f, 2.0f, 3.0f},
        {-7.0f, 4.0f, 2.0f, 2.0f},
        {-13.0f, 8.0f, 2.0f, 2.0f},
        {-9.0f, 13.0f, 1.0f, 3.0f},
        {-13.0f, 17.0f, 3.0f, 4.0f},
        {-6.0f, 23.0f, 2.0f, 3.0f},
        {-12.0f, 27.0f, 1.0f, 2.0f},
        {-8.0f, 32.0f, 2.0f, 3.0f},
        {-10.0f, 37.0f, 3.0f, 3.0f},
        {-11.0f, 42.0f, 2.0f, 2.0f},


        {15.0f, 5.0f, 2.0f, 3.0f},
        {15.0f, 10.0f, 2.0f, 3.0f},
        {15.0f, 15.0f, 2.0f, 3.0f},
        {15.0f, 20.0f, 2.0f, 3.0f},
        {15.0f, 25.0f, 2.0f, 3.0f},
        {15.0f, 30.0f, 2.0f, 3.0f},
        {15.0f, 35.0f, 2.0f, 3.0f},
        {15.0f, 40.0f, 2.0f, 3.0f},
        {15.0f, 45.0f, 2.0f, 3.0f},

        {25.0f, 5.0f, 2.0f, 3.0f},
        {25.0f, 10.0f, 2.0f, 3.0f},
        {25.0f, 15.0f, 2.0f, 3.0f},
        {25.0f, 20.0f, 2.0f, 3.0f},
        {25.0f, 25.0f, 2.0f, 3.0f},
        {25.0f, 30.0f, 2.0f, 3.0f},
        {25.0f, 35.0f, 2.0f, 3.0f},
        {25.0f, 40.0f, 2.0f, 3.0f},
        {25.0f, 45.0f, 2.0f, 3.0f}
    };

    void drawForest(GLFWwindow* window) {
        for(const TreeData& tree : g_forest) {
            const TreeData& currTree = tree;
            MatrixStack modelToCameraStack;
            modelToCameraStack.Translate(glm::vec3(currTree.fXPos, 0.0f, currTree.fZPos));
            drawTree(modelToCameraStack, currTree.fTrunkHeight, currTree.fConeHeight);
        }
    };

    void drawTree(MatrixStack modelToCameraStack, float fTrunkHeight = 2.0f, float fConeHeight = 3.0f) const {
        // Draw trunk.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Scale(glm::vec3(1.0f, fTrunkHeight, 1.0f));
            modelToCameraStack.Translate(glm::vec3(0.0f, 0.5f, 0.0f));

            glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
            glBindVertexArray(unitCylinderVAO1);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriFan1);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitCylinderVertexIndicesTriFan1.size()),
                           GL_UNSIGNED_INT, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriFan2);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitCylinderVertexIndicesTriFan2.size()),
                           GL_UNSIGNED_INT, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriStrip);
            glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLsizei>(unitCylinderVertexIndicesTriStrip.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        // Draw the treetop.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Translate(glm::vec3(0.0f, fTrunkHeight, 0.0f));
            modelToCameraStack.Scale(glm::vec3(3.0f, fConeHeight, 3.0f));

            glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
            glBindVertexArray(unitConeVAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitConeEBOTriFan1);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitConeVertexIndicesTriFan1.size()),
                           GL_UNSIGNED_INT, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitConeEBOTriFan2);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitConeVertexIndicesTriFan2.size()),
                           GL_UNSIGNED_INT, nullptr);
        }
    }

    void drawParthenon(MatrixStack modelToCameraStack) {
        const float g_fParthenonWidth = 14.0f;
        const float g_fParthenonLength = 20.0f;
        const float g_fParthenonColumnHeight = 5.0f;
        const float g_fParthenonBaseHeight = 1.0f;
        const float g_fParthenonTopHeight = 2.0f;

        // Draw base.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Scale(glm::vec3(g_fParthenonWidth, g_fParthenonBaseHeight, g_fParthenonLength));
            modelToCameraStack.Translate(glm::vec3(0.0f, 0.5f, 0.0f));
            glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
            glBindVertexArray(unitCubeVAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndicesTri.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        // Draw top.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Translate(glm::vec3(0.0f, g_fParthenonColumnHeight + g_fParthenonBaseHeight, 0.0f));
            modelToCameraStack.Scale(glm::vec3(g_fParthenonWidth, g_fParthenonBaseHeight, g_fParthenonLength));
            modelToCameraStack.Translate(glm::vec3(0.0f, 0.5f, 0.0f));
            glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
            glBindVertexArray(unitCubeVAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndicesTri.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        //Draw columns.
        const float fFrontZVal = (g_fParthenonLength / 2.0f) - 1.0f;
        const float fRightXVal = (g_fParthenonWidth / 2.0f) - 1.0f;

        for (int iColumnNum = 0; iColumnNum < int(g_fParthenonWidth / 2.0f); iColumnNum++) {
            {
                modelToCameraStack.Push();
                modelToCameraStack.Translate(glm::vec3((2.0f * static_cast<float>(iColumnNum)) - (g_fParthenonWidth / 2.0f) + 1.0f,
                                                g_fParthenonBaseHeight, fFrontZVal));

                drawColumn(modelToCameraStack, g_fParthenonColumnHeight);
                modelToCameraStack.Pop();
            }
            {
                modelToCameraStack.Push();
                modelToCameraStack.Translate(glm::vec3((2.0f * static_cast<float>(iColumnNum)) - (g_fParthenonWidth / 2.0f) + 1.0f,
                                                g_fParthenonBaseHeight, -fFrontZVal));

                drawColumn(modelToCameraStack, g_fParthenonColumnHeight);
                modelToCameraStack.Pop();
            }
        }

        // Don't draw the first or last columns, since they've been drawn already.
        for (int iColumnNum = 1; iColumnNum < int((g_fParthenonLength - 2.0f) / 2.0f); iColumnNum++) {
            {
                modelToCameraStack.Push();
                modelToCameraStack.Translate(glm::vec3(fRightXVal,
                                                g_fParthenonBaseHeight,
                                                (2.0f * static_cast<float>(iColumnNum)) - (g_fParthenonLength / 2.0f) + 1.0f));

                drawColumn(modelToCameraStack, g_fParthenonColumnHeight);
                modelToCameraStack.Pop();
            }
            {
                modelToCameraStack.Push();
                modelToCameraStack.Translate(glm::vec3(-fRightXVal,
                                                       g_fParthenonBaseHeight,
                                                       (2.0f * static_cast<float>(iColumnNum)) - (g_fParthenonLength / 2.0f) + 1.0f));

                drawColumn(modelToCameraStack, g_fParthenonColumnHeight);
                modelToCameraStack.Pop();
            }
        }

        // Draw interior.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Translate(glm::vec3(0.0f, 1.0f, 0.0f));
            modelToCameraStack.Scale(glm::vec3(g_fParthenonWidth - 6.0f, g_fParthenonColumnHeight,
                                        g_fParthenonLength - 6.0f));
            modelToCameraStack.Translate(glm::vec3(0.0f, 0.5f, 0.0f));

            glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
            glBindVertexArray(unitCubeVAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndicesTri.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        // Draw headpiece.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Translate(glm::vec3(
                    0.0f,
                    g_fParthenonColumnHeight + g_fParthenonBaseHeight + (g_fParthenonTopHeight / 2.0f),
                    g_fParthenonLength / 2.0f));
            modelToCameraStack.RotateX(-135.0f);
            modelToCameraStack.RotateY(45.0f);

            glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
            glBindVertexArray(unitCubeVAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndicesTri.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }
    }

    // Columns are 1x1 in the X/Z, and fHeight units in the Y.
    void drawColumn(MatrixStack modelToCameraStack, float fHeight = 5.0f) {
        const float g_fColumnBaseHeight = 0.25f;

        //Draw the bottom of the column.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Scale(glm::vec3(1.0f, g_fColumnBaseHeight, 1.0f));
            modelToCameraStack.Translate(glm::vec3(0.0f, 0.5f, 0.0f));

            glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
            glBindVertexArray(unitCylinderVAO2);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriFan1);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitCylinderVertexIndicesTriFan1.size()),
                           GL_UNSIGNED_INT, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriFan2);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitCylinderVertexIndicesTriFan2.size()),
                           GL_UNSIGNED_INT, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriStrip);
            glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLsizei>(unitCylinderVertexIndicesTriStrip.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        // Draw the top of the column.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Translate(glm::vec3(0.0f, fHeight - g_fColumnBaseHeight, 0.0f));
            modelToCameraStack.Scale(glm::vec3(1.0f, g_fColumnBaseHeight, 1.0f));
            modelToCameraStack.Translate(glm::vec3(0.0f, 0.5f, 0.0f));

            glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
            glBindVertexArray(unitCylinderVAO2);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriFan1);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitCylinderVertexIndicesTriFan1.size()),
                           GL_UNSIGNED_INT, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriFan2);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitCylinderVertexIndicesTriFan2.size()),
                           GL_UNSIGNED_INT, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriStrip);
            glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLsizei>(unitCylinderVertexIndicesTriStrip.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        //Draw the main column.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Translate(glm::vec3(0.0f, g_fColumnBaseHeight, 0.0f));
            modelToCameraStack.Scale(glm::vec3(0.8f, fHeight - (g_fColumnBaseHeight * 2.0f), 0.8f));
            modelToCameraStack.Translate(glm::vec3(0.0f, 0.5f, 0.0f));

            glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
            glBindVertexArray(unitCylinderVAO2);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriFan1);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitCylinderVertexIndicesTriFan1.size()),
                           GL_UNSIGNED_INT, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriFan2);
            glDrawElements(GL_TRIANGLE_FAN, static_cast<GLsizei>(unitCylinderVertexIndicesTriFan2.size()),
                           GL_UNSIGNED_INT, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCylinderEBOTriStrip);
            glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLsizei>(unitCylinderVertexIndicesTriStrip.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }
    }

    void drawLookAtPoint() {
        MatrixStack modelToCameraStack;

        modelToCameraStack.Translate(g_cameraTarget);
        modelToCameraStack.Scale(glm::vec3(1.0f, 0.1f, 1.0f));

        glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelToCameraStack.Top()));
        glBindVertexArray(unitCubeVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndicesTri.size()),
                       GL_UNSIGNED_INT, nullptr);
    }

    // GLFW key callback function
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            // Check if the Shift key is pressed
            bool shift_pressed = (mods & GLFW_MOD_SHIFT) != 0;

            switch (key) {
                case GLFW_KEY_W: {
                    wKeyPressed = true;
                    g_angles.fAngleX += SMALL_ANGLE_INCREMENT;
                    break;
                }
                case GLFW_KEY_S: {
                    sKeyPressed = true;
                    g_angles.fAngleX -= SMALL_ANGLE_INCREMENT;
                    break;
                }
                case GLFW_KEY_A: {
                    aKeyPressed = true;
                    g_angles.fAngleY += SMALL_ANGLE_INCREMENT;
                    break;
                }
                case GLFW_KEY_D: {
                    dKeyPressed = true;
                    g_angles.fAngleY -= SMALL_ANGLE_INCREMENT;
                    break;
                }
                case GLFW_KEY_Q: {
                    qKeyPressed = true;
                    g_angles.fAngleZ += SMALL_ANGLE_INCREMENT;
                    break;
                }
                case GLFW_KEY_E: {
                    eKeyPressed = true;
                    g_angles.fAngleZ -= SMALL_ANGLE_INCREMENT;
                    break;
                }
                case GLFW_KEY_SPACE: {
                    g_boolDrawGimbals = !g_boolDrawGimbals;
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

bool Renderer::g_boolDrawLookatPoint = false;
glm::vec3 Renderer::g_cameraTarget{0.0f, 0.4f, 0.0f};
glm::vec3 Renderer::g_sphereCameraRelativePosition{67.5f, -46.0f, 150.0f};

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


    // Create starting plane, cube, cone and cylinder
    renderer.createUnitPlane();
    renderer.createUnitCube();
    renderer.createUnitCone();
    renderer.createUnitCylinder();

    // Set initial positions
    renderer.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // Set the user-defined pointer to the Renderer instance
    glfwSetWindowUserPointer(window, &renderer);

    // Set the key callback
    glfwSetKeyCallback(window, Renderer::keyCallback);

    // Set the resize callback
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

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