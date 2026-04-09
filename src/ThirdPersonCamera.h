#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * Orbit camera (Genshin / RoR2 style): mouse orbits yaw/pitch around pivot, scroll zooms.
 * Movement uses camera look direction flattened to XZ.
 */
class ThirdPersonCamera {
public:
	ThirdPersonCamera();

	void processMouseMovement(double dx, double dy);
	void processScroll(double dy);

	/**
	 * Keeps eye.y >= groundY: eye = pivot - forward*dist => sin(pitch) <= (pivotY - groundY - margin) / dist.
	 * Call after mouse / zoom and each tick (pivot moves with player).
	 */
	void applyFloorConstraint(float groundWorldY, float pivotWorldY);

	void setDistance(float d);
	float getDistance() const { return distance_; }

	/** World-space eye position looking at pivot. */
	glm::vec3 getEye(const glm::vec3 &pivot) const;

	glm::mat4 getViewMatrix(const glm::vec3 &pivot) const;

	/** Walk forward on XZ (where the camera is looking), for WASD. */
	glm::vec3 getMoveForwardXZ() const;
	glm::vec3 getMoveRightXZ() const;

	float getYaw() const { return yaw_; }
	float getPitch() const { return pitch_; }

private:
	glm::vec3 forwardFromAngles() const;

	float yaw_;
	float pitch_;
	float distance_;
	float minDist_;
	float maxDist_;
	float sensitivity_;
	float minPitch_;
	float maxPitch_;
};
