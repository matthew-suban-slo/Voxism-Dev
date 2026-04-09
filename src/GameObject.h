#pragma once

#include <glm/glm.hpp>
#include <memory>

class Shape;

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
	bool overlaps(const AABB &o) const;
};

/**
 * One collectible / moving mesh instance: position, yaw toward velocity, AABB, collected state.
 */
class GameObject {
public:
	GameObject();

	void setMesh(const std::shared_ptr<Shape> &shape, float uniformScale);
	const std::shared_ptr<Shape> &getShape() const { return shape_; }

	void setPosition(const glm::vec3 &p) { position_ = p; }
	glm::vec3 getPosition() const { return position_; }

	void setVelocity(const glm::vec3 &v);
	glm::vec3 getVelocity() const { return velocity_; }

	float getYaw() const { return yaw_; }
	bool isCollected() const { return collected_; }

	glm::vec3 getDiffuse() const { return diffuse_; }
	glm::vec3 getAmbient() const { return ambient_; }
	glm::vec3 getSpecular() const { return specular_; }
	float getShininess() const { return shininess_; }

	/** Integrate position; keeps y on ground plane. */
	void update(float dt, float groundY);

	void setCollected();
	void reverseVelocity();

	/** World-space AABB after scale, Y rotation, translation. */
	AABB getWorldAABB() const;

	glm::mat4 getModelMatrix() const;

	float getUniformScale() const { return uniformScale_; }

private:
	void recomputeYawFromVelocity();

	std::shared_ptr<Shape> shape_;
	glm::vec3 position_;
	glm::vec3 velocity_;
	float yaw_;
	float uniformScale_;

	bool collected_;
	glm::vec3 diffuse_;
	glm::vec3 ambient_;
	glm::vec3 specular_;
	float shininess_;
};
