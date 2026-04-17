#include "GameObject.h"

#include "Shape.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool AABB::overlaps(const AABB &o) const
{
	return 	min.x <= o.max.x && 
			max.x >= o.min.x && 
			min.y <= o.max.y && 
			max.y >= o.min.y && 
			min.z <= o.max.z && 
			max.z >= o.min.z;
}

GameObject::GameObject()
	: position_(0.0f),
	  velocity_(0.0f),
	  yaw_(0.0f),
	  uniformScale_(1.0f),
	  collected_(false),
	  diffuse_(0.85f, 0.35f, 0.2f),
	  ambient_(0.15f, 0.08f, 0.05f),
	  specular_(0.6f, 0.5f, 0.45f),
	  shininess_(32.0f)
{
}

void GameObject::setMesh(const std::shared_ptr<Shape> &shape, float uniformScale)
{
	shape_ = shape;
	uniformScale_ = uniformScale;
}

void GameObject::setVelocity(const glm::vec3 &v)
{
	velocity_ = v;
	if (!collected_)
		recomputeYawFromVelocity();
}

void GameObject::recomputeYawFromVelocity()
{
	float len2 = velocity_.x * velocity_.x + velocity_.z * velocity_.z;
	if (len2 < 1e-8f) {
		return;
	}
	// Face +Z in model space -> world forward is velocity on XZ
	yaw_ = std::atan2(velocity_.x, velocity_.z);
}

void GameObject::update(float dt, float groundY)
{
	if (collected_ || !shape_)
		return;

	position_ += velocity_ * dt;
	position_.y = groundY;
}

void GameObject::setCollected()
{
	collected_ = true;
	velocity_ = glm::vec3(0.0f);
	diffuse_ = glm::vec3(0.25f, 0.75f, 0.35f);
	ambient_ = glm::vec3(0.05f, 0.15f, 0.08f);
	specular_ = glm::vec3(0.2f, 0.35f, 0.25f);
}

void GameObject::reverseVelocity()
{
	velocity_ = -velocity_;
	recomputeYawFromVelocity();
}

AABB GameObject::getWorldAABB() const
{
	AABB w;
	if (!shape_) {
		w.min = position_;
		w.max = position_;
		return w;
	}

	glm::vec3 smin = shape_->min * uniformScale_;
	glm::vec3 smax = shape_->max * uniformScale_;

	glm::mat3 R = glm::mat3(glm::rotate(glm::mat4(1.0f), yaw_, glm::vec3(0.0f, 1.0f, 0.0f)));

	glm::vec3 corners[8] = {
		R * glm::vec3(smin.x, smin.y, smin.z),
		R * glm::vec3(smax.x, smin.y, smin.z),
		R * glm::vec3(smin.x, smax.y, smin.z),
		R * glm::vec3(smax.x, smax.y, smin.z),
		R * glm::vec3(smin.x, smin.y, smax.z),
		R * glm::vec3(smax.x, smin.y, smax.z),
		R * glm::vec3(smin.x, smax.y, smax.z),
		R * glm::vec3(smax.x, smax.y, smax.z),
	};

	w.min = position_ + corners[0];
	w.max = position_ + corners[0];
	for (int i = 1; i < 8; ++i) {
		glm::vec3 p = position_ + corners[i];
		w.min = glm::min(w.min, p);
		w.max = glm::max(w.max, p);
	}
	return w;
}

glm::mat4 GameObject::getModelMatrix() const
{
	glm::mat4 T = glm::translate(glm::mat4(1.0f), position_);
	glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), yaw_, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(uniformScale_));
	return T * Ry * S;
}
