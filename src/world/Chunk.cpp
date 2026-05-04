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
                // fillMeterGrid(&occupancyInt, x, y, z);
            }
        }
    }
}

// Generate the vertex array, vertex buffer, and color buffer.
void Chunk::bindMesh()
{
    int size = 1000000;
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
    // 0 = oringal
    // 1 = binaryMeshing
    // 2 = binaryMeshing and greedy meshing (WIP)
    int updateType = 1;
    int debugMode = 1; // 0 = off, 1 = on.
    float start;
    if (debugMode == 1) start  = glfwGetTime();
    
    // Clear Buffers before generating a new mesh.
    vBuff.clear();
    eBuff.clear();  
    cBuff.clear();

    if (updateType == 1){
        // CREATE BITMASKS

        // Allocate Memory 
        // - FUTURE: Some overhead since vectors initialize to zero.
        // - FUTURE: Possibly keep as a field variable to speed up updates.
        // - CURRENT: allocation time is about 0.0025 for Paul's computer.
                // float allocStart = glfwGetTime();

        // - BIG FUTURE CHANGEF 
        //   if (x != cm.occupancyXsize-1)
        //   if (x != 0)
        //   These checks could be optimized if make each int 30 bits of voxels and 1 padding on either side.

        // - CURRENT
        //   __builtin_clz() //count leading zeros.
        //   __builtin_ctz() //count trailing zeros.

        std::vector<uint32_t> mask = std::vector<uint32_t>(occupancyInts.size()*6);
        // get Pointers to each array.
        uint32_t *posXMask = mask.data() + occupancyInts.size()*0;
        uint32_t *negXMask = mask.data() + occupancyInts.size()*1;
        uint32_t *posYMask = mask.data() + occupancyInts.size()*2;
        uint32_t *negYMask = mask.data() + occupancyInts.size()*3;
        uint32_t *posZMask = mask.data() + occupancyInts.size()*4;
        uint32_t *negZMask = mask.data() + occupancyInts.size()*5;

        int yxOffset = cm.occupancyXsize*cm.occupancyYsize;
        
        // indexes into occupancyBits
        for (int z=0; z<cm.occupancyZsize; z++)
        {
            for (int y=0; y<cm.occupancyYsize; y++)
            {
                for (int x=0; x<cm.occupancyXsize; x++)
                {
                    // Get occupancyInt
                    int occupancyIndex = z*yxOffset + y*cm.occupancyXsize + x;
                    uint32_t occupancyInt = occupancyInts[occupancyIndex];
                    if (occupancyInt == 0u) continue;
                    // reused variables for each case.
                    uint32_t occupancyIntMod;
                    uint32_t neighbor;
                    bool carry;

                    

                //   0 i-1   ->   i+1 +x
                    // 001 011100 100 orig
                    // 001 111001 100 shift left carry (right lmb)
                    // 001 000110 100 not
                    // 001 000100 100 and
                    // Positive X Faces
                    if (x != cm.occupancyXsize-1){
                        neighbor = (occupancyInts[occupancyIndex+1]);
                        carry = (neighbor!=0) && (0==__builtin_clz(neighbor));
                        if (carry){
                            occupancyIntMod = (occupancyInt << 1u) | (1u); //carry one from the right
                        } else {
                            occupancyIntMod = occupancyInt << 1u;
                        }
                    } else {
                        occupancyIntMod = occupancyInt << 1u;
                    }
                    occupancyIntMod = occupancyInt & ~(occupancyIntMod);
                    posXMask[occupancyIndex] = occupancyIntMod;

                //   0 i-1   ->   i+1 +X
                    // 001 011100 100 orig
                    // 001 001110 100 shift Right carry (left rmb)
                    // 001 110001 100 not
                    // 001 010000 100 and
                    // negative X faces
                    if (x!=0){
                        neighbor = occupancyInts[occupancyIndex-1];
                        carry = (neighbor!=0) && (0==__builtin_ctz(neighbor));
                        if (carry){
                            occupancyIntMod = (occupancyInt >> 1u) | (1u << 31u); //carry one from the left
                        } else {
                            occupancyIntMod = occupancyInt >> 1u;
                        }
                    } else {
                        occupancyIntMod = occupancyInt >> 1u;
                    }
                    occupancyIntMod = occupancyInt & ~(occupancyIntMod);
                    negXMask[occupancyIndex] = occupancyIntMod;

                    // positive Y faces
                    if (y != cm.occupancyYsize-1){
                        neighbor = occupancyInts[occupancyIndex + cm.occupancyXsize];
                        
                        occupancyIntMod = occupancyInt & ~(neighbor);
                        posYMask[occupancyIndex] = occupancyIntMod;
                    } else {
                        posYMask[occupancyIndex] = occupancyInt;
                    } 

                    // negative Y faces
                    if (y != 0){
                        neighbor = occupancyInts[occupancyIndex - cm.occupancyXsize];

                        occupancyIntMod = occupancyInt & ~(neighbor);
                        negYMask[occupancyIndex] = occupancyIntMod;
                    } else {
                        negYMask[occupancyIndex] = occupancyInt;
                    }

                    // positive Z faces
                    if (z != cm.occupancyZsize-1){
                        neighbor = occupancyInts[occupancyIndex + cm.occupancyXsize*cm.occupancyYsize];
                        
                        occupancyIntMod = occupancyInt & ~(neighbor);
                        posZMask[occupancyIndex] = occupancyIntMod;
                    } else {
                        posZMask[occupancyIndex] = occupancyInt;
                    } 

                    // negative z faces
                    if (z != 0){
                        neighbor = occupancyInts[occupancyIndex - cm.occupancyXsize*cm.occupancyYsize];

                        occupancyIntMod = occupancyInt & ~(neighbor);
                        negZMask[occupancyIndex] = occupancyIntMod;
                    } else {
                        negZMask[occupancyIndex] = occupancyInt;
                    }
                }
            }
        }
        
        
        // GENERATE QUADS FROM BITMASKS
        for (int z=0; z<cm.occupancyZsize; z++)
        {
            for (int y=0; y<cm.occupancyYsize; y++)
            {
                for (int x=0; x<cm.occupancyXsize; x++)
                {
                    // get mask ints
                    int maskIndex = z*yxOffset + y*cm.occupancyXsize + x;
                    uint32_t posXint = posXMask[maskIndex];
                    uint32_t negXint = negXMask[maskIndex];
                    uint32_t posYint = posYMask[maskIndex];
                    uint32_t negYint = negYMask[maskIndex];
                    uint32_t posZint = posZMask[maskIndex];
                    uint32_t negZint = negZMask[maskIndex];
                    
                    float xPos = x*32*cm.voxSizeMeters; // x position of the voxel.
                    float yPos = y*cm.voxSizeMeters;
                    float zPos = z*cm.voxSizeMeters;

                    // loops until posXint is 0
                    while (posXint){
                        // counts the number of trailing zeros.
                        int bit = __builtin_ctz(posXint); // 0 to 31
                        // removes the closest trailing bit
                        posXint &= posXint-1;
                        addQuad(0, xPos+(31-bit)*cm.voxSizeMeters, yPos, zPos);
                    }
                    while (negXint){
                        int bit = __builtin_ctz(negXint);
                        negXint &= negXint-1;
                        addQuad(1, xPos+(31-bit)*cm.voxSizeMeters, yPos, zPos);
                    }
                    while(posYint){
                        int bit = __builtin_ctz(posYint);
                        posYint &= posYint-1;
                        addQuad(2, xPos+(31-bit)*cm.voxSizeMeters, yPos, zPos);
                    }
                    while(negYint){
                        int bit = __builtin_ctz(negYint);
                        negYint &= negYint-1;
                        addQuad(3, xPos+(31-bit)*cm.voxSizeMeters, yPos, zPos);
                    }
                    while(posZint){
                        int bit = __builtin_ctz(posZint);
                        posZint &= posZint-1;
                        addQuad(4, xPos+(31-bit)*cm.voxSizeMeters, yPos, zPos);
                    }
                    while(negZint){
                        int bit = __builtin_ctz(negZint);
                        negZint &= negZint-1;
                        addQuad(5, xPos+(31-bit)*cm.voxSizeMeters, yPos, zPos);
                    }
                }
            }
        }
    }
    if (updateType == 0){
        for (int z=0; z<cm.occupancyZsize; z++){
            for (int y=0; y<cm.occupancyYsize; y++){
                for (int x=0; x<cm.occupancyXsize; x++){
                    // Get occupancyInt
                    int occupancyIndex = z*cm.occupancyXsize*cm.occupancyYsize + y*cm.occupancyXsize + x;
                    uint32_t occupancyInt = occupancyInts[occupancyIndex];
                    // skip checking each bit if the whole int is empty.
                    if (occupancyInt == 0u) continue;
                    // check each bit.
                    for (int bit=0; bit<32; bit++){
                        // check if the bit is set.
                        if (occupancyInt & (1u << bit)){
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
    }
 
    if (debugMode == 1) std::cout << "MeshUpdate: " << std::fixed << std::setprecision(4) << glfwGetTime()-start << "s updateType:" << updateType << std::endl;
    // std::cout << "eBuff Size:" << eBuff.size() << std::endl;
    // std::cout << "cBuff Size:" << cBuff.size() << std::endl;
    // std::cout << "vBuff Size:" << vBuff.size() << std::endl;
    
    updateBuffer();

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
    int bitShifts = 32;
    for(int shiftN = 0; shiftN < bitShifts; shiftN += cm.voxPerMeter){
        if (z%cm.voxPerMeter == 0 || y%cm.voxPerMeter == 0){
            // *occupancyInt |= (0b1 << cm.voxPerMeter*shiftN);
            *occupancyInt |= (1u << (31-shiftN));
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
    if ((x == cm.occupancyXsize-1) &&
             ((y == 0 || y == cm.occupancyYsize-1) ||
              (z == 0 || z == cm.occupancyZsize-1))){
         *occupancyInt |= 0b00000000000000000000000000000001;
    }
    // fill wall on the -x side
    if ((x == 0) &&
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

void Chunk::addQuad(int side, float xPos, float yPos, float zPos){

    // posX
    if (side == 0){
        // 0 ---- 3
        // | \__  |
        // |    \ |
        // 1------2
        int vertIndex = vBuff.size() / 3; // index of the first vertex for this voxel.
        // 0 = top left
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 1 = bottom left
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 2 = bottom right
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 3 = top right
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // tri 1
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 2);
        eBuff.push_back(vertIndex + 3);
        // tri 2
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 1);
        eBuff.push_back(vertIndex + 2);
    }
    // negX
    else if (side == 1){
        // 0 ---- 3
        // | \__  |
        // |    \ |
        // 1------2
        int vertIndex = vBuff.size() / 3; // index of the first vertex for this voxel.
        // 0 = top left
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 1 = bottom left
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 2 = bottom right
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 3 = top right
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // tri 1
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 2);
        eBuff.push_back(vertIndex + 3);
        // tri 2
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 1);
        eBuff.push_back(vertIndex + 2);
    }
    // posY
    else if (side == 2){
        // 0 ---- 3
        // | \__  |
        // |    \ |
        // 1------2
        int vertIndex = vBuff.size() / 3; // index of the first vertex for this voxel.
        // 0 = top left
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 1 = bottom left
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 2 = bottom right
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 3 = top right
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // tri 1
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 2);
        eBuff.push_back(vertIndex + 3);
        // tri 2
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 1);
        eBuff.push_back(vertIndex + 2);
    }
    // negY
    else if (side == 3){
        // 0 ---- 3
        // | \__  |
        // |    \ |
        // 1------2
        int vertIndex = vBuff.size() / 3; // index of the first vertex for this voxel.
        // 0 = top left
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 1 = bottom left
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 2 = bottom right
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 3 = top right
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // tri 1
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 2);
        eBuff.push_back(vertIndex + 3);
        // tri 2
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 1);
        eBuff.push_back(vertIndex + 2);
    }
    // posZ
    else if (side == 4){
        // 0 ---- 3
        // | \__  |
        // |    \ |
        // 1------2
        int vertIndex = vBuff.size() / 3; // index of the first vertex for this voxel.
        // 0 = top left
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 1 = bottom left
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 2 = bottom right
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 3 = top right
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 1*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // tri 1
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 2);
        eBuff.push_back(vertIndex + 3);
        // tri 2
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 1);
        eBuff.push_back(vertIndex + 2);
    }
    // negZ
    else if (side == 5){
        // 0 ---- 3
        // | \__  |
        // |    \ |
        // 1------2
        int vertIndex = vBuff.size() / 3; // index of the first vertex for this voxel.
        // 0 = top left
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 1 = bottom left
        vBuff.push_back(worldcp.x + xPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 2 = bottom right
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // 3 = top right
        vBuff.push_back(worldcp.x + xPos + 0*cm.voxSizeMeters);
        vBuff.push_back(worldcp.y + yPos + 1*cm.voxSizeMeters);
        vBuff.push_back(worldcp.z + zPos + 0*cm.voxSizeMeters);
        cBuff.push_back((xPos)/16.0f); // R
        cBuff.push_back((yPos)/16.0f); // G
        cBuff.push_back((zPos)/16.0f); // B
        // tri 1
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 2);
        eBuff.push_back(vertIndex + 3);
        // tri 2
        eBuff.push_back(vertIndex + 0);
        eBuff.push_back(vertIndex + 1);
        eBuff.push_back(vertIndex + 2);
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