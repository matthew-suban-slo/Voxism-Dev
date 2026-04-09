#include "Texture.h"
#include "GLSL.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

Texture::Texture() :
	filename(""),
	width(0),
	height(0),
	tid(0),
	unit(0)
{
}

Texture::~Texture()
{
}

void Texture::init()
{
	int w, h, ncomps;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename.c_str(), &w, &h, &ncomps, 0);
	if (!data) {
		cerr << "Texture: could not load '" << filename << "'" << endl;
		return;
	}

	GLenum format = GL_RGB;
	GLenum internal = GL_RGB8;

	if (ncomps == 1) {
		format = GL_RED;
		internal = GL_R8;
	} else if (ncomps == 3) {
		format = GL_RGB;
		internal = GL_RGB8;
	} else if (ncomps == 4) {
		format = GL_RGBA;
		internal = GL_RGBA8;
	} else {
		cerr << "Texture: unsupported component count " << ncomps << " in '" << filename << "'" << endl;
		stbi_image_free(data);
		return;
	}

	width = w;
	height = h;

	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_2D, tid);
	glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);
}

void Texture::setWrapModes(GLint wrapS, GLint wrapT)
{
	glBindTexture(GL_TEXTURE_2D, tid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
}

void Texture::bind(GLint handle)
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, tid);
	glUniform1i(handle, unit);
}

void Texture::unbind()
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, 0);
}
