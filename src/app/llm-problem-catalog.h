// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <string>
#include <vector>

namespace EvoFlow
{
    namespace App
    {
        struct ProblemTemplateMaterial
        {
            std::string type;
            std::string family;
            std::string title;
            std::string classification;
            std::string guide;
            std::string example;
        };

        struct ProblemTemplateCatalog
        {
            bool ok = false;
            std::string error;
            std::vector<ProblemTemplateMaterial> materials;

            const ProblemTemplateMaterial* find(const std::string& type) const;
        };

        // Pure discovery entry point used by tests and by the process-wide runtime
        // catalog. registeredTypes is kept explicit so discovery can be tested
        // without mutating the singleton problem-template registry.
        ProblemTemplateCatalog discoverProblemTemplateCatalog(
            const std::string& root,
            const std::vector<std::string>& registeredTypes);

        // Lazily discovers _pdata from the current working directory. A process
        // restart intentionally refreshes this immutable catalog.
        const ProblemTemplateCatalog& runtimeProblemTemplateCatalog();
    }
}
