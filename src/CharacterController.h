#pragma once

#include <glm/glm.hpp>

/** Third-person avatar: horizontal smoothing, gravity, double jump. Position Y is feet height. */
class CharacterController {
public:
	CharacterController();

	glm::vec3 getFeetPosition() const { return position_; }
	void setFeetPosition(const glm::vec3 &p) { position_ = p; }

	void setGroundY(float y) { groundY_ = y; }
	float getGroundY() const { return groundY_; }

	/** moveWorld is desired horizontal direction in world XZ (typically unit or axis combo). */
	void physicsStep(float dt, const glm::vec3 &moveWorldXZUnit);

	void tryJump();

	float getVerticalVelocity() const { return verticalVel_; }
	int getJumpsRemaining() const { return jumpsRemaining_; }
	glm::vec3 getHorizontalVelocity() const { return horizontalVel_; }

	void setMoveSpeed(float s) { moveSpeed_ = s; }
	float getMoveSpeed() const { return moveSpeed_; }

private:
	glm::vec3 position_;
	glm::vec3 horizontalVel_;
	float verticalVel_;
	float moveSpeed_;
	float horizResponse_;
	float groundY_;
	float gravity_;
	float jumpImpulsePrimary_;
	float jumpImpulseSecondary_;
	int jumpsRemaining_;
};
