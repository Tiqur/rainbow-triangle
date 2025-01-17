#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>

using std::cout, std::endl;
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  (void)window;
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

std::vector<float> vertices = {
    -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, //
    0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, //
    0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f  //
};

class Shader {
public:
  Shader(std::string shaderSource, GLenum shaderType) {
    type = shaderType;
    m_id = glCreateShader(type);
    const char *shaderSourcePtr = shaderSource.c_str();
    glShaderSource(m_id, 1, &shaderSourcePtr, NULL);
    glCompileShader(m_id);
  }
  ~Shader() { glDeleteShader(m_id); }
  GLuint id() { return m_id; }

private:
  GLuint m_id{};
  GLenum type{};
};

class ShaderProgram {
public:
  ShaderProgram(GLuint vertexShaderID, GLuint fragmentShaderID) {
    m_id = glCreateProgram();
    glAttachShader(m_id, vertexShaderID);
    glAttachShader(m_id, fragmentShaderID);
    glLinkProgram(m_id);
  }
  void use() { glUseProgram(m_id); }
  ~ShaderProgram() { glDeleteProgram(m_id); }
  GLuint id() { return m_id; }

private:
  GLuint m_id{};
};

class VAO {
public:
  VAO() {
    glGenVertexArrays(1, &m_id);
    if (m_id == 0) {
      cout << "Failed to generate Vertex Array Object" << endl;
      return;
    }
  }
  ~VAO() { glDeleteVertexArrays(1, &m_id); }
  void setAttribPointer(GLuint index, GLuint size, GLenum type,
                        GLboolean normalized, GLsizei stride,
                        const void *pointer) {
    bind();
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
  }
  void bind() { glBindVertexArray(m_id); }
  void unbind() { glBindVertexArray(0); }
  GLuint id() { return m_id; }

private:
  GLuint m_id{};
};

class VBO {
public:
  VBO(const std::vector<float> &vertices) {
    glGenBuffers(1, &m_id);
    if (m_id == 0) {
      cout << "Failed to generate Vertex Buffer Object" << endl;
      return;
    }
    bind();
    glBufferData(GL_ARRAY_BUFFER, std::size(vertices) * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);
  }
  ~VBO() { glDeleteBuffers(1, &m_id); }
  void bind() { glBindBuffer(GL_ARRAY_BUFFER, m_id); }
  void unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }
  GLuint id() { return m_id; }

private:
  GLuint m_id{};
};

int main() {
  // Initialize ImGui
  std::cout << "Initializing ImGui..." << std::endl;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  std::cout << "Initializing ImGui GLFW backend..." << std::endl;
  if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    std::cerr << "Failed to initialize ImGui GLFW backend!" << std::endl;
    return -1;
  }

  std::cout << "Initializing ImGui OpenGL backend..." << std::endl;
  if (!ImGui_ImplOpenGL3_Init("#version 330")) {
    std::cerr << "Failed to initialize ImGui OpenGL backend!" << std::endl;
    return -1;
  }

  // Make the OpenGL context current
  glfwMakeContextCurrent(window);

  // Initialize GLEW
  glewExperimental = GL_TRUE; // Ensure GLEW uses modern OpenGL techniques
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  // Set the viewport
  glViewport(0, 0, 800, 600);

  // Register the framebuffer size callback
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  std::string vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 fColor;
    void main() {
      gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
      fColor = aColor;
    }
  )";
  std::string fragmentShaderSource = R"(
    #version 330 core
    in vec3 fColor;
    out vec4 FragColor;
    uniform float aColorDelta;

    void main() {
      float r = fColor[0] * cos(aColorDelta) - fColor[1] * sin(aColorDelta);
      float g = fColor[1] * sin(aColorDelta) - fColor[2] * cos(aColorDelta);
      float b = fColor[2];

      r = clamp(r, 0.0, 1.0);
      g = clamp(g, 0.0, 1.0);

      FragColor = vec4(r, g, b, 1.0f);
    }
  )";

  VBO vbo(vertices);
  VAO vao;
  Shader fragmentShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
  Shader vertexShader(vertexShaderSource, GL_VERTEX_SHADER);
  ShaderProgram program(fragmentShader.id(), vertexShader.id());
  vao.setAttribPointer(0, 3, GL_FLOAT, false, 6 * sizeof(float), (void *)0);
  vao.setAttribPointer(1, 3, GL_FLOAT, false, 6 * sizeof(float),
                       (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  program.use();

  GLint uniformLocation = glGetUniformLocation(program.id(), "aColorDelta");

  float colorDelta = 1.0f;
  static float colorDeltaSpeed = 0.01;

  // Main render loop
  while (!glfwWindowShouldClose(window)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Settings");
    ImGui::SliderFloat("Color Delta Speed", &colorDeltaSpeed, 0.0f, 0.3f,
                       "%.2f");
    ImGui::End();

    glUniform1f(uniformLocation, colorDelta);
    colorDelta += colorDeltaSpeed;

    // Render OpenGL
    glClearColor(0.2f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    program.use();
    glBindVertexArray(vao.id());
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Process user input
    processInput(window);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Clean up and exit
  glfwDestroyWindow(window);
  glfwTerminate();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  return 0;
}
