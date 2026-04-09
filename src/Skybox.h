#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include <glad/glad.h>

class Program;

/** Fullscreen-direction cube + equirectangular 2D texture (same mapping as CSC471 skybox). */
class Skybox {
public:
	Skybox();
	~Skybox();

	bool init(const std::string &resourceDir, const std::string &equirectFile);

	void draw(const glm::mat4 &P, const glm::mat4 &viewRotationOnly);

private:
	std::shared_ptr<Program> prog_;
	GLuint tex_ = 0;
	GLuint vao_ = 0;
	GLuint vbo_ = 0;
};
