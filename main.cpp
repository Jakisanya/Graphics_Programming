#include "libraries/glew-2.1.0/include/GL/glew.h"
#include "libraries/glfw-master/include/GLFW/glfw3.h"
#include "libraries/glm-master/glm/glm.hpp"
#include "libraries/glm-master/glm/ext.hpp"
#include <cmath>
#include <iostream>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

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
    GLuint VBO;
    GLuint VAO;
    GLuint EBO;
    std::vector<GLuint> vertex_indices;
    glm::mat4 modelMatrix;
    void createCube(GLuint &VAO_, GLuint &VBO_, GLuint &EBO_) {
        // Vertex data for the prism 1
        std::vector<GLfloat> prismVertexData = {
                // Prism 1
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

        vertex_indices = {
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

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // Add data to VBO and EBO
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(prismVertexData.size() * sizeof(GLfloat)),
                     prismVertexData.data(), GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertex_indices.size() * sizeof(GLint)),
                     vertex_indices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    GLuint shaderProgram;
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

    Renderer() : shaderProgram(0), VAO(0), VBO(0), EBO(0), vertex_indices(0),
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

    void drawGrabber(glm::mat4 view, glm::mat4 model, glm::mat4 projection, GLFWwindow* window) const {
        MatrixStack modelToCameraStack;

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

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

        // Base Spin	A	D
        static float baseSpinAngle{0.0f};
        // Function to handle keyboard input
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            if (baseSpinAngle < 45.0f) {
                baseSpinAngle += 15.0f;
            }
            if (baseSpinAngle == 45.0f) {
                std::cout << "MAXIMUM ANTI-CLOCKWISE BASE SPIN REACHED!" << "\n";
            }
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            if (baseSpinAngle > -45.0f) {
                baseSpinAngle -= 15.0f;
            }
            if (baseSpinAngle == -45.0f) {
                std::cout << "MAXIMUM CLOCKWISE BASE SPIN REACHED!" << "\n";
            }
        }

        //Draw left base.
        {
            modelToCameraStack.Push();
            modelToCameraStack.Translate(posBaseLeft);
            modelToCameraStack.RotateY(baseSpinAngle);
            modelToCameraStack.Scale(scaleBaseZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vertex_indices.size()),
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
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vertex_indices.size()),
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
            glm::vec3 translateYZ = {0.0f, 2.0f, 1.0f};
            modelToCameraStack.Translate(translateYZ);
            modelToCameraStack.RotateX(35.0f);
            modelToCameraStack.Scale(scaleBaseY);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vertex_indices.size()),
                           GL_UNSIGNED_INT, nullptr);
            modelToCameraStack.Pop();
        }

        // Draw arm neck
        {
            modelToCameraStack.Push();
            glm::vec3 translateYZ = {0.0f, 3.65f, 3.3f};
            glm::vec3 scaleBaseYZ = {0.75f, 0.65, 2.0f};
            modelToCameraStack.Translate(translateYZ);
            modelToCameraStack.RotateX(15.0f);
            modelToCameraStack.Scale(scaleBaseYZ);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
                               glm::value_ptr(modelToCameraStack.Top()));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vertex_indices.size()),
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
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vertex_indices.size()),
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
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vertex_indices.size()),
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
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vertex_indices.size()),
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
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vertex_indices.size()),
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
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(vertex_indices.size()),
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
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
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

    // Create prism vertex data
    renderer.createCube(renderer.VAO, renderer.VBO, renderer.EBO);  // First pyramid

    // Set initial positions
    renderer.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // Set the user-defined pointer to the Renderer instance
    glfwSetWindowUserPointer(window, &renderer);

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