#pragma once
#ifndef _CHUNK_H_
#define _CHUNK_H_

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../Program.h"
#include <iostream>

class Chunk
{
    public:
        Chunk(); 

        void updateMesh();
        void bindMesh();
        void drawMesh(const Program& prog);
    
    private:
        // Information to be extracted later.
        float voxSizeMeters; // size of a voxel in meters.
        int chunkSizeInts; // number of ints needed to store the length of a chunk.

        // int is 32 bits, so we can store 32 voxels in one int.
        std::vector<uint32_t>  occupancyInts;
        int occupancyXsize, occupancyYsize, occupancyZsize; 

        GLuint vaoID; // vertex array object ID.
        GLuint vBuffID; // vertex buffer ID.
        std::vector<GLfloat>vBuff; // vertex buffer.

        GLuint eBuffID; // element buffer object ID.
        std::vector<unsigned int> eBuff; // element buffer.

        GLuint cBuffID; // color buffer ID.
        std::vector<GLfloat>cBuff; // color buffer.
};

#endif