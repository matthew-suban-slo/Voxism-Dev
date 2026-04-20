/**
 * Property test for generateSSAOKernel()
 *
 * Property 3: SSAO kernel distribution bias
 *   For any N >= 2, sample i's magnitude does not exceed
 *   lerp(0.1, 1.0, (i/N)^2), ensuring earlier samples cluster closer to
 *   the origin.
 *
 *   Formally: glm::length(kernel[i]) <= lerp(0.1, 1.0, (i/N)^2) + epsilon
 *   where lerp(0.1, 1.0, t) = 0.1 + t * 0.9
 *
 * Validates: Requirement 6.3
 *
 * Compile standalone (requires GLM in ext/):
 *   g++ -std=c++14 -I../../ext -o test_ssao_bias test_ssao_bias.cpp && ./test_ssao_bias
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// Free-function duplicate of Application::generateSSAOKernel() from main.cpp
// (same implementation as in test_ssao_kernel.cpp)
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

static void verifyDistributionBias(int N) {
    const float epsilon = 1e-5f;  // small tolerance for float precision

    std::vector<glm::vec3> kernel = generateSSAOKernel(N);

    // Verify each sample i satisfies: length(kernel[i]) <= lerp(0.1, 1.0, (i/N)^2) + epsilon
    std::string name = "N=" + std::to_string(N) + " | all length(s[i]) <= lerp(0.1,1.0,(i/N)^2)";
    bool ok = true;
    int failIdx = -1;
    float failLen = 0.0f;
    float failBound = 0.0f;

    for (int i = 0; i < N; ++i) {
        float t = (float)i / (float)N;
        float maxMagnitude = 0.1f + t * t * 0.9f;  // lerp(0.1, 1.0, t^2)
        float len = glm::length(kernel[i]);
        if (len > maxMagnitude + epsilon) {
            ok = false;
            failIdx = i;
            failLen = len;
            failBound = maxMagnitude;
            break;
        }
    }

    std::string detail = "";
    if (!ok) {
        detail = "sample[" + std::to_string(failIdx) + "] has length " +
                 std::to_string(failLen) + " > bound " +
                 std::to_string(failBound) + " (i/N=" +
                 std::to_string((float)failIdx / (float)N) + ")";
    }
    reportCheck(name, ok, detail);
}

// ---------------------------------------------------------------------------
// Main — test multiple values of N >= 2
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=== Property test: generateSSAOKernel() distribution bias ===\n\n";

    const int testCounts[] = {2, 8, 32, 64};
    for (int N : testCounts) {
        verifyDistributionBias(N);
    }

    std::cout << "\n=== Results: " << gPassed << " passed, " << gFailed << " failed ===\n";

    return (gFailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
