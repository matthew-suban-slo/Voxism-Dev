#include "CharacterController.h"

#include <algorithm>
#include <cmath>

CharacterController::CharacterController()
	: position_(0.0f, 0.05f, 8.0f),
	  horizontalVel_(0.0f),
	  verticalVel_(0.0f),
	  moveSpeed_(10.0f),
	  horizResponse_(16.0f),
	  groundY_(0.05f),
	  gravity_(-38.0f),
	  jumpImpulsePrimary_(11.5f),
	  jumpImpulseSecondary_(9.0f),
	  jumpsRemaining_(2)
{
}

void CharacterController::tryJump()
{
	if (jumpsRemaining_ <= 0)
		return;
	float imp = (jumpsRemaining_ == 2) ? jumpImpulsePrimary_ : jumpImpulseSecondary_;
	verticalVel_ = imp;
	--jumpsRemaining_;
}

void CharacterController::physicsStep(float dt, const glm::vec3 &moveWorldXZUnit)
{
	glm::vec3 targetH(0.0f);
	if (glm::length(moveWorldXZUnit) > 1e-6f) {
		glm::vec3 u = moveWorldXZUnit;
		u.y = 0.0f;
		float len = glm::length(u);
		if (len > 1e-6f)
			targetH = (u / len) * moveSpeed_;
	}

	glm::vec3 curH(horizontalVel_.x, 0.0f, horizontalVel_.z);
	float blend = std::min(1.0f, horizResponse_ * dt);
	glm::vec3 newH = curH + (targetH - curH) * blend;
	horizontalVel_.x = newH.x;
	horizontalVel_.z = newH.z;
	horizontalVel_.y = 0.0f;

	verticalVel_ += gravity_ * dt;

	position_.x += horizontalVel_.x * dt;
	position_.y += verticalVel_ * dt;
	position_.z += horizontalVel_.z * dt;

	const float eps = 0.03f;
	if (position_.y <= groundY_ + eps && verticalVel_ <= 0.0f) {
		position_.y = groundY_;
		verticalVel_ = 0.0f;
		jumpsRemaining_ = 2;
	}
}
