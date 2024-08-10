#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;

out vec2 uv;
out vec3 worldPosition;

uniform mat4 world, view, projection;

uniform sampler2D mainTex;

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{
    vec3 pos = aPos;

    // Generate a random number using UV coordinates
    float randHeight = random(vUV) * 10.0;

    // Apply the random height offset
    //pos.y += randHeight;

    vec4 worldPos = world * vec4(pos, 1.0);
    
    gl_Position = projection * view * worldPos;

    uv = vUV;
    worldPosition = mat3(world) * pos;
}