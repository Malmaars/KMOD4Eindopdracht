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
#include <random>

#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"




struct Light {
	glm::vec3 position;
	glm::vec3 color;

	Light(const glm::vec3& pos, const glm::vec3& col) : position(pos), color(col) {}
};

//Build -> rebuild Solution, Debug -> Start without debugging

//Waarom doen we de functies met kleine letters? Is dat iets wat moet met opengl
//Of is dat een regel bij opengl programmers?
//En waarom moeten we überhaupt de functies declareren voordat we ze ook echt aanmaken?

//Forward Declaration
int init(GLFWwindow*& window);
void processInput(GLFWwindow* window);
void createShaders();
void createProgram(GLuint& programID, const char* vertex, const char* fragment);
void createTGeometry(GLuint& vao, int& size, int& numIndices);
GLuint loadTexture(const char* path, int comp = 0);
void renderBox(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);
void renderSkyBox();
void renderTerrain();
void renderModel(Model* model, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);
unsigned int GeneratePlane(const char* heightmap, unsigned char*& data, GLenum format, int comp, float hScale, float xzScale, unsigned int& indexCount, unsigned int& heightmapID);
float RandomFloat(float min, float max);
unsigned int GenerateRandomPlane(int width, int height, float hScale, float xzScale, unsigned int& indexCount);
void movePlane();

//Window callbacks
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

bool keys[1024];

//Util
void loadFile(const char* filename, char*& output);

//Program IDs
GLuint simpleProgram, skyProgram, terrainProgram, modelProgram;

const int WIDTH = 1280, HEIGHT = 720;

//World data

glm::vec3 lightDirection = glm::normalize(glm::vec3(0, -0.05f, -1));
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint boxTex, boxNormal;
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
int terrainWidth = 256;
int terrainHeight = 256;
float* vertices;
float* bufferVertices; // this is so I can rearrange the vertices without losing the values of where they were before

float terrainHScale, terrainXZScale; //public variables so I can use them to render the plan on the correct location

float scrollTimer = 0; //timer so the scrolling of the terrain isn't insanely fast;


GLuint synth;

int main() {

	GLFWwindow* window;
	int res = init(window);

	if (res != 0) {
		return res;
	}

	//stbi_set_flip_vertically_on_load(true);


	createShaders();
	createTGeometry(boxVAO, boxSize, boxIndexCount);

	terrainHScale = 100.0f;
	terrainXZScale = 5.0f;
	terrainVAO = GenerateRandomPlane(terrainWidth, terrainHeight, terrainHScale, terrainXZScale, terrainIndexCount);
	heightNormalID = loadTexture("textures/emptyNormal.png");

	boxTex = loadTexture("textures/waterColor.png");
	boxNormal = loadTexture("textures/emptyNormal.png");

	synth = loadTexture("textures/synth.jpg");

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

		//events pollen
		glfwPollEvents();

		//rendering
		// 
		//glClearColor(0.1f, 0.8f, 0.76f, 0); 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(simpleProgram);

		renderSkyBox();

		if (scrollTimer > 0.03f) {
			movePlane();
			scrollTimer = 0;
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

void renderBox(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale) {

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glm::mat4 world = glm::mat4(1.0f);
	world = glm::translate(world, pos);
	world = world * glm::toMat4(glm::quat(rot));
	world = glm::scale(world, scale);

	glUseProgram(simpleProgram);

	glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform3fv(glGetUniformLocation(simpleProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(simpleProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, boxTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, boxNormal);

	glBindVertexArray(boxVAO);
	//glDrawArrays(GL_TRIANGLES, 0, triangleSize);
	glDrawElements(GL_TRIANGLES, boxIndexCount, GL_UNSIGNED_INT, 0);

	glDisable(GL_BLEND);
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

	glUseProgram(terrainProgram);

	//Matrices!
	glm::mat4 world = glm::mat4(1.0f);
	world = glm::translate(world, glm::vec3(-((terrainWidth * terrainXZScale) / 2), -15, 0));

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

// Function to generate a random float between a given range
float RandomFloat(float min, float max) {
	static std::default_random_engine e;
	static std::uniform_real_distribution<> dis(0, 1); // range [0, 1)
	return min + dis(e) * (max - min);
}

unsigned int GenerateRandomPlane(int width,int height, float hScale, float xzScale, unsigned int& indexCount) {

	int stride = 8; // position (3) + normal (3) + uv (2)
	vertices = new float[(width * height) * stride];
	unsigned int* indices = new unsigned int[(width - 1) * (height - 1) * 6];

	int index = 0;
	float* randomHeights = new float[width * height];

	//power of 2 to make sure the grid alligns
	int interval = 2;


	// take in account the previous random height, so the difference won't be that big
	// Initialize the random heights for every 20th vertex
	for (int z = 0; z < height; z += interval) {
		for (int x = 0; x < width; x += interval) {
			randomHeights[z * width + x] = ((rand() % 100) / 1000.0f) * hScale;
		}
	}

	// Interpolate the heights between the random vertices
	for (int z = 0; z < height; ++z) {
		for (int x = 0; x < width; ++x) {
			if ((x % interval == 0) && (z % interval == 0)) {
				// Keep the random height
			}
			else {
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

unsigned int GeneratePlane(const char* heightmap, unsigned char*& data, GLenum format, int comp, float hScale, float xzScale, unsigned int& indexCount, unsigned int& heightmapID) {
	int width, height, channels;
	data = nullptr;
	if (heightmap != nullptr) {
		data = stbi_load(heightmap, &width, &height, &channels, comp);
		if (data) {
			glGenTextures(1, &heightmapID);
			glBindTexture(GL_TEXTURE_2D, heightmapID);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	int stride = 8;
	float* vertices = new float[(width * height) * stride];
	unsigned int* indices = new unsigned int[(width - 1) * (height - 1) * 6];

	int index = 0;
	for (int i = 0; i < (width * height); i++) {
		// TODO: calculate x/z values
		int x = i % width;
		int z = i / width;

		float texHeight = (float)data[i * comp];

		// TODO: set position
		vertices[index++] = x * xzScale;
		vertices[index++] = (texHeight / 255.0f) * hScale;
		vertices[index++] = z * xzScale;

		// TODO: set normal
		vertices[index++] = 0;
		vertices[index++] = 1;
		vertices[index++] = 2;

		// TODO: set uv
		vertices[index++] = x / (float)width;
		vertices[index++] = z / (float)height;
	}

	// OPTIONAL TODO: Calculate normal
	// TODO: Set normal

	index = 0;
	for (int i = 0; i < (width - 1) * (height - 1); i++) {

		int x = i % (width - 1);
		int z = i / (width - 1);

		int vertex = z * width + x;

		indices[index++] = vertex;
		indices[index++] = vertex + width;
		indices[index++] = vertex + width + 1;

		indices[index++] = vertex;
		indices[index++] = vertex + width + 1;
		indices[index++] = vertex + 1;

	}

	unsigned int vertSize = (width * height) * stride * sizeof(float);
	indexCount = ((width - 1) * (height - 1) * 6);

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	// vertex information!
	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, 0);
	glEnableVertexAttribArray(0);
	// normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);
	// uv
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 6));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	delete[] vertices;
	delete[] indices;

	//stbi_image_free(data);

	return VAO;
}

void movePlane() {

	bufferVertices = vertices;
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
			if (index + (terrainWidth * stride) < vertexArraySize) {
				//ONLY the y position, I changed the x and z as well and that just results in every vertex moving to the exact same spot as the target
				vertices[index] = bufferVertices[index + terrainWidth * stride];
			}

			else {
				vertices[index] = bufferVertices[index + terrainWidth * stride - vertexArraySize];
			}
			
			index += 7;
		}
	}

	// Correctly update the vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexArraySize * sizeof(float), vertices);  // Correctly calculating size
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	float moveSpeed = 0.1f;

	bool camChanged = false;

	if (keys[GLFW_KEY_W]) {
		cameraPosition += camQuat * glm::vec3(0, 0, 1) * moveSpeed;
		camChanged = true;
	}
	if (keys[GLFW_KEY_S]) {
		cameraPosition += camQuat * glm::vec3(0, 0, -1) * moveSpeed;
		camChanged = true;
	}
	if (keys[GLFW_KEY_A]) {
		cameraPosition += camQuat * glm::vec3(1, 0, 0) * moveSpeed;
		camChanged = true;
	}
	if (keys[GLFW_KEY_D]) {
		cameraPosition += camQuat * glm::vec3(-1, 0, 0) * moveSpeed;
		camChanged = true;
	}
	if (keys[GLFW_KEY_SPACE]) {
		cameraPosition += camQuat * glm::vec3(0, 1, 0) * moveSpeed;
		camChanged = true;
	}
	if (keys[GLFW_KEY_LEFT_CONTROL]) {
		cameraPosition += camQuat * glm::vec3(0, -1, 0) * moveSpeed;
		camChanged = true;
	}

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

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	float x = (float)xpos;
	float y = (float)ypos;

	if (firstMouse) {
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	float dx = x - lastX;
	float dy = y - lastY;
	lastX = x;
	lastY = y;

	camYaw -= dx * 0.2f;
	camPitch = glm::clamp(camPitch + dy * 0.2f, -90.0f, 90.0f);

	if (camYaw > 180.0f) {
		camYaw -= 360.0f;
	}
	if (camYaw < -180.0f) {
		camYaw += 360.0f;
	}

	camQuat = glm::quat(glm::vec3(glm::radians(camPitch), glm::radians(camYaw), 0));

	glm::vec3 camForward = camQuat * glm::vec3(0, 0, 1);
	glm::vec3 camUp = camQuat * glm::vec3(0, 1, 0);
	view = glm::lookAt(cameraPosition, cameraPosition + camForward, camUp);
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
	createProgram(simpleProgram, "shaders/simpleVertexShader.shader", "shaders/fragmentShader.shader");

	//set texture channels
	glUseProgram(simpleProgram);
	glUniform1i(glGetUniformLocation(simpleProgram, "mainTex"), 0);
	glUniform1i(glGetUniformLocation(simpleProgram, "normalTex"), 1);

	createProgram(skyProgram, "shaders/skyVertex.shader", "shaders/skyFragment.shader");
	createProgram(terrainProgram, "shaders/terrainVertex.shader", "shaders/terrainFragment.shader");

	glUseProgram(terrainProgram);

	createProgram(modelProgram, "shaders/model.vs", "shaders/model.fs");

	glUseProgram(modelProgram);
	glUniform1i(glGetUniformLocation(modelProgram, "texture_diffuse1"), 0);
	glUniform1i(glGetUniformLocation(modelProgram, "texture_specular1"), 1);
	glUniform1i(glGetUniformLocation(modelProgram, "texture_normal1"), 2);
	glUniform1i(glGetUniformLocation(modelProgram, "texture_roughness1"), 3);
	glUniform1i(glGetUniformLocation(modelProgram, "texture_ao1"), 4);

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

void renderModel(Model* model, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale) {

	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glUseProgram(modelProgram);

	glm::mat4 world = glm::mat4(1.0f);
	world = glm::translate(world, pos);
	world = world * glm::toMat4(glm::quat(rot));
	world = glm::scale(world, scale);

	glUniformMatrix4fv(glGetUniformLocation(modelProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(modelProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(modelProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform3fv(glGetUniformLocation(modelProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(modelProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

	model->Draw(modelProgram);
}

//https://glad.dav1d.de/