#include "Chunk.h"

Chunk::Chunk(){
    // Input parameters for chunks
    voxPerMeter = 4; // must be 2^n.
    int chunkSizeMeters = 16; 
    bufferUpdateMethod = 0;
    time = 0;
    assert(bufferUpdateMethod >= 0 && bufferUpdateMethod <= 2);

    // Calculations 
    voxSizeMeters = 1.0f/voxPerMeter; // in meters.
    // Best estimate for how many ints needed to store a chunk.
    if (voxPerMeter >= 32)
    {
        // each meter has 32 bits.
        int intsPerMeter = voxPerMeter/32;
        chunkSizeInts = chunkSizeMeters*intsPerMeter;
    }
    else
    {
        // each meter has less than 32 bits.
        int MetersPerInt = 32/voxPerMeter;
        chunkSizeInts = glm::ceil(chunkSizeMeters/(double)MetersPerInt);
    }
    std::cout << "chunkSizeInts " << chunkSizeInts << std::endl;
    occupancyXsize = chunkSizeInts;
    occupancyYsize = occupancyZsize = chunkSizeInts*32;
    occupancyInts = std::vector<uint32_t>(occupancyXsize*occupancyYsize*occupancyZsize, 0); 
    std::cout << "occupancyBits Size " << occupancyInts.size() << std::endl;
}

void Chunk::updateChunk(float deltaTime, bool gridFill, bool floor, bool sphere){
    float start = glfwGetTime();
    // SPHERE DISPLAYING
    time += deltaTime;
    glm::vec3 spherePos = calculateSphere(deltaTime);
    // indexes into occupancyInts
    for (int z=0; z<occupancyZsize; z++)
    {
        for (int y=0; y<occupancyYsize; y++)
        {
            for (int x=0; x<occupancyXsize; x++)
            {
                uint32_t occupancyInt = 0;
                // position is 0,0,0 for the first voxel.
                glm::vec3 voxPosCenter = glm::vec3(
                    x*32*voxSizeMeters+0.5*voxSizeMeters, // x position of the voxel.
                    y*voxSizeMeters+0.5*voxSizeMeters, // y position of the voxel.
                    z*voxSizeMeters+0.5*voxSizeMeters // z position of the voxel.
                );

                if (gridFill) fillMeterGrid(&occupancyInt, x, y, z);
                if (floor) fillFloor(&occupancyInt, &voxPosCenter, z, x);
                if (sphere) checkSphere(&occupancyInt, &voxPosCenter, &spherePos);
                
                occupancyInts[z*occupancyXsize*occupancyYsize + y*occupancyXsize + x] = occupancyInt;
            }
        }
    }
    std::cout << "Mesh Update Time: " << glfwGetTime()-start << std::endl;
}

void Chunk::updateMesh()
{
    float start = glfwGetTime();
    // Clear Buffers before generating a new mesh.
    vBuff.clear();
    eBuff.clear();  
    cBuff.clear();

    int cCount = 0;
    // indexes into occupancyBits
    for (int z=0; z<occupancyZsize; z++)
    {
        for (int y=0; y<occupancyYsize; y++)
        {
            for (int x=0; x<occupancyXsize; x++)
            {
                // for each bit in the int.
                uint32_t occupancyInt = occupancyInts[z*occupancyXsize*occupancyYsize + y*occupancyXsize + x];
                // skip checking each bit if the whole int is empty.
                if (occupancyInt == 0u) continue;

                for (int bit=0; bit<32; bit++)
                {
                    // check if the bit is set.
                    if (occupancyInt & (1u << bit))
                    {
                        // position is 0,0,0 for the first voxel.
                        glm::vec3 voxPos = glm::vec3(
                            x*32*voxSizeMeters + bit*voxSizeMeters, // x position of the voxel.
                            y*voxSizeMeters, // y position of the voxel.
                            z*voxSizeMeters // z position of the voxel.
                        );

                        int vertIndex = vBuff.size() / 3; // index of the first vertex for this voxel.

                        addCubePrimitive(&voxPos, vertIndex);
                    }
                }
            }
        }
    }

    std::cout << "Mesh Gen Time: " << glfwGetTime()-start << std::endl;
    start = glfwGetTime();
    updateBuffer();
    std::cout << "Mesh Upload Time: " << glfwGetTime()-start << std::endl;

}

// Generate the vertex array, vertex buffer, and color buffer.
void Chunk::bindMesh()
{
    int size = 9000000;
    // VAO
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);
    
    // Vertex Buffer
    glGenBuffers(1, &vBuffID);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffID);
    if (bufferUpdateMethod == 2){
        glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
    }
    else if (bufferUpdateMethod == 1){
        glBufferStorage(GL_ARRAY_BUFFER, size, NULL,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        vPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, size,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    }

    // Color Buffer
    glGenBuffers(1, &cBuffID);
    glBindBuffer(GL_ARRAY_BUFFER, cBuffID);
    if (bufferUpdateMethod == 2){
        glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
    }
    else if (bufferUpdateMethod == 1){
        glBufferStorage(GL_ARRAY_BUFFER, size, NULL,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        cPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, size,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    }

    // Element Buffer
    glGenBuffers(1, &eBuffID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eBuffID);
    if (bufferUpdateMethod == 2){
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
    }
    else if (bufferUpdateMethod == 1){
        glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, size, NULL,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        ePtr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, size,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    }
}

// Bind buffers and draw.
void Chunk::drawMesh(const Program& prog)
{
    float start = glfwGetTime();
    // Quick Sanity Checks
    assert(vBuff.size() % 3 == 0);
    assert(cBuff.size() % 3 == 0);

    // Enable and Bind arrays and buffers
    glBindVertexArray(vaoID);

    GLuint vertAttr = prog.getAttribute("vertPos");
    glEnableVertexAttribArray(vertAttr);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffID);
    glVertexAttribPointer(vertAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    GLuint colorAttr = prog.getAttribute("vertColor");
    glEnableVertexAttribArray(colorAttr);
    glBindBuffer(GL_ARRAY_BUFFER, cBuffID);
    glVertexAttribPointer(colorAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eBuffID);
    
    // Draw mesh
    glDrawElements(GL_TRIANGLES, (int)eBuff.size(), GL_UNSIGNED_INT, (const void *)0);

    glDisableVertexAttribArray(vertAttr);
    glDisableVertexAttribArray(colorAttr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::cout << "Mesh Draw Time: " << glfwGetTime()-start << std::endl;
}   


// Private Methods

void Chunk::fillMeterGrid(uint32_t* occupancyInt, int x, int y, int z)
{
    int bitShifts = 32/voxPerMeter;
    for(int shiftN = 0; shiftN < bitShifts; shiftN++){
        if (z%voxPerMeter == 0 || y%voxPerMeter == 0){
            *occupancyInt |= (0b1 << voxPerMeter*shiftN);
        }
        if (y%voxPerMeter == 0 && z%voxPerMeter == 0){
            *occupancyInt |= 0b11111111111111111111111111111111;
        }
    } 
}

void Chunk::fillFloor(uint32_t* occupancyInt, glm::vec3* voxPosCenter, int z, int x)
{
    
    if (voxPosCenter->y <= voxSizeMeters){
        *occupancyInt |= 0b11111111111111111111111111111111;
    }
    else if (voxPosCenter->y <= voxSizeMeters*3 && z%(x+5) == 0){
        *occupancyInt |= 0b1000000000001000000000001000000 >> (z%5);
    }
}

glm::vec3 Chunk::calculateSphere(float deltaTime){
    float radius = 2;
    glm::vec3 center = glm::vec3(8.0f, 4.0f, 8.0f);
    glm::vec3 offset = glm::vec3(4*glm::sin(time/1.0f), 2*glm::sin(time/1.0f + 3.14f), 4*glm::cos(time/1.0f));
    return center+offset;
}

void Chunk::checkSphere(uint32_t* occupancyInt, glm::vec3* voxPosCenter, glm::vec3* spherePos){
    if ((voxPosCenter->y > spherePos->y+1) || (voxPosCenter->y < spherePos->y-1)) return;
    if ((voxPosCenter->z > spherePos->z+1) || (voxPosCenter->z < spherePos->z-1)) return;
    for (int bit=0; bit<32; bit++)
    {
        glm::vec3 voxPosCenterNew = *voxPosCenter + glm::vec3(0.0f+bit*voxSizeMeters, 0.0f, 0.0f);
        if (glm::length(*spherePos-voxPosCenterNew)<=1){
            *occupancyInt |= 1u << bit;
        }
    }
}

void Chunk::updateBuffer(){
    std::cout << "cBuff data: " << cBuff.size()*sizeof(GLfloat) << std::endl;
    std::cout << "vBuff data: " << vBuff.size()*sizeof(GLfloat) << std::endl;
    std::cout << "eBuff data: " << eBuff.size()*sizeof(unsigned int) << std::endl;


    if (bufferUpdateMethod == 2){
        // MORE ADVANCED AND FASTER
        // buffer orphaning and SubData update
        glBindBuffer(GL_ARRAY_BUFFER, cBuffID); 
        // glBufferData(GL_ARRAY_BUFFER, cBuff.size()*sizeof(GLfloat), NULL, GL_STREAM_DRAW);   
        glBufferSubData(GL_ARRAY_BUFFER, 0, cBuff.size()*sizeof(GLfloat), cBuff.data());

        glBindBuffer(GL_ARRAY_BUFFER, vBuffID);
        // glBufferData(GL_ARRAY_BUFFER, vBuff.size()*sizeof(GLfloat), NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vBuff.size()*sizeof(GLfloat), vBuff.data());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eBuffID);
        // glBufferData(GL_ELEMENT_ARRAY_BUFFER, eBuff.size()*sizeof(unsigned int), NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eBuff.size()*sizeof(unsigned int), eBuff.data());
    }
    else if (bufferUpdateMethod == 1){
        memcpy(cPtr, cBuff.data(), cBuff.size()*sizeof(GLfloat));
        memcpy(vPtr, vBuff.data(), vBuff.size()*sizeof(GLfloat));
        memcpy(ePtr, eBuff.data(), eBuff.size()*sizeof(unsigned int));
    }
    else if (bufferUpdateMethod == 0){
        // PRIMITIVE BUFFER MANAGEMENT
        glBindBuffer(GL_ARRAY_BUFFER, cBuffID); 
        glBufferData(GL_ARRAY_BUFFER, cBuff.size()*sizeof(GLfloat), cBuff.data(), GL_STREAM_DRAW);   

        glBindBuffer(GL_ARRAY_BUFFER, vBuffID);
        glBufferData(GL_ARRAY_BUFFER, vBuff.size()*sizeof(GLfloat), vBuff.data(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eBuffID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, eBuff.size()*sizeof(unsigned int), eBuff.data(), GL_STREAM_DRAW);
    }
}

void Chunk::addCubePrimitive(glm::vec3* voxPos, int vertIndex)
{
    // ADD VERTEXES & COLOR
    // ID 0: 0,0,0 corner
    // ID 1: 1,0,0 +x
    // ID 2: 0,1,0 +y
    // ID 3: 1,1,0 +x+y
    // ID 4: 0,0,1 +z
    // ID 5: 1,0,1 +x+z
    // ID 6: 0,1,1 +y+z
    // ID 7: 1,1,1 +z+y+z
    for (int dz=0; dz<=1; dz++)
    {
        for (int dy=0; dy<=1; dy++)
        {
            for (int dx=0; dx<=1; dx++)
            {
                vBuff.push_back(voxPos->x + dx*voxSizeMeters);
                vBuff.push_back(voxPos->y + dy*voxSizeMeters);
                vBuff.push_back(voxPos->z + dz*voxSizeMeters);

                cBuff.push_back((voxPos->x)/16.0f); // R
                cBuff.push_back((voxPos->y)/16.0f); // G
                cBuff.push_back((voxPos->z)/16.0f); // B
            }
        }
    }

    // ADD TRIANGLES
    // Add 12 triangles for the cube.
    // +x Face:
    eBuff.push_back(vertIndex + 3); // +x+y
    eBuff.push_back(vertIndex + 5); // +x+z
    eBuff.push_back(vertIndex + 1); // +x

    eBuff.push_back(vertIndex + 3); // +x+y 
    eBuff.push_back(vertIndex + 7); // +x+y+z
    eBuff.push_back(vertIndex + 5); // +x+z

    //-x Face
    eBuff.push_back(vertIndex + 6); // +y+z
    eBuff.push_back(vertIndex + 0); // corner
    eBuff.push_back(vertIndex + 4); // +Z
    
    eBuff.push_back(vertIndex + 6); // +y+z
    eBuff.push_back(vertIndex + 2); // +y
    eBuff.push_back(vertIndex + 0); // corner

    // +y Face:
    eBuff.push_back(vertIndex + 6); // +y+z
    eBuff.push_back(vertIndex + 3); // +x+y
    eBuff.push_back(vertIndex + 2); // +y
    
    eBuff.push_back(vertIndex + 6); // +y+z
    eBuff.push_back(vertIndex + 7); // +x+y+z
    eBuff.push_back(vertIndex + 3); // +x+y

    //-y Face
    eBuff.push_back(vertIndex + 5); // +x+z
    eBuff.push_back(vertIndex + 0); // corner
    eBuff.push_back(vertIndex + 1); // +x
    
    eBuff.push_back(vertIndex + 5); // +x+z
    eBuff.push_back(vertIndex + 4); // +z
    eBuff.push_back(vertIndex + 0); // corner

    // +z Face
    eBuff.push_back(vertIndex + 7); // +x+y+z
    eBuff.push_back(vertIndex + 4); // +z
    eBuff.push_back(vertIndex + 5); // +x+z
    
    eBuff.push_back(vertIndex + 7); // +x+y+z
    eBuff.push_back(vertIndex + 6); // +y+z
    eBuff.push_back(vertIndex + 4); // +z

    // -z Face
    eBuff.push_back(vertIndex + 2); // +y 
    eBuff.push_back(vertIndex + 1); // +x
    eBuff.push_back(vertIndex + 0); // corner
    
    eBuff.push_back(vertIndex + 2); // +y
    eBuff.push_back(vertIndex + 3); // +x+y
    eBuff.push_back(vertIndex + 1); // +x
}