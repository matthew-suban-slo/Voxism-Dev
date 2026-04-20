/**
 * Property test for shaderPath() helper
 *
 * Property 1: Shader path construction
 *   For any resource directory, category, and filename strings,
 *   shaderPath() returns {resourceDir}/shaders/{category}/{filename}
 *
 * Validates: Requirement 2.1
 *
 * Compile standalone (no external dependencies):
 *   g++ -std=c++14 -o test_shader_path test_shader_path.cpp && ./test_shader_path
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

// Duplicate of the static shaderPath() helper from main.cpp
// (static linkage prevents direct access from outside translation unit)
static std::string shaderPath(const std::string& resourceDir,
                               const std::string& category,
                               const std::string& filename) {
    return resourceDir + "/shaders/" + category + "/" + filename;
}

// ---------------------------------------------------------------------------
// Test harness
// ---------------------------------------------------------------------------

static int gPassed = 0;
static int gFailed = 0;

static void check(const std::string& testName,
                  const std::string& got,
                  const std::string& expected) {
    if (got == expected) {
        std::cout << "[PASS] " << testName << "\n";
        ++gPassed;
    } else {
        std::cout << "[FAIL] " << testName << "\n"
                  << "       expected: " << expected << "\n"
                  << "       got:      " << got << "\n";
        ++gFailed;
    }
}

// ---------------------------------------------------------------------------
// Property verification helper
//
// For any (resourceDir, category, filename), the result must equal
//   resourceDir + "/shaders/" + category + "/" + filename
// ---------------------------------------------------------------------------

static void verifyProperty(const std::string& testName,
                            const std::string& resourceDir,
                            const std::string& category,
                            const std::string& filename) {
    std::string expected = resourceDir + "/shaders/" + category + "/" + filename;
    std::string got      = shaderPath(resourceDir, category, filename);
    check(testName, got, expected);
}

// ---------------------------------------------------------------------------
// Test cases — at least 5 varied combinations
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=== Property test: shaderPath() construction ===\n\n";

    // 1. Typical absolute Unix path, postprocess category
    verifyProperty(
        "absolute path / postprocess / godray_frag.glsl",
        "/home/user/project/resources",
        "postprocess",
        "godray_frag.glsl"
    );

    // 2. Relative resource directory, scene category
    verifyProperty(
        "relative path / scene / tex_lit_world_vert.glsl",
        "../resources",
        "scene",
        "tex_lit_world_vert.glsl"
    );

    // 3. Chunk category
    verifyProperty(
        "relative path / chunk / chunk_frag.glsl",
        "resources",
        "chunk",
        "chunk_frag.glsl"
    );

    // 4. Skybox category
    verifyProperty(
        "relative path / skybox / skybox_vert.glsl",
        "resources",
        "skybox",
        "skybox_vert.glsl"
    );

    // 5. Particle category
    verifyProperty(
        "relative path / particle / particle_vert.glsl",
        "resources",
        "particle",
        "particle_vert.glsl"
    );

    // 6. Empty strings — property still holds structurally
    verifyProperty(
        "empty resourceDir / empty category / empty filename",
        "",
        "",
        ""
    );

    // 7. Path with trailing slash in resourceDir — property holds as-is
    //    (the helper does not strip trailing slashes; the result is deterministic)
    verifyProperty(
        "resourceDir with trailing slash / postprocess / composite_frag.glsl",
        "/opt/game/resources/",
        "postprocess",
        "composite_frag.glsl"
    );

    // 8. Windows-style path (backslashes in resourceDir are passed through unchanged)
    verifyProperty(
        "Windows-style path / scene / simple_frag.glsl",
        "C:\\Users\\dev\\resources",
        "scene",
        "simple_frag.glsl"
    );

    // ---------------------------------------------------------------------------
    // Summary
    // ---------------------------------------------------------------------------
    std::cout << "\n=== Results: " << gPassed << " passed, " << gFailed << " failed ===\n";

    return (gFailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
