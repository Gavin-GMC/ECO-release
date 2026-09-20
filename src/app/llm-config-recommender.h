// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <string>

#include "app-facade.h"
#include "llm-client.h"

namespace EvoFlow
{
    namespace App
    {
        struct LlmConfigRecommendationResult
        {
            bool        ok = false;
            std::string errorCode;
            std::string error;
            int         httpStatus = 0;
            std::string configId;
            std::string sourceVersion;
            std::string canonicalJson;
            int         candidateCount = 0;
            int         attempts = 0;
        };

        // Selects one existing, mechanically decoded config. Model output is only
        // an exact candidate ID; no model-authored config text is ever accepted.
        LlmConfigRecommendationResult recommendConfig(
            const LlmSettings& settings,
            const ProblemDescription& problem,
            const std::string& configDirectory = "config");
    }
}
