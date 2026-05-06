#include "FirstPersonCamera.h"
#include "world/ChunkManager.h"

#include <algorithm>
#include <cmath>
#include <GLFW/glfw3.h>

FirstPersonCamera::FirstPersonCamera(){
    player_pos = glm::vec3(0,1,0);
    cam_pos = glm::vec3(0,1,0);
    look_at = glm::vec3(0,1,-1);
}

FirstPersonCamera::FirstPersonCamera(glm::vec3 initial_pos, glm::vec3 look_at_, float height_){
    player_pos = initial_pos;
    cam_pos = initial_pos;
    look_at = look_at_;
	height = height_;
}

void FirstPersonCamera::ProcessMouseMovement(double dx, double dy){
    yaw += static_cast<float>(dx) * rot_sensitivity;
	pitch += static_cast<float>(dy) * rot_sensitivity;
	pitch = std::max(min_pitch, std::min(max_pitch, pitch));
}

void FirstPersonCamera::ProcessScroll(double dy){
}

void FirstPersonCamera::ProcessKeypress(int key, int action){
    bool is_pressed = (action != GLFW_RELEASE);

    switch (key){
    case GLFW_KEY_W:
        key_forward = is_pressed;
        break;
    case GLFW_KEY_S:
        key_backward = is_pressed;
        break;
    case GLFW_KEY_A:
        key_left = is_pressed;
        break;
    case GLFW_KEY_D:
        key_right = is_pressed;
        break;
	case GLFW_KEY_Q:
        key_yaw_left = is_pressed;
        break;
	case GLFW_KEY_E:
        key_yaw_right = is_pressed;
        break;
	case GLFW_KEY_R:
        key_pitch_up = is_pressed;
        break;
	case GLFW_KEY_F:
        key_pitch_down = is_pressed;
        break;
	case GLFW_KEY_SPACE:
        key_jump = is_pressed;
        break;
	case GLFW_KEY_LEFT_SHIFT:
        key_sprint = is_pressed;
        break;
	case GLFW_KEY_X:
		if(is_pressed) use_free_cam = !use_free_cam;
        break;
    default:
        return;
    }
}

void FirstPersonCamera::SetPlayerPos(glm::vec3 new_pos){
	player_pos = new_pos;
	cam_pos = new_pos;
	look_at = cam_pos + GetForward();
}

void FirstPersonCamera::UpdatePlayerPos(glm::vec3 delta_pos){
	player_pos += delta_pos;
	cam_pos += delta_pos;
	look_at += delta_pos;
}

glm::mat4 FirstPersonCamera::GetViewMatrix() {
	glm::vec3 forward = glm::normalize(look_at - cam_pos);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

	float roll_rad = glm::radians(roll);
    glm::vec3 rolled_up = (float)cos(roll_rad) * up  + (float)sin(roll_rad) * right;

	return glm::lookAt(cam_pos, look_at, rolled_up);
}

void FirstPersonCamera::UpdateCamera(float dt, const ChunkManager *world){
	if(use_free_cam){
		UpdateFreeCam(dt);
		return;
	}

	// yaw/pitch
	if (key_yaw_left){
		yaw -= rot_sensitivity_key;
	}
	if (key_yaw_right){
		yaw += rot_sensitivity_key;
	}
	if (key_pitch_up){
		pitch += rot_sensitivity_key;
	}
	if (key_pitch_down){
		pitch -= rot_sensitivity_key;
	}

	pitch = std::max(min_pitch, std::min(max_pitch, pitch));
	float pitch_rad = glm::radians(pitch);
	float yaw_rad = glm::radians(yaw);

	// compute new look at
    glm::vec3 dir = look_at - player_pos;
	glm::vec3 new_dir;
	new_dir.x = length(dir) * cos(pitch_rad) * cos(yaw_rad);
	new_dir.y = length(dir) * sin(pitch_rad);
	new_dir.z = length(dir) * cos(pitch_rad) * sin(yaw_rad);

	glm::vec3 flat_forward = glm::normalize(glm::vec3(new_dir.x, 0.0f, new_dir.z));
    glm::vec3 right = glm::normalize(glm::cross(flat_forward, glm::vec3(0,1,0)));

	// movement intent
    float trans_speed = dt * trans_sensitivity;
	if(key_sprint) trans_speed *= sprint_multiple;
	roll_target = 0.0f;

	glm::vec3 planarDelta(0.0f);
	float target_fov = base_fov;
	if (key_left){
		planarDelta -= trans_speed * right;
		roll_target -= max_roll;
	}
	if (key_right){
		planarDelta += trans_speed * right;
		roll_target += max_roll;
	}
	if (key_forward){
		planarDelta += trans_speed * flat_forward;
		target_fov = move_fov;
	}
	if (key_backward){
		planarDelta -= trans_speed * flat_forward;
		target_fov = move_back_fov;
	}
	const bool is_moving = glm::length(planarDelta) > 1e-5f;

	// roll on strafe
	roll += (roll_target - roll) * std::min(1.0f, dt * roll_lerp_speed);

	// dynamic fov
	if(key_sprint && target_fov == move_fov) target_fov = sprint_fov;
	fov += (target_fov - fov) * std::min(1.0f, dt * fov_lerp_speed);

	// jumping
	if (is_grounded && key_jump){
        vertical_velocity = jump_velocity;
        is_grounded = false;
    }

	vertical_velocity -= gravity * dt;
	const float voxelStep = world ? world->voxSizeMeters : 0.1f;
	const int maxStepVoxels = std::max(1, static_cast<int>(std::round(max_step_meters / voxelStep)));
	const float halfX = player_half_extents.x;
	const float halfZ = player_half_extents.z;

	auto collidesAt = [&](const glm::vec3 &p) -> bool {
		if (!world) {
			return false;
		}
		glm::vec3 minP(p.x - halfX, p.y - height, p.z - halfZ);
		glm::vec3 maxP(p.x + halfX, p.y, p.z + halfZ);
		return world->aabbCollides(minP, maxP);
	};

	// Unstick: if the player AABB is already embedded in solid voxels (spawn,
	// terrain regen, teleport, or any prior frame that ended in penetration),
	// snap straight up onto the first clear voxel-aligned eye position before
	// integrating physics. Cancels falling so we don't immediately redrop in.
	if (world && collidesAt(player_pos)) {
		const float feetY = player_pos.y - height;
		const float feetVoxFloor = std::floor(feetY / voxelStep) * voxelStep;
		constexpr int kMaxUnstickSteps = 256;
		float resolvedFeetY = feetVoxFloor;
		for (int i = 0; i <= kMaxUnstickSteps; ++i) {
			const float candidateFeet = feetVoxFloor + static_cast<float>(i) * voxelStep;
			if (!collidesAt(glm::vec3(player_pos.x, candidateFeet + height, player_pos.z))) {
				resolvedFeetY = candidateFeet;
				break;
			}
		}
		player_pos.y = resolvedFeetY + height;
		vertical_velocity = 0.0f;
		is_grounded = true;
		was_grounded = true; // suppress the landing-dip animation on snap
	}

	const float dy = vertical_velocity * dt;
	is_grounded = false;
	if (world) {
		const float targetEyeY = player_pos.y + dy;

		if (!collidesAt(glm::vec3(player_pos.x, targetEyeY, player_pos.z))) {
			player_pos.y = targetEyeY;
		} else if (dy <= 0.0f) {
			// Falling onto a surface: snap feet exactly to the top of the highest
			// solid voxel in the player footprint. Walking voxel-aligned candidates
			// upward avoids the off-by-one rubberband pop.
			const float targetFeetY = targetEyeY - height;
			const float feetVoxFloor = std::floor(targetFeetY / voxelStep) * voxelStep;
			constexpr int kMaxSnapSteps = 16;
			float resolvedFeetY = feetVoxFloor;
			bool resolved = false;
			for (int i = 0; i <= kMaxSnapSteps; ++i) {
				const float candidateFeet = feetVoxFloor + static_cast<float>(i) * voxelStep;
				if (!collidesAt(glm::vec3(player_pos.x, candidateFeet + height, player_pos.z))) {
					resolvedFeetY = candidateFeet;
					resolved = true;
					break;
				}
			}
			player_pos.y = resolvedFeetY + height;
			vertical_velocity = 0.0f;
			is_grounded = resolved;
		} else {
			// Rising into a ceiling: snap eye to the bottom of the lowest solid
			// voxel layer above and zero upward motion.
			const float topVoxFloor = std::floor(targetEyeY / voxelStep) * voxelStep;
			constexpr int kMaxSnapSteps = 16;
			float resolvedTopY = topVoxFloor;
			for (int i = 0; i <= kMaxSnapSteps; ++i) {
				const float candidateTop = topVoxFloor - static_cast<float>(i) * voxelStep;
				if (!collidesAt(glm::vec3(player_pos.x, candidateTop, player_pos.z))) {
					resolvedTopY = candidateTop;
					break;
				}
			}
			player_pos.y = resolvedTopY;
			vertical_velocity = 0.0f;
		}
	} else {
		player_pos.y += dy;
		if (player_pos.y <= floor_height + height){
			player_pos.y = floor_height + height;
			vertical_velocity = 0.0f;
			is_grounded = true;
		}
	}

	// Horizontal collision with step-up
	if (world) {
		glm::vec3 targetPos = player_pos;

		auto moveAxisWithStep = [&](int axis, float delta) {
			if (std::abs(delta) <= 1e-6f) {
				return;
			}
			glm::vec3 trial = targetPos;
			if (axis == 0) {
				trial.x += delta;
			} else {
				trial.z += delta;
			}
			if (!collidesAt(trial)) {
				targetPos = trial;
				return;
			}

			for (int step = 1; step <= maxStepVoxels; ++step) {
				const float lift = static_cast<float>(step) * voxelStep;
				glm::vec3 lifted = trial;
				lifted.y += lift;
				if (collidesAt(lifted)) {
					continue;
				}
				glm::vec3 supportCheck = lifted;
				supportCheck.y -= voxelStep * 0.6f;
				if (!collidesAt(supportCheck)) {
					continue;
				}
				targetPos = lifted;
				step_offset_y -= lift;
				is_grounded = true;
				vertical_velocity = 0.0f;
				return;
			}
		};

		moveAxisWithStep(0, planarDelta.x);
		moveAxisWithStep(2, planarDelta.z);
		player_pos = targetPos;
	} else {
		player_pos += planarDelta;
	}

	// view bobbing
	if (is_moving && is_grounded){
		bob_time += dt * bob_speed;
		bob_weight = std::min(1.0f, bob_weight + dt * bob_fade_speed);
	} else {
		bob_weight = std::max(0.0f, bob_weight - dt * bob_fade_speed);
	}

	glm::vec3 bob_offset(0.0f);
	bob_offset.y = std::abs(sin(bob_time)) * bob_amount_y * bob_weight;
	bob_offset.x = sin(bob_time * 0.5f) * bob_amount_x * bob_weight;

	step_offset_y += (0.0f - step_offset_y) * std::min(1.0f, dt * step_recover_speed);
	step_offset_y = std::max(step_offset_y, -1.5f * max_step_meters);
	cam_pos = player_pos + bob_offset;

	// jump landing dip
	if (!was_grounded && is_grounded){
		landing_velocity = -landing_dip_amount;
	}
	landing_velocity += (0.0f - landing_offset) * landing_spring_strength * dt;
	landing_velocity -= landing_velocity * landing_recover_speed * dt;
	landing_offset += landing_velocity * dt;

    cam_pos.y += landing_offset;
	cam_pos.y += step_offset_y;
	was_grounded = is_grounded;

	look_at = cam_pos + new_dir;
}

void FirstPersonCamera::UpdateFreeCam(float dt){
	pitch = std::max(min_pitch, std::min(max_pitch, pitch));
	float pitch_rad = glm::radians(pitch);
	float yaw_rad = glm::radians(yaw);

	// compute new look at
    glm::vec3 dir = look_at - cam_pos;
	glm::vec3 new_dir;
	new_dir.x = length(dir) * cos(pitch_rad) * cos(yaw_rad);
	new_dir.y = length(dir) * sin(pitch_rad);
	new_dir.z = length(dir) * cos(pitch_rad) * sin(yaw_rad);

	glm::vec3 forward = glm::normalize(glm::vec3(new_dir.x, new_dir.y, new_dir.z));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));

	float trans_speed = dt * trans_sensitivity;
	if (key_left){
		cam_pos -= trans_speed * right;
	}
	if (key_right){
		cam_pos += trans_speed * right;
	}
	if (key_forward){
		cam_pos += trans_speed * forward;
	}
	if (key_backward){
		cam_pos -= trans_speed * forward;
	}

	look_at = cam_pos + new_dir;
}

glm::vec3 FirstPersonCamera::GetForward() const{
	float yawRad = glm::radians(yaw);
	float pitchRad = glm::radians(pitch);

	glm::vec3 forward;
	forward.x = cosf(pitchRad) * cosf(yawRad);
	forward.y = sinf(pitchRad);
	forward.z = cosf(pitchRad) * sinf(yawRad);
	return glm::normalize(forward);
}

glm::vec3 FirstPersonCamera::GetRight() const{
	const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
	return glm::normalize(glm::cross(GetForward(), worldUp));
}

glm::vec3 FirstPersonCamera::GetUp() const{
	return glm::normalize(glm::cross(GetRight(), GetForward()));
}