#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#include <glad/glad.h>		
#include <GLFW/glfw3.h>		
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <vector>

#include "cCamera.h"
#include "cGameObject.h"
#include "cMeshTypeManager.h"
#include "cShaderManager.h"
#include "vert_XYZ_RGB.h"

// Redefine the new operator for memory leak checking
#ifdef _DEBUG
	#ifndef DBG_NEW
		#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
		#define new DBG_NEW
	#endif
#endif

// OpenGL related variables
GLFWwindow* window;
GLint mvp_location;
GLuint shadProgID;
glm::mat4x4 p, v;
GLuint UniformLoc_ID_objectColour = 0;
GLuint UniformLoc_ID_isWireframe = 0;

// Custom objects
std::vector< cGameObject* > g_vec_pGOs;
cMeshTypeManager* g_pTheMeshTypeManager = 0;
cShaderManager* g_pTheShaderManager = 0;
cCamera g_Camera(
	0.6f, 1200.f / 800.f, 0.01f, 100.0f,
	
	glm::vec3(0.0f, 12.5f, 25.0f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f)
);

// Method declarations
void DrawScene();
void ImportScene();
void InitWindow();

// Callback functions
static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);

	return;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	float g_CameraMovementSpeed = 0.1f;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	switch (key) {
	case GLFW_KEY_A:
		::g_Camera.eye.x -= g_CameraMovementSpeed; // "Left"
		break;
	case GLFW_KEY_D:
		::g_Camera.eye.x += g_CameraMovementSpeed; // "Right"
		break;
	case GLFW_KEY_W:
		::g_Camera.eye.z -= g_CameraMovementSpeed; // "Forward"
		break;
	case GLFW_KEY_S:
		::g_Camera.eye.z += g_CameraMovementSpeed; // "Back"
		break;
	case GLFW_KEY_Q:
		::g_Camera.eye.y -= g_CameraMovementSpeed; // "Down"
		break;
	case GLFW_KEY_E:
		::g_Camera.eye.y += g_CameraMovementSpeed; // "Up"
		break;
	}

	std::stringstream ssTitle;
	ssTitle << "Camera: " << ::g_Camera.eye.x << ", " << ::g_Camera.eye.y << ", " << ::g_Camera.eye.z;
	glfwSetWindowTitle(window, ssTitle.str().c_str());

	return;
}

int main(void) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Initialize the GLFW window
	InitWindow();

	// Display information about system
	std::cout << glGetString(GL_VENDOR) << " " << glGetString(GL_RENDERER) << ", " << glGetString(GL_VERSION) << std::endl;
	std::cout << "Shader language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	// Create shader objects
	::g_pTheShaderManager = new cShaderManager();
	cShaderManager::cShader verShader;
	cShaderManager::cShader fragShader;

	// Import the shaders from files
	verShader.fileName = "simpleVert.glsl";
	fragShader.fileName = "simpleFrag.glsl";
	if (!::g_pTheShaderManager->createProgramFromFile("simple", verShader, fragShader)) {
		std::cout << ::g_pTheShaderManager->getLastError();
		return -1;
	}

	// Setup the mesh object to mediate importing mesh from files
	::g_pTheMeshTypeManager = new cMeshTypeManager();
	shadProgID = ::g_pTheShaderManager->getIDFromFriendlyName("simple");

	// Read the scene description from a file
	ImportScene();

	// Get uniform locations from the shader
	mvp_location = glGetUniformLocation(shadProgID, "MVP");
	UniformLoc_ID_objectColour = glGetUniformLocation(shadProgID, "objectColour");
	UniformLoc_ID_isWireframe = glGetUniformLocation(shadProgID, "isWireframe");

	// Main application loop
	while (!glfwWindowShouldClose(window)) {
		// Update the apect ratio of the camera based on window size
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		::g_Camera.aspect = width / (float)height;

		// Reset the viewport and clear the screen
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Calculate the projection matrix and the view matrix
		p = glm::perspective(::g_Camera.fovy, ::g_Camera.aspect, ::g_Camera.zNear, ::g_Camera.zFar);
		v = glm::lookAt(::g_Camera.eye, ::g_Camera.center, ::g_Camera.up);

		// Render loop
		DrawScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup objects
	delete ::g_pTheMeshTypeManager;
	delete ::g_pTheShaderManager;

	for (int i = 0; i < ::g_vec_pGOs.size(); ++i) {
		delete ::g_vec_pGOs[i];
	}

	// Final exit commands
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

// Update the scene for the next frame
void DrawScene() {
	// Loop through the object array
	for (std::vector<cGameObject*>::iterator itGO = ::g_vec_pGOs.begin(); itGO != ::g_vec_pGOs.end(); ++itGO) {
		std::string meshModelName = (*itGO)->meshName;

		// Get the mesh info for the current object
		GLuint VAO_ID = 0;
		int numberOfIndices = 0;
		float unitScale = 1.0f;
		if (!::g_pTheMeshTypeManager->LookUpMeshInfo(meshModelName, VAO_ID, numberOfIndices, unitScale)) {
			continue;
		}

		// Setup the model * view * projection matrix and model matrix
		glm::mat4x4 mvp(1.0f);
		glm::mat4x4 m = glm::mat4x4(1.0f);

		// Pre-rotation
		m = glm::rotate(m, (*itGO)->pre_Rot_X, glm::vec3(1.0f, 0.0f, 0.0f));
		m = glm::rotate(m, (*itGO)->pre_Rot_Y, glm::vec3(0.0f, 1.0f, 0.0f));
		m = glm::rotate(m, (*itGO)->pre_Rot_Z, glm::vec3(0.0f, 0.0f, 1.0f));

		// Translation
		m = glm::translate(m, glm::vec3((*itGO)->x, (*itGO)->y, (*itGO)->z));

		// Post-rotation
		m = glm::rotate(m, (*itGO)->post_Rot_X, glm::vec3(1.0f, 0.0f, 0.0f));
		m = glm::rotate(m, (*itGO)->post_Rot_Y, glm::vec3(0.0f, 1.0f, 0.0f));
		m = glm::rotate(m, (*itGO)->post_Rot_Z, glm::vec3(0.0f, 0.0f, 1.0f));

		float actualScale = (*itGO)->scale * unitScale;

		// Scale
		m = glm::scale(m, glm::vec3(actualScale, actualScale, actualScale));

		// Determine if object is to be drawn as a wireframe
		if ((*itGO)->bIsWireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDisable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
		}
		else {
			glCullFace(GL_BACK);
			glEnable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		// Calculate model * view * projection matrix
		mvp = p * v * m;

		// Update shader information
		::g_pTheShaderManager->useShaderProgram("simple");

		glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)glm::value_ptr(mvp));
		glUniform3f(UniformLoc_ID_objectColour, (*itGO)->solid_R, (*itGO)->solid_G, (*itGO)->solid_B);

		if ((*itGO)->bIsWireframe) {
			glUniform1i(UniformLoc_ID_isWireframe, TRUE);
		}
		else {
			glUniform1i(UniformLoc_ID_isWireframe, FALSE);
		}

		glBindVertexArray(VAO_ID);
		glDrawElements(GL_TRIANGLES, numberOfIndices, GL_UNSIGNED_INT, (GLvoid*)0);
		glBindVertexArray(0);
	}

	return;
}

// Read the scene description from a file
void ImportScene() {
	// Array of strings to read in the object attributes
	std::string line;
	std::getline(std::cin, line); // Header

	while (std::getline(std::cin, line)) {
		cGameObject* pObject = new cGameObject();

		std::stringstream sstream(line);
		sstream >>
			pObject->meshName >>
			pObject->scale >>
			pObject->x >>
			pObject->y >>
			pObject->z >>
			pObject->solid_R >>
			pObject->solid_G >>
			pObject->solid_B >>
			pObject->post_Rot_X >>
			pObject->post_Rot_Y >>
			pObject->post_Rot_Z >>
			//pObject->bIsUpdatedByCollision >>
			pObject->bIsUpdatedByPhysics >>
			pObject->bIsWireframe;

		// Get the mesh name and determine if it still needs to be imported
		GLuint VAO_ID = 0;
		int numberOfIndices = 0;
		float unitScale = 1.0f;
		if (!::g_pTheMeshTypeManager->LookUpMeshInfo(pObject->meshName, VAO_ID, numberOfIndices, unitScale)) {
			::g_pTheMeshTypeManager->LoadPlyFileIntoGLBuffer(shadProgID, pObject->meshName);
		}

		// Add the resulting object into the vector of all objects
		::g_vec_pGOs.push_back(pObject);
	}
	// Cleanup the attribute array

	return;
}

// GLFW window creation
void InitWindow() {
	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	window = glfwCreateWindow(1200, 800, "Simple example", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);

	return;
}