#pragma once

#include "AreaTool.h"
#include "CubeTool.h"
#include "OrganicSphereTool.h"
#include "SphereTool.h"

#include "../world/ChunkManager.h"

class ToolManager {
public:
    bool beginPrimaryAction(ChunkManager &chunkManager,
        const glm::vec3 &origin,
        const glm::vec3 &direction);
    bool updatePrimaryAction(ChunkManager &chunkManager,
        const glm::vec3 &origin,
        const glm::vec3 &direction);
    void endPrimaryAction();
    bool supportsContinuousPrimaryAction() const;
    ToolPreview getPreview(ChunkManager &chunkManager,
        const glm::vec3 &origin,
        const glm::vec3 &direction) const;
    void drawImGui();

private:
    void clearInactiveToolState();

    CubeTool cubeTool_;
    AreaTool areaTool_;
    SphereTool sphereTool_;
    OrganicSphereTool organicSphereTool_;
    ToolKind activeTool_ = ToolKind::Cube;
    float maxUseDistance_ = 8.0f;
};
