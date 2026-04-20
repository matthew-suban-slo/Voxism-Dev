/**
 * Property test for generateSSAOKernel()
 *
 * Property 2: SSAO kernel hemisphere invariant
 *   For any positive sample count N, generateSSAOKernel(N) returns exactly N
 *   samples, and each sample vector s satisfies:
 *     - glm::length(s) <= 1.0f  (within unit sphere)
 *     - s.z >= 0.0f             (upper hemisphere)
 *
 * Validates: Requirements 6.1, 6.2
 *
 * Compile standalone (requires GLM in ext/):
 *   g++ -std=c++14 -I../../ext -o test_ssao_kernel test_ssao_kernel.cpp && ./test_ssao_kernel
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// Free-function duplicate of Application::generateSSAOKernel() from main.cpp
// (the original is a member function; extracted here for standalone testing)
// ---------------------------------------------------------------------------
static std::vector<glm::vec3> generateSSAOKernel(int sampleCount) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine gen;
    std::vector<glm::vec3> kernel;
    kernel.reserve(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        glm::vec3 sample(
            dist(gen) * 2.0f - 1.0f,
            dist(gen) * 2.0f - 1.0f,
            dist(gen)
        );
        sample = glm::normalize(sample);
        sample *= dist(gen);
        float scale = (float)i / (float)sampleCount;
        scale = 0.1f + scale * scale * 0.9f;
        sample *= scale;
        kernel.push_back(sample);
    }
    return kernel;
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
// Property verification for a single N
// ---------------------------------------------------------------------------

static void verifyHemisphereInvariant(int N) {
    const float epsilon = 1e-5f;  // small tolerance for float precision

    std::vector<glm::vec3> kernel = generateSSAOKernel(N);

    // --- Sub-property A: exact count ---
    {
        std::string name = "N=" + std::to_string(N) + " | count == N";
        bool ok = (static_cast<int>(kernel.size()) == N);
        std::string detail = ok ? "" :
            "expected " + std::to_string(N) + " samples, got " +
            std::to_string(kernel.size());
        reportCheck(name, ok, detail);
    }

    // --- Sub-property B: each sample in unit sphere ---
    {
        std::string name = "N=" + std::to_string(N) + " | all length(s) <= 1.0";
        bool ok = true;
        int failIdx = -1;
        float failLen = 0.0f;
        for (int i = 0; i < static_cast<int>(kernel.size()); ++i) {
            float len = glm::length(kernel[i]);
            if (len > 1.0f + epsilon) {
                ok = false;
                failIdx = i;
                failLen = len;
                break;
            }
        }
        std::string detail = ok ? "" :
            "sample[" + std::to_string(failIdx) + "] has length " +
            std::to_string(failLen) + " > 1.0";
        reportCheck(name, ok, detail);
    }

    // --- Sub-property C: each sample in upper hemisphere (z >= 0) ---
    {
        std::string name = "N=" + std::to_string(N) + " | all s.z >= 0.0";
        bool ok = true;
        int failIdx = -1;
        float failZ = 0.0f;
        for (int i = 0; i < static_cast<int>(kernel.size()); ++i) {
            if (kernel[i].z < 0.0f) {
                ok = false;
                failIdx = i;
                failZ = kernel[i].z;
                break;
            }
        }
        std::string detail = ok ? "" :
            "sample[" + std::to_string(failIdx) + "] has z=" +
            std::to_string(failZ) + " < 0.0";
        reportCheck(name, ok, detail);
    }
}

// ---------------------------------------------------------------------------
// Main — test multiple values of N
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=== Property test: generateSSAOKernel() hemisphere invariant ===\n\n";

    const int testCounts[] = {1, 4, 16, 32, 64, 128};
    for (int N : testCounts) {
        verifyHemisphereInvariant(N);
    }

    std::cout << "\n=== Results: " << gPassed << " passed, " << gFailed << " failed ===\n";

    return (gFailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
