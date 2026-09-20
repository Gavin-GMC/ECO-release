// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace EvoFlow
{
namespace App
{
    struct DataPackageReport
    {
        std::string status = "missing";
        std::filesystem::path root;
        std::uintmax_t manifestExpected = 0;
        std::uintmax_t manifestPresent = 0;
        std::uintmax_t loadableInstances = 0;
        std::vector<std::string> issues;

        bool ok() const { return status == "minimal" || status == "full"; }
    };

    namespace DataPackageDetail
    {
        struct ManifestEntry
        {
            std::string path;
            std::uintmax_t bytes = 0;
        };

        inline bool isSafeRelativePath(const std::filesystem::path& path)
        {
            if (path.empty() || path.is_absolute() || path.has_root_path()) return false;
            for (const auto& part : path)
                if (part == "..") return false;
            return true;
        }

        inline bool readManifest(const std::filesystem::path& path,
                                 std::vector<ManifestEntry>& entries,
                                 std::string& error)
        {
            std::ifstream input(path, std::ios::binary);
            if (!input)
            {
                error = "missing " + path.filename().u8string();
                return false;
            }
            std::string text((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());
            if (text.find("\"schema\"") == std::string::npos
                || text.find("evoflow-data-manifest-v1") == std::string::npos)
            {
                error = "unsupported schema in " + path.filename().u8string();
                return false;
            }
            const std::regex pathPattern("\\\"path\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
            const std::regex bytesPattern("\\\"bytes\\\"\\s*:\\s*([0-9]+)");
            std::sregex_iterator pathIt(text.begin(), text.end(), pathPattern), end;
            std::sregex_iterator bytesIt(text.begin(), text.end(), bytesPattern);
            for (; pathIt != end; ++pathIt)
            {
                if (bytesIt == end)
                {
                    error = "missing byte count in " + path.filename().u8string();
                    return false;
                }
                ManifestEntry entry;
                entry.path = (*pathIt)[1].str();
                try { entry.bytes = static_cast<std::uintmax_t>(std::stoull((*bytesIt)[1].str())); }
                catch (...)
                {
                    error = "invalid byte count in " + path.filename().u8string();
                    return false;
                }
                entries.push_back(entry);
                ++bytesIt;
            }
            if (entries.empty())
            {
                error = "empty " + path.filename().u8string();
                return false;
            }
            const std::regex countPattern("\\\"instance_count\\\"\\s*:\\s*([0-9]+)");
            std::smatch countMatch;
            if (!std::regex_search(text, countMatch, countPattern))
            {
                error = "missing instance count in " + path.filename().u8string();
                return false;
            }
            try
            {
                if (static_cast<std::uintmax_t>(std::stoull(countMatch[1].str()))
                    != entries.size())
                {
                    error = "invalid file count in " + path.filename().u8string();
                    return false;
                }
            }
            catch (...)
            {
                error = "invalid instance count in " + path.filename().u8string();
                return false;
            }
            return true;
        }

        inline void verifyEntries(const std::filesystem::path& root,
                                  const std::vector<ManifestEntry>& entries,
                                  DataPackageReport& report)
        {
            report.manifestExpected += entries.size();
            for (const ManifestEntry& entry : entries)
            {
                const std::filesystem::path relative = std::filesystem::u8path(entry.path);
                if (!isSafeRelativePath(relative))
                {
                    report.issues.push_back("unsafe manifest path: " + entry.path);
                    continue;
                }
                std::error_code error;
                const std::filesystem::path candidate = root / relative;
                if (!std::filesystem::is_regular_file(candidate, error))
                {
                    report.issues.push_back("missing " + entry.path);
                    continue;
                }
                const std::uintmax_t bytes = std::filesystem::file_size(candidate, error);
                if (error || bytes != entry.bytes)
                {
                    report.issues.push_back("size mismatch: " + entry.path);
                    continue;
                }
                report.manifestPresent++;
            }
        }

        inline std::uintmax_t countInstances(const std::filesystem::path& root)
        {
            static const std::set<std::string> extensions = {
                ".cflp", ".cpmp", ".cvrp", ".fjsp", ".mds", ".mis",
                ".mkp", ".qap", ".sr", ".stp", ".tsp", ".wfs"
            };
            std::uintmax_t count = 0;
            std::error_code error;
            std::filesystem::recursive_directory_iterator iterator(
                root, std::filesystem::directory_options::skip_permission_denied, error);
            const std::filesystem::recursive_directory_iterator end;
            while (!error && iterator != end)
            {
                const auto& entry = *iterator;
                if (entry.is_symlink(error))
                {
                    if (entry.is_directory(error)) iterator.disable_recursion_pending();
                }
                else if (entry.is_regular_file(error)
                         && extensions.count(entry.path().extension().u8string()) != 0)
                    count++;
                iterator.increment(error);
            }
            return count;
        }
    }

    inline DataPackageReport inspectDataPackage(const std::filesystem::path& root)
    {
        DataPackageReport report;
        report.root = root;
        std::error_code filesystemError;
        if (!std::filesystem::is_directory(root, filesystemError))
        {
            report.issues.push_back("data root is not a directory");
            return report;
        }

        report.loadableInstances = DataPackageDetail::countInstances(root);
        std::vector<DataPackageDetail::ManifestEntry> minimal;
        std::string manifestError;
        const bool minimalOk = DataPackageDetail::readManifest(
            root / "manifest-minimal.json", minimal, manifestError);
        if (!minimalOk)
        {
            report.status = "incomplete";
            report.issues.push_back(manifestError);
            return report;
        }
        DataPackageDetail::verifyEntries(root, minimal, report);

        const std::filesystem::path fullPath = root / "manifest-full.json";
        const bool hasFull = std::filesystem::is_regular_file(fullPath, filesystemError);
        if (hasFull)
        {
            std::vector<DataPackageDetail::ManifestEntry> full;
            manifestError.clear();
            if (!DataPackageDetail::readManifest(fullPath, full, manifestError))
                report.issues.push_back(manifestError);
            else
                DataPackageDetail::verifyEntries(root, full, report);
            if (!std::filesystem::is_regular_file(root / "DATA_LICENSES.md", filesystemError))
                report.issues.push_back("missing DATA_LICENSES.md");
        }

        if (!report.issues.empty()) report.status = "incomplete";
        else report.status = hasFull ? "full" : "minimal";
        return report;
    }
}
}
