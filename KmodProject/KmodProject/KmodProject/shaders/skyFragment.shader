#version 330 core
out vec4 FragColor;

in vec4 worldPosition;

uniform vec3 lightDirection;
uniform vec3 cameraPosition;

vec3 lerp(vec3 a, vec3 b, float t){
	return a + (b - a) * t;
	}

void main()
{	
	vec3 topColor = vec3(125.0 / 255.0, 25.0 / 255.0, 156.0/ 255.0);
	vec3 botColor = vec3(125.0 / 255.0, 25.0 / 255.0, 50.0/ 255.0);
	//vec3 botColor = vec3(187.0 / 255.0, 25.0 / 255.0, 29.0/ 255.0);

	vec3 sunColor = vec3(249.0 / 255.0, 200 / 255.0, 31 / 255.0);

	//calculate view
	vec3 viewDir = normalize(worldPosition.rgb - cameraPosition);
	float sun = max(pow(dot(-viewDir, lightDirection), 256), 0.0);


	FragColor = vec4(lerp(botColor, topColor, max(viewDir.y, 0.0)) + sun * sunColor, 1);

}

