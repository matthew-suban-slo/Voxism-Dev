#include "GltfMesh.h"

#include "GLSL.h"
#include "Program.h"

#include <cmath>
#include <iostream>
#include <vector>

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using std::cerr;
using std::endl;
using std::vector;

GltfMesh::GltfMesh() = default;

GltfMesh::~GltfMesh()
{
	if (eleBuf_)
		glDeleteBuffers(1, &eleBuf_);
	if (texBuf_)
		glDeleteBuffers(1, &texBuf_);
	if (norBuf_)
		glDeleteBuffers(1, &norBuf_);
	if (posBuf_)
		glDeleteBuffers(1, &posBuf_);
	if (vao_)
		glDeleteVertexArrays(1, &vao_);
}

bool GltfMesh::loadFromFile(const char *path)
{
	cgltf_options options = {};
	cgltf_data *data = nullptr;
	if (cgltf_parse_file(&options, path, &data) != cgltf_result_success) {
		cerr << "cgltf_parse_file failed: " << path << endl;
		return false;
	}
	if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
		cerr << "cgltf_load_buffers failed: " << path << endl;
		cgltf_free(data);
		return false;
	}
	if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0) {
		cerr << "No mesh in glTF: " << path << endl;
		cgltf_free(data);
		return false;
	}

	cgltf_primitive &prim = data->meshes[0].primitives[0];
	if (prim.type != cgltf_primitive_type_triangles) {
		cerr << "Unsupported primitive type" << endl;
		cgltf_free(data);
		return false;
	}

	cgltf_accessor *posAcc = nullptr;
	cgltf_accessor *norAcc = nullptr;
	cgltf_accessor *texAcc = nullptr;
	for (cgltf_size ai = 0; ai < prim.attributes_count; ++ai) {
		cgltf_attribute &a = prim.attributes[ai];
		if (a.type == cgltf_attribute_type_position)
			posAcc = a.data;
		else if (a.type == cgltf_attribute_type_normal)
			norAcc = a.data;
		else if (a.type == cgltf_attribute_type_texcoord && texAcc == nullptr)
			texAcc = a.data;
	}
	if (!posAcc) {
		cerr << "Missing POSITION" << endl;
		cgltf_free(data);
		return false;
	}

	const cgltf_size vcount = posAcc->count;
	vector<float> pos(vcount * 3);
	if (!cgltf_accessor_unpack_floats(posAcc, pos.data(), vcount * 3)) {
		cgltf_free(data);
		return false;
	}

	vector<float> nor;
	if (norAcc && norAcc->count == vcount) {
		nor.resize(vcount * 3);
		cgltf_accessor_unpack_floats(norAcc, nor.data(), vcount * 3);
	} else {
		nor.assign(vcount * 3, 0.0f);
		for (cgltf_size i = 0; i < vcount; ++i) {
			nor[3 * i + 1] = 1.0f;
		}
	}

	vector<float> tex;
	if (texAcc && texAcc->count == vcount) {
		tex.resize(vcount * 2);
		cgltf_accessor_unpack_floats(texAcc, tex.data(), vcount * 2);
	} else {
		tex.resize(vcount * 2);
		for (cgltf_size i = 0; i < vcount; ++i) {
			tex[2 * i + 0] = pos[3 * i + 0] * 0.5f + 0.5f;
			tex[2 * i + 1] = pos[3 * i + 2] * 0.5f + 0.5f;
		}
	}

	min_ = glm::vec3(1e30f);
	max_ = glm::vec3(-1e30f);
	for (cgltf_size i = 0; i < vcount; ++i) {
		glm::vec3 p(pos[3 * i + 0], pos[3 * i + 1], pos[3 * i + 2]);
		min_ = glm::min(min_, p);
		max_ = glm::max(max_, p);
	}

	// Many glTF bind poses (e.g. Khronos RiggedFigure) lie in the XZ plane with thin Y.
	// Rotate so the long horizontal axis becomes +Y (standing on the floor).
	{
		glm::vec3 ext = max_ - min_;
		const float ratio = 0.88f;
		if (ext.y < ext.x * ratio && ext.y < ext.z * ratio) {
			glm::mat4 R(1.0f);
			if (ext.z >= ext.x)
				R = glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
			else
				R = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat3 R3 = glm::mat3(R);
			for (cgltf_size i = 0; i < vcount; ++i) {
				glm::vec4 p(pos[3 * i + 0], pos[3 * i + 1], pos[3 * i + 2], 1.0f);
				p = R * p;
				pos[3 * i + 0] = p.x;
				pos[3 * i + 1] = p.y;
				pos[3 * i + 2] = p.z;
				glm::vec3 n(nor[3 * i + 0], nor[3 * i + 1], nor[3 * i + 2]);
				n = glm::normalize(R3 * n);
				nor[3 * i + 0] = n.x;
				nor[3 * i + 1] = n.y;
				nor[3 * i + 2] = n.z;
			}
			min_ = glm::vec3(1e30f);
			max_ = glm::vec3(-1e30f);
			for (cgltf_size i = 0; i < vcount; ++i) {
				glm::vec3 p(pos[3 * i + 0], pos[3 * i + 1], pos[3 * i + 2]);
				min_ = glm::min(min_, p);
				max_ = glm::max(max_, p);
			}
		}
	}

	pivot_ = glm::vec3((min_.x + max_.x) * 0.5f, min_.y, (min_.z + max_.z) * 0.5f);

	vector<unsigned int> indices;
	if (prim.indices) {
		indexCount_ = prim.indices->count;
		indices.resize(indexCount_);
		for (cgltf_size i = 0; i < indexCount_; ++i)
			indices[static_cast<size_t>(i)] = static_cast<unsigned int>(cgltf_accessor_read_index(prim.indices, i));

	} else {
		cerr << "Non-indexed primitive not supported" << endl;
		cgltf_free(data);
		return false;
	}

	cgltf_free(data);

	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);

	glGenBuffers(1, &posBuf_);
	glBindBuffer(GL_ARRAY_BUFFER, posBuf_);
	glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), pos.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &norBuf_);
	glBindBuffer(GL_ARRAY_BUFFER, norBuf_);
	glBufferData(GL_ARRAY_BUFFER, nor.size() * sizeof(float), nor.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &texBuf_);
	glBindBuffer(GL_ARRAY_BUFFER, texBuf_);
	glBufferData(GL_ARRAY_BUFFER, tex.size() * sizeof(float), tex.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &eleBuf_);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBuf_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return true;
}

float GltfMesh::uniformScaleForHeight(float targetMeters) const
{
	float h = max_.y - min_.y;
	if (h < 1e-5f)
		return 1.0f;
	return targetMeters / h;
}

void GltfMesh::draw(const std::shared_ptr<Program> &prog) const
{
	if (!valid() || !prog)
		return;

	int h_pos = prog->getAttribute("vertPos");
	int h_nor = prog->getAttribute("vertNor");
	int h_tex = prog->getAttribute("vertTex");

	glBindVertexArray(vao_);

	GLSL::enableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBuf_);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	if (h_nor >= 0) {
		GLSL::enableVertexAttribArray(h_nor);
		glBindBuffer(GL_ARRAY_BUFFER, norBuf_);
		glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	}

	if (h_tex >= 0) {
		GLSL::enableVertexAttribArray(h_tex);
		glBindBuffer(GL_ARRAY_BUFFER, texBuf_);
		glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBuf_);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount_), GL_UNSIGNED_INT, (const void *)0);

	GLSL::disableVertexAttribArray(h_pos);
	if (h_nor >= 0)
		GLSL::disableVertexAttribArray(h_nor);
	if (h_tex >= 0)
		GLSL::disableVertexAttribArray(h_tex);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
