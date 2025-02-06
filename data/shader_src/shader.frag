#version 450

layout(set = 0, binding = 0) uniform Ubo {
    mat4 vp;
    vec3 viewPos;
    vec4 time;
    vec3 lightPos;
    vec3 lightColor;
};

layout(set = 1, binding = 0) uniform MaterialParameters {
    vec3 ambient;
    vec3 diffuse;
    vec4 specularAndShininess;
} material;
layout(set = 1, binding = 1) uniform sampler2D tex0;

layout(location = 0) in vec3 positionWorld;
layout(location = 1) in vec3 positionModel;
layout(location = 2) in vec3 normalWorld;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec4 outColor;

vec3 phong(vec3 color)
{
    vec3 normal = normalize(normalWorld);
    vec3 lightDirection = normalize(lightPos - positionWorld);
    vec3 reflection = normalize(reflect(-lightDirection, normal));
    vec3 viewDir = normalize(viewPos - positionWorld);

    vec3 diffuse = max(dot(lightDirection, normal) * material.diffuse, 0.0);
    vec3 specular = min(
        pow(max(dot(reflection, viewDir), 0.0), material.specularAndShininess.w) * material.specularAndShininess.xyz,
        1.0
    );
    return clamp(material.ambient + diffuse + specular, 0.0, 1.0) * color;
}

void main()
{
    vec4 texColor = texture(tex0, uv);
    texColor = vec4(phong(texColor.xyz), texColor.a);
    outColor = texColor;
}
