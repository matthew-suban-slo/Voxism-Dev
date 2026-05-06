
#include "Materials.h"

Materials::Materials():
    materials(){}

void Materials::init(GLuint bindingPoint){
    // lambda to help add materials.
    auto addMaterial  = [&](
        glm::vec4 ambient, 
        glm::vec4 diffuse,
        glm::vec4 specular,
        float shininess
    ) {
        materials.push_back(
            Material{ambient, diffuse, specular, shininess});
    };
    // basic grass
    addMaterial(
        glm::vec4(0.04, 0.2, 0.08, 1.0), // amb
        glm::vec4(0.2, 1.0, 0.4, 1.0), // diff
        glm::vec4(0.9, 1.0, 0.9, 1.0), // spec
        15 // shine
    );
    // basic Stone
    addMaterial(
        glm::vec4(0.1, 0.1, 0.1, 1.0), // amb
        glm::vec4(0.6, 0.6, 0.6, 1.0), // diff
        glm::vec4(0.8, 0.8, 0.8, 1.0), // spec
        6 // shine
    );

    
    
    // generate buffer.
    glGenBuffers(1, &matBuffID);
    glBindBuffer(GL_UNIFORM_BUFFER, matBuffID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Material)*materials.size(), materials.data(), GL_STATIC_DRAW);
    // remember index for later and bind buffer to the index.
    bindingPoint = bindingPoint;
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, matBuffID);
}

// typically a set once and forget.
void Materials::bind(){
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, matBuffID);
}