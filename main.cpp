#include "libraries/glew-2.1.0/include/GL/glew.h"
#include "libraries/glfw-master/include/GLFW/glfw3.h"
#include "libraries/glm-master/glm/glm.hpp"
#include "libraries/glm-master/glm/ext.hpp"
#include "libraries/rapidxml-master/rapidxml.hpp"
#include "libraries/rapidxml-master/rapidxml_utils.hpp"
#include <cmath>
#include <iostream>
#include <vector>
#include <sstream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
// Define a global variable to track whether the A or D key is pressed
bool aKeyPressed = false;
bool dKeyPressed = false;


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
};

void initializeGLEW() {
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(EXIT_FAILURE);
    }
};

// GLFW key callback function
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) {
            aKeyPressed = true;
        } else if (action == GLFW_RELEASE) {
            aKeyPressed = false;
        }
    }

    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            dKeyPressed = true;
        } else if (action == GLFW_RELEASE) {
            dKeyPressed = false;
        }
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

private:
    std::vector<glm::mat4> stack;
};

class Renderer {
public:
    GLuint shaderProgram;
    GLuint unitPlaneVBO, unitCubeVBO, unitCylinderVBO, unitConeVBO;
    GLuint unitPlaneVAO, unitCubeVAO, unitCylinderVAO, unitConeVAO;
    GLuint unitPlaneEBO, unitCubeEBO, unitCylinderEBO, unitConeEBO;
    std::vector<GLuint> unitPlaneVertexIndices, unitCubeVertexIndices, unitCylinderVertexIndices, unitConeVertexIndices;

    glm::mat4 modelMatrix;

    // Build the objects in the world

    //      -- Create a VBO, VAO and EBO for the data
    //      -- Put this functionality in a createPlane() function
    // (2) Load the trees/forest; parse xml file; make sure scale is what I want
    //      -- Create a VBO, VAO and EBO for the data
    //      -- Put this functionality in a createForest() function
    // (3) Load the columns for the Parthenon xml data; make sure the scale is what I want
    //      -- Create a VBO, VAO and EBO for the column data
    //      -- Put the functionality in a createColumn() function; could start from a cube so include createCube()
    //      -- Create a Matrix Stack for the Parthenon; I'm thinking everything starts from the unit cube
    //      -- The base and ceiling are made using the Matrix Stack

    void createUnitPlane() {
        // Vertex data for the plan
        std::vector<GLfloat> unitPlaneVertexData = {
                0.5f, 0.0f, -0.5f,
                0.5f, 0.0f, 0.5f,
                -0.5f, 0.0f, 0.5f,
                -0.5f, 0.0f, -0.5f
        };

        unitPlaneVertexIndices = {
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(unitPlaneVertexIndices.size() * sizeof(GLint)),
        unitPlaneVertexIndices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    };

    // Unit cube
    void createUnitCube() {
        // Vertex data for the unit cube
        std::vector<GLfloat> unitCubeVertexData = {
                // Face (front)
                -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,  // Bottom-left-red
                0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,   // Bottom-right-green
                0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,    // Top-right-blue
                -0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,   // Top-left-yellow

                // Face (back)
                -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  // Bottom-left-red
                0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,   // Bottom-right-green
                0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f,    // Top-right-blue
                -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f    // Top-left-yellow
        };

        unitCubeVertexIndices = {
                // prism1 indices
                0, 1, 2, // front
                2, 3, 0, // front
                4, 5, 6, // back
                6, 7, 4, // back
                4, 0, 2, // left
                2, 7, 4, // left
                1, 5, 6, // right
                6, 2, 1, // right
                3, 2, 6, // top
                6, 7, 3, // top
                0, 1, 5, // bottom
                5, 4, 0,  // bottom
        };

        glGenVertexArrays(1, &unitCubeVAO);
        glGenBuffers(1, &unitCubeVBO);
        glGenBuffers(1, &unitCubeEBO);

        glBindVertexArray(unitCubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, unitCubeVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // Add data to VBO and EBO
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(unitCubeVertexData.size() * sizeof(GLfloat)),
                     unitCubeVertexData.data(), GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(unitCubeVertexIndices.size() * sizeof(GLint)),
                     unitCubeVertexIndices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void createUnitCylinder() {

        std::vector<GLfloat> unitCylinderVertexData{
                0.0f, 0.5f, 0.0f,
                0.5f, 0.5f, 0.0f,
                0.5f, -0.5f, 0.0f,
                0.48907381875731f, 0.5f, 0.1039557588888f,
                0.48907381875731f, -0.5f, 0.1039557588888f,
                0.45677280077542f, 0.5f, 0.20336815992623f,
                0.45677280077542f, -0.5f, 0.20336815992623f,
                0.40450865316151f, 0.5f, 0.29389241146627f,
                0.40450865316151f, -0.5f, 0.29389241146627f,
                0.33456556611288f, 0.5f, 0.37157217599218f,
                0.33456556611288f, -0.5f, 0.37157217599218f,
                0.2500003830126f, 0.5f, 0.43301248075957f,
                0.2500003830126f, -0.5f, 0.43301248075957f,
                0.15450900193016f, 0.5f, 0.47552809414644f,
                0.15450900193016f, -0.5f, 0.47552809414644f,
                0.052264847412855f, 0.5f, 0.49726088296277f,
                0.052264847412855f, -0.5f, 0.49726088296277f,
                -0.052263527886268f, 0.5f, 0.49726102165048f,
                -0.052263527886268f, -0.5f, 0.49726102165048f,
                -0.15450774007312f, 0.5f, 0.47552850414828f,
                -0.15450774007312f, -0.5f, 0.47552850414828f,
                -0.24999923397422f, 0.5f, 0.43301314415651f,
                -0.24999923397422f, -0.5f, 0.43301314415651f,
                -0.33456458011157f, 0.5f, 0.37157306379065f,
                -0.33456458011157f, -0.5f, 0.37157306379065f,
                -0.40450787329018f, 0.5f, 0.29389348486527f,
                -0.40450787329018f, -0.5f, 0.29389348486527f,
                -0.45677226111814f, 0.5f, 0.20336937201315f,
                -0.45677226111814f, -0.5f, 0.20336937201315f,
                -0.48907354289964f, 0.5f, 0.10395705668972f,
                -0.48907354289964f, -0.5f, 0.10395705668972f,
                -0.49999999999824f, 0.5f, 1.3267948966764e-006f,
                -0.49999999999824f, -0.5f, 1.3267948966764e-006f,
                -0.48907409461153f, 0.5f, -0.10395446108714f,
                -0.48907409461153f, -0.5f, -0.10395446108714f,
                -0.45677334042948f, 0.5f, -0.20336694783787f,
                -0.45677334042948f, -0.5f, -0.20336694783787f,
                -0.40450943302999f, 0.5f, -0.2938913380652f,
                -0.40450943302999f, -0.5f, -0.2938913380652f,
                -0.33456655211184f, 0.5f, -0.3715712881911f,
                -0.33456655211184f, -0.5f, -0.3715712881911f,
                -0.25000153204922f, 0.5f, -0.43301181735958f,
                -0.25000153204922f, -0.5f, -0.43301181735958f,
                -0.15451026378611f, 0.5f, -0.47552768414126f,
                -0.15451026378611f, -0.5f, -0.47552768414126f,
                -0.052266166939075f, 0.5f, -0.49726074427155f,
                -0.052266166939075f, -0.5f, -0.49726074427155f,
                0.052262208359312f, 0.5f, -0.4972611603347f,
                0.052262208359312f, -0.5f, -0.4972611603347f,
                0.15450647821499f, 0.5f, -0.47552891414676f,
                0.15450647821499f, -0.5f, -0.47552891414676f,
                0.24999808493408f, 0.5f, -0.4330138075504f,
                0.24999808493408f, -0.5f, -0.4330138075504f,
                0.3345635941079f, 0.5f, -0.37157395158649f,
                0.3345635941079f, -0.5f, -0.37157395158649f,
                0.40450709341601f, 0.5f, -0.2938945582622f,
                0.40450709341601f, -0.5f, -0.2938945582622f,
                0.45677172145764f, 0.5f, -0.20337058409865f,
                0.45677172145764f, -0.5f, -0.20337058409865f,
                0.48907326703854f, 0.5f, -0.10395835448992f,
                0.48907326703854f, -0.5f, -0.10395835448992f,
                0.0f, -0.5f, 0.0f
        };

        std::vector<GLfloat> unitCylinderVertexIndices{
                1, 2, 3,
                4, 5, 6,
                7, 8, 9,
                10, 11, 12,
                13, 14, 15,
                16, 17, 18,
                19, 20, 21,
                22, 23, 24,
                25, 26, 27,
                28, 29, 30,
                31, 32, 33,
                34, 35, 36,
                37, 38, 39,
                40, 41, 42,
                43, 44, 45,
                46, 47, 48,
                49, 50, 51,
                52, 53, 54,
                55, 56, 57,
                58, 59, 60,
                1, 2
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
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(unitVertexData.size() * sizeof(GLfloat)),
                     unitPlaneVertexData.data(), GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(unitPlaneVertexIndices.size() * sizeof(GLint)),
                     unitPlaneVertexIndices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);


    }

    void createUnitCone() {

    }

    bool resizeFlag = false;
    void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        // Update viewport dimensions on window resize
        glUseProgram(shaderProgram);
        int windowWidthLocation = glGetUniformLocation(shaderProgram, "windowWidth");
        int windowHeightLocation = glGetUniformLocation(shaderProgram, "windowHeight");
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
            uniform mat4 viewMatrix;
            uniform mat4 projectionMatrix;

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

    Renderer() : shaderProgram(0), unitCubeVAO(0), unitCubeVBO(0), unitCubeEBO(0), unitCubeVertexIndices(0),
                 unitPlaneVAO(0), unitPlaneVBO(0), unitPlaneEBO(0), unitPlaneVertexIndices(0),
                 unitCylinderVAO(0), unitCylinderVBO(0), unitCylinderEBO(0), unitCylinderVertexIndices(0),
                 unitConeVAO(0), unitConeVBO(0), unitConeEBO(0), unitConeVertexIndices(0),
                 modelMatrix(0), vertexShaderSource(nullptr), fragmentShaderSource(nullptr) {

        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
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
        glGetProgramiv (shaderProgram, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint infoLogLength;
            glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infoLogLength);

            auto* strInfoLog = new GLchar[infoLogLength + 1];
            glGetProgramInfoLog(shaderProgram, infoLogLength, nullptr, strInfoLog);
            std::cerr << "Linker failure: \n" << strInfoLog;
            delete[] strInfoLog;
        }
    }

    void create_and_link_program() {
        shaderProgram = glCreateProgram();

        for (GLuint shader : shaders) {
            glAttachShader(shaderProgram, shader);
        }

        glLinkProgram(shaderProgram);
        error_check_program();

        for (GLuint shader : shaders) {
            glDetachShader(shaderProgram, shader);
            glDeleteShader(shader);
        }
    }
    GLfloat baseSpinAngle{0};
    void drawGrabber(glm::mat4 view, glm::mat4 model, glm::mat4 projection, GLFWwindow* window) const {
        MatrixStack modelToCameraStack;

        glUseProgram(shaderProgram);
        glBindVertexArray(unitCubeVAO);

        glm::vec3 posBase = {0.0f, 0.0f, 0.0f};
        GLfloat angBase = -60.0f;
        modelToCameraStack.Translate(posBase);
        modelToCameraStack.RotateY(angBase);

        glm::vec3 posBaseLeft = {-1.0f, 0.0f, 0.0f};
        glm::vec3 posBaseRight = {1.0f, 0.0f, 0.0f};
        glm::vec3 scaleBaseZ = {1.0f, 1.0f, 3.0f};

        GLint viewMatrixLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(view));
        GLint modelMatrixLocation = glGetUniformLocation(shaderProgram, "modelMatrix");
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(model));
        GLint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(projection));

        //Draw left base.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Translate(posBaseLeft);
            modelToCameraStack.RotateY(baseSpinAngle);
            modelToCameraStack.Scale(scaleBaseZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndices.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        //Draw right base.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Translate(posBaseRight);
            modelToCameraStack.RotateY(baseSpinAngle);
            modelToCameraStack.Scale(scaleBaseZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndices.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        //Draw main arm.
        DrawUpperArm(modelToCameraStack, modelMatrixLocation, window);

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void DrawUpperArm(MatrixStack modelToCameraStack, GLint modelMatrixLocation, GLFWwindow* window) const {
        // Function to handle keyboard input
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }


        // Arm Raise	W	S
        // Elbow Raise	R	F
        // Wrist Raise	T	G
        // Wrist Spin	Z	C
        // Finger Open/Close	Q	E

        // Draw arm spine
        {
            modelToCameraStack.Push();
            glm::vec3 scaleBaseY = {1.0f, 4.5f, 1.0f};
            glm::vec3 translateYZ = {0.0f, 1.75f, 1.0f};
            modelToCameraStack.Translate(translateYZ);
            modelToCameraStack.RotateX(35.0f);
            modelToCameraStack.RotateY(baseSpinAngle);
            modelToCameraStack.Scale(scaleBaseY);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndices.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        // Draw arm neck
        {
            modelToCameraStack.Push();
            glm::vec3 translateYZ = {0.25f, 3.4f, 3.3f};
            glm::vec3 scaleBaseYZ = {0.75f, 0.65, 2.0f};
            modelToCameraStack.Translate(translateYZ);
            modelToCameraStack.RotateX(15.0f);
            modelToCameraStack.RotateY(baseSpinAngle);
            modelToCameraStack.Scale(scaleBaseYZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndices.size()),
                           GL_UNSIGNED_INT, nullptr);
        }

        // Draw arm head
        {
            modelToCameraStack.Push();
            glm::vec3 translateYZ = {0.0f, 0.0f, 0.75f};
            glm::vec3 scaleYZ = {1.15f, 1.25f, 0.4f};
            modelToCameraStack.Translate(translateYZ);
            modelToCameraStack.Scale(scaleYZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndices.size()),
                           GL_UNSIGNED_INT, nullptr);
        }

        // Draw arm grabber base left
        {
            modelToCameraStack.Push();
            glm::vec3 translateYZ = {0.5f, -1.0f, 0.0f};
            glm::vec3 scaleYZ = {0.27f, 1.5f, 0.27f};
            modelToCameraStack.Translate(translateYZ);
            modelToCameraStack.RotateZ(20.0f);
            modelToCameraStack.Scale(scaleYZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndices.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        // Draw arm grabber base right
        {
            modelToCameraStack.Push();
            glm::vec3 translateYZ = {-0.7f, -1.0f, 0.0f};
            glm::vec3 scaleYZ = {0.27f, 1.5f, 0.27f};
            modelToCameraStack.Translate(translateYZ);
            modelToCameraStack.RotateZ(-20.0f);
            modelToCameraStack.Scale(scaleYZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndices.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        // Draw arm grabber top left
        {
            modelToCameraStack.Push();
            glm::vec3 translateYZ = {-0.7f, -2.15f, 0.0f};
            glm::vec3 scaleYZ = {0.27f, 1.2f, 0.27f};
            modelToCameraStack.Translate(translateYZ);
            modelToCameraStack.RotateZ(20.0f);
            modelToCameraStack.Scale(scaleYZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndices.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        // Draw arm grabber top right
        {
            modelToCameraStack.Push();
            glm::vec3 translateYZ = {0.6f, -2.15f, 0.0f};
            glm::vec3 scaleYZ = {0.27f, 1.2f, 0.27f};
            modelToCameraStack.Translate(translateYZ);
            modelToCameraStack.RotateZ(-20.0f);
            modelToCameraStack.Scale(scaleYZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitCubeEBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(unitCubeVertexIndices.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }
    }

    /*
    static std::vector<GLfloat> ComputePositionOffsets(float elapsedTime) {
        std::vector<GLfloat> Offsets;
        float fLoopDuration = 5.0f;
        float fScale = (2.0f * (3.14159f * 3.0f)) / fLoopDuration;
        float fCurrTimeThroughLoop = glm::mod(elapsedTime, fLoopDuration);
        float fXOffset = glm::cos(fCurrTimeThroughLoop * fScale) * 2.5f;
        float fYOffset = glm::sin(fCurrTimeThroughLoop * fScale) * 2.5f;
        float fZOffset = glm::sin(fCurrTimeThroughLoop * fScale) * 2.5f;
        Offsets.push_back(fXOffset);
        Offsets.push_back(fYOffset);
        Offsets.push_back(fZOffset);
        return Offsets;
    }
    */

    void perform_render_sequence(GLFWwindow* window) const {
        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Define camera parameters
        glm::vec3 cameraPosition = glm::vec3(-4.0f, 2.0f, 20.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

        // Set up view matrix
        glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);

        // Set up projection matrix
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f),
                                                      static_cast<GLfloat>(WINDOW_WIDTH) /
                                                      static_cast<GLfloat>(WINDOW_HEIGHT),
                                                      0.1f, 100.0f);

        drawGrabber(viewMatrix, modelMatrix, projectionMatrix, window);
    }

    ~Renderer() {
        // Cleanup
        glDeleteVertexArrays(1, &unitCubeVAO);
        glDeleteBuffers(1, &unitCubeVBO);
        glDeleteProgram(shaderProgram);
    }

private:
    std::vector<GLuint> shaders;
    const char* vertexShaderSource;
    const char* fragmentShaderSource;
};

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

    // Set uniform variable values in Shader
    GLint windowWidthLocation = glGetUniformLocation(renderer.shaderProgram, "windowWidth");
    GLint windowHeightLocation = glGetUniformLocation(renderer.shaderProgram, "windowHeight");
    glUseProgram(renderer.shaderProgram);
    glUniform1i(windowWidthLocation, WINDOW_WIDTH);
    glUniform1i(windowHeightLocation, WINDOW_HEIGHT);

    // Create starting plane, cube, and ... vertex data
    renderer.createUnitPlane();
    renderer.createUnitCube();

    // Set initial positions
    renderer.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // Set the user-defined pointer to the Renderer instance
    glfwSetWindowUserPointer(window, &renderer);

    // Set the key callback
    glfwSetKeyCallback(window, keyCallback);

    // Set the resize callback
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Process input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Base Spin	A	D
        // Function to handle keyboard input
        if (aKeyPressed) {
            renderer.baseSpinAngle += 5.0f;
            aKeyPressed = false;
            if (renderer.baseSpinAngle > 20.0f) {
                renderer.baseSpinAngle = 20.0f;
                std::cout << "MAXIMUM ANTI-CLOCKWISE BASE SPIN REACHED!" << "\n";
                aKeyPressed = false;
            }
        }
        else if (dKeyPressed) {
            renderer.baseSpinAngle -= 5.0f;
            dKeyPressed = false;
            if (renderer.baseSpinAngle < -20.0f) {
                renderer.baseSpinAngle = -20.0f;
                std::cout << "MAXIMUM CLOCKWISE BASE SPIN REACHED!" << "\n";
                dKeyPressed = false;
            }
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