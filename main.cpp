#include<iostream>
#include"Headers/JGL.h"
#include<cmath>
#include<stdexcept>
#include<string>
#include<vector>

namespace {

	constexpr int ballSegments = 40; // Number of segments to approximate the ball
	constexpr float pi = 3.14159265358979323846f; // Value of pi
	constexpr float pointRadius = 0.03f; // Radius of the point mass
	constexpr float restitution = 0.4f; // Coefficient of restitution
	constexpr float gravity = -1.8f; // Gravity acceleration
	constexpr float stiffness = 400.0f; // Stiffness for collision response
	constexpr float damping = 4.0f;

	struct PointMass {
		float x, y; // Position of the point mass
		float vx, vy; // Velocity of the point mass
		float mass; // Mass of the point mass
	};

	struct Spring {
		int a, b;
		float restLength;
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

	float distance(const PointMass& a, const PointMass& b) {
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		return sqrtf(dx * dx + dy * dy);
	}

	void applySpringForce(std::vector<PointMass>& points, const Spring& spring, float deltaTime) {
		PointMass& a = points[spring.a];
		PointMass& b = points[spring.b];

		float dist = distance(a, b);
		if (dist < 1e-6f) return;

		Vector2 dir = { (b.x - a.x) / dist, (b.y - a.y) / dist };
		float stretch = dist - spring.restLength;
		float relVelAlongDir = (b.vx - a.x) * dir.x + (b.y - a.y) * dir.y;
		float forceMagnitude = stiffness * stretch + damping * relVelAlongDir;

		float forceX = dir.x * forceMagnitude;
		float forceY = dir.y * forceMagnitude;

		a.vx += forceX / a.mass * deltaTime;
		a.vy += forceY / a.mass * deltaTime;
		b.vx -= forceX / b.mass * deltaTime;
		b.vy -= forceY / b.mass * deltaTime;
	}

	void applyGravity(PointMass& point, float deltaTime) {
		point.vy += gravity * deltaTime;
	}

	void updatePoint(PointMass& point, float deltaTime) {
		point.x += point.vx * deltaTime;
		point.y += point.vy * deltaTime;
	}

	void resolveWallCollision(PointMass& point) {
		if (point.x - pointRadius < -1.0f) {
			point.x = -1.0f + pointRadius;
			point.vx = -point.vx;
		}
		if (point.x + pointRadius > 1.0f) {
			point.x = 1.0f - pointRadius;
			point.vx = -point.vx;
		}
		if (point.y - pointRadius < -1.0f) {
			point.y = -1.0f + pointRadius;
			point.vy = -point.vy * restitution;
		}
		if (point.y + pointRadius > 1.0f) {
			point.y = 1.0f - pointRadius;
			point.vy = -point.vy;
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

	GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

	std::vector<float> circleVerts = generateCircleVertices(pointRadius, ballSegments);
	GLuint circleVBO = 0;
	GLuint circleVAO = createDynamicVao(circleVBO);
	GLsizei circleVertexCount = static_cast<GLsizei>(circleVerts.size() / 2);

	GLuint lineVBO = 0;
	GLuint lineVAO = createDynamicVao(lineVBO);

	std::vector<PointMass> points{
		{-0.2621f, 0.7063f, 0.0f, 0.0f, 1.0f},
		{0.1063f, 0.8621f, 0.0f, 0.0f, 1.0f},
		{0.2621f, 0.4937f, 0.0f, 0.0f, 1.0f},
		{-0.1063f, 0.3379f, 0.0f, 0.0f, 1.0f},
	};
	
	std::vector<Spring> springs = {
		{0, 1, distance(points[0], points[1])},
		{1, 2, distance(points[1], points[2])},
		{2, 3, distance(points[2], points[3])},
		{3, 0, distance(points[3], points[0])},
		{0, 2, distance(points[0], points[2])},
		{1, 3, distance(points[1], points[3])},
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

		for (const Spring& spring : springs) {
			applySpringForce(points, spring, deltaTime);
		}

		for (PointMass& point : points) {
			applyGravity(point, deltaTime);
			updatePoint(point, deltaTime);
			resolveWallCollision(point);
		}

		/* Render here */

		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(shaderProgram);

		std::vector<float> lineVerts;
		for (const Spring& spring : springs) {
			lineVerts.push_back(points[spring.a].x);
			lineVerts.push_back(points[spring.a].y);
			lineVerts.push_back(points[spring.b].x);
			lineVerts.push_back(points[spring.b].y);
		}
		uploadDynamic(lineVBO, lineVerts);
		glUniform4f(colorLoc, 0.31f, 0.76f, 0.97f, 1.0f);
		glBindVertexArray(lineVAO);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVerts.size() / 2));


		glBindVertexArray(circleVAO);
		for (const PointMass& point : points) {
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