#include <iostream>
#include <stdint.h>
#include <windows.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define Assert(expression)                                                     \
  if (!(expression)) {                                                         \
    uint8 *assert_fail = 0;                                                    \
    *assert_fail = 0;                                                          \
  }

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct debug_read_file_result {
  void *contents;
  uint32 contentsSize;
};

struct input_state {
  double mousePosX;
  double mousePosY;  
  float normalizedMousePosX;
  float normalizedMousePosY;
  bool32 wKeyDown;
  bool32 sKeyDown;
  bool32 aKeyDown;
  bool32 dKeyDown;
  bool32 upKeyDown;
  bool32 downKeyDown;
};

struct input_delta {
  double mouseDeltaX;
  double mouseDeltaY;
  int32 wKey;
  int32 sKey;
  int32 aKey;
  int32 dKey;
  int32 upKey;
  int32 downKey;
};

static float mixAlpha = 0.5f;

static input_state inputState = {};
static input_state prevInputState = {};
static input_delta inputDelta = {};

static int screenWidth = 1920;
static int screenHeight = 1080;

//static char debugConsoleBuffer[];

inline uint32 SafeTruncateUInt64(uint64 value) {
  Assert(value < 0xFFFFFFFF);
  uint32 result = (uint32)value;
  return (result);
}

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name)                                  \
  debug_read_file_result name(const char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory);

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
    screenWidth = width;
    screenHeight = height;

    glViewport(0, 0, width, height);
}

static void copyInputState(input_state *source, input_state *dest)
{
    dest->mousePosX = source->mousePosX;
    dest->mousePosY = source->mousePosY;
    dest->wKeyDown = source->wKeyDown;
    dest->aKeyDown = source->aKeyDown;
    dest->sKeyDown = source->sKeyDown;
    dest->dKeyDown = source->dKeyDown;
    dest->upKeyDown = source->upKeyDown;
    dest->downKeyDown = source->downKeyDown;    
}

static input_delta getInputStateDelta(input_state* current, input_state* prev)
{
    input_delta result = {};

    result.mouseDeltaX = current->mousePosX - prev->mousePosX;
    result.mouseDeltaY = current->mousePosY - prev->mousePosY;
    result.wKey = current->wKeyDown - prev->wKeyDown;
    result.aKey = current->aKeyDown - prev->aKeyDown;
    result.sKey = current->sKeyDown - prev->sKeyDown;
    result.dKey = current->dKeyDown - prev->dKeyDown;
    result.upKey = current->upKeyDown - prev->upKeyDown;
    result.downKey = current->downKeyDown - prev->downKeyDown;

    return (result);
}

static void processInput(GLFWwindow *window, input_state *inputState, input_state *prev) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    mixAlpha += 0.01f;
    if (mixAlpha > 1.0f)
      mixAlpha = 1.0f;
  }

  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    mixAlpha -= 0.01f;
    if (mixAlpha < 0.0f)
      mixAlpha = 0.0f;
  }
  
  inputState->wKeyDown = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
  inputState->sKeyDown = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
  inputState->aKeyDown = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
  inputState->dKeyDown = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
  inputState->upKeyDown = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
  inputState->downKeyDown = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
  glfwGetCursorPos(window, &inputState->mousePosX, &inputState->mousePosY);
  
  inputState->normalizedMousePosX = (float)(inputState->mousePosX / screenWidth);
  inputState->normalizedMousePosY = (float)(inputState->mousePosY / screenHeight);

  inputDelta = getInputStateDelta(inputState, prev);
  copyInputState(inputState, prev);
}

static bool32 CreateVertexProgram(const char *vertFileName,
                                  _Out_ uint32 *o_vertShader) {
  debug_read_file_result vertShaderFile =
      DEBUGPlatformReadEntireFile(vertFileName);
  uint32 vertShader;
  *o_vertShader = 0;
  if (vertShaderFile.contentsSize > 0) {
    const GLchar *vertShaderText = (const GLchar *)vertShaderFile.contents;
    vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertShaderText, NULL);
    glCompileShader(vertShader);
    int32 success;
    char infoLog[512];
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                << infoLog << std::endl;
      return false;
    }
  } else {
    std::cout << "ERROR::SHADER::VERTEX\n" << "File is empty!" << std::endl;
    return false;
  }
  *o_vertShader = vertShader;
  return true;
}

static bool32 CreateFragmentProgram(const char *fragFileName,
                                    _Out_ uint32 *o_fragShader) {
  debug_read_file_result fragShaderFile =
      DEBUGPlatformReadEntireFile(fragFileName);
  uint32 fragShader;
  *o_fragShader = 0;
  if (fragShaderFile.contentsSize > 0) {
    const GLchar *fragShaderText = (const GLchar *)fragShaderFile.contents;
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragShaderText, NULL);
    glCompileShader(fragShader);
    int32 success;
    char infoLog[512];
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                << infoLog << std::endl;
      return false;
    }
  } else {
    std::cout << "ERROR::SHADER::FRAGMENT\n" << "File is empty!" << std::endl;
    return false;
  }
  *o_fragShader = fragShader;
  return true;
}

static bool32 CreateShaderProgram(uint32 vertShader, uint32 fragShader,
                                  _Out_ uint32 *o_shaderProgram) {
  *o_shaderProgram = 0;

  uint32 shaderProgram;
  shaderProgram = glCreateProgram();

  glAttachShader(shaderProgram, vertShader);
  glAttachShader(shaderProgram, fragShader);
  glLinkProgram(shaderProgram);

  {
    int32 success;
    char infoLog[512];

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::PROGRAM::LINKER_FAILED\n"
                << infoLog << std::endl;
      return false;
    }
  }

  *o_shaderProgram = shaderProgram;

  return true;
}

static bool32 CreateShaderProgram(const char *vertFileName,
                                  const char *fragFileName,
                                  _Out_ uint32 *o_shaderProgram) {
  uint32 vertShader;
  uint32 fragShader;

  *o_shaderProgram = 0;

  if (!CreateVertexProgram(vertFileName, &vertShader))
    return false;

  if (!CreateFragmentProgram(fragFileName, &fragShader))
    return false;

  if (!CreateShaderProgram(vertShader, fragShader, o_shaderProgram))
    return false;

  glDeleteShader(vertShader);
  glDeleteShader(fragShader);

  return true;
}

class Camera {
  glm::vec3 position;
  glm::vec3 direction;
  glm::vec3 up;
  float fov;
  float aspectRatio;
  float nearPlane;
  float farPlane;
  float yaw;
  float pitch;
  float speed;

public:
  Camera(glm::vec3 position, glm::vec3 direction, glm::vec3 up, float fov,
         float aspectRatio, float nearPlane, float farPlane, float speed)

      : position(position), direction(direction), up(up), fov(fov),
        aspectRatio(aspectRatio), nearPlane(nearPlane), farPlane(farPlane),
        yaw(glm::degrees(atan2(direction.z, direction.x))),
        pitch(glm::degrees(asin(direction.y))), speed(speed) {    
  }

  float GetSpeed() const { return speed; }

  float GetYaw() const { return yaw; }

  float GetPitch() const { return pitch; }

  glm::vec3 GetPosition() const { return position; }

  glm::vec3 GetDirection() const { return direction; }

  glm::mat4 GetViewMatrix() const {
    return glm::lookAt(position, position + direction, up);
  }

  glm::mat4 GetProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane,
                            farPlane);
  }

  void SetSpeed(float newSpeed) { speed = newSpeed; }

  void SetPosition(const glm::vec3 &newPosition) { position = newPosition; }

  void SetDirection(const glm::vec3 &newDirection) {
    direction = newDirection;
    yaw = glm::degrees(atan2(direction.z, direction.x));
    pitch = glm::degrees(asin(direction.y));
  }

  void SetYawAndPitch(float newYaw, float newPitch) {
    yaw = newYaw;
    pitch = newPitch;

    UpdateDirectionFromAngles();
  }

  void AddYawAndPitch(float addYaw, float addPitch)
  {
      yaw += addYaw;
      pitch += addPitch;

      UpdateDirectionFromAngles();
  }

private:
    void UpdateDirectionFromAngles()
    {
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction = glm::normalize(direction);
    }
};

static void updateCameraPosition(Camera *camera, const input_state *inputState,
                          float deltaTime) {
  const float cameraSpeed = camera->GetSpeed() * deltaTime;
  glm::vec3 position = camera->GetPosition();
  glm::vec3 direction = camera->GetDirection();
  glm::vec3 right =
      glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
  if (inputState->wKeyDown)
    position += cameraSpeed * direction;
  if (inputState->sKeyDown)
    position -= cameraSpeed * direction;
  if (inputState->aKeyDown)
    position -= right * cameraSpeed;
  if (inputState->dKeyDown)
    position += right * cameraSpeed;
  camera->SetPosition(position);
}

static void printDebugInfo(float deltaTime)
{
    std::cout << "\x1b[?25l"; //hide cursor
    std::cout << "\x1b[4A";   //move up lines

    std::cout << "\r\033[K";  //clear line
    std::cout << "Frame time: " << deltaTime * 1000.0 << '\n';
    std::cout << "\r\033[K";
    std::cout << "Mouse pos: " << inputState.mousePosX << ' ' << inputState.mousePosY << '\n';
    std::cout << "\r\033[K";
    std::cout << "Mouse pos normalized: " << inputState.normalizedMousePosX << ' ' << inputState.normalizedMousePosY << '\n';
    std::cout << "\r\033[K";
    std::cout << "Mouse delta: " << inputDelta.mouseDeltaX << ' ' << inputDelta.mouseDeltaY << '\n';
    std::cout << std::flush;
}

// int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE
// hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "LearnOpenGL", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  uint32 vertProgram = 0;
  bool32 vertProgramSuccess =
      CreateVertexProgram("assets/shaders/vert.glsl", &vertProgram);

  uint32 frag1Program = 0;
  bool32 frag1ProgramSuccess =
      CreateFragmentProgram("assets/shaders/frag1.glsl", &frag1Program);

  uint32 frag2Program = 0;
  bool32 frag2ProgramSuccess =
      CreateFragmentProgram("assets/shaders/frag2.glsl", &frag2Program);

  uint32 shader1Program;
  bool32 shader1ProgramSuccess =
      CreateShaderProgram(vertProgram, frag1Program, &shader1Program);
  if (!shader1ProgramSuccess)
    return -1;

  uint32 shader2Program;
  bool32 shader2ProgramSuccess =
      CreateShaderProgram(vertProgram, frag2Program, &shader2Program);
  if (!shader2ProgramSuccess)
    return -1;

  glDeleteShader(vertProgram);
  glDeleteShader(frag1Program);
  glDeleteShader(frag2Program);

  // Setup vertex data and buffers and configure vertex attributes
  float triangle1Vertices[] = {
      -0.75f, -0.5f, 0.0f, 1.0f,   0.0f, 0.0f, -0.35f, -0.5f, 0.0f,
      0.0f,   1.0f,  0.0f, -0.55f, 0.5f, 0.0f, 0.0f,   0.0f,  1.0f,
  };

  float triangle2Vertices[] = {0.75f, -0.5f, 0.0f, 0.35f, -0.5f,
                               0.0f,  0.55f, 0.5f, 0.0f};

  float quadVertices[] = {
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
      0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

      -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f};

  uint32 quadIndices[] = {
      // note that we start from 0!
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
  };

  glm::vec3 cubePositions[] = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
      glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

  uint32 VBOs[4] = {};
  glGenBuffers(4, VBOs);

  // Setup triangle1 VAO and VBO
  uint32 triangle1VAO;
  glGenVertexArrays(1, &triangle1VAO);
  glBindVertexArray(triangle1VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle1Vertices), triangle1Vertices,
               GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Setup triangle2 VAO and VBO
  uint32 triangle2VAO;
  glGenVertexArrays(1, &triangle2VAO);
  glBindVertexArray(triangle2VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle2Vertices), triangle2Vertices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Setup quad VAO, VBO, and EBO
  uint32 quadVAO;
  glGenVertexArrays(1, &quadVAO);
  glBindVertexArray(quadVAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBOs[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOs[3]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(2);

  stbi_set_flip_vertically_on_load(true);

  int width, height, nrChannels;

  unsigned char *texture1Data =
      stbi_load("assets/fire.jpg", &width, &height, &nrChannels, 0);
  Assert(texture1Data);

  uint32 texture1;
  glGenTextures(1, &texture1);
  glBindTexture(GL_TEXTURE_2D, texture1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, texture1Data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(texture1Data);

  unsigned char *texture2Data =
      stbi_load("assets/letter_b.jpg", &width, &height, &nrChannels, 0);
  Assert(texture2Data);
  uint32 texture2;
  glGenTextures(1, &texture2);
  glBindTexture(GL_TEXTURE_2D, texture2);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, texture2Data);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(texture2Data);

  glEnable(GL_DEPTH_TEST);

  glUseProgram(shader1Program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, texture2);

  int32 baseMapLoc = glGetUniformLocation(shader1Program, "uBaseMap");
  int32 auxMapLoc = glGetUniformLocation(shader1Program, "uAuxMap");
  glUniform1i(baseMapLoc, 0);
  glUniform1i(auxMapLoc, 1);

  int32 mixAlphaLoc = glGetUniformLocation(shader1Program, "uMixAlpha");

  int32 modelLoc = glGetUniformLocation(shader1Program, "uModel");
  int32 viewLoc = glGetUniformLocation(shader1Program, "uView");
  int32 projectionLoc = glGetUniformLocation(shader1Program, "uProjection");

  constexpr float pi = glm::pi<float>();

  Camera cam = Camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                      glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 1920.0f / 1080.0f,
                      0.1f, 100.0f, 2.5f);

  float deltaTime = 0.0f;
  double lastFrame = 0.0f;

  glm::mat4 modelRot = glm::mat4(1.0f);

  glfwSwapInterval(0);
  
  float sensitivity = .1f;
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
  processInput(window, &inputState, &prevInputState);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    float timeValue = glfwGetTime();

    double currentFrame = timeValue;
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;    

    processInput(window, &inputState, &prevInputState);

    printDebugInfo(deltaTime);
    
    float greenValue = (-cos(timeValue) / 2.0f) + 0.5f;
    int32 cycles = timeValue / pi;

    updateCameraPosition(&cam, &inputState, deltaTime);
    cam.AddYawAndPitch(sensitivity * inputDelta.mouseDeltaX, -sensitivity * inputDelta.mouseDeltaY);

    glm::mat4 view = cam.GetViewMatrix();
    glm::mat4 proj = cam.GetProjectionMatrix();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform1f(mixAlphaLoc, mixAlpha);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(proj));

    glBindVertexArray(quadVAO);

    for (int i = 0; i < 10; i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      model = glm::rotate(model, glm::radians(20.0f * i),
                          glm::vec3(0.0f, 1.0f, 0.0f));
      if (i % 3 == 0) {
        modelRot = glm::rotate(modelRot, glm::radians(10.0f * deltaTime),
                               glm::vec3(0.0f, 1.0f, 0.0f));
        model = model * modelRot;
      }

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

      glDrawArrays(GL_TRIANGLES, 0, 36);
    }    

    glfwSwapBuffers(window);
  }

  return 0;
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory) {
  VirtualFree(memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile) {
  debug_read_file_result result = {};
  HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                  OPEN_EXISTING, 0, 0);
  if (fileHandle == INVALID_HANDLE_VALUE)
    return result;

  LARGE_INTEGER fileSize64;
  if (!GetFileSizeEx(fileHandle, &fileSize64))
    return result;

  uint32 fileSize32 = SafeTruncateUInt64(fileSize64.QuadPart);
  result.contents =
      VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!result.contents)
    return result;

  DWORD bytesRead;
  if (!ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) ||
      bytesRead != fileSize32) {
    DEBUGPlatformFreeFileMemory(result.contents);
    return result;
  }

  result.contentsSize = fileSize32;

  CloseHandle(fileHandle);

  return (result);
}
