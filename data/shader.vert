#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform pushConstants
{
    mat4 mvp;
};

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    fragColor = color;
}
