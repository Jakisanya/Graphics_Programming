#include "libraries/glew-2.1.0/include/GL/glew.h"
#include "libraries/glfw-master/include/GLFW/glfw3.h"
#include "libraries/glm-master/glm/glm.hpp"
#include "libraries/glm-master/glm/ext.hpp"
#include "xmlparser.cpp"
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
    GLint gimbalColourLocation;
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

struct GimbalAngles {
    GimbalAngles()
            : fAngleX(0.0f)
            , fAngleY(0.0f)
            , fAngleZ(0.0f)
    {}

    float fAngleX;
    float fAngleY;
    float fAngleZ;
};

GimbalAngles g_angles;

#define STANDARD_ANGLE_INCREMENT 11.25f
#define SMALL_ANGLE_INCREMENT 9.0f

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
    std::vector<GLuint> smallGimbalEBOs{5}, mediumGimbalEBOs{5}, largeGimbalEBOs{5};
    std::vector<std::vector<GLuint>> unitPlaneVertexIndicesTriStrip, unitPlaneVertexIndicesTri, unitPlaneVertexIndicesTriFan,
    smallGimbalVertexIndicesTriStrip, smallGimbalVertexIndicesTri, smallGimbalVertexIndicesTriFan,
    mediumGimbalVertexIndicesTriStrip, mediumGimbalVertexIndicesTri, mediumGimbalVertexIndicesTriFan,
    largeGimbalVertexIndicesTriStrip, largeGimbalVertexIndicesTri, largeGimbalVertexIndicesTriFan;
    glm::mat4 modelMatrix{};

    static bool g_boolDrawGimbals;
    static glm::vec3 g_cameraTarget;
    static glm::vec3 g_sphereCameraRelativePosition;

    Renderer() {
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
    }

    void createUnitPlane() {
        std::vector<GLfloat> unitPlaneVertexData = parseMeshXMLVertexData(
                R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\UnitPlane.xml)");

        std::tuple<std::vector<std::vector<unsigned int>>,
        std::vector<std::vector<unsigned int>>,
        std::vector<std::vector<unsigned int>>>
        unitPlaneIndexDataTuple = parseMeshXMLIndexData(
                R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\UnitPlane.xml)");

        std::tie(unitPlaneVertexIndicesTriStrip, unitPlaneVertexIndicesTri, unitPlaneVertexIndicesTriFan) = unitPlaneIndexDataTuple;

        glGenVertexArrays(1, &unitPlaneVAO);
        glGenBuffers(1, &unitPlaneVBO);
        glGenBuffers(1, &unitPlaneEBO);

        glBindVertexArray(unitPlaneVAO);
        glBindBuffer(GL_ARRAY_BUFFER, unitPlaneVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitPlaneEBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)nullptr);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // Add data to VBO and EBO
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(unitPlaneVertexData.size() * sizeof(GLfloat)),
        unitPlaneVertexData.data(), GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(unitPlaneVertexIndicesTri[0].size() * sizeof(GLint)),
        unitPlaneVertexIndicesTri[0].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    };

    void createSmallGimbal() {
        std::vector<GLfloat> smallGimbalVertexData = parseMeshXMLVertexData(
                R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\SmallGimbal.xml)");

        std::tuple<std::vector<std::vector<unsigned int>>,
                std::vector<std::vector<unsigned int>>,
                std::vector<std::vector<unsigned int>>>
                smallGimbalIndexDataTuple = parseMeshXMLIndexData(
                R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\SmallGimbal.xml)");


        std::tie(smallGimbalVertexIndicesTriStrip, smallGimbalVertexIndicesTri, smallGimbalVertexIndicesTriFan) = smallGimbalIndexDataTuple;

        glGenVertexArrays(1, &smallGimbalVAO);
        glGenBuffers(1, &smallGimbalVBO);
        glGenBuffers(1, &smallGimbalEBOs[0]);
        glGenBuffers(1, &smallGimbalEBOs[1]);
        glGenBuffers(1, &smallGimbalEBOs[2]);
        glGenBuffers(1, &smallGimbalEBOs[3]);
        glGenBuffers(1, &smallGimbalEBOs[4]);

        glBindVertexArray(smallGimbalVAO);
        glBindBuffer(GL_ARRAY_BUFFER, smallGimbalVBO);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(smallGimbalVertexData.size() * sizeof(GLfloat)),
                     smallGimbalVertexData.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)nullptr);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[0]);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(smallGimbalVertexIndicesTriStrip[0].size() * sizeof(GLint)),
                     smallGimbalVertexIndicesTriStrip[0].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(smallGimbalVertexIndicesTriStrip[1].size() * sizeof(GLint)),
                     smallGimbalVertexIndicesTriStrip[1].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[2]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(smallGimbalVertexIndicesTriStrip[2].size() * sizeof(GLint)),
                     smallGimbalVertexIndicesTriStrip[2].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[3]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(smallGimbalVertexIndicesTriStrip[3].size() * sizeof(GLint)),
                     smallGimbalVertexIndicesTriStrip[3].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[4]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(smallGimbalVertexIndicesTri[0].size() * sizeof(GLint)),
                     smallGimbalVertexIndicesTri[0].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void createMediumGimbal() {
        std::vector<GLfloat> mediumGimbalVertexData = parseMeshXMLVertexData(
                R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\MediumGimbal.xml)");

        std::tuple<std::vector<std::vector<unsigned int>>,
                std::vector<std::vector<unsigned int>>,
                std::vector<std::vector<unsigned int>>>
                mediumGimbalIndexDataTuple = parseMeshXMLIndexData(
                R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\MediumGimbal.xml)");

        std::tie(mediumGimbalVertexIndicesTriStrip, mediumGimbalVertexIndicesTri, mediumGimbalVertexIndicesTriFan) = mediumGimbalIndexDataTuple;

        glGenVertexArrays(1, &mediumGimbalVAO);
        glGenBuffers(1, &mediumGimbalVBO);
        glGenBuffers(1, &mediumGimbalEBOs[0]);
        glGenBuffers(1, &mediumGimbalEBOs[1]);
        glGenBuffers(1, &mediumGimbalEBOs[2]);
        glGenBuffers(1, &mediumGimbalEBOs[3]);
        glGenBuffers(1, &mediumGimbalEBOs[4]);

        glBindVertexArray(mediumGimbalVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mediumGimbalVBO);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mediumGimbalVertexData.size() * sizeof(GLfloat)),
                     mediumGimbalVertexData.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)nullptr);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[0]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mediumGimbalVertexIndicesTriStrip[0].size() * sizeof(GLint)),
                     mediumGimbalVertexIndicesTriStrip[0].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mediumGimbalVertexIndicesTriStrip[1].size() * sizeof(GLint)),
                     mediumGimbalVertexIndicesTriStrip[1].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[2]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mediumGimbalVertexIndicesTriStrip[2].size() * sizeof(GLint)),
                     mediumGimbalVertexIndicesTriStrip[2].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[3]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mediumGimbalVertexIndicesTriStrip[3].size() * sizeof(GLint)),
                     mediumGimbalVertexIndicesTriStrip[3].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[4]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mediumGimbalVertexIndicesTri[0].size() * sizeof(GLint)),
                     mediumGimbalVertexIndicesTri[0].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void createLargeGimbal() {
        std::vector<GLfloat> largeGimbalVertexData = parseMeshXMLVertexData(
                R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\LargeGimbal.xml)");

        std::tuple<std::vector<std::vector<unsigned int>>,
                std::vector<std::vector<unsigned int>>,
                std::vector<std::vector<unsigned int>>>
                largeGimbalIndexDataTuple = parseMeshXMLIndexData(
                R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\LargeGimbal.xml)");

        std::tie(largeGimbalVertexIndicesTriStrip, largeGimbalVertexIndicesTri, largeGimbalVertexIndicesTriFan) = largeGimbalIndexDataTuple;

        glGenVertexArrays(1, &largeGimbalVAO);
        glGenBuffers(1, &largeGimbalVBO);
        glGenBuffers(1, &largeGimbalEBOs[0]);
        glGenBuffers(1, &largeGimbalEBOs[1]);
        glGenBuffers(1, &largeGimbalEBOs[2]);
        glGenBuffers(1, &largeGimbalEBOs[3]);
        glGenBuffers(1, &largeGimbalEBOs[4]);

        glBindVertexArray(largeGimbalVAO);
        glBindBuffer(GL_ARRAY_BUFFER, largeGimbalVBO);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(largeGimbalVertexData.size() * sizeof(GLfloat)),
                     largeGimbalVertexData.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)nullptr);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[0]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(largeGimbalVertexIndicesTriStrip[0].size() * sizeof(GLint)),
                     largeGimbalVertexIndicesTriStrip[0].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(largeGimbalVertexIndicesTriStrip[1].size() * sizeof(GLint)),
                     largeGimbalVertexIndicesTriStrip[1].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[2]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(largeGimbalVertexIndicesTriStrip[2].size() * sizeof(GLint)),
                     largeGimbalVertexIndicesTriStrip[2].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[3]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(largeGimbalVertexIndicesTriStrip[3].size() * sizeof(GLint)),
                     largeGimbalVertexIndicesTriStrip[3].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[4]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(largeGimbalVertexIndicesTri[0].size() * sizeof(GLint)),
                     largeGimbalVertexIndicesTri[0].data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
            out vec4 vertex_colour;
            uniform float fElapsedTime;
            uniform mat4 modelMatrix;
            uniform vec4 gimbalColour;
            layout(std140) uniform GlobalMatrices {
                mat4 viewMatrix;
                mat4 projectionMatrix;
            };

            void main() {
                gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0f);
                vertex_colour = gimbalColour;
            }
        )";
        fragmentShaderSource = R"(
            #version 330 core
            in vec4 vertex_colour;
            out vec4 FragColour;
            uniform float fElapsedTime;

            void main() {
                FragColour = vertex_colour;
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

    enum GimbalAxis
    {
        GIMBAL_X_AXIS,
        GIMBAL_Y_AXIS,
        GIMBAL_Z_AXIS,
    };

    void perform_render_sequence(GLFWwindow* window) {
        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Define camera parameters
        glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 300.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f),
                                                      static_cast<GLfloat>(WINDOW_WIDTH) /
                                                      static_cast<GLfloat>(WINDOW_HEIGHT),
                                                      0.1f, 600.0f);

        data.modelMatrixLocation = glGetUniformLocation(data.shaderProgram, "modelMatrix");
        glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        data.gimbalColourLocation = glGetUniformLocation(data.shaderProgram, "gimbalColour");

        glBindBuffer(GL_UNIFORM_BUFFER, g_GlobalMatricesUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(viewMatrix));
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projectionMatrix));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        MatrixStack modelMatrixStack;
        modelMatrixStack.Translate(glm::vec3(0.0f, 0.0f, -200.0f));
        modelMatrixStack.RotateX(g_angles.fAngleX);
        DrawGimbal(modelMatrixStack, GIMBAL_X_AXIS, glm::vec4(0.4f, 0.4f, 1.0f, 1.0f));
        modelMatrixStack.RotateX(g_angles.fAngleY);
        DrawGimbal(modelMatrixStack, GIMBAL_Y_AXIS, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        modelMatrixStack.RotateX(g_angles.fAngleZ);
        DrawGimbal(modelMatrixStack, GIMBAL_Z_AXIS, glm::vec4(1.0f, 0.3f, 0.3f, 1.0f));

        /*
        // Draw ship
        modelMatrixStack.Scale(glm::vec3(3.0, 3.0, 3.0));
        modelMatrixStack.RotateX(-90);
         */
    }

    void DrawGimbal(MatrixStack modelMatrixStack, GimbalAxis eAxis, glm::vec4 gimbalColour) {
        if (!g_boolDrawGimbals)
            return;

        glUseProgram(data.shaderProgram);

        switch (eAxis) {
            case GIMBAL_X_AXIS:
                glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrixStack.Top()));
                glUniformMatrix4fv(data.gimbalColourLocation, 1, GL_FALSE, glm::value_ptr(gimbalColour));
                glBindVertexArray(smallGimbalVAO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[0]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(smallGimbalVertexIndicesTriStrip[0].size()),
                               GL_UNSIGNED_INT,
                               smallGimbalVertexIndicesTriStrip[0].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[1]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(smallGimbalVertexIndicesTriStrip[1].size()),
                               GL_UNSIGNED_INT,
                               smallGimbalVertexIndicesTriStrip[1].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[2]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(smallGimbalVertexIndicesTriStrip[2].size()),
                               GL_UNSIGNED_INT,
                               smallGimbalVertexIndicesTriStrip[2].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[3]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(smallGimbalVertexIndicesTriStrip[3].size()),
                               GL_UNSIGNED_INT,
                               smallGimbalVertexIndicesTriStrip[3].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smallGimbalEBOs[4]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(smallGimbalVertexIndicesTri[0].size()),
                               GL_UNSIGNED_INT,
                               smallGimbalVertexIndicesTri[0].data());
                break;
            case GIMBAL_Y_AXIS:
                modelMatrixStack.RotateZ(90.0f);
                modelMatrixStack.RotateX(90.0f);
                glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrixStack.Top()));
                glUniformMatrix4fv(data.gimbalColourLocation, 1, GL_FALSE, glm::value_ptr(gimbalColour));
                glBindVertexArray(mediumGimbalVAO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[0]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(mediumGimbalVertexIndicesTriStrip[0].size()),
                               GL_UNSIGNED_INT,
                               mediumGimbalVertexIndicesTriStrip[0].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[1]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(mediumGimbalVertexIndicesTriStrip[1].size()),
                               GL_UNSIGNED_INT,
                               mediumGimbalVertexIndicesTriStrip[1].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[2]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(mediumGimbalVertexIndicesTriStrip[2].size()),
                               GL_UNSIGNED_INT,
                               mediumGimbalVertexIndicesTriStrip[2].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[3]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(mediumGimbalVertexIndicesTriStrip[3].size()),
                               GL_UNSIGNED_INT,
                               mediumGimbalVertexIndicesTriStrip[3].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mediumGimbalEBOs[4]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(mediumGimbalVertexIndicesTri[0].size()),
                               GL_UNSIGNED_INT,
                               mediumGimbalVertexIndicesTri[0].data());
                break;
            case GIMBAL_Z_AXIS:
                modelMatrixStack.RotateY(90.0f);
                modelMatrixStack.RotateX(90.0f);
                glUniformMatrix4fv(data.modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrixStack.Top()));
                glUniformMatrix4fv(data.gimbalColourLocation, 1, GL_FALSE, glm::value_ptr(gimbalColour));
                glBindVertexArray(largeGimbalVAO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[0]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(largeGimbalVertexIndicesTriStrip[0].size()),
                               GL_UNSIGNED_INT,
                               largeGimbalVertexIndicesTriStrip[0].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[1]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(largeGimbalVertexIndicesTriStrip[1].size()),
                               GL_UNSIGNED_INT,
                               largeGimbalVertexIndicesTriStrip[1].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[2]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(largeGimbalVertexIndicesTriStrip[2].size()),
                               GL_UNSIGNED_INT,
                               largeGimbalVertexIndicesTriStrip[2].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[3]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(largeGimbalVertexIndicesTriStrip[3].size()),
                               GL_UNSIGNED_INT,
                               largeGimbalVertexIndicesTriStrip[3].data());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, largeGimbalEBOs[4]);
                glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLint>(largeGimbalVertexIndicesTri[0].size()),
                               GL_UNSIGNED_INT,
                               largeGimbalVertexIndicesTri[0].data());
                break;
        }

        glUseProgram(0);
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

bool Renderer::g_boolDrawGimbals = false;

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
    std::cout << "Creating Unit Plane..." << "\n";
    renderer.createUnitPlane();
    std::cout << "Creating Small Gimbal..." << "\n";
    renderer.createSmallGimbal();
    std::cout << "Creating Medium Gimbal..." << "\n";
    renderer.createMediumGimbal();
    std::cout << "Creating Large Gimbal..." << "\n";
    renderer.createLargeGimbal();

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