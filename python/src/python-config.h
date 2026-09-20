//------------------------Description------------------------
// Python optimizer-configuration state shared by config and run bindings.

// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "config-codec.h"

namespace EvoFlow
{
    namespace Python
    {
        struct ConfigState
        {
            FullConfig config;
            std::string sourceVersion = "v3.1";
            std::size_t workflowGeneration = 0;
            std::size_t subpopulationGeneration = 0;
        };

        class Config
        {
        public:
            Config();
            Config(FullConfig value, std::string sourceVersion);

            FullConfig snapshot() const;
            const FullConfig& value() const;
            std::shared_ptr<ConfigState> sharedState() const;

        private:
            std::shared_ptr<ConfigState> _state;
        };
    }
}
