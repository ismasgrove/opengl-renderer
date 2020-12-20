#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <glm/matrix.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Light.h"
#include "Model.h"

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = false;

glm::vec3 lightPos;
glm::vec3 lightColor;

void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadTexture(char const* path);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "neo stormtrooper x miku", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glEnable(GL_DEPTH_TEST);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	stbi_set_flip_vertically_on_load(true);

	//Shader shader("shader.vert", "shader.frag");
	Shader lightShader("light.vert", "light.frag");
	Shader meshShader("mesh.vert", "mesh.frag");
	Model miku("dancing-anime/source/Samba.fbx");
	Model stormtrooper("dancing-stormtrooper/source/silly_dancing.fbx");
	//Model backpack("backpack/backpack.obj");

	float vertices[] = {
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
	};
	unsigned int VBO, cubeVAO, lightVAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &cubeVAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	//
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	Dirlight dirlight = {
		glm::vec3(-0.2f, -1.0f, -0.3f),
		glm::vec3(0.1f, 0.1f, 0.1f),
		glm::vec3(0.8f, 0.8f, 0.8f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f)
	};
	
	Spotlight spotlight = {
		camera.Position,
		camera.Front,
		glm::vec3(0.0f, 0.0f, 0.0f), // ambient
		glm::vec3(0.8f, 0.8f, 0.8f), // diffuse
		glm::vec3(1.0f, 1.0f, 1.0f), // specular
		glm::vec3(1.0f), // color
		glm::cos(glm::radians(2.5f)), // inner cutoff
		glm::cos(glm::radians(5.0f)), // outer cutoff
		1.0f, // constant
		0.09f, // linear
		0.032f // quadratic
	};

	std::vector<Pointlight> pointlights = {
		{/*
			glm::vec3(4.0f, 5.0f, 0.0f), // position
			glm::vec3(0.2f, 0.2f, 0.2f), // ambient
			glm::vec3(0.5f, 0.5f, 0.5f), // diffuse
			glm::vec3(1.0f, 1.0f, 1.0f), // specular
			glm::vec3(0.6f, 0.0f, 0.6f), // color
			1.0f, // constant
			0.1f, // linear
			0.032f // quadratic
		},*/
		{
			glm::vec3(0.0f, 2.0f, -20.0f), // position
			glm::vec3(0.2f, 0.2f, 0.2f), // ambient
			glm::vec3(0.7f, 0.7f, 0.7f), // diffuse
			glm::vec3(1.0f, 1.0f, 1.0f), // specular
			glm::vec3(1.0f), // color
			1.0f, // constant
			0.1f, // linear
			0.032f // quadratic
		}/*,
		{
			glm::vec3(20.0f, 5.0f, -5.0f), // position
			glm::vec3(0.2f, 0.2f, 0.2f), // ambient
			glm::vec3(0.5f, 0.5f, 0.5f), // diffuse
			glm::vec3(1.0f, 1.0f, 1.0f), // specular
			glm::vec3(0.2f, 1.0f, 0.0f), // color
			0.5f, // constant
			0.1f, // linear
			0.032f // quadratic
		},
		{
			glm::vec3(-20.0f, -2.0f, -5.0f), // position
			glm::vec3(0.2f, 0.2f, 0.2f), // ambient
			glm::vec3(0.5f, 0.5f, 0.5f), // diffuse
			glm::vec3(1.0f, 1.0f, 1.0f), // specular
			glm::vec3(0.2f, 1.0f, 0.0f), // color
			1.0f, // constant
			0.1f, // linear
			0.032f
		}*/
		}
	};

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		//
		glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		spotlight.position = camera.Position;
		spotlight.direction = camera.Front;

		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
		glm::mat4 model = glm::mat4(1.0f);
		view = camera.GetViewMatrix();

		lightShader.use();
		for (auto& s : pointlights) {
			lightShader.setMat4("projection", projection);
			lightShader.setMat4("view", view);

			model = glm::mat4(1.0f);
			lightShader.setVec3("color", s.color);
			model = glm::translate(model, s.position);
			model = glm::scale(model, glm::vec3(10.0f));
			lightShader.setMat4("model", model);

			glBindVertexArray(lightVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		meshShader.use();
		meshShader.setFloat("material.shininess", 64.0f);
		meshShader.setDirectionalLight("dirlight", dirlight);

		for (size_t i = 0; i < pointlights.size(); i++)
		{
			meshShader.setPointLight("pointlights[" + std::to_string(i) + "]", pointlights[i]);
		}
		meshShader.setSpotLight("spotlight", spotlight);

		meshShader.setMat4("projection", projection);
		meshShader.setMat4("view", view);
		meshShader.setVec3("viewPos", camera.Position);
		meshShader.setFloat("light.constant", 1.0f);
		meshShader.setFloat("light.constant", 0.09f);
		meshShader.setFloat("light.quadratic", 0.032f);
		meshShader.setMat4("projection", projection);
		meshShader.setMat4("view", view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, -2.0f, -4.0f));
		model = glm::scale(model, glm::vec3(2.0f));
		meshShader.setMat4("model", model);
		miku.Draw(meshShader);
		model = glm::scale(model, glm::vec3(0.5f));
		model = glm::translate(model, glm::vec3(-4.0f, 0.0f, 0.0f));
		meshShader.setMat4("model", model);
		//stormtrooper.Draw(meshShader);
		model = glm::translate(model, glm::vec3(-1.0f, 2.0f, 0.0f));
		//backpack.Draw(meshShader);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	const float cameraSpeed = 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera_Movement::DOWN, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;
	
	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}