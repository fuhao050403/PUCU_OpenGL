/*------------------------------------------------------------
* Course: Software Project on CG,VR and Image Processing 4
* Team: IMIM
* Game Name: PUCU(PlayerUnknown's ChuoUniversity)
* Chuo University
* Information and System Engineering(Makino Lab)
------------------------------------------------------------*/

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Customized classes
#include "shader.h"
#include "camera.h"
#include "model.h"

//pre-definition of functions
void processInput(GLFWwindow* window, float _movSpeed);
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void RenderCube(Shader shader);
void RenderFloor(Shader shader);
void RenderLamp(Shader shader);
void RenderSkybox(Shader shader);
unsigned int LoadTexture(const char* filepath);
unsigned int LoadCubeMapTexture(std::vector<std::string> faces);
void RenderText(Shader shader, std::string text, float x, float y, float scale, std::string font, glm::vec3 color);

//pre-define width and height of the scene would being generated
unsigned int SCREEN_WIDTH = 800;
unsigned int SCREEN_HEIGHT = 600;

//pre-define width and height of shadow frame buffer object
const unsigned int SHADOW_WIDTH = 1024;
const unsigned int SHADOW_HEIGHT = 1024;

//cursor pos(look rotation) initlization factors
Camera camera(glm::vec3(0.0f, 1.0f, 5.0f));
float lastX = SCREEN_WIDTH / 2.0f;
float lastY = SCREEN_HEIGHT / 2.0f;
bool firstMouse = true;

//Mouse button click varibles
bool isLeftMouseClicked = false;
bool isRightMouseClicked = false;

//properties of lamps in the scene
glm::vec3 lampPositions[] =
{
	glm::vec3(2.0f, 3.0f, 3.0f),
	glm::vec3(-1.0f, 2.5f, -1.5f),
	glm::vec3(0.5f, 2.0f, -2.5f)
};
const unsigned int NUMBER_OF_LAMP = 3;

//pre-define freetype class instances and structs
FT_Library ft;
FT_Face face;

struct Character
{
	unsigned int texture;
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;
};
std::map<GLchar, Character> Characters;

//VAO & VBO definition
unsigned int cubeVAO = 0, cubeVBO;
unsigned int floorVAO = 0, floorVBO;
unsigned int lampVAO = 0, lampVBO;
unsigned int textVAO, textVBO;
unsigned int skyboxVAO = 0, skyboxVBO;
unsigned int depthMapFBO[NUMBER_OF_LAMP];

//Texture objects definition
unsigned int cubeTexture = 0;
unsigned int floorTexture = 0;
unsigned int depthMap[NUMBER_OF_LAMP];

std::vector<std::string> faces
{
	//filepaths of cubemap faces
	"Resources/Textures/skybox/right.jpg",
	"Resources/Textures/skybox/left.jpg",
	"Resources/Textures/skybox/top.jpg",
	"Resources/Textures/skybox/bottom.jpg",
	"Resources/Textures/skybox/back.jpg",
	"Resources/Textures/skybox/front.jpg"
};
unsigned int skyboxTexture = 0;

//definition of varibles which performs FPS calculation
float deltaTime = 0.0f;
float lastTime = 0.0f;
float timeCounter = 0.0f;
float fps = 0.0f;

int main()
{
	if (!glfwInit()) { return -1; }

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Create glfw window with opengl property
	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Opengl win32", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//Set cursor to invisible while window is running
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader(GLADloadproc(glfwGetProcAddress)))
	{
		glfwTerminate();
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	//Enable cull face function
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//Enable freetype text rendering blend. Ohterwize text will be shown as blocks
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Assign value to instances of shader programs
	Shader cubeShader("Shaders/object.glvs", "Shaders/object.glfs");
	Shader floorShader("Shaders/object.glvs", "Shaders/object.glfs");
	Shader lampShader("Shaders/lamp.glvs", "Shaders/lamp.glfs");
	Shader shadowMapShader("Shaders/shadowMap.glvs", "Shaders/shadowMap.glfs");
	Shader skyboxShader("Shaders/cubemap.glvs", "Shaders/cubemap.glfs");
	Shader textShader("Shaders/text.glvs", "Shaders/text.glfs");

	Shader testModelShader("Shaders/object.glvs", "Shaders/object.glfs");

	//Depth map frame buffer object
	for (int i = 0; i < NUMBER_OF_LAMP; i++)
	{
		glGenTextures(1, &depthMap[i]);
		glBindTexture(GL_TEXTURE_2D, depthMap[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glGenFramebuffers(1, &depthMapFBO[i]);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap[i], 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	Model testModel("Resources/Models/backpack/backpack.obj");

	while (!glfwWindowShouldClose(window))
	{
		//invoke keyboard callback functions
		processInput(window, 0.1f);

		//setup shadow map frame buffer data
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix[NUMBER_OF_LAMP];

		lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 7.5f);

		for (int i = 0; i < NUMBER_OF_LAMP; i++)
		{
			lightView = glm::lookAt(lampPositions[i], glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			//set individual space matrix for each lamp in the scene for shadow mapping
			lightSpaceMatrix[i] = lightProjection * lightView;

			glm::mat4 shadow_cubeModel, shadow_floorModel;

			shadowMapShader.use();
			shadowMapShader.setMat4("lightSpaceMatrix", lightSpaceMatrix[i]);

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

			//bind specific depth map object before rendering the scene
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);

			//essential to claer depth buffer data otherwise depth buffer will store the depth data of last frame
			glClear(GL_DEPTH_BUFFER_BIT);

			//render scene for gaining depth data for shadow framebuffer object
			shadow_cubeModel = glm::translate(shadow_cubeModel, glm::vec3(0.0f, 0.5f, 0.0f));
			shadowMapShader.setMat4("model", shadow_cubeModel);

			glBindVertexArray(cubeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);

			shadow_floorModel = glm::translate(shadow_floorModel, glm::vec3(0.0f, 0.0f, 0.0f));
			shadowMapShader.setMat4("model", shadow_floorModel);

			glBindVertexArray(floorVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		//refresh background color buffer and depth test buffer
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

		glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Initilize matrix which send to uniforms of vertex shader
		glm::mat4 projection = glm::perspective(glm::radians(camera.Fov), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 cubeModel, floorModel, lampModel[NUMBER_OF_LAMP];

		//Rendering cube object in the scene
		cubeShader.use();
		//Setup lighting and matrix parameters
		cubeShader.setFloat("material.shininess", 64.0f);
		for (int i = 0; i < NUMBER_OF_LAMP; i++)
		{
			cubeShader.setFloat(std::string("light[" + std::to_string(i) + "].constant").c_str(), 1.0f);
			cubeShader.setFloat(std::string("light[" + std::to_string(i) + "].linear").c_str(), 0.09f);
			cubeShader.setFloat(std::string("light[" + std::to_string(i) + "].quadratic").c_str(), 0.032f);
			cubeShader.setVec3(std::string("light[" + std::to_string(i) + "].ambient").c_str(), glm::vec3(0.1f));
			cubeShader.setVec3(std::string("light[" + std::to_string(i) + "].diffuse").c_str(), glm::vec3(0.5f));
			cubeShader.setVec3(std::string("light[" + std::to_string(i) + "].specular").c_str(), glm::vec3(1.0f));
			cubeShader.setVec3(std::string("light[" + std::to_string(i) + "].lightPos").c_str(), lampPositions[i]);
		}
		cubeShader.setVec3("viewPos", camera.Position);
		cubeShader.setMat4("projection", projection);
		cubeShader.setMat4("view", view);
		for (int i = 0; i < NUMBER_OF_LAMP; i++)
		{
			cubeShader.setMat4(std::string("lightSpaceMatrix[" + std::to_string(i) + "]").c_str(), lightSpaceMatrix[i]);
		}
		cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 0.5f, 0.0f));
		cubeShader.setMat4("model", cubeModel);

		RenderCube(cubeShader);

		//Rendering model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
		testModelShader.setMat4("model", model);
		testModel.Draw(testModelShader);


		//Rendering floor object in the scene
		floorModel = glm::translate(floorModel, glm::vec3(0.0f, 0.0f, 0.0f));
		floorShader.setMat4("model", floorModel);

		RenderFloor(floorShader);

		//Rendering lamp objects in the scene
		lampShader.use();
		lampShader.setMat4("projection", projection);
		lampShader.setMat4("view", view);
		for (int i = 0; i < NUMBER_OF_LAMP; i++)
		{
			lampModel[i] = glm::translate(lampModel[i], lampPositions[i]);
			lampModel[i] = glm::scale(lampModel[i], glm::vec3(0.25f));
			lampShader.setMat4("model", lampModel[i]);

			RenderLamp(lampShader);
		}

		//Rendering cubemap skybox
		/*
		glDepthFunc(GL_LEQUAL);
		glUseProgram(skyboxShader);
		glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		view = glm::mat4(glm::mat3(glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp)));
		glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(view));

		RenderSkybox();
		*/

		//Rendering FPS text in the scene
		float currentTime = glfwGetTime();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		timeCounter += deltaTime;
		//get FPS value every 1 second
		if (timeCounter > 1.0f)
		{
			fps = 1 / deltaTime;
			timeCounter = 0.0f;
		}
		std::string str_fps = "FPS: " + std::to_string(fps);
		RenderText(textShader, str_fps, 10.0f, (float)SCREEN_HEIGHT - 22.0f, 0.3f, "Roboto", glm::vec3(1.0f));

		//Render mouse click text
		std::string str_leftMouseClick = "Left Mouse clicked";
		if (isLeftMouseClicked) { RenderText(textShader, str_leftMouseClick, 10.0f, (float)SCREEN_HEIGHT - 44.0f, 0.3f, "Roboto", glm::vec3(1.0f)); }
		std::string str_RightMouseClick = "Right Mouse clicked";
		if (isRightMouseClicked) { RenderText(textShader, str_RightMouseClick, 10.0f, (float)SCREEN_HEIGHT - 66.0f, 0.3f, "Roboto", glm::vec3(1.0f)); }

		//buffer and event manipulation every single frame
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return 0;
}

void processInput(GLFWwindow* window, float _movSpeed)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE))
	{
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}
	/*
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		cameraPos += glm::normalize(cameraUp) * _movSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		cameraPos -= glm::normalize(cameraUp) * _movSpeed;
	}
	*/
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		isLeftMouseClicked = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		isLeftMouseClicked = false;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		isRightMouseClicked = true;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		isRightMouseClicked = false;
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void RenderCube(Shader shader)
{
	if (cubeVAO == 0)
	{
		float cubeVertices[] =
		{
			-0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f,  0.0f,
			 0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f,  0.0f,
			 0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.0f,  1.0f,
			 0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.0f,  1.0f,
			-0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    0.0f,  1.0f,
			-0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f,  0.0f,

			 0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    1.0f,  0.0f,
			-0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f,  1.0f,
			-0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f,  1.0f,
			 0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    0.0f,  1.0f,
			 0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f,  0.0f,

			-0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f,  0.0f,
			-0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f,  0.0f,
			-0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f,  1.0f,
			-0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f,  1.0f,
			-0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f,  1.0f,
			-0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f,  0.0f,

			 0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f,  0.0f,
			 0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f,  0.0f,
			 0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f,  1.0f,
			 0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f,  1.0f,
			 0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f,  1.0f,
			 0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f,  0.0f,

			-0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f,  0.0f,
			 0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f,  0.0f,
			 0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f,  1.0f,
			 0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f,  1.0f,
			-0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f,  1.0f,
			-0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f,  0.0f,

			 0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f,  0.0f,
			-0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f,  1.0f,
			-0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f,  1.0f,
			 0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f,  1.0f,
			 0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f,  0.0f
		};

		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		glBindVertexArray(cubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		cubeTexture = LoadTexture("Resources/Textures/cube.png");

		shader.use();
		shader.setInt("material.diffuse", 0);

		for (int i = 0; i < NUMBER_OF_LAMP; i++)
		{
			shader.setInt(std::string("shadowMap[" + std::to_string(i) + "]").c_str(), i + 1);
		}
	}

	glBindVertexArray(cubeVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cubeTexture);
	//generate shadow map which generated from every single lamp in the scene
	for (int i = 0; i < NUMBER_OF_LAMP; i++)
	{
		glActiveTexture(GL_TEXTURE1 + i);
		glBindTexture(GL_TEXTURE_2D, depthMap[i]);
	}
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void RenderFloor(Shader shader)
{
	if (floorVAO == 0)
	{
		float floorVertices[] =
		{
			-10.0f,  0.0f,  10.0f,    0.0f,  1.0f,  0.0f,     0.0f,   0.0f,
			 10.0f,  0.0f,  10.0f,    0.0f,  1.0f,  0.0f,    10.0f,   0.0f,
			 10.0f,  0.0f, -10.0f,    0.0f,  1.0f,  0.0f,    10.0f,  10.0f,
			 10.0f,  0.0f, -10.0f,    0.0f,  1.0f,  0.0f,    10.0f,  10.0f,
			-10.0f,  0.0f, -10.0f,    0.0f,  1.0f,  0.0f,     0.0f,  10.0f,
			-10.0f,  0.0f,  10.0f,    0.0f,  1.0f,  0.0f,     0.0f,   0.0f
		};

		glGenVertexArrays(1, &floorVAO);
		glGenBuffers(1, &floorVBO);
		glBindVertexArray(floorVAO);
		glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		floorTexture = LoadTexture("Resources/Textures/floor.png");

		shader.use();
		shader.setInt("material.diffuse", 0);

		for (int i = 0; i < NUMBER_OF_LAMP; i++)
		{
			shader.setInt(std::string("shadowMap[" + std::to_string(i) + "]").c_str(), i + 1);
		}
	}

	glBindVertexArray(floorVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floorTexture);
	//generate shadow map which generated from every single lamp in the scene
	for (int i = 0; i < NUMBER_OF_LAMP; i++)
	{
		glActiveTexture(GL_TEXTURE1 + i);
		glBindTexture(GL_TEXTURE_2D, depthMap[i]);
	}
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void RenderLamp(Shader shader)
{
	if (lampVAO == 0)
	{
		float lampVertices[] =
		{
			-0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f,  0.5f,

			 0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f, -0.5f,

			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,

			 0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,

			-0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,

			 0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f
		};

		glGenVertexArrays(1, &lampVAO);
		glGenBuffers(1, &lampVBO);

		glBindVertexArray(lampVAO);
		glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices), lampVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
	}

	glBindVertexArray(lampVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void RenderSkybox(Shader shader)
{
	if (skyboxVAO == 0)
	{
		//Cubemap(skybox) VAO and VBO
		float skyboxVertices[] =
		{
			-1.0f,  1.0f, -1.0f,   -1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,    1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,   -1.0f, -1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

			 1.0f, -1.0f, -1.0f,    1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,    1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,    1.0f, -1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f,    1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f,  1.0f
		};

		glGenVertexArrays(1, &skyboxVAO);
		glGenBuffers(1, &skyboxVBO);

		glBindVertexArray(skyboxVAO);
		glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		skyboxTexture = LoadCubeMapTexture(faces);

		shader.use();
		shader.setInt("skybox", 0);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}

unsigned int LoadTexture(const char* filepath)
{
	unsigned int texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nChannel;
	unsigned char* data = stbi_load(filepath, &width, &height, &nChannel, NULL);
	if (data)
	{
		GLenum format;
		switch (nChannel)
		{
		case 1:
			format = GL_RED;
			break;
		case 3:
			format = GL_RGB;
			break;
		case 4:
			format = GL_RGBA;
			break;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "ERROR: TEXTURE FAILED TO LOAD." << std::endl;
	}

	return texture;
}

unsigned int LoadCubeMapTexture(std::vector<std::string> faces)
{
	unsigned int texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nChannel;
	for (int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nChannel, 0);
		if (data)
		{
			GLenum format;
			switch (nChannel)
			{
			case 1:
				format = GL_RED;
				break;
			case 3:
				format = GL_RGB;
				break;
			case 4:
				format = GL_RGBA;
				break;
			}

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			std::cout << "ERROR: CUBEMAP TEXTURE FAILED TO LOAD." << std::endl;
		}
		stbi_image_free(data);
	}

	return texture;
}

// project and windows configuration:
// copy freetype6.dll and zlib1.dll to system folder
// add freetype.lib to Linkder/Input of project properties
void RenderText(Shader shader, std::string text, float x, float y, float scale, std::string font, glm::vec3 color)
{
	if (textVAO == 0)
	{
		//freetype initialization
		if (FT_Init_FreeType(&ft))
		{
			std::cout << "ERROR:FREETYPE: COULD NOT INITIALIZE FREETYPE" << std::endl;
		}
		if (FT_New_Face(ft, std::string("Fonts/" + font + ".ttf").c_str(), 0, &face))
		{
			std::cout << "ERROR:FREETYPE: COULD NOT LOAD FACE" << std::endl;
		}
		FT_Set_Pixel_Sizes(face, 0, 48);
		if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
		{
			std::cout << "ERROR:FREETYPE: FAILED TO LOAD GLYPH" << std::endl;
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		for (GLubyte c = 0; c < 128; c++)
		{
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				std::cout << "ERROR:FREETYPE: FAILED TO LOAD GLYPH" << std::endl;
				continue;
			}
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			Character character =
			{
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				face->glyph->advance.x
			};
			Characters.insert(std::pair<GLchar, Character>(c, character));
		}

		//allocate vao & vbo
		glGenVertexArrays(1, &textVAO);
		glGenBuffers(1, &textVBO);
		glBindVertexArray(textVAO);
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
	}

	glm::mat4 projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT);

	shader.use();
	shader.setMat4("projection", projection);
	shader.setVec3("textColor", color);

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		Character ch = Characters[*c];

		float xpos = x + ch.bearing.x * scale;
		float ypos = y - (ch.size.y - ch.bearing.y) * scale;

		float w = ch.size.x * scale;
		float h = ch.size.y * scale;

		float vertices[6][4] =
		{
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};
		glBindTexture(GL_TEXTURE_2D, ch.texture);
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		x += (ch.advance >> 6) * scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}