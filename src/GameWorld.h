#pragma once

#include "GameObject.h"

#include <memory>
#include <random>
#include <vector>

class Shape;

/**
 * Spawns objects, integrates movement (fixed dt), AABB collisions vs walls and each other,
 * player collection. O(n^2) pairwise checks per lab spec.
 */
class GameWorld {
public:
	static constexpr float kGridHalf = 10.0f;
	static constexpr float kGroundY = 0.0f;
	static constexpr int kMaxObjects = 30;
	static constexpr float kSpawnIntervalSec = 2.2f;

	GameWorld();

	void setCollectibleMesh(const std::shared_ptr<Shape> &shape, float uniformScale);

	void reset();

	/** Fixed-step simulation (semi-fixed timestep caller passes dt here). */
	void step(float dt, const glm::vec3 &playerPos, float playerHalfExtent);

	const std::vector<GameObject> &getObjects() const { return objects_; }

	int getActiveObjectCount() const;
	int getCollectedCount() const { return collectedCount_; }
	int getTotalSpawned() const { return totalSpawned_; }

private:
	void trySpawn();
	bool findNonOverlappingSpawn(glm::vec3 &outPos, float margin);
	void resolveWallCollisions(GameObject &obj);
	void resolveObjectObjectCollisions();
	void checkPlayerCollection(const glm::vec3 &playerPos, float playerHalfExtent);

	std::shared_ptr<Shape> collectibleShape_;
	float collectibleScale_;

	std::vector<GameObject> objects_;
	float spawnAccumulator_;
	int collectedCount_;
	int totalSpawned_;

	std::mt19937 rng_;
	std::uniform_real_distribution<float> distPos_;
	std::uniform_real_distribution<float> distAngle_;
	std::uniform_real_distribution<float> distSpeed_;
};
