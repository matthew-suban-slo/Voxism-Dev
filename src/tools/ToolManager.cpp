#include "ToolManager.h"

#include <imgui.h>

namespace {
bool raycastToolHit(ChunkManager &chunkManager,
    const glm::vec3 &origin,
    const glm::vec3 &direction,
    float maxDistance,
    ToolRaycastHit &toolHit)
{
    ChunkManager::VoxelRaycastHit hit;
    if (!chunkManager.raycastVoxels(origin, direction, maxDistance, hit)) {
        return false;
    }

    toolHit.voxel = hit.voxel;
    toolHit.adjacent = hit.adjacent;
    toolHit.normal = hit.normal;
    toolHit.position = hit.position;
    toolHit.distance = hit.distance;
    return true;
}
}

bool ToolManager::beginPrimaryAction(ChunkManager &chunkManager,
    const glm::vec3 &origin,
    const glm::vec3 &direction)
{
    ToolRaycastHit toolHit;
    if (!raycastToolHit(chunkManager, origin, direction, maxUseDistance_, toolHit)) {
        return false;
    }

    ToolUseResult result;
    const int chunkSizeVoxels = chunkManager.chunkSizeInts * 32;
    if (activeTool_ == ToolKind::Cube) {
        result = cubeTool_.use(toolHit, chunkSizeVoxels);
    } else if (activeTool_ == ToolKind::Area) {
        result = areaTool_.use(toolHit, chunkSizeVoxels);
    } else if (activeTool_ == ToolKind::Sphere) {
        result = sphereTool_.use(toolHit, chunkSizeVoxels);
    } else {
        result = organicSphereTool_.beginStroke(origin, direction, toolHit, chunkSizeVoxels, chunkManager.voxSizeMeters);
    }

    if (result.modifier) {
        chunkManager.modifyChunks(result.modifier);
    }
    return result.consumed;
}

bool ToolManager::updatePrimaryAction(ChunkManager &chunkManager,
    const glm::vec3 &origin,
    const glm::vec3 &direction)
{
    if (!supportsContinuousPrimaryAction()) {
        return false;
    }

    const int chunkSizeVoxels = chunkManager.chunkSizeInts * 32;
    ToolUseResult result = organicSphereTool_.continueStroke(origin, direction, chunkSizeVoxels, chunkManager.voxSizeMeters);
    if (result.modifier) {
        chunkManager.modifyChunks(result.modifier);
    }
    return result.consumed;
}

void ToolManager::endPrimaryAction()
{
    if (activeTool_ == ToolKind::OrganicSphere) {
        organicSphereTool_.endStroke();
    }
}

bool ToolManager::supportsContinuousPrimaryAction() const
{
    return activeTool_ == ToolKind::OrganicSphere;
}

ToolPreview ToolManager::getPreview(ChunkManager &chunkManager,
    const glm::vec3 &origin,
    const glm::vec3 &direction) const
{
    if (activeTool_ == ToolKind::OrganicSphere && supportsContinuousPrimaryAction()) {
        ChunkManager::VoxelRaycastHit hit;
        if (chunkManager.raycastVoxels(origin, direction, maxUseDistance_, hit)) {
            ToolRaycastHit toolHit;
            toolHit.voxel = hit.voxel;
            toolHit.adjacent = hit.adjacent;
            toolHit.normal = hit.normal;
            toolHit.position = hit.position;
            toolHit.distance = hit.distance;
            return organicSphereTool_.preview(origin, direction, &toolHit, chunkManager.voxSizeMeters);
        }
        return organicSphereTool_.preview(origin, direction, nullptr, chunkManager.voxSizeMeters);
    }

    ChunkManager::VoxelRaycastHit hit;
    if (!chunkManager.raycastVoxels(origin, direction, maxUseDistance_, hit)) {
        return ToolPreview {};
    }

    ToolRaycastHit toolHit;
    toolHit.voxel = hit.voxel;
    toolHit.adjacent = hit.adjacent;
    toolHit.normal = hit.normal;
    toolHit.position = hit.position;
    toolHit.distance = hit.distance;

    if (activeTool_ == ToolKind::Cube) {
        return cubeTool_.preview(toolHit);
    }
    if (activeTool_ == ToolKind::Area) {
        return areaTool_.preview(toolHit);
    }
    if (activeTool_ == ToolKind::Sphere) {
        return sphereTool_.preview(toolHit);
    }
    return organicSphereTool_.preview(origin, direction, &toolHit, chunkManager.voxSizeMeters);
}

void ToolManager::drawImGui()
{
    const ToolKind previousTool = activeTool_;
    int toolIndex = static_cast<int>(activeTool_);
    ImGui::RadioButton("Cube Tool", &toolIndex, static_cast<int>(ToolKind::Cube));
    ImGui::SameLine();
    ImGui::RadioButton("Area Tool", &toolIndex, static_cast<int>(ToolKind::Area));
    ImGui::SameLine();
    ImGui::RadioButton("Sphere Tool", &toolIndex, static_cast<int>(ToolKind::Sphere));
    ImGui::SameLine();
    ImGui::RadioButton("Organic Tool", &toolIndex, static_cast<int>(ToolKind::OrganicSphere));
    activeTool_ = static_cast<ToolKind>(toolIndex);
    if (activeTool_ != previousTool) {
        clearInactiveToolState();
    }

    ImGui::SliderFloat("Tool Range", &maxUseDistance_, 2.0f, 20.0f, "%.1f m");

    if (activeTool_ == ToolKind::Cube) {
        cubeTool_.drawImGui();
    } else if (activeTool_ == ToolKind::Area) {
        areaTool_.drawImGui();
    } else if (activeTool_ == ToolKind::Sphere) {
        sphereTool_.drawImGui();
    } else {
        organicSphereTool_.drawImGui();
    }
}

void ToolManager::clearInactiveToolState()
{
    if (activeTool_ != ToolKind::Area) {
        areaTool_.clearPending();
    }
    if (activeTool_ != ToolKind::OrganicSphere) {
        organicSphereTool_.endStroke();
    }
}
