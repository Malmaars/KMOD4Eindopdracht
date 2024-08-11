#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include<fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <string>
#include <cmath>
#include <algorithm> 
#include <random>

#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Build -> rebuild Solution, Debug -> Start without debugging

//Waarom doen we de functies met kleine letters? Is dat iets wat moet met opengl?
//Of is dat een regel bij opengl programmers?

//Forward Declaration
int init(GLFWwindow*& window);
void processInput(GLFWwindow* window);
void createShaders();
void createProgram(GLuint& programID, const char* vertex, const char* fragment);
void createTGeometry(GLuint& vao, int& size, int& numIndices);
GLuint loadTexture(const char* path, int comp = 0);
void renderSkyBox();
void renderTerrain();
unsigned int GenerateRandomPlane(int width, int height, float hScale, float xzScale, unsigned int& indexCount);
void movePlane();
void initiateShockwave(float dampingMin, float dampingMax, float offsetMin, float offsetMax);
void shockwave();

//Window callbacks
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

bool keys[1024];

//Util
void loadFile(const char* filename, char*& output);

//Program IDs
GLuint skyProgram, terrainProgram;

const int WIDTH = 1920, HEIGHT = 1080;

//World data

glm::vec3 lightDirection = glm::normalize(glm::vec3(0, -0.05f, -1));
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint boxVAO, boxEBO;
int boxSize;
int boxIndexCount;

glm::mat4 view;
glm::mat4 projection;

float lastX, lastY;
bool firstMouse = true;
float camYaw, camPitch;
glm::quat camQuat = glm::quat(glm::vec3(glm::radians(camPitch), glm::radians(camYaw), 0));

//Terrain data
GLuint terrainVAO, terrainVBO, terrainIndexCount, heightmapID, heightNormalID;
unsigned char* heightmapTexture;

glm::vec3 groundColor = glm::vec3(51.0f / 255.0f, 31.0f / 255.0f, 64.0f / 255.0f);
glm::vec3 lineColor = glm::vec3(193.0f / 255.0f, 63.0f / 255.0f, 153.0f / 255.0f);
int terrainWidth = 512;
int terrainHeight = 512;
float* vertices;
float* actualVertices;
float* bufferVertices; // this is so I can rearrange the vertices without losing the values of where they were before

float terrainHScale, terrainXZScale; //public variables so I can use them to render the plane on the correct location

float scrollTimer = 0; //timer so the scrolling of the terrain isn't insanely fast;

//shockwave values
float* shockWaveVertices;
float* shockWaveVerticesBuffer;
float shockWaveTimer = 0;
float* heightWaveVertices;
float* heightWaveVerticesBuffer;

GLuint synth;

int main() {

	GLFWwindow* window;
	int res = init(window);

	if (res != 0) {
		return res;
	}

	createShaders();
	createTGeometry(boxVAO, boxSize, boxIndexCount);

	terrainHScale = 1.0f;
	terrainXZScale = 5.0f;
	terrainVAO = GenerateRandomPlane(terrainWidth, terrainHeight, terrainHScale, terrainXZScale, terrainIndexCount);
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);


	//Create gl Viewport
	glViewport(0, 0, WIDTH, HEIGHT);

	//Matrices!

	glm::mat4 world = glm::mat4(1.0f);
	glm::vec3 camForward = camQuat * glm::vec3(0, 0, 1);
	glm::vec3 camUp = camQuat * glm::vec3(0, 1, 0);
	view = glm::lookAt(cameraPosition, cameraPosition + camForward, camUp);
	projection = glm::perspective(glm::radians(45.0f), WIDTH / (float)HEIGHT, 0.1f, 5000.0f);



	float lastFrameTime = glfwGetTime();

	//Render Loop
	while (!glfwWindowShouldClose(window)) {

		processInput(window);

		float currentFrameTime = glfwGetTime();
		float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;
		scrollTimer += deltaTime;
		shockWaveTimer += deltaTime;

		//events pollen
		glfwPollEvents();

		//rendering
		// 
		//glClearColor(0.1f, 0.8f, 0.76f, 0); 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderSkyBox();


		//if (shockWaveTimer > 3) {
		//	initiateShockwave(); 
		//	shockWaveTimer = 0;
		//}

		if (scrollTimer > 0.035f) {
			movePlane();
			scrollTimer = 0;

			shockwave();
		}
		renderTerrain();

		//buffers swappen
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Disable depth testing
	glDisable(GL_DEPTH_TEST);

	glfwTerminate();
	return 0;
}

void renderSkyBox() {

	glDisable(GL_DEPTH);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(skyProgram);

	//Matrices!
	glm::mat4 world = glm::mat4(1.0f);
	world = glm::translate(world, cameraPosition);
	world = glm::scale(world, glm::vec3(100, 100, 100));

	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform3fv(glGetUniformLocation(skyProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(skyProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

	//rendering
	glBindVertexArray(boxVAO);
	glDrawElements(GL_TRIANGLES, boxIndexCount, GL_UNSIGNED_INT, 0);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);
}

void renderTerrain() {
	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//Render de environment twee keer. Eerst een solid color, daarna de wireframe
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	glUseProgram(terrainProgram);

	//Matrices!
	glm::mat4 world = glm::mat4(1.0f);
	world = glm::translate(world, glm::vec3(-((terrainWidth * terrainXZScale) / 2), -5, -100));

	glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


	glUniform3fv(glGetUniformLocation(terrainProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(terrainProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));
	//set the color to purple for the fill
	glUniform3fv(glGetUniformLocation(terrainProgram, "terrainColor"), 1, glm::value_ptr(groundColor));
	//rendering
	glBindVertexArray(terrainVAO);
	glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);


	//doing just glLineWidth(5.0f); makes every pixel the same thickness, I want to change the thickness with its distance from the camera
	glLineWidth(1.0f); // Set the line width to 2 pixels
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//set the color to purple for the fill
	glUniform3fv(glGetUniformLocation(terrainProgram, "terrainColor"), 1, glm::value_ptr(lineColor));

	//rendering

	//I'm giving the outline a little offset so there won't be any overlap with the ground beneath, it looks doesn't render nicely if I keep them at exactly the same position.
	//z fighting and all that
	world = glm::translate(world, glm::vec3(0,0.1f,0));
	glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glBindVertexArray(terrainVAO);
	glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

unsigned int GenerateRandomPlane(int width,int height, float hScale, float xzScale, unsigned int& indexCount) {

	int stride = 8; // position (3) + normal (3) + uv (2)
	vertices = new float[(width * height) * stride];
	unsigned int* indices = new unsigned int[(width - 1) * (height - 1) * 6];
	
	//save this info for the shockwave
	//this may look weird, but the shockwave flows per row, so we only need to save the height really
	shockWaveVertices = new float[terrainHeight];
	shockWaveVerticesBuffer = new float[terrainHeight];
	heightWaveVertices = new float[terrainHeight];
	heightWaveVerticesBuffer = new float[terrainHeight];

	for (int i = 0; i < terrainHeight; i++) {
		shockWaveVertices[i] = 0;
		shockWaveVerticesBuffer[i] = 0;
		heightWaveVertices[i] = 0;
		heightWaveVerticesBuffer[i] = 0;
	}

	int index = 0;
	float* randomHeights = new float[width * height];

	//power of 2 to make sure the grid alligns
	int interval = 2;

	// maybe take in account the previous random height, so the difference won't be that big
	// Initialize the random heights for every 20th vertex
	for (int z = 0; z < height; z += interval) {
		for (int x = 0; x < width; x += interval) {

			int vertexPosition = index;

			//say the plane is 256 wide, the center is at vertex 128. If the width is one, I will make 127, 128, and 129 flat. So 1 to each side
			int centerVertex = width / 2;
			int row = (int)(vertexPosition / width);
			float distanceFromCenter = std::pow(std::pow((vertexPosition - (width * row) - centerVertex), 2), 0.5f);

			int heightValue = (int)(distanceFromCenter / 8);
			//std::cout << heightValue << std::endl;

			//randomHeights[z * width + x] = (rand() / (1000.0f * (heightValue + 1))) * hScale;

			//I want a maximum value of 8
			if (heightValue > 10)
				heightValue = 10;
			if (heightValue <= 0)
				heightValue = 1;

			if (distanceFromCenter > 100)
				distanceFromCenter = 100;

			randomHeights[z * width + x] = std::pow(distanceFromCenter, (rand() % heightValue) * 0.05f) * hScale;

			index++;
		}
	}

	index = 0;

	// Interpolate the heights between the random vertices
	for (int z = 0; z < height; ++z) {
		for (int x = 0; x < width; ++x) {
			

			//I want the middle couple rows to be flat.
			//(int) float round down. I can use this info
			bool flat = false;
			int vertexPosition = (int)(index / stride);
			int roadWidth = 3; //the width of the road in the center.

			//say the plane is 256 wide, the center is at vertex 128. If the width is one, I will make 127, 128, and 129 flat. So 1 to each side
			int centerVertex = width / 2;
			int row = (int)(vertexPosition / width);
			float distanceFromCenter = std::pow(std::pow((vertexPosition - (width * row) - centerVertex), 2), 0.5f);

			if (distanceFromCenter < roadWidth)
			{
				flat = true;
			}

			 if ((x % interval != 0) || (z % interval != 0)) {
				// Interpolate between the nearest random vertices
				int x0 = (x / interval) * interval;
				int z0 = (z / interval) * interval;
				int x1 = x0 + interval < width ? x0 + interval : x0;
				int z1 = z0 + interval < height ? z0 + interval : z0;

				float sx = (float)(x - x0) / (float)interval;
				float sz = (float)(z - z0) / (float)interval;

				float h0 = randomHeights[z0 * width + x0] * (1 - sx) + randomHeights[z0 * width + x1] * sx;
				float h1 = randomHeights[z1 * width + x0] * (1 - sx) + randomHeights[z1 * width + x1] * sx;

				randomHeights[z * width + x] = h0 * (1 - sz) + h1 * sz;
			}


			// Set position
			vertices[index++] = x * xzScale;
			if(flat)
				vertices[index++] = 0;
			else
				vertices[index++] = randomHeights[z * width + x];

			vertices[index++] = z * xzScale;

			// Set normal (placeholder for now)
			vertices[index++] = 0;
			vertices[index++] = 1;
			vertices[index++] = 2;

			// Set UV coordinates
			vertices[index++] = x / (float)(width - 1);
			vertices[index++] = z / (float)(height - 1);
		}
	}

	// Generate indices for the grid
	index = 0;
	for (int z = 0; z < height - 1; ++z) {
		for (int x = 0; x < width - 1; ++x) {
			int vertex = z * width + x;
			indices[index++] = vertex;
			indices[index++] = vertex + width;
			indices[index++] = vertex + width + 1;

			indices[index++] = vertex;
			indices[index++] = vertex + width + 1;
			indices[index++] = vertex + 1;
		}
	}

	actualVertices = new float[width * height * stride];
	bufferVertices = new float[width * height * stride];
	for (int i = 0; i < width * height * stride; i++) {
		actualVertices[i] = vertices[i];
	}

	unsigned int vertSize = (width * height) * stride * sizeof(float);
	indexCount = (width - 1) * (height - 1) * 6;

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	// Set vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 6));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	terrainVBO = VBO; 

	return VAO;
}

void movePlane() {

	int stride = 8; // position (3) + normal (3) + uv (2)
	int totalVertices = terrainWidth * terrainHeight;
	int vertexArraySize = totalVertices * stride;

	// Assuming `vertices` is allocated somewhere globally or passed in with the correct size
	int index = 0;

	int interval = 1;


	for (int i = 0; i < terrainHeight * terrainWidth * stride; i++) {
		bufferVertices[i] = actualVertices[i];
	}

	//change the position of the vertex to the one below it (visually), so i + width
	for (int z = 0; z < terrainHeight; ++z) {
		for (int x = 0; x < terrainWidth; ++x) {

			index++;
			if (index + (terrainWidth * stride) < vertexArraySize) {
				//ONLY the y position, I changed the x and z as well and that just results in every vertex moving to the exact same spot as the target
				vertices[index] = bufferVertices[index + terrainWidth * stride];
				actualVertices[index] = bufferVertices[index + terrainWidth * stride];
			}

			else {
				vertices[index] = bufferVertices[index + terrainWidth * stride - vertexArraySize];
				actualVertices[index] = bufferVertices[index + terrainWidth * stride - vertexArraySize];
			}
			
			index += stride - 1;
		}
	}

	// Correctly update the vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexArraySize * sizeof(float), vertices);  // Correctly calculating size
}

void initiateShockwave(float dampingMin, float dampingMax, float offsetMin, float offsetMax) {

	if (dampingMin != 0 || dampingMax != 0) {
		shockWaveVertices[7] = dampingMax;
		for (int i = 0; i <= 6; i++) {
			shockWaveVertices[i] = dampingMin + ((dampingMax - dampingMin) / 6 * i);
			shockWaveVertices[14 - i] = dampingMin + ((dampingMax - dampingMin) / 6 * i);
		}

		for (int i = terrainHeight - 1; i >= 0; i--) {
			shockWaveVerticesBuffer[i] = shockWaveVertices[i];
		}
	}

	if (offsetMin != 0 || offsetMax != 0) {
		heightWaveVertices[7] += offsetMax;

		if (heightWaveVertices[7] > 4)
			heightWaveVertices[7] = 4;

		for (int i = 0; i <= 6; i++) {
			heightWaveVertices[i] += offsetMin + ((offsetMax - offsetMin) / 6 * i);
			heightWaveVertices[14 - i] += offsetMin + ((offsetMax - offsetMin) / 6 * i);

			if (heightWaveVertices[i] > 4)
				heightWaveVertices[i] = 4;
			if (heightWaveVertices[14-i] > 4)
				heightWaveVertices[14-i] = 4;
			std::cout << heightWaveVertices[7] << std::endl;
		}

		for (int i = terrainHeight - 1; i >= 0; i--) {
			heightWaveVerticesBuffer[i] = heightWaveVertices[i];
		}
	}
}
//send a shockwave through the terrain
void shockwave() {
	//the shockwave moves in the opposite direction as the terrain, but will deform it as well.

	//the shockwave vertices carry an offset, this offset move down the terrain.
	int stride = 8; // position (3) + normal (3) + uv (2)
	int totalVertices = terrainWidth * terrainHeight;
	int vertexArraySize = totalVertices * stride;

	// Assuming `vertices` is allocated somewhere globally or passed in with the correct size
	int index = 0;

	int interval = 1;

	//change the position of the vertex to the one below it (visually), so i + width
	for (int z = 0; z < terrainHeight; ++z) {
		for (int x = 0; x < terrainWidth; ++x) {

			index++;

			//both flatten the environment and give it a small downward offset
			if (shockWaveVertices[(int)(index / 8 / terrainWidth)] != 0)
				vertices[index] = (actualVertices[index] / shockWaveVertices[(int)(index / 8 / terrainWidth)]) + heightWaveVertices[(int)(index / 8 / terrainWidth)];
			else
				vertices[index] = actualVertices[index] + heightWaveVertices[(int)(index / 8 / terrainWidth)];
			index += stride - 1;
		}
	}

	// Correctly update the vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexArraySize * sizeof(float), vertices);  // Correctly calculating size

	for (int i = terrainHeight - 1; i >= 0; i--) {
		shockWaveVertices[i] = shockWaveVerticesBuffer[i - 1];
	}
	shockWaveVertices[0] = 0;

	for (int i = terrainHeight - 1; i >= 0; i--) {
		shockWaveVerticesBuffer[i] = shockWaveVertices[i];
	}

	for (int i = terrainHeight - 1; i >= 0; i--) {
		heightWaveVertices[i] = heightWaveVerticesBuffer[i - 1];
	}
	heightWaveVertices[0] = 0;

	for (int i = terrainHeight - 1; i >= 0; i--) {
		heightWaveVerticesBuffer[i] = heightWaveVertices[i];
	}

}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	float moveSpeed = 0.1f;

	bool camChanged = false;

	if (keys[GLFW_KEY_E]) {
		initiateShockwave(2,8,0,0);
		keys[GLFW_KEY_E] = false; // make it single press 
	}
	if (keys[GLFW_KEY_Q]) {
		initiateShockwave(0.9f, 0.4f, 0, 0);
		keys[GLFW_KEY_Q] = false; // make it single press 
	}
	if (keys[GLFW_KEY_W]) {
		initiateShockwave(0, 0, 0, 3);
		keys[GLFW_KEY_W] = false; // make it single press 
	}
	if (keys[GLFW_KEY_S]) {
		initiateShockwave(0, 0, 0, -3);
		keys[GLFW_KEY_S] = false; // make it single press 
	}
	//if (keys[GLFW_KEY_W]) {
	//	cameraPosition += camQuat * glm::vec3(0, 0, 1) * moveSpeed;
	//	camChanged = true;
	//}
	//if (keys[GLFW_KEY_S]) {
	//	cameraPosition += camQuat * glm::vec3(0, 0, -1) * moveSpeed;
	//	camChanged = true;
	//}
	//if (keys[GLFW_KEY_A]) {
	//	cameraPosition += camQuat * glm::vec3(1, 0, 0) * moveSpeed;
	//	camChanged = true;
	//}
	//if (keys[GLFW_KEY_D]) {
	//	cameraPosition += camQuat * glm::vec3(-1, 0, 0) * moveSpeed;
	//	camChanged = true;
	//}
	//if (keys[GLFW_KEY_SPACE]) {
	//	cameraPosition += camQuat * glm::vec3(0, 1, 0) * moveSpeed;
	//	camChanged = true;
	//}
	//if (keys[GLFW_KEY_LEFT_CONTROL]) {
	//	cameraPosition += camQuat * glm::vec3(0, -1, 0) * moveSpeed;
	//	camChanged = true;
	//}

	if (camChanged) {
		glm::vec3 camForward = camQuat * glm::vec3(0, 0, 1);
		glm::vec3 camUp = camQuat * glm::vec3(0, 1, 0);
		view = glm::lookAt(cameraPosition, cameraPosition + camForward, camUp);
	}
}

int init(GLFWwindow*& window) {
	//init glfw
	glfwInit();
	//window hints
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//create window & make context current
	window = glfwCreateWindow(WIDTH, HEIGHT, "Hello World!", NULL, NULL);

	if (window == NULL) {
		//error
		std::cout << "Failed to create window!" << std::endl;
		glfwTerminate();
		return-1;
	}


	//register callbacks
	//glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);

	//Load GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		//store key is pressed
		keys[key] = true;
	}
	else if (action == GLFW_RELEASE) {
		//stpre key is not pressed
		keys[key] = false;
	}
}

void createTGeometry(GLuint& vao, int& size, int& numIndices) {

	//to create different triangles, I'd want this as a parameter, but I don't understand c++ well enough to do that
	//in  fact, I understand so little of c++, I don't even know what to look up to get this to work

	// need 24 vertices for normal/uv-mapped Cube
	float vertices[] = {
		// positions            //colors            // tex coords   // normals          //tangents      //bitangents
		0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,

		0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,
		0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   1.f, 0.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,

		0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,
		-0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,

		-0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 0.f,      -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   0.f, 1.f,      -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,

		-0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   0.f, 1.f,      0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,
		0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,      0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,

		-0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,

		0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,
		-0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,

		-0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   1.f, 1.f,       -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,

		-0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,
		0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,

		0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,
		0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 0.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,

		0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f
	};

	unsigned int indices[] = {  // note that we start from 0!
		// DOWN
		0, 1, 2,   // first triangle
		0, 2, 3,    // second triangle
		// BACK
		14, 6, 7,   // first triangle
		14, 7, 15,    // second triangle
		// RIGHT
		20, 4, 5,   // first triangle
		20, 5, 21,    // second triangle
		// LEFT
		16, 8, 9,   // first triangle
		16, 9, 17,    // second triangle
		// FRONT
		18, 10, 11,   // first triangle
		18, 11, 19,    // second triangle
		// UP
		22, 12, 13,   // first triangle
		22, 13, 23,    // second triangle
	};

	int stride = (3 + 3 + 2 + 3 + 3 + 3) * sizeof(float);

	size = sizeof(vertices) / stride;
	numIndices = sizeof(indices) / sizeof(int);

	unsigned int EBO;
	glGenBuffers(1, &EBO);

	glGenVertexArrays(1, &vao);
	//ik snap nog niet helemaal hoe de & nou werkt. Waarom hier dan weer niet?
	glBindVertexArray(vao);

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//ik hoef niet heel goed te begrijpen hoe dit werkt. cool, want ik begrijp het niet :)
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);

	//colors
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, stride, (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(4, 3, GL_FLOAT, GL_TRUE, stride, (void*)(11 * sizeof(float)));
	glEnableVertexAttribArray(4);

	glVertexAttribPointer(5, 3, GL_FLOAT, GL_TRUE, stride, (void*)(14 * sizeof(float)));
	glEnableVertexAttribArray(5);
}

void createShaders() {
	createProgram(skyProgram, "shaders/skyVertex.shader", "shaders/skyFragment.shader");

	createProgram(terrainProgram, "shaders/terrainVertex.shader", "shaders/terrainFragment.shader");
}

void createProgram(GLuint& programID, const char* vertex, const char* fragment) {
	//Create a GL program with a vertex & fragment shader
	char* vertexSrc;
	char* fragmentSrc;
	loadFile(vertex, vertexSrc);
	loadFile(fragment, fragmentSrc);

	GLuint vertexShaderID, fragmentShaderID;

	vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderID, 1, &vertexSrc, nullptr);
	glCompileShader(vertexShaderID);

	int success;
	char infoLog[512];
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShaderID, 512, nullptr, infoLog);
		std::cout << "ERROR COMPILING VERTEX SHADER\n" << infoLog << std::endl;
	}

	fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderID, 1, &fragmentSrc, nullptr);
	glCompileShader(fragmentShaderID);

	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShaderID, 512, nullptr, infoLog);
		std::cout << "ERROR COMPILING FRAGMENT SHADER\n" << infoLog << std::endl;
	}

	programID = glCreateProgram();
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	glLinkProgram(programID);

	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(programID, 512, nullptr, infoLog);
		std::cout << "ERROR LINKING PROGRAM\n" << infoLog << std::endl;
	}

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	delete vertexSrc;
	delete fragmentSrc;
}

void loadFile(const char* filename, char*& output) {
	//open the file
	std::ifstream file(filename, std::ios::binary);

	//if the file was succesfully opened
	if (file.is_open()) {
		//get length of file
		file.seekg(0, file.end);
		int length = file.tellg();
		file.seekg(0, file.beg);

		//allocate memort for the char pointer
		output = new char[length + 1];

		//read data as a block
		file.read(output, length);

		//add null terminator to end of char pointer
		output[length] = '\0';

		//close the file
		file.close();
	}
	else {
		//if the file failed to open, set the char pointer to NULL
		output = NULL;
	}


}

GLuint loadTexture(const char* path, int comp)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, numChannels;
	unsigned char* data = stbi_load(path, &width, &height, &numChannels, comp);
	if (data) {
		if (comp != 0) {
			numChannels = comp;
		}
		if (numChannels == 3) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}

		if (numChannels == 4) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);

	}
	else {
		std::cout << "Error loading texture: " << path << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);
	return textureID;
}

//https://glad.dav1d.de/