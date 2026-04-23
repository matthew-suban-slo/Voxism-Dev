#include "Crosshair.h"

#include <vector>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>

static std::string shaderPath(const std::string &resourceDir,
                              const std::string &category,
                              const std::string &filename)
{
	return resourceDir + "/shaders/" + category + "/" + filename;
}

Crosshair::~Crosshair()
{
	if (vao_)
		glDeleteVertexArrays(1, &vao_);
	if (vbo_)
		glDeleteBuffers(1, &vbo_);
}

bool Crosshair::init(const std::string &resourceDirectory)
{
	prog_ = std::make_shared<Program>();
	prog_->setVerbose(true);
	prog_->setShaderNames(
		shaderPath(resourceDirectory, "ui", "crosshair_vert.glsl"),
		shaderPath(resourceDirectory, "ui", "crosshair_frag.glsl")
	);
	if (!prog_->init()) {
		std::cerr << "Crosshair shader failed" << std::endl;
		return false;
	}

	prog_->addAttribute("vertPos");
	prog_->addUniform("crossColor");

	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);

	return true;
}

void Crosshair::buildGeometry(int width, int height)
{
	cachedW_ = width;
	cachedH_ = height;

	float cx = width * 0.5f;
	float cy = height * 0.5f;
	float g = gapPx_;
	float s = sizePx_;
	float t = thicknessPx_ * 0.5f;

	auto pxToNdcX = [width](float x) {
		return (x / (float)width) * 2.0f - 1.0f;
	};
	auto pxToNdcY = [height](float y) {
		return 1.0f - (y / (float)height) * 2.0f;
	};

	std::vector<float> verts;

	auto addRect = [&](float x0, float y0, float x1, float y1) {
		float ax = pxToNdcX(x0), ay = pxToNdcY(y0);
		float bx = pxToNdcX(x1), by = pxToNdcY(y1);

		verts.insert(verts.end(), {
			ax, ay,  bx, ay,  bx, by,
			ax, ay,  bx, by,  ax, by
		});
	};

	// top
	addRect(cx - t, cy - g - s, cx + t, cy - g);
	// bottom
	addRect(cx - t, cy + g, cx + t, cy + g + s);
	// left
	addRect(cx - g - s, cy - t, cx - g, cy + t);
	// right
	addRect(cx + g, cy - t, cx + g + s, cy + t);

	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verts.size(), verts.data(), GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Crosshair::draw(int width, int height)
{
	if (!visible_ || !prog_ || width <= 0 || height <= 0)
		return;

	if (width != cachedW_ || height != cachedH_)
		buildGeometry(width, height);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	prog_->bind();
	glUniform3fv(prog_->getUniform("crossColor"), 1, glm::value_ptr(color_));

	glBindVertexArray(vao_);
	glDrawArrays(GL_TRIANGLES, 0, 24);
	glBindVertexArray(0);

	prog_->unbind();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}