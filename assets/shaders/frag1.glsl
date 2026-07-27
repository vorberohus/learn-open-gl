#version 330 core

in vec3 vertexColor;
in vec3 position;
in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D baseMap;

void main()
{
    FragColor = vec4(vertexColor, 1.0);
    //texture(baseMap, texCoord) * vec4(texCoord, 0.0, 1.0);
    //vec4(position, 1.0);
    //vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
