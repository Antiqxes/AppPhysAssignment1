#include<iostream>
#include"Headers/JGL.h"
#include<cmath>
#include<stdexcept>
#include<string>
#include<vector>

namespace {
	constexpr float ballRadius = 0.1f; // Radius of the ball
	constexpr int ballSegments = 40; // Number of segments to approximate the ball
	constexpr float pi = 3.14159265358979323846f; // Value of pi
	constexpr float gravity = -1.8f; // Gravity acceleration

	struct Ball {
		float x, y; // Position of the ball
		float vx, vy; // Velocity of the ball
		float radius; // Radius of the ball
		float restitution;
	};

	constexpr const char* kVertexShaderSource = R"(#version 330 core
layout (location = 0) in vec2 aPos;
uniform vec2 uCenter;
void main() { gl_Position = vec4(aPos + uCenter, 0.0, 1.0); }
)";

	constexpr const char* kFragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(0.85, 0.85, 0.9, 1.0); }
)";

	GLuint compileShader(GLenum type, const char* source) {
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
			throw std::runtime_error("Shader compilation failed");
		}
		return shader;
	}

	GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
		GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
		GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

		GLuint shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		GLint success;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
			std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			throw std::runtime_error("Shader program linking failed");
		}
		return shaderProgram;
	}

	std::vector<float> generateCircleVertices(float radius, int segments) {
		std::vector<float> vertices;
		vertices.push_back(0.0f); // Center x
		vertices.push_back(0.0f); // Center y

		for (int i = 0; i <= segments; ++i) {
			//float angle = (float)i / segments * 2.0f * pi;
			float angle = 2.0f * pi * i / segments;
			vertices.push_back(radius * cos(angle));
			vertices.push_back(radius * sin(angle));
		}
		return vertices;
	}

	struct CircleMesh {
		GLuint vao = 0;
		GLuint vbo = 0;
		GLsizei vertexCount = 0;
	};

	CircleMesh createCircleMesh(float radius, int segments) {
		CircleMesh mesh;
		std::vector<float> vertices = generateCircleVertices(radius, segments);
		mesh.vertexCount = static_cast<GLsizei>(vertices.size() / 2);

		glGenVertexArrays(1, &mesh.vao);
		glGenBuffers(1, &mesh.vbo);

		glBindVertexArray(mesh.vao);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		return mesh;
	}

	void drawBall(GLuint shaderProgram, const CircleMesh& mesh, const Ball& ball) {
		glUseProgram(shaderProgram);
		glUniform2f(glGetUniformLocation(shaderProgram, "uCenter"), ball.x, ball.y);
		glBindVertexArray(mesh.vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, mesh.vertexCount);
		glBindVertexArray(0);
	}

	void applyGravity(Ball& ball, float deltaTime) {
		ball.vy += gravity * deltaTime;
	}

	void updateBall(Ball& ball, float deltaTime) {
		ball.x += ball.vx * deltaTime;
		ball.y += ball.vy * deltaTime;
	}

	void resolveWallCollision(Ball& ball) {
		if (ball.x - ball.radius < -1.0f) {
			ball.x = -1.0f + ball.radius;
			ball.vx = -ball.vx;
		}
		if (ball.x + ball.radius > 1.0f) {
			ball.x = 1.0f - ball.radius;
			ball.vx = -ball.vx;
		}
		if (ball.y - ball.radius < -1.0f) {
			ball.y = -1.0f + ball.radius;
			ball.vy = -ball.vy * ball.restitution;
		}
		if (ball.y + ball.radius > 1.0f) {
			ball.y = 1.0f - ball.radius;
			ball.vy = -ball.vy;
		}
	}

	

} // namespace

int main(void)
{
	setOpenGLContextVersionJ(3, 3); // Set OpenGL context version to 3.3

    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(600, 600, "BartulinAppPhysAssignment2", NULL, NULL);
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

	GLuint shaderProgram = createShaderProgram(kVertexShaderSource, kFragmentShaderSource);
	CircleMesh mesh = createCircleMesh(ballRadius, ballSegments);
	std::vector<Ball> balls = {
			{-0.5f, 0.3f, 0.6f, 0.4f, ballRadius, 0.75},
	};

	glClearColor(0.0f, 0.2f, 0.0f, 1.0f);

	float lastFrameTime = static_cast<float>(glfwGetTime());

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
		float currentFrameTime = static_cast<float>(glfwGetTime());
		float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		glfwSetKeyCallback(window, glfwKeyCallbackJ); // Set the key callback for the window

		for (Ball& ball : balls) {
			applyGravity(ball, deltaTime);
			updateBall(ball, deltaTime);
			resolveWallCollision(ball);
		}

		/* Render here */

		glClear(GL_COLOR_BUFFER_BIT);
		for (const Ball& ball : balls) {
			drawBall(shaderProgram, mesh, ball);
		}

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(1, &mesh.vbo);
	glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}