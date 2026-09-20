// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <string>

namespace EvoFlow
{
    namespace App
    {
        constexpr int LLM_MAX_STAGE_ATTEMPTS = 3;

        struct LlmSettings
        {
            std::string endpoint;
            std::string model;
            std::string apiKey;
        };

        struct LlmPrompt
        {
            std::string system;
            std::string user;
        };

        struct LlmResult
        {
            bool ok = false;
            std::string errorCode;
            std::string error;
            int httpStatus = 0;
            std::string content;
            int attempts = 0;
        };

        // maxAttempts counts actual HTTP requests, including the first request.
        // Only transient transport/provider failures are retried here. Workflow
        // callers pass their remaining stage budget so transport and repair
        // retries can never multiply beyond LLM_MAX_STAGE_ATTEMPTS.
        LlmResult complete(const LlmSettings& settings, const LlmPrompt& prompt,
                           int maxAttempts = LLM_MAX_STAGE_ATTEMPTS);
        LlmResult testLlmConnection(const LlmSettings& settings);
    }
}
