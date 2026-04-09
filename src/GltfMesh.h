#pragma once

#include <glm/glm.hpp>
#include <memory>

#include <glad/glad.h>

class Program;

/**
 * Loads first mesh/primitive from a .glb (cgltf). Bind-pose geometry only — no GPU skinning.
 * pivotLocal() is bottom-center of the mesh AABB for placing feet on the ground.
 */
class GltfMesh {
public:
	GltfMesh();
	~GltfMesh();

	GltfMesh(const GltfMesh &) = delete;
	GltfMesh &operator=(const GltfMesh &) = delete;

	bool loadFromFile(const char *path);

	void draw(const std::shared_ptr<Program> &prog) const;

	glm::vec3 minBounds() const { return min_; }
	glm::vec3 maxBounds() const { return max_; }
	/** Bottom-center of axis-aligned bounds in model space. */
	glm::vec3 pivotLocal() const { return pivot_; }

	/** Scale so bounding height ~= targetMeters. */
	float uniformScaleForHeight(float targetMeters) const;

	bool valid() const { return vao_ != 0 && indexCount_ > 0; }

private:
	GLuint vao_ = 0;
	GLuint posBuf_ = 0;
	GLuint norBuf_ = 0;
	GLuint texBuf_ = 0;
	GLuint eleBuf_ = 0;
	size_t indexCount_ = 0;

	glm::vec3 min_{0.0f};
	glm::vec3 max_{0.0f};
	glm::vec3 pivot_{0.0f};
};
