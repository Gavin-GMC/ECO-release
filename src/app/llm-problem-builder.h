// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <functional>
#include <string>

#include "llm-client.h"

namespace EvoFlow
{
    namespace App
    {
        struct LlmProblemResult
        {
            bool ok = false;
            std::string errorCode;
            std::string error;
            int httpStatus = 0;
            std::string type;
            std::string problemData;
            int attempts = 0;
        };

        struct LlmProblemValidationResult
        {
            bool ok = false;
            bool repairable = true;
            std::string errorCode;
            std::string error;
        };

        struct LlmProblemAttachment
        {
            std::string fileName;
            std::string text;
        };

        using LlmProblemValidator = std::function<LlmProblemValidationResult(
            const std::string& type, const std::string& problemData)>;

        LlmProblemResult classifyProblemDescription(
            const LlmSettings& settings, const std::string& description,
            const LlmProblemAttachment& attachment = LlmProblemAttachment());

        LlmProblemResult generateProblemData(
            const LlmSettings& settings, const std::string& description,
            const std::string& type,
            const LlmProblemValidator& validator = LlmProblemValidator(),
            const LlmProblemAttachment& attachment = LlmProblemAttachment());
    }
}
