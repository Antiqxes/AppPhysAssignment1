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
	constexpr int iterations = 5;


	struct Point {
		float x, y, px, py;
		bool pinned = false;
	};

	struct Vector2 {
		float x, y;
	};

	struct Stick {
		int a, b;
		float restLength;
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

	float distance(const Point& a, const Point& b) {
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		return std::sqrt(dx*dx+dy*dy);
	}

	Point makePoint(float x, float y) {
		return { x, y, x, y, false };
	}

	void updatePoint(Point& point, float dt) {
		if (point.pinned) return;
		float oldX = point.x, oldY = point.y;
		point.x += point.x - point.px;
		point.y += point.y - point.py + gravity * dt * dt;
		point.px = oldX;
		point.py = oldY;
	}

	void applyStick(std::vector<Point>& points, const Stick& stick) {
		Point& a = points[stick.a];
		Point& b = points[stick.b];
		float dist = distance(a, b);
		if (dist < 1e-6f) return;
		float error = dist - stick.restLength;
		Vector2 dir = { (b.x - a.x) / dist, (b.y - a.y) / dist };
		float wA = a.pinned ? 0.0f : (b.pinned ? 1.0f : 0.5f);
		float wB = b.pinned ? 0.0f : (a.pinned ? 1.0f : 0.5f);
		a.x += dir.x * error * wA;
		a.y += dir.y * error * wA;
		b.x -= dir.x * error * wB;
		b.y -= dir.y * error * wB;
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
	GLuint lineVBO = 0;
	GLuint lineVAO = createDynamicVao(lineVBO);

	std::vector<Point> points = { 
		makePoint(0.00f,  0.80f), makePoint(0.00f,  0.65f),
		makePoint(0.00f,  0.25f), makePoint(-0.12f,  0.50f),
		makePoint(-0.20f,  0.35f), makePoint(0.12f,  0.50f),
		makePoint(0.20f,  0.35f), makePoint(-0.07f,  0.05f),
		makePoint(-0.10f, -0.15f), makePoint(0.07f,  0.05f),
		makePoint(0.10f, -0.15f),
	};

	std::vector<Stick> sticks = {
		{0, 1, distance(points[0], points[1])}, {1, 2, distance(points[1], points[2])},
		{1, 3, distance(points[1], points[3])}, {3, 4, distance(points[3], points[4])},
		{1, 5, distance(points[1], points[5])}, {5, 6, distance(points[5], points[6])},
		{2, 7, distance(points[2], points[7])}, {7, 8, distance(points[7], points[8])},
		{2, 9, distance(points[2], points[9])}, {9, 10, distance(points[9], points[10])},
	};
	points[0].pinned = true;

	glClearColor(0.0f, 0.2f, 0.0f, 1.0f);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {

		glfwSetKeyCallback(window, glfwKeyCallbackJ); // Set the key callback for the window

		for (Point& point : points){
			updatePoint(point, dt);
		}

		for (int i = 0; i < iterations; ++i) {
			for (const Stick& stick : sticks) {
				applyStick(points, stick);
			}
		}

		/* Render here */

		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(shaderProgram);

		std::vector<float> lineVerts;
		for (const Stick& stick : sticks) {
			lineVerts.push_back(points[stick.a].x);
			lineVerts.push_back(points[stick.a].y);
			lineVerts.push_back(points[stick.b].x);
			lineVerts.push_back(points[stick.b].y);
		}
		uploadDynamic(lineVBO, lineVerts);
		glUniform4f(colorLoc, 0.31f, 0.76f, 0.97f, 1.0f);
		glBindVertexArray(lineVAO);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVerts.size() / 2));

		glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
		glBindVertexArray(circleVAO);
		for (const Point& point : points) {
			std::vector<float> translated = circleVerts;
			for (size_t i = 0; i < translated.size(); i += 2) {
				translated[i] += point.x;
				translated[i + 1] += point.y;
			}
			uploadDynamic(circleVBO, translated);
			
			glDrawArrays(GL_TRIANGLE_FAN, 0, circleVertexCount);
		}

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

	glDeleteVertexArrays(1, &circleVAO);
	glDeleteBuffers(1, &circleVBO);
	glDeleteVertexArrays(1, &lineVAO);
	glDeleteBuffers(1, &lineVBO);
	glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}