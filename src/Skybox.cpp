#include "Skybox.h"

#include "Program.h"

#include <iostream>

#include "stb_image.h"

Skybox::Skybox() = default;

Skybox::~Skybox()
{
	if (vbo_)
		glDeleteBuffers(1, &vbo_);
	if (vao_)
		glDeleteVertexArrays(1, &vao_);
	if (tex_)
		glDeleteTextures(1, &tex_);
}

bool Skybox::init(const std::string &resourceDir, const std::string &equirectFile)
{
	std::string path = resourceDir + "/" + equirectFile;
	int w = 0, h = 0, n = 0;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(path.c_str(), &w, &h, &n, 3);
	if (!data) {
		std::cerr << "Skybox: failed to load " << path << std::endl;
		return false;
	}

	glGenTextures(1, &tex_);
	glBindTexture(GL_TEXTURE_2D, tex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);

	prog_ = std::make_shared<Program>();
	prog_->setVerbose(true);
	prog_->setShaderNames(resourceDir + "/shaders/skybox/skybox_vert.glsl", resourceDir + "/shaders/skybox/skybox_frag.glsl");
	if (!prog_->init())
		return false;
	prog_->addUniform("P");
	prog_->addUniform("V");
	prog_->addUniform("skybox");

	float skyboxVertices[] = {
		-1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
		-1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
		1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
	};

	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glBindVertexArray(0);

	return true;
}

void Skybox::draw(const glm::mat4 &P, const glm::mat4 &viewRotationOnly)
{
	if (!tex_ || !prog_)
		return;
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	prog_->bind();
	glUniformMatrix4fv(prog_->getUniform("P"), 1, GL_FALSE, &P[0][0]);
	glUniformMatrix4fv(prog_->getUniform("V"), 1, GL_FALSE, &viewRotationOnly[0][0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_);
	glUniform1i(prog_->getUniform("skybox"), 0);
	glBindVertexArray(vao_);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	prog_->unbind();
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}
