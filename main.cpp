#include<iostream>
#include"Headers/JGL.h"

int main(void)
{
	setOpenGLContextVersionJ(3, 3); // Set OpenGL context version to 3.3

    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(600, 600, "BartulinAppPhysAssignment1", NULL, NULL);
	// check if the window was created successfully
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
	//set the current context to the window we just created
    glfwMakeContextCurrent(window);

	// Initialize GLAD before calling any OpenGL functions
    initializeGLADJ();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glfwSetKeyCallback(window, glfwKeyCallbackJ); // Set the key callback for the window

        /* Render here */
		glClearColor(0.0f, 0.2f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}