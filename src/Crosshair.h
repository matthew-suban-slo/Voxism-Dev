#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "Program.h"

class Crosshair {
public:
	Crosshair() = default;
	~Crosshair();

	bool init(const std::string &resourceDirectory);
	void draw(int width, int height);

	void setVisible(bool v) { visible_ = v; }
	void setSize(float s) { sizePx_ = s; }
	void setThickness(float t) { thicknessPx_ = t; }
	void setGap(float g) { gapPx_ = g; }
	void setColor(const glm::vec3 &c) { color_ = c; }

private:
	void buildGeometry(int width, int height);

private:
	std::shared_ptr<Program> prog_;
	GLuint vao_ = 0;
	GLuint vbo_ = 0;

	bool visible_ = true;
	float sizePx_ = 8.0f;
	float thicknessPx_ = 2.0f;
	float gapPx_ = 5.0f;
	glm::vec3 color_ = glm::vec3(1.0f, 1.0f, 1.0f);
	int cachedW_ = 0;
	int cachedH_ = 0;
};