#pragma once
#ifndef _CHUNK_H_
#define _CHUNK_H_

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "../Program.h"
#include <iostream>
#include <cstring>

class Chunk
{
    public:
        Chunk(); 

        // update OccupancyInts
        void updateChunk(float deltaTime, bool gridFill, bool floor, bool sphere);
        // update Buffers
        void updateMesh();
        // bond buffers
        void bindMesh();
        // Draw Mesh
        void drawMesh(const Program& prog);

        void addVoxelAtWorldPos(const glm::vec3 &worldPos);
        void addVoxelAtIndex(int vx, int vy, int vz);
    
    private:
        // For each voxel in occupancyInts will fill an outlining grid.
        // Works with verious voxel and chunk sizes.
        void fillMeterGrid(uint32_t* occupancyInt, int x, int y, int z);
        void fillFloor(uint32_t* occupancyInt, glm::vec3* voxPosCenter, int z, int x);
        glm::vec3 calculateSphere(float deltaTime);
        void checkSphere(uint32_t* occupancyInt, glm::vec3* voxPosCenter, glm::vec3* spherePos);
        float time;

        //
        void updateBuffer();
        int bufferUpdateMethod;
        // adds a complete cube primitive to the buffers for a given voxel position.
        void addCubePrimitive(glm::vec3* voxPos, int vertIndex);

        // Information to be extracted later.
        float voxSizeMeters; // size of a voxel in meters.
        int chunkSizeInts; // number of ints needed to store the length of a chunk.
        int voxPerMeter;
    
        // int is 32 bits, so we can store 32 voxels in one int.
        std::vector<uint32_t>  occupancyInts;
        int occupancyXsize, occupancyYsize, occupancyZsize; 
        std::vector<uint32_t> userVoxels;

        GLuint vaoID; // vertex array object ID.
        GLuint vBuffID; // vertex buffer ID.
        std::vector<GLfloat>vBuff; // vertex buffer.

        GLuint eBuffID; // element buffer object ID.
        std::vector<unsigned int> eBuff; // element buffer.

        GLuint cBuffID; // color buffer ID.
        std::vector<GLfloat>cBuff; // color buffer.

        void *vPtr; 
        void *ePtr; 
        void *cPtr;
};

#endif