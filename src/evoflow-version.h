// SPDX-License-Identifier: BSD-3-Clause

#pragma once

// Public product version. Configuration and result-file format versions are
// independent and must not be derived from this value.
#define EVOFLOW_VERSION_MAJOR 3
#define EVOFLOW_VERSION_MINOR 0
#define EVOFLOW_VERSION_PATCH 1
#define EVOFLOW_VERSION_REVISION 5
#define EVOFLOW_VERSION_STRING "3.0.1.5"

namespace EvoFlow
{
    namespace ProductVersion
    {
        inline constexpr int Major = EVOFLOW_VERSION_MAJOR;
        inline constexpr int Minor = EVOFLOW_VERSION_MINOR;
        inline constexpr int Patch = EVOFLOW_VERSION_PATCH;
        inline constexpr int Revision = EVOFLOW_VERSION_REVISION;
        inline constexpr const char* String = EVOFLOW_VERSION_STRING;
    }
}
