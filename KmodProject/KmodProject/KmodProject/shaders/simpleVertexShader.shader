#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec3 vNormal;
layout(location = 4) in vec3 vTangent;
layout(location = 5) in vec3 vBitangent;

out vec3 color;
out vec2 uv;
out mat3 tbn;
out vec3 worldPosition;

uniform mat4 world, view, projection;

void main()
{	
	gl_Position = projection * view * world * vec4(aPos, 1.0);
	color = vColor;

	uv = vUV;
	vec3 T = normalize(mat3(world) * vTangent);
	vec3 B = normalize(mat3(world) * vBitangent);
	vec3 N = normalize(mat3(world) * vNormal);

	tbn = mat3(T, B, N);
	
	worldPosition = mat3(world) * aPos;
}