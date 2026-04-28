#pragma once
#ifndef _CHUNKMANAGER_H_
#define _CHUNKMANAGER_H_

#include "Chunk.h"
#include <unordered_map>
#include <memory>
#include <deque>
#include <ChunkModifier.h>

// Stuct used to store the chunk position relative to other chunk positions.
struct ChunkPos {
    int x, y, z;
    bool operator==(const ChunkPos& otherPos) const{
        return x == otherPos.x && 
                y == otherPos.y && 
                z == otherPos.z;
    }
};

class ChunkManager {
    public:
        //initialize with the standard size of the world chunks.
        ChunkManager(int voxPerMeter, int chunkSizeMeters);
        int voxPerMeter; // how many voxels there are per meter
        float voxSizeMeters; // how large in meters each voxel is.
        float chunkSizeMeters; // how many meters is the width of the chunk.
        int chunkSizeInts; // how many ints in the x direction of the chunk.
        int occupancyXsize, occupancyYsize, occupancyZsize; //used to iterate through chunks.

        // Function returns the chunk position for the passed position.
        ChunkPos getChunkPos(glm::vec3& pos);

        // Modify chunk is given the modifier that is then parsed and attached to the chunks it effects.
        // Chunks are then marked and setup for occupancy updates.
        void modifyChunks(IChunkModifier& chunkMod);

        // updates the occupancy array and any color information for some
        // chunks in the update array.
        void updateChunks();

        // Determines what chunks should be drawn and then
        // binds the chunk data and draws it.
        void drawChunks(const Program& prog);

        // void updateOccupancy();
        // void updateMesh();
       
    private:
        // Stuct necessary for mapping an xyz of the chunk to the chunk.
        struct ChunkPosHash {
            size_t operator()(const ChunkPos& cp) const {
                size_t xh = std::hash<int>()(cp.x);
                size_t yh = std::hash<int>()(cp.y);
                size_t zh = std::hash<int>()(cp.z);
                return xh ^ (yh << 1) ^ (zh >> 1);
            }
        };
        std::unordered_map<ChunkPos, std::shared_ptr<Chunk>, ChunkPosHash> chunkMap;

        std::deque<std::shared_ptr<Chunk>> occupancyUpdateQueue;
        std::deque<std::shared_ptr<Chunk>> meshUpdateQueue;



};

#endif