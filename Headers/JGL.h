#pragma once

#include<glad/glad.h>
#include <GLFW/glfw3.h>
#include<iostream>

//make functions in this header file instead of main

void setOpenGLContextVersionJ(int major, int minor) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void initializeGLADJ() {
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		exit(-1);
	}
}

void glfwKeyCallbackJ(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // GLFW automatically populates 'key' and 'action' based on what the user pressed
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // You can easily add more user inputs here later:
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        // Move player forward, etc.
    }
}