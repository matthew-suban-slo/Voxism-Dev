#include "Chunk.h"
#include "ChunkManager.h"

Chunk::Chunk(ChunkManager& cm, ChunkPos& cp):
    cm(cm),
    bufferUpdateMethod(0), // set to either 0,1, or 2;
    modifierUpdateQueue()
    {
    // intiialize chunk to zeros.
    occupancyInts = std::vector<uint32_t>(cm.occupancyXsize*cm.occupancyYsize*cm.occupancyZsize, 0); 
    // intialize const for world position of chunk.
    worldcp = glm::vec3(cp.x*cm.chunkSizeMeters, 
                        cp.y*cm.chunkSizeMeters, 
                        cp.z*cm.chunkSizeMeters);
    // sanity check for update method.
    assert(bufferUpdateMethod >= 0 && bufferUpdateMethod <= 2);
}

void Chunk::generate(){
    for (int z=0; z<cm.occupancyZsize; z++)
    {
        for (int y=0; y<cm.occupancyYsize; y++)
        {
            for (int x=0; x<cm.occupancyXsize; x++)
            {
                int occupancyIndex = z * cm.occupancyXsize * cm.occupancyYsize + y * cm.occupancyXsize + x;
                uint32_t& occupancyInt = occupancyInts[occupancyIndex];

                glm::vec3 voxPosCenter = glm::vec3(
                    x*32*cm.voxSizeMeters+0.5*cm.voxSizeMeters, // x position of the voxel.
                    y*cm.voxSizeMeters+0.5*cm.voxSizeMeters, // y position of the voxel.
                    z*cm.voxSizeMeters+0.5*cm.voxSizeMeters // z position of the voxel.
                );
                fillFloor(&occupancyInt, &voxPosCenter, x, z);
                fillChunkGrid(&occupancyInt, x, y, z);
            }
        }
    }
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

void Chunk::updateOccupancy(){
    // float start = glfwGetTime();

    // indexes into occupancyInts
    for (int z=0; z<cm.occupancyZsize; z++)
    {
        for (int y=0; y<cm.occupancyYsize; y++)
        {
            for (int x=0; x<cm.occupancyXsize; x++)
            {
                int occupancyIndex = z * cm.occupancyXsize * cm.occupancyYsize + y * cm.occupancyXsize + x;
                uint32_t& occupancyInt = occupancyInts[occupancyIndex];

                // For each ChunkModifier in updateQueue call checkAndFill()
                for (auto chunkMod : modifierUpdateQueue){
                    chunkMod->checkAndFill(occupancyInt, x, y, z);
                }
            }
        }
    }
    modifierUpdateQueue.clear();
    // std::cout << "Mesh Update Time: " << glfwGetTime()-start << std::endl;
}

void Chunk::updateMesh()
{
    // float start = glfwGetTime();
    
    // Clear Buffers before generating a new mesh.
    vBuff.clear();
    eBuff.clear();  
    cBuff.clear();

    int cCount = 0;
    // indexes into occupancyBits
    for (int z=0; z<cm.occupancyZsize; z++)
    {
        for (int y=0; y<cm.occupancyYsize; y++)
        {
            for (int x=0; x<cm.occupancyXsize; x++)
            {
                // Get occupancyInt
                int occupancyIndex = z*cm.occupancyXsize*cm.occupancyYsize + y*cm.occupancyXsize + x;
                uint32_t occupancyInt = occupancyInts[occupancyIndex];
                
                // skip checking each bit if the whole int is empty.
                if (occupancyInt == 0u) continue;

                // check each bit.
                for (int bit=0; bit<32; bit++)
                {
                    // check if the bit is set.
                    if (occupancyInt & (1u << bit))
                    {
                        // position is 0,0,0 for the first voxel.
                        glm::vec3 voxPos = glm::vec3(
                            x*32*cm.voxSizeMeters + bit*cm.voxSizeMeters, // x position of the voxel.
                            y*cm.voxSizeMeters, // y position of the voxel.
                            z*cm.voxSizeMeters // z position of the voxel.
                        );

                        int vertIndex = vBuff.size() / 3; // index of the first vertex for this voxel.

                        addCubePrimitive(&voxPos, vertIndex);
                    }
                }
            }
        }
    }

    // std::cout << "Mesh Gen Time: " << glfwGetTime()-start << std::endl;
    // start = glfwGetTime();
    updateBuffer();
    // std::cout << "Mesh Upload Time: " << glfwGetTime()-start << std::endl;

}

// Bind buffers and draw.
void Chunk::drawMesh(const Program& prog)
{
    // float start = glfwGetTime();
    // Quick Sanity Checks
    
    assert(vBuff.size() % 3 == 0);
    assert(cBuff.size() % 3 == 0);

    // Enable and Bind arrays and buffers
    glBindVertexArray(vaoID);

    GLuint vertAttr = prog.getAttribute("vertPos");
    if (vertAttr == -1) {
    std::cerr << "Shader vertex attribute not found in Chunk draw" << std::endl;
    return;
    }
    glEnableVertexAttribArray(vertAttr);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffID);
    glVertexAttribPointer(vertAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    GLuint colorAttr = prog.getAttribute("vertColor");
    if (colorAttr == -1) {
    std::cerr << "Shader color attribute not found in Chunk draw" << std::endl;
    return;
    }
    glEnableVertexAttribArray(colorAttr);
    glBindBuffer(GL_ARRAY_BUFFER, cBuffID);
    glVertexAttribPointer(colorAttr, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eBuffID);
    
    // Draw mesh
    glDrawElements(GL_TRIANGLES, (int)eBuff.size(), GL_UNSIGNED_INT, (const void *)0);

    glDisableVertexAttribArray(vertAttr);
    glDisableVertexAttribArray(colorAttr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

    // std::cout << "Mesh Draw Time: " << glfwGetTime()-start << std::endl;
}   


// Private Methods
void Chunk::fillMeterGrid(uint32_t* occupancyInt, int x, int y, int z)
{
    int bitShifts = 32/cm.voxPerMeter;
    for(int shiftN = 0; shiftN < bitShifts; shiftN++){
        if (z%cm.voxPerMeter == 0 || y%cm.voxPerMeter == 0){
            *occupancyInt |= (0b1 << cm.voxPerMeter*shiftN);
        }
        if (y%cm.voxPerMeter == 0 && z%cm.voxPerMeter == 0){
            *occupancyInt |= 0b11111111111111111111111111111111;
        }
    } 
}

void Chunk::fillChunkGrid(uint32_t* occupancyInt, int x, int y, int z)
{
    // fill lines in the X direction
    if ((y == 0 || y == cm.occupancyYsize-1) && 
        (z == 0 || z == cm.occupancyZsize-1)){
        *occupancyInt |= 0b11111111111111111111111111111111;
    }
    // fill wall on the +x Side
    if ((x == 0) &&
             ((y == 0 || y == cm.occupancyYsize-1) ||
              (z == 0 || z == cm.occupancyZsize-1))){
         *occupancyInt |= 0b00000000000000000000000000000001;
    }
    // fill wall on the -x side
    if ((x == cm.occupancyXsize-1) &&
             ((y == 0 || y == cm.occupancyYsize-1) ||
              (z == 0 || z == cm.occupancyZsize-1))){
         *occupancyInt |= 0b10000000000000000000000000000000;
    }
}

void Chunk::fillFloor(uint32_t* occupancyInt, glm::vec3* voxPosCenter, int x, int z)
{
    
    if (voxPosCenter->y <= cm.voxSizeMeters){
        *occupancyInt |= 0b11111111111111111111111111111111;
    }
    // random grass stuff
    // else if (voxPosCenter->y <= cm.voxSizeMeters*3 && z%(x+5) == 0){
    //     *occupancyInt |= 0b1000000000001000000000001000000 >> (z%5);
    // }
}

void Chunk::updateBuffer(){
    // std::cout << "cBuff data: " << cBuff.size()*sizeof(GLfloat) << std::endl;
    // std::cout << "vBuff data: " << vBuff.size()*sizeof(GLfloat) << std::endl;
    // std::cout << "eBuff data: " << eBuff.size()*sizeof(unsigned int) << std::endl;


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
                vBuff.push_back(worldcp.x + voxPos->x + dx*cm.voxSizeMeters);
                vBuff.push_back(worldcp.y + voxPos->y + dy*cm.voxSizeMeters);
                vBuff.push_back(worldcp.z + voxPos->z + dz*cm.voxSizeMeters);

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