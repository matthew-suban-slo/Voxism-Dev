
#pragma once
#ifndef _MATERIALS_H_
#define _MATERIALS_H_

#include <glm/glm.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include "../Program.h"

struct Material{
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    float padding[3];
};

class Materials {
    public:
        Materials();

        void init(GLuint bindingPoint);

        // void addMaterial(Material);

        void bind();

    private:
        std::vector<Material> materials;
        GLuint matBuffID;
        GLuint bindingPoint;
};

#endif