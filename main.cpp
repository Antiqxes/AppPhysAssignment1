#include<iostream>
#include"Headers/JGL.h"
#include<cmath>
#include<stdexcept>
#include<string>
#include<vector>

namespace {

	constexpr int ballSegments = 40;
	constexpr float pi = 3.14159265358979323846f;
	constexpr float pointRadius = 0.03f;

	constexpr float gravity = -1.8f;
	constexpr float dt = 1.0f / 120.0f;


	struct Point {
		float x, y, px, py;
	};

	struct Vector2 {
		float x, y;
	};

	constexpr const char* kVertexShaderSource = R"(#version 330 core
layout (location = 0) in vec2 aPos;

void main() { gl_Position = vec4(aPos, 0.0, 1.0); }
)";

	constexpr const char* kFragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
void main() { FragColor = uColor; }
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

	GLuint createDynamicVao(GLuint& vbo){
		GLuint vao = 0;

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		return vao;
	}

	void uploadDynamic(GLuint vbo, const std::vector<float>& data) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void updatePoint(Point& point, float dt) {
		float oldX = point.x, oldY = point.y;
		point.x += point.x - point.px;
		point.y += point.y - point.py + gravity * dt * dt;
		point.px = oldX;
		point.py = oldY;
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

	GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

	std::vector<float> circleVerts = generateCircleVertices(pointRadius, ballSegments);
	GLuint circleVBO = 0;
	GLuint circleVAO = createDynamicVao(circleVBO);
	GLsizei circleVertexCount = static_cast<GLsizei>(circleVerts.size() / 2);

	std::vector<Point> points = { {0.0f, 0.8f, 0.0f, 0.8f} };

	glClearColor(0.0f, 0.2f, 0.0f, 1.0f);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {

		glfwSetKeyCallback(window, glfwKeyCallbackJ); // Set the key callback for the window

		for (Point& point : points){
			updatePoint(point, dt);
		}

		/* Render here */

		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(shaderProgram);

		glBindVertexArray(circleVAO);
		for (const Point& point : points) {
			std::vector<float> translated = circleVerts;
			for (size_t i = 0; i < translated.size(); i += 2) {
				translated[i] += point.x;
				translated[i + 1] += point.y;
			}
			uploadDynamic(circleVBO, translated);
			glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
			glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);
		}

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

	glDeleteVertexArrays(1, &circleVAO);
	glDeleteBuffers(1, &circleVBO);
	glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}