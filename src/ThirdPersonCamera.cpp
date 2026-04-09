#include "ThirdPersonCamera.h"

#include <algorithm>
#include <cmath>

namespace {
template <typename T>
T clampVal(T v, T lo, T hi) { return std::max(lo, std::min(hi, v)); }
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void ThirdPersonCamera::setDistance(float d)
{
	distance_ = clampVal(d, minDist_, maxDist_);
}

ThirdPersonCamera::ThirdPersonCamera()
	: yaw_(static_cast<float>(M_PI)),
	  pitch_(-0.42f),
	  distance_(6.0f),
	  minDist_(2.0f),
	  maxDist_(18.0f),
	  sensitivity_(0.004f),
	  // Negative pitch = camera above pivot (normal third-person). Positive = under the character.
	  minPitch_(glm::radians(-78.0f)),
	  maxPitch_(glm::radians(35.0f))
{
}

glm::vec3 ThirdPersonCamera::forwardFromAngles() const
{
	return glm::normalize(glm::vec3(std::cos(pitch_) * std::cos(yaw_), std::sin(pitch_), std::cos(pitch_) * std::sin(yaw_)));
}

glm::vec3 ThirdPersonCamera::getEye(const glm::vec3 &pivot) const
{
	return pivot - forwardFromAngles() * distance_;
}

glm::mat4 ThirdPersonCamera::getViewMatrix(const glm::vec3 &pivot) const
{
	glm::vec3 e = getEye(pivot);
	return glm::lookAt(e, pivot, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 ThirdPersonCamera::getMoveForwardXZ() const
{
	glm::vec3 f = forwardFromAngles();
	f.y = 0.0f;
	float len = glm::length(f);
	if (len < 1e-5f)
		return glm::vec3(0.0f, 0.0f, 1.0f);
	return f / len;
}

glm::vec3 ThirdPersonCamera::getMoveRightXZ() const
{
	return glm::normalize(glm::cross(getMoveForwardXZ(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

void ThirdPersonCamera::applyFloorConstraint(float groundWorldY, float pivotWorldY)
{
	const float margin = 0.22f;
	float denom = std::max(0.35f, distance_);
	// eye.y = pivotY - dist * sin(pitch) >= ground + margin  =>  sin(pitch) <= (pivotY - ground - margin) / dist
	float sinCap = (pivotWorldY - groundWorldY - margin) / denom;
	sinCap = clampVal(sinCap, -1.0f, 1.0f);
	float pitchCeiling = std::asin(sinCap);
	float hi = std::min(maxPitch_, pitchCeiling);
	pitch_ = clampVal(pitch_, minPitch_, hi);
}

void ThirdPersonCamera::processMouseMovement(double dx, double dy)
{
	yaw_ += static_cast<float>(dx) * sensitivity_;
	pitch_ += static_cast<float>(dy) * sensitivity_;
	pitch_ = clampVal(pitch_, minPitch_, maxPitch_);
}

void ThirdPersonCamera::processScroll(double dy)
{
	setDistance(distance_ - static_cast<float>(dy) * 0.7f);
}
