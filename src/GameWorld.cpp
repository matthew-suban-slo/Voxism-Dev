#include "GameWorld.h"

#include "Shape.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GameWorld::GameWorld()
	: collectibleScale_(0.35f),
	  spawnAccumulator_(0.0f),
	  collectedCount_(0),
	  totalSpawned_(0),
	  rng_(std::random_device{}()),
	  distPos_(-kGridHalf + 1.5f, kGridHalf - 1.5f),
	  distAngle_(0.0f, static_cast<float>(2.0 * M_PI)),
	  distSpeed_(1.2f, 3.5f)
{
}

void GameWorld::setCollectibleMesh(const std::shared_ptr<Shape> &shape, float uniformScale)
{
	collectibleShape_ = shape;
	collectibleScale_ = uniformScale;
}

void GameWorld::reset()
{
	objects_.clear();
	spawnAccumulator_ = 0.0f;
	collectedCount_ = 0;
	totalSpawned_ = 0;
}

int GameWorld::getActiveObjectCount() const
{
	int n = 0;
	for (const auto &o : objects_) {
		if (!o.isCollected())
			++n;
	}
	return n;
}

bool GameWorld::findNonOverlappingSpawn(glm::vec3 &outPos, float margin)
{
	const int kMaxTries = 40;
	for (int t = 0; t < kMaxTries; ++t) {
		glm::vec3 p(distPos_(rng_), kGroundY, distPos_(rng_));
		AABB trial;
		trial.min = p - glm::vec3(margin);
		trial.max = p + glm::vec3(margin);
		bool ok = true;
		for (const auto &o : objects_) {
			if (o.getWorldAABB().overlaps(trial)) {
				ok = false;
				break;
			}
		}
		if (ok) {
			outPos = p;
			return true;
		}
	}
	outPos = glm::vec3(distPos_(rng_), kGroundY, distPos_(rng_));
	return false;
}

void GameWorld::trySpawn()
{
	if (!collectibleShape_ || static_cast<int>(objects_.size()) >= kMaxObjects)
		return;

	float margin = 1.2f * collectibleScale_;
	glm::vec3 pos;
	findNonOverlappingSpawn(pos, margin);

	float angle = distAngle_(rng_);
	float speed = distSpeed_(rng_);
	glm::vec3 vel(std::sin(angle) * speed, 0.0f, std::cos(angle) * speed);

	GameObject go;
	go.setMesh(collectibleShape_, collectibleScale_);
	go.setPosition(pos);
	go.setVelocity(vel);
	objects_.push_back(go);
	++totalSpawned_;
}

void GameWorld::resolveWallCollisions(GameObject &obj)
{
	if (obj.isCollected() || !obj.getShape())
		return;

	const float pad = 0.05f;
	// Recompute AABB after each correction (corners may affect multiple axes).
	for (int iter = 0; iter < 6; ++iter) {
		AABB box = obj.getWorldAABB();
		glm::vec3 pos = obj.getPosition();
		glm::vec3 vel = obj.getVelocity();
		bool changed = false;

		if (box.min.x < -kGridHalf + pad) {
			pos.x += (-kGridHalf + pad) - box.min.x;
			vel.x = -vel.x;
			changed = true;
		} else if (box.max.x > kGridHalf - pad) {
			pos.x -= box.max.x - (kGridHalf - pad);
			vel.x = -vel.x;
			changed = true;
		}

		if (box.min.z < -kGridHalf + pad) {
			pos.z += (-kGridHalf + pad) - box.min.z;
			vel.z = -vel.z;
			changed = true;
		} else if (box.max.z > kGridHalf - pad) {
			pos.z -= box.max.z - (kGridHalf - pad);
			vel.z = -vel.z;
			changed = true;
		}

		if (!changed)
			break;
		obj.setPosition(pos);
		obj.setVelocity(vel);
	}
}

void GameWorld::resolveObjectObjectCollisions()
{
	const size_t n = objects_.size();
	for (size_t i = 0; i < n; ++i) {
		for (size_t j = i + 1; j < n; ++j) {
			GameObject &a = objects_[i];
			GameObject &b = objects_[j];
			if (a.isCollected() || b.isCollected())
				continue;
			AABB ba = a.getWorldAABB();
			AABB bb = b.getWorldAABB();
			if (!ba.overlaps(bb))
				continue;
			// Reverse both to avoid deadlock stacking; simple bounce.
			glm::vec3 va = a.getVelocity();
			glm::vec3 vb = b.getVelocity();
			a.setVelocity(-va);
			b.setVelocity(-vb);
		}
	}
}

void GameWorld::checkPlayerCollection(const glm::vec3 &playerPos, float playerHalfExtent)
{
	AABB playerBox;
	playerBox.min = playerPos - glm::vec3(playerHalfExtent);
	playerBox.max = playerPos + glm::vec3(playerHalfExtent);
	// Eye height: still use horizontal overlap primarily; extend vertically
	playerBox.min.y -= 0.5f;
	playerBox.max.y += 0.5f;

	for (auto &o : objects_) {
		if (o.isCollected())
			continue;
		if (playerBox.overlaps(o.getWorldAABB())) {
			o.setCollected();
			++collectedCount_;
		}
	}
}

void GameWorld::step(float dt, const glm::vec3 &playerPos, float playerHalfExtent)
{
	spawnAccumulator_ += dt;
	while (spawnAccumulator_ >= kSpawnIntervalSec) {
		spawnAccumulator_ -= kSpawnIntervalSec;
		trySpawn();
	}

	for (auto &o : objects_) {
		o.update(dt, kGroundY);
	}

	for (auto &o : objects_) {
		resolveWallCollisions(o);
	}

	resolveObjectObjectCollisions();

	// Second wall pass after object-object may push AABB out
	for (auto &o : objects_) {
		resolveWallCollisions(o);
	}

	checkPlayerCollection(playerPos, playerHalfExtent);
}
