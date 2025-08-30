#version 450

layout(binding = 0) uniform Ubo {
    mat4 vp;
    vec3 viewPos;
    vec4 time;
    vec3 lightDirection;
    vec3 lightPos;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragPositionWorld;
layout(location = 1) out vec3 fragPositionModel;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec3 fragColor;
layout(location = 4) out vec2 fragUv;

layout(push_constant) uniform pushConstants
{
    mat4 model;
    mat4 modelInvT;
};

void main() {
    vec4 positionWorld = model * vec4(position, 1.f);
    vec4 positionView = vp * positionWorld;
    vec4 normalWorld = modelInvT * vec4(normal, 1.f);

    gl_Position = positionView;
    fragPositionWorld = positionWorld.xyz;
    fragPositionModel = position;
    fragNormalWorld = normalWorld.xyz;
    fragColor = color;
    fragUv = uv;
}
