#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec3 vertexColor;
out vec3 position;
out vec2 texCoord;

uniform mat4 uTransform;

void main()
{
    vertexColor = aColor;
    texCoord = aTexCoord;

    vec3 finalPosition = aPos;
    position = finalPosition;
    gl_Position = uTransform * vec4(finalPosition, 1.0);
}
