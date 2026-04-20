/**
 * Property test for noise texture vector generation
 *
 * Property 4: Noise vector normalization
 *   Each generated noise vector has z = 0 and XY magnitude ≈ 1.0
 *   (normalized in the XY plane).
 *
 * Validates: Requirement 7.2
 *
 * Compile standalone (requires GLM in ext/):
 *   g++ -std=c++14 -I../../ext -o test_noise_vectors test_noise_vectors.cpp && ./test_noise_vectors
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// ---------------------------------------------------------------------------
// Free-function duplicate of the CPU-side noise vector generation from
// Application::generateNoiseTexture() in main.cpp.
// Only the vector generation is duplicated — no GL texture upload.
// ---------------------------------------------------------------------------
static std::vector<glm::vec3> generateNoiseVectors() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine gen;
    std::vector<glm::vec3> noiseData;
    noiseData.reserve(16);
    for (int i = 0; i < 16; ++i) {
        glm::vec3 noise(
            dist(gen) * 2.0f - 1.0f,
            dist(gen) * 2.0f - 1.0f,
            0.0f
        );
        noise = glm::normalize(noise);
        noiseData.push_back(noise);
    }
    return noiseData;
}

// ---------------------------------------------------------------------------
// Test harness
// ---------------------------------------------------------------------------

static int gPassed = 0;
static int gFailed = 0;

static void reportCheck(const std::string& testName, bool passed,
                         const std::string& detail = "") {
    if (passed) {
        std::cout << "[PASS] " << testName << "\n";
        ++gPassed;
    } else {
        std::cout << "[FAIL] " << testName;
        if (!detail.empty()) std::cout << "\n       " << detail;
        std::cout << "\n";
        ++gFailed;
    }
}

// ---------------------------------------------------------------------------
// Main — test all 16 noise vectors
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=== Property test: noise texture vector normalization ===\n\n";

    const float epsilon = 1e-5f;

    std::vector<glm::vec3> noiseVectors = generateNoiseVectors();

    // Sub-property A: exactly 16 vectors generated
    {
        bool ok = (noiseVectors.size() == 16);
        std::string detail = ok ? "" :
            "expected 16 vectors, got " + std::to_string(noiseVectors.size());
        reportCheck("count == 16", ok, detail);
    }

    // Sub-property B: each vector has z == 0.0f exactly
    for (int i = 0; i < static_cast<int>(noiseVectors.size()); ++i) {
        const glm::vec3& v = noiseVectors[i];
        bool ok = (v.z == 0.0f);
        std::string name = "v[" + std::to_string(i) + "].z == 0.0";
        std::string detail = ok ? "" :
            "z = " + std::to_string(v.z) + " (expected exactly 0.0)";
        reportCheck(name, ok, detail);
    }

    // Sub-property C: each vector's XY magnitude ≈ 1.0 (within epsilon)
    for (int i = 0; i < static_cast<int>(noiseVectors.size()); ++i) {
        const glm::vec3& v = noiseVectors[i];
        float xyMag = glm::length(glm::vec2(v.x, v.y));
        bool ok = (std::abs(xyMag - 1.0f) < epsilon);
        std::string name = "v[" + std::to_string(i) + "] XY magnitude ≈ 1.0";
        std::string detail = ok ? "" :
            "|XY| = " + std::to_string(xyMag) + " (expected ~1.0, epsilon=" +
            std::to_string(epsilon) + ")";
        reportCheck(name, ok, detail);
    }

    std::cout << "\n=== Results: " << gPassed << " passed, " << gFailed << " failed ===\n";

    return (gFailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
