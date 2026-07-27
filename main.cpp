#include <windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdint.h>
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

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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
		//position         //color           //texture coords
        0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,// top right
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,// bottom right
        0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,// bottom left
        0.0f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,// top left 
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
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);        
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

	int32 posOffsetLocation = glGetUniformLocation(shader1Program, "uPosOffset");

    int width, height, nrChannels;
    unsigned char* data = stbi_load("assets/fire.jpg", &width, &height, &nrChannels, 0);    
    Assert(data);

	uint32 texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

	stbi_image_free(data);

    while (!glfwWindowShouldClose(window))
    {
		processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);       		
                
        glBindVertexArray(triangle1VAO);
        glUseProgram(shader1Program);

        float timeValue = glfwGetTime();
        float greenValue = (-cos(timeValue) / 2.0f) + 0.5f;
		int32 cycles = timeValue / (3.14159f);        

        triangle1Vertices[3 + cycles % 3] = cycles % 2 == 0 ? 1 - greenValue : greenValue;
        triangle1Vertices[3 + (1 + cycles) % 3] = cycles % 2 == 0 ? greenValue : 1 - greenValue;
        
        triangle1Vertices[9 + (1 + cycles) % 3] = cycles % 2 == 0 ? 1 - greenValue : greenValue;
        triangle1Vertices[9 + (2 + cycles) % 3] = cycles % 2 == 0 ? greenValue : 1 - greenValue;
        
        triangle1Vertices[15 + (2 + cycles) % 3] = cycles % 2 == 0 ? 1 - greenValue : greenValue;
        triangle1Vertices[15 + (3 + cycles) % 3] = cycles % 2 == 0 ? greenValue : 1 - greenValue;

        glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle1Vertices), triangle1Vertices, GL_STREAM_DRAW);
        //glUniform3f(posOffsetLocation, greenValue, 0.0f, 0.0f);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUseProgram(shader2Program);
        glBindVertexArray(triangle2VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUseProgram(shader1Program);

		//glBindVertexArray(quadVAO);
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
