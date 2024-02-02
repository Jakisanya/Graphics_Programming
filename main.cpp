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

class Renderer {
public:
    std::vector<GLuint> VBOs;
    std::vector<GLuint> VAOs;
    std::vector<GLuint> EBOs;
    std::vector<GLuint> vertex_indices;
    std::vector<glm::mat4> modelMatrices;
    void createPrism(GLuint &VAO, GLuint &VBO, GLuint &EBO) {
        // Vertex data for the prism 1
        std::vector<GLfloat> prismVertexData = {
                // Prism 1
                // Face (front)
                -0.25f, -0.25f, 0.75f, 1.0f, 0.0f, 0.0f,  // Bottom-left-red
                0.25f, -0.25f, 0.75f, 0.0f, 1.0f, 0.0f,   // Bottom-right-green
                0.25f, 0.25f, 0.75f, 0.0f, 0.0f, 1.0f,    // Top-right-blue
                -0.25f, 0.25f, 0.75f, 1.0f, 1.0f, 0.0f,   // Top-left-yellow

                // Face (back)
                -0.25f, -0.25f, -0.75f, 1.0f, 0.0f, 0.0f,  // Bottom-left-red
                0.25f, -0.25f, -0.75f, 0.0f, 1.0f, 0.0f,   // Bottom-right-green
                0.25f, 0.25f, -0.75f, 0.0f, 0.0f, 1.0f,    // Top-right-blue
                -0.25f, 0.25f, -0.75f, 1.0f, 1.0f, 0.0f    // Top-left-yellow
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

    Renderer() : shaderProgram(0), VAOs(3), VBOs(3), EBOs(3), vertex_indices(0),
                 modelMatrices(3), vertexShaderSource(nullptr), fragmentShaderSource(nullptr) {

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

    void drawPrism(GLuint VAO, GLuint EBO, glm::mat4 view, glm::mat4 model, glm::mat4 projection, const std::vector<GLuint>& ebo_indices) const{
        glUseProgram(shaderProgram);

        GLint elapsedTimeLocation = glGetUniformLocation(shaderProgram, "fElapsedTime");
        glUniform1d(elapsedTimeLocation, glfwGetTime());

        GLint viewMatrixLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(view));

        GLint modelMatrixLocation = glGetUniformLocation(shaderProgram, "modelMatrix");
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(model));

        GLint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, static_cast<GLint>(ebo_indices.size()),
                       GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(0);
        glUseProgram(0);
    }

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

    void perform_render_sequence() {
        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Define camera parameters
        glm::vec3 cameraPosition = glm::vec3(-4.0f, 7.0f, 10.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

        // Set up view matrix
        glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);

        // Set up projection matrix
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f),
                                                      static_cast<GLfloat>(WINDOW_WIDTH) /
                                                      static_cast<GLfloat>(WINDOW_HEIGHT),
                                                      0.1f, 100.0f);

        // move modelMatrices[1] and modelMatrices[2]
        std::vector<float> Offsets = ComputePositionOffsets(static_cast<float>(glfwGetTime()));
        modelMatrices[1] = glm::translate(modelMatrices[0], glm::vec3(Offsets[0], 0.0f, Offsets[2]));
        modelMatrices[2] = glm::translate(modelMatrices[1], glm::vec3(Offsets[0], 0.0f, Offsets[2]));

        /*
        // Set up model matrix and update for rotation
        float rotationSpeed = 0.1f; // Adjust the speed as needed
        float angle = rotationSpeed * static_cast<float>(glfwGetTime()); // Using GLFW's time function
        modelMatrices[0] = glm::rotate(modelMatrices[0], angle, glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrices[1] = glm::rotate(modelMatrices[1], angle, glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrices[2] = glm::rotate(modelMatrices[2], angle, glm::vec3(0.0f, 1.0f, 0.0f));
        */

        drawPrism(VAOs[0], EBOs[0], viewMatrix, modelMatrices[0], projectionMatrix, vertex_indices);
        drawPrism(VAOs[1], EBOs[1], viewMatrix, modelMatrices[1], projectionMatrix, vertex_indices);
        drawPrism(VAOs[2], EBOs[2], viewMatrix, modelMatrices[2], projectionMatrix, vertex_indices);
    }

    ~Renderer() {
        // Cleanup
        glDeleteVertexArrays(1, &VAOs[0]);
        glDeleteVertexArrays(1, &VAOs[1]);
        glDeleteVertexArrays(1, &VAOs[2]);
        glDeleteBuffers(1, &VBOs[0]);
        glDeleteBuffers(1, &VBOs[1]);
        glDeleteBuffers(1, &VBOs[2]);
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
    renderer.createPrism(renderer.VAOs[0], renderer.VBOs[0], renderer.EBOs[0]);  // First pyramid
    renderer.createPrism(renderer.VAOs[1], renderer.VBOs[1], renderer.EBOs[1]);  // Second pyramid
    renderer.createPrism(renderer.VAOs[2], renderer.VBOs[2], renderer.EBOs[2]);  // Third pyramid

    // Set initial positions
    renderer.modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.modelMatrices[1] = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
    renderer.modelMatrices[2] = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f));

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
            renderer.perform_render_sequence();
            renderer.resizeFlag = false; // Reset the flag
        }

        renderer.perform_render_sequence();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();

    return 0;
}