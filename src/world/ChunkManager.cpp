#include "ChunkManager.h"

ChunkManager::ChunkManager(int voxPerMeter, int chunkSizeMeters):
    voxPerMeter(voxPerMeter), 
    chunkSizeMeters(chunkSizeMeters),
    occupancyUpdateQueue(),
    meshUpdateQueue()
{
    // Calculations for voxel and chunk sizes
    voxSizeMeters = 1.0f/voxPerMeter; // in meters.
    // Best estimate for how many ints needed to store a chunk.
    if (voxPerMeter >= 32)
    {
        // each meter has 32 bits or more.
        int intsPerMeter = voxPerMeter/32;
        chunkSizeInts = chunkSizeMeters*intsPerMeter;
    }
    else
    {
        // each meter has less than 32 bits.
        int MetersPerInt = 32/voxPerMeter;
        chunkSizeInts = glm::ceil(chunkSizeMeters/(double)MetersPerInt);
        chunkSizeMeters = chunkSizeInts*MetersPerInt;
    }

    occupancyXsize = chunkSizeInts;
    occupancyYsize = occupancyZsize = chunkSizeInts*32;
};

ChunkPos ChunkManager::getChunkPos(glm::vec3& pos){
    // This math keeps the chunk position rounded down even if negative.
    ChunkPos chunkPos = {
        glm::floor(pos.x/chunkSizeMeters),
        glm::floor(pos.y/chunkSizeMeters),
        glm::floor(pos.z/chunkSizeMeters),
    };
    return chunkPos;
};

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
    for (const auto& pair : chunkMap){
        pair.second->drawMesh(prog);
    }
    
}


