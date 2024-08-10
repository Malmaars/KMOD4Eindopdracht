#version 330 core
out vec4 FragColor;

in vec2 uv;
in vec3 worldPosition;

uniform vec3 terrainColor;
uniform vec3 lightDirection;
uniform vec3 cameraPosition;
 
vec3 lerp(vec3 a, vec3 b, float t){
	return a + (b - a) * t;
	}

void main()
{	

	//specular data
	vec3 viewDir = normalize(worldPosition.rgb - cameraPosition);
	//vec3 reflDir = normalize(reflect(lightDirection, normal));

	//Lighting
	//float lightValue = max(lightDirection, 0.0);
	// float specular = pow(max(-dot(reflDir, viewDir), 0.0), 8);

	//build color
	float y = worldPosition.y;

	float dist = length(worldPosition.xyz - cameraPosition);
	float uvLerp = clamp((dist - 250) / 25, -1, 1) * 0.5 + 0.5;

	vec3 synthColor = terrainColor;


	vec3 diffuse = synthColor;

	float fog = pow(clamp((dist - 250) / 1000, 0, 1), 2);

	vec3 topColor = vec3(125.0 / 255.0, 25.0 / 255.0, 156.0/ 255.0);
	vec3 botColor = vec3(187.0 / 255.0, 25.0 / 255.0, 29.0/ 255.0);

	vec3 fogColor = lerp(botColor, topColor, max(viewDir.y, 0.0));
	
	vec4 fragOutput = vec4(lerp(diffuse, fogColor, fog), 1.0);

	FragColor = fragOutput;


}

