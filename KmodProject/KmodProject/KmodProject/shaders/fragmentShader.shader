#version 330 core
out vec4 FragColor;

in vec3 color;
in vec2 uv;
in mat3 tbn;
in vec3 worldPosition;

uniform sampler2D mainTex;
uniform sampler2D normalTex;

uniform vec3 lightDirection;
uniform vec3 cameraPosition;

vec3 lerp(vec3 a, vec3 b, float t){
	return a + (b - a) * t;
	}

void main()
{	//Normal map
	vec3 normal = texture(normalTex, uv).rgb;
	normal = normalize(normal * 2.0 - 1.0);

	//transform with tbn
	normal = tbn * normal;

    float lightValue = max(-dot(normal, lightDirection), 0.0);

	//specular data
	vec3 viewDir = normalize(worldPosition - cameraPosition);

	float dist = length(worldPosition.xyz - cameraPosition);
	float uvLerp = clamp((dist - 250) / 25, -1, 1) * 0.5 + 0.5;
    // Texture color
    vec4 texColor = texture(mainTex, uv);
    vec3 fragColor = texColor.rgb * color;

    float fog = pow(clamp((dist - 250) / 1000, 0, 1), 2);

	vec3 topColor = vec3(68.0 / 255.0, 118.0 / 255.0, 189.0/ 255.0);
	vec3 botColor = vec3(188.0 / 255.0, 214.0 / 255.0, 231.0/ 255.0);
	
	vec3 fogColor = lerp(botColor, topColor, max(viewDir.y, 0.0));

	vec4 fragOutput = vec4(lerp(fragColor * min(lightValue + 0.1, 1.0), fogColor, fog), 1);

	FragColor = fragOutput;

}

