#version 330 core

in vec3 vertexColor;
in vec3 position;
in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D uBaseMap;
uniform sampler2D uAuxMap;
uniform float uMixAlpha;

void main()
{
    FragColor = mix(texture(uBaseMap, texCoord), texture(uAuxMap, vec2(1 - texCoord.x, texCoord.y)), uMixAlpha);
}
