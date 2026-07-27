#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec3 vertexColor;
out vec3 position;
out vec2 texCoord;

uniform vec3 uPosOffset;

void main()
{
    vec3 finalPosition = aPos + uPosOffset;
    position = finalPosition;
    gl_Position = vec4(finalPosition, 1.0);
    vertexColor = aColor;
    texCoord = aTexCoord;
}
