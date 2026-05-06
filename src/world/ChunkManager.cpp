#include "ChunkManager.h"
#include <cmath>

ChunkManager::ChunkManager(
    int voxPerMeter,
    float chunkSizeMeters,
    int renderDistance, 
    int renderHeight,
    int terrainMinChunks,
    int terrainMaxChunks):
    voxPerMeter(voxPerMeter), 
    chunkSizeMeters(chunkSizeMeters),
    renderDistance(renderDistance),
    renderHeight(renderHeight),
    terrainMinChunks(terrainMinChunks),
    terrainMaxChunks(terrainMaxChunks),
    occupancyUpdateQueue(),
    meshUpdateQueue()
{
    // Calculations for voxel and chunk sizes
    voxSizeMeters = 1.0f/voxPerMeter; // in meters.

    // chunkSizeMeters
    chunkSizeInts = glm::ceil((voxPerMeter*chunkSizeMeters)/32.0f); //1
    int chunkSizeVoxels = chunkSizeInts*32;
    this->chunkSizeMeters = chunkSizeVoxels*voxSizeMeters;

    occupancyXsize = chunkSizeInts;
    occupancyYsize = occupancyZsize = chunkSizeInts*32;
    terrainGenerator = std::make_unique<TerrainGenerator>(this->terrainMinChunks, this->terrainMaxChunks, this->chunkSizeMeters, 1337u);
};

ChunkPos ChunkManager::getChunkPos(glm::vec3& pos){
    // This math keeps the chunk position rounded down even if negative.
    ChunkPos chunkPos = {
        static_cast<int>(glm::floor(pos.x/chunkSizeMeters)),
        static_cast<int>(glm::floor(pos.y/chunkSizeMeters)),
        static_cast<int>(glm::floor(pos.z/chunkSizeMeters)),
    };
    return chunkPos;
};

std::shared_ptr<Chunk> ChunkManager::generateChunk(ChunkPos& chunkPos){
    std::shared_ptr<Chunk> newChunk = std::make_shared<Chunk>(*this, chunkPos);
    // std::cout << "binding" << std::endl;
    newChunk->bindMesh();
    // std::cout << "generating" << std::endl;
    newChunk->generate();
    // std::cout << "updating" << std::endl;
    newChunk->updateMesh();
    // std::cout << "Return" << std::endl;
    return newChunk; 
}

void ChunkManager::modifyChunks(IChunkModifier& chunkMod){
    // Figure out which chunks the modifer effects.
    // Add the modifier to the chunk.
    // Add chunk to the updateQueue (possibly front if close to camera and back if far from camera)
};

void ChunkManager::updateChunks(){
    // Update Occupancy.
    // Current updates all but could be multithreaded and limited to N updates for performance.
    if (!occupancyUpdateQueue.empty()){
        std::shared_ptr<Chunk> C = occupancyUpdateQueue.front();
        C->updateOccupancy();
        occupancyUpdateQueue.pop_front();
        meshUpdateQueue.push_back(C);
    }

    // Update Meshes
    if (!meshUpdateQueue.empty()){
        std::shared_ptr<Chunk> C = meshUpdateQueue.front();
        C->updateMesh(); //also updates the buffers.
        meshUpdateQueue.pop_front();
    }
}

void ChunkManager::drawChunks(const Program& prog){
    
    for (int z = -renderDistance/chunkSizeMeters; z<renderDistance/chunkSizeMeters; z++){
        for (int y = terrainMinChunks; y<=terrainMaxChunks; y++){
            for (int x = -renderDistance/chunkSizeMeters; x<renderDistance/chunkSizeMeters; x++){
                ChunkPos chunkPos = ChunkPos{x, y, z};
                auto chunk = chunkMap.find(chunkPos);
                if (chunk == chunkMap.end()){
                    
                    float startTime = glfwGetTime();
                    chunkMap[chunkPos] = generateChunk(chunkPos);
                    float totalTime = glfwGetTime()-startTime;
                    std::cout << "ChunkGen (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ") " << std::fixed << std::setprecision(4) << totalTime << "s" << std::endl;
                }
                chunkMap[chunkPos]->drawMesh(prog);
            }
        }
    }
}

bool ChunkManager::isSolid(float wx, float wy, float wz) const
{
    ChunkPos cp = {
        static_cast<int>(std::floor(wx / chunkSizeMeters)),
        static_cast<int>(std::floor(wy / chunkSizeMeters)),
        static_cast<int>(std::floor(wz / chunkSizeMeters))
    };

    auto chunkIt = chunkMap.find(cp);
    if (chunkIt == chunkMap.end()) {
        return false;
    }

    const float localX = wx - static_cast<float>(cp.x) * chunkSizeMeters;
    const float localY = wy - static_cast<float>(cp.y) * chunkSizeMeters;
    const float localZ = wz - static_cast<float>(cp.z) * chunkSizeMeters;
    const int xVox = static_cast<int>(std::floor(localX / voxSizeMeters));
    const int yVox = static_cast<int>(std::floor(localY / voxSizeMeters));
    const int zVox = static_cast<int>(std::floor(localZ / voxSizeMeters));
    return chunkIt->second->isSolidLocal(xVox, yVox, zVox);
}

bool ChunkManager::aabbCollides(const glm::vec3& minP, const glm::vec3& maxP) const
{
    const float eps = 1e-4f;
    const int minXi = static_cast<int>(std::floor(minP.x / voxSizeMeters));
    const int minYi = static_cast<int>(std::floor(minP.y / voxSizeMeters));
    const int minZi = static_cast<int>(std::floor(minP.z / voxSizeMeters));
    const int maxXi = static_cast<int>(std::floor((maxP.x - eps) / voxSizeMeters));
    const int maxYi = static_cast<int>(std::floor((maxP.y - eps) / voxSizeMeters));
    const int maxZi = static_cast<int>(std::floor((maxP.z - eps) / voxSizeMeters));

    for (int z = minZi; z <= maxZi; ++z) {
        const float wz = (static_cast<float>(z) + 0.5f) * voxSizeMeters;
        for (int y = minYi; y <= maxYi; ++y) {
            const float wy = (static_cast<float>(y) + 0.5f) * voxSizeMeters;
            for (int x = minXi; x <= maxXi; ++x) {
                const float wx = (static_cast<float>(x) + 0.5f) * voxSizeMeters;
                if (isSolid(wx, wy, wz)) {
                    return true;
                }
            }
        }
    }
    return false;
}


