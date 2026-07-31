#include <windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdint.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define Assert(expression)                                                                                             \
    if (!(expression))                                                                                                 \
    {                                                                                                                  \
        uint8 *assert_fail = 0;                                                                                        \
        *assert_fail = 0;                                                                                              \
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

struct debug_read_file_result
{
    void* contents;
    uint32 contentsSize;
};

static float mixAlpha = 0.5f;

inline uint32
SafeTruncateUInt64(uint64 value)
{
    Assert(value < 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return (result);
}

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(const char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory);

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        mixAlpha += 0.01f;
        if (mixAlpha > 1.0f)
            mixAlpha = 1.0f;
	}

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        mixAlpha -= 0.01f;
        if (mixAlpha < 0.0f)
            mixAlpha = 0.0f;
	}
}

static bool32 CreateVertexProgram(const char *vertFileName, _Out_ uint32 *o_vertShader)
{
    debug_read_file_result vertShaderFile = DEBUGPlatformReadEntireFile(vertFileName);
    uint32 vertShader;
    *o_vertShader = 0;
    if (vertShaderFile.contentsSize > 0)
    {
        const GLchar* vertShaderText = (const GLchar*)vertShaderFile.contents;
        vertShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertShader, 1, &vertShaderText, NULL);
        glCompileShader(vertShader);
        int32 success;
        char infoLog[512];
        glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "ERROR::SHADER::VERTEX\n" << "File is empty!" << std::endl;
        return false;
    }
    *o_vertShader = vertShader;
    return true;
}

static bool32 CreateFragmentProgram(const char *fragFileName, _Out_ uint32 *o_fragShader)
{
    debug_read_file_result fragShaderFile = DEBUGPlatformReadEntireFile(fragFileName);
    uint32 fragShader;
    *o_fragShader = 0;
    if (fragShaderFile.contentsSize > 0)
    {
        const GLchar* fragShaderText = (const GLchar*)fragShaderFile.contents;
        fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragShader, 1, &fragShaderText, NULL);
        glCompileShader(fragShader);
        int32 success;
        char infoLog[512];
        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "ERROR::SHADER::FRAGMENT\n" << "File is empty!" << std::endl;
        return false;
    }
    *o_fragShader = fragShader;
    return true;
}

static bool32 CreateShaderProgram(uint32 vertShader, uint32 fragShader, _Out_ uint32* o_shaderProgram)
{
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
            std::cout << "ERROR::SHADER::PROGRAM::LINKER_FAILED\n" << infoLog << std::endl;
            return false;
        }
    }

    *o_shaderProgram = shaderProgram;

    return true;
}

static bool32 CreateShaderProgram(const char *vertFileName, const char *fragFileName, _Out_ uint32 *o_shaderProgram)
{       
    uint32 vertShader;
	uint32 fragShader;
    
    *o_shaderProgram = 0;

    if(!CreateVertexProgram(vertFileName, &vertShader))
        return false;

    if(!CreateFragmentProgram(fragFileName, &fragShader))
		return false;

    if (!CreateShaderProgram(vertShader, fragShader, o_shaderProgram))
        return false;
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return true;
}

//int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);    

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }       

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    uint32 vertProgram = 0;
    bool32 vertProgramSuccess = CreateVertexProgram("assets/shaders/vert.glsl", &vertProgram);

	uint32 frag1Program = 0;
	bool32 frag1ProgramSuccess = CreateFragmentProgram("assets/shaders/frag1.glsl", &frag1Program);

    uint32 frag2Program = 0;
    bool32 frag2ProgramSuccess = CreateFragmentProgram("assets/shaders/frag2.glsl", &frag2Program);

    uint32 shader1Program;
    bool32 shader1ProgramSuccess = CreateShaderProgram(vertProgram, frag1Program, &shader1Program);
    if (!shader1ProgramSuccess)
        return -1;    

    uint32 shader2Program;
    bool32 shader2ProgramSuccess = CreateShaderProgram(vertProgram, frag2Program, &shader2Program);
    if (!shader2ProgramSuccess)
        return -1;

    glDeleteShader(vertProgram);
    glDeleteShader(frag1Program);
    glDeleteShader(frag2Program);

	// Setup vertex data and buffers and configure vertex attributes
    float triangle1Vertices[] = {
		-0.75f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.35f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.55f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float triangle2Vertices[] = {
         0.75f, -0.5f, 0.0f,
         0.35f, -0.5f, 0.0f,
         0.55f,  0.5f, 0.0f
    };

    float quadVertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    uint32 quadIndices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };    

    uint32 VBOs[4] = {};
	glGenBuffers(4, VBOs);

	//Setup triangle1 VAO and VBO
    uint32 triangle1VAO;
    glGenVertexArrays(1, &triangle1VAO);    
    glBindVertexArray(triangle1VAO);    
    
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle1Vertices), triangle1Vertices, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //Setup triangle2 VAO and VBO
    uint32 triangle2VAO;
    glGenVertexArrays(1, &triangle2VAO);
    glBindVertexArray(triangle2VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle2Vertices), triangle2Vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

	//Setup quad VAO, VBO, and EBO
	uint32 quadVAO;
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);    

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOs[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);            
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);	

    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;

    unsigned char* texture1Data = stbi_load("assets/fire.jpg", &width, &height, &nrChannels, 0);    
    Assert(texture1Data);

	uint32 texture1;
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture1Data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(texture1Data);

    unsigned char* texture2Data = stbi_load("assets/letter_b.jpg", &width, &height, &nrChannels, 0);
    Assert(texture2Data);
    uint32 texture2;
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture2Data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(texture2Data);

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

	glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
		processInput(window);
        glUniform1f(mixAlphaLoc, mixAlpha);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       		
                
        glBindVertexArray(triangle1VAO);
        glUseProgram(shader1Program);

        float timeValue = glfwGetTime();
        float greenValue = (-cos(timeValue) / 2.0f) + 0.5f;
		int32 cycles = timeValue / pi;        
        
        //triangle1Vertices[3 + cycles % 3] = cycles % 2 == 0 ? 1 - greenValue : greenValue;
        //triangle1Vertices[3 + (1 + cycles) % 3] = cycles % 2 == 0 ? greenValue : 1 - greenValue;
        //
        //triangle1Vertices[9 + (1 + cycles) % 3] = cycles % 2 == 0 ? 1 - greenValue : greenValue;
        //triangle1Vertices[9 + (2 + cycles) % 3] = cycles % 2 == 0 ? greenValue : 1 - greenValue;
        //
        //triangle1Vertices[15 + (2 + cycles) % 3] = cycles % 2 == 0 ? 1 - greenValue : greenValue;
        //triangle1Vertices[15 + (3 + cycles) % 3] = cycles % 2 == 0 ? greenValue : 1 - greenValue;
        //
        //glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(triangle1Vertices), triangle1Vertices, GL_STREAM_DRAW);
        //glUniform3f(posOffsetLocation, greenValue, 0.0f, 0.0f);
        
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //glUseProgram(shader2Program);
        //glBindVertexArray(triangle2VAO);
        //glDrawArrays(GL_TRIANGLES, 0, 6);        

		glBindVertexArray(quadVAO);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));        
        model = glm::rotate(model, timeValue, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 view = glm::mat4(1.0f);        
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));        
        //view = glm::rotate(view, timeValue, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(proj));

		glDrawArrays(GL_TRIANGLES, 0, 36);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		//float scaleValue = sin(timeValue);
        //
        //trans = glm::mat4(1.0f);        
        //trans = glm::translate(trans, glm::vec3(-0.5f, 0.5f, 0.0f));       
        //trans = glm::scale(trans, glm::vec3(scaleValue, scaleValue, scaleValue));
        //glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

	return 0;
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result result = {};
    HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return result;

    LARGE_INTEGER fileSize64;
    if (!GetFileSizeEx(fileHandle, &fileSize64))
        return result;

    uint32 fileSize32 = SafeTruncateUInt64(fileSize64.QuadPart);
    result.contents = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!result.contents)
        return result;

    DWORD bytesRead;
    if (!ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) || bytesRead != fileSize32)
    {
        DEBUGPlatformFreeFileMemory(result.contents);
        return result;
    }

    result.contentsSize = fileSize32;

    CloseHandle(fileHandle);

    return (result);
}
