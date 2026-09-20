// SPDX-License-Identifier: BSD-3-Clause

#include "llm-problem-catalog.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "problem-template.h"

namespace EvoFlow
{
    namespace App
    {
        namespace
        {
            namespace fs = std::filesystem;

            constexpr std::size_t MAX_MATERIAL_BYTES = 256ull * 1024ull;
            constexpr std::size_t MAX_CLASSIFICATION_BYTES = 8192;
            constexpr std::size_t MAX_DISCOVERED_FILES = 4096;
            constexpr std::size_t MAX_DISCOVERED_DIRECTORIES = 128;

            struct Readme
            {
                std::string family;
                std::string title;
                std::string classification;
                std::string content;
            };

            struct Example
            {
                std::string type;
                std::string parent;
                std::string relativePath;
                std::string content;
                std::uintmax_t size = 0;
            };

            std::string trim(const std::string& value)
            {
                std::size_t first = 0;
                while (first < value.size()
                       && std::isspace(static_cast<unsigned char>(value[first])))
                    first++;
                std::size_t last = value.size();
                while (last > first
                       && std::isspace(static_cast<unsigned char>(value[last - 1])))
                    last--;
                return value.substr(first, last - first);
            }

            std::string upper(std::string value)
            {
                std::transform(value.begin(), value.end(), value.begin(),
                               [](unsigned char ch) {
                                   return static_cast<char>(std::toupper(ch));
                               });
                return value;
            }

            std::string normalizedPath(const fs::path& path)
            {
                std::string value = path.generic_u8string();
#ifdef _WIN32
                std::transform(value.begin(), value.end(), value.begin(),
                               [](unsigned char ch) {
                                   return static_cast<char>(std::tolower(ch));
                               });
#endif
                return value;
            }

            bool confinedTo(const fs::path& root, const fs::path& path)
            {
                std::string base = normalizedPath(root);
                const std::string child = normalizedPath(path);
                if (!base.empty() && base.back() != '/') base.push_back('/');
                return child == normalizedPath(root)
                    || (child.size() > base.size()
                        && child.compare(0, base.size(), base) == 0);
            }

            bool readBounded(const fs::path& path, std::string& text,
                             std::string& error)
            {
                std::error_code statusError;
                const std::uintmax_t size = fs::file_size(path, statusError);
                if (statusError || size == 0 || size > MAX_MATERIAL_BYTES)
                {
                    error = "Material file is empty, unavailable or larger than 256 KiB: "
                        + path.u8string();
                    return false;
                }
                std::ifstream input(path, std::ios::binary);
                if (!input)
                {
                    error = "Cannot read problem material: " + path.u8string();
                    return false;
                }
                text.resize(static_cast<std::size_t>(size));
                input.read(&text[0], static_cast<std::streamsize>(text.size()));
                if (!input
                    || input.gcount() != static_cast<std::streamsize>(text.size())
                    || text.find('\0') != std::string::npos)
                {
                    error = "Problem material is not a complete text file: "
                        + path.u8string();
                    return false;
                }
                return true;
            }

            bool readCandidate(const fs::path& path, std::uintmax_t size,
                               std::string& text, std::string& error)
            {
                std::ifstream input(path, std::ios::binary);
                if (!input)
                {
                    error = "Cannot read problem material candidate: "
                        + path.u8string();
                    return false;
                }
                text.resize(static_cast<std::size_t>(size));
                input.read(&text[0], static_cast<std::streamsize>(text.size()));
                if (!input
                    || input.gcount() != static_cast<std::streamsize>(text.size()))
                {
                    error = "Problem material candidate could not be read completely: "
                        + path.u8string();
                    return false;
                }
                // _pdata also contains bounded binary benchmark support files
                // (for example CEC .mat files). They are ordinary files but can
                // never be text problem examples, so skip them without weakening
                // the strict README/material checks.
                if (text.find('\0') != std::string::npos) text.clear();
                return true;
            }

            bool parseReadme(const std::string& text, std::string& title,
                             std::string& classification)
            {
                std::istringstream input(text);
                std::string line;
                bool inIntroduction = false;
                bool foundIntroduction = false;
                std::ostringstream summary;
                while (std::getline(input, line))
                {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (title.empty())
                    {
                        if (line.size() >= 3
                            && static_cast<unsigned char>(line[0]) == 0xef
                            && static_cast<unsigned char>(line[1]) == 0xbb
                            && static_cast<unsigned char>(line[2]) == 0xbf)
                            line.erase(0, 3);
                        if (line.rfind("# ", 0) == 0)
                            title = trim(line.substr(2));
                    }
                    if (line.rfind("## 1.", 0) == 0
                        || line.rfind("## 1 ", 0) == 0)
                    {
                        inIntroduction = true;
                        foundIntroduction = true;
                        continue;
                    }
                    if (inIntroduction && line.rfind("## ", 0) == 0)
                        inIntroduction = false;
                    const std::string clean = trim(line);
                    if (inIntroduction && !clean.empty())
                        summary << clean << ' ';
                    if (clean.find("`TYPE:`") != std::string::npos)
                        summary << clean << ' ';
                }
                classification = trim(summary.str());
                if (classification.size() > MAX_CLASSIFICATION_BYTES)
                    classification.resize(MAX_CLASSIFICATION_BYTES);
                return !title.empty() && foundIntroduction
                    && !classification.empty();
            }

            std::string typeHeader(const std::string& text)
            {
                std::istringstream input(text);
                std::string line;
                std::string found;
                while (std::getline(input, line))
                {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    const std::string clean = trim(line);
                    if (clean.empty()) continue;
                    std::istringstream row(clean);
                    std::string key;
                    if (!(row >> key)) continue;
                    if (!key.empty() && key.back() == ':') key.pop_back();
                    if (upper(key) != "TYPE") continue;
                    std::string value;
                    if (!(row >> value)) return std::string();
                    if (value == ":" && !(row >> value)) return std::string();
                    if (!found.empty()) return std::string();
                    found = value;
                }
                return found;
            }

            bool replaceTypeHeader(std::string& text, const std::string& type)
            {
                std::size_t begin = 0;
                bool replaced = false;
                while (begin <= text.size())
                {
                    std::size_t end = text.find('\n', begin);
                    if (end == std::string::npos) end = text.size();
                    std::string line = text.substr(begin, end - begin);
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    std::istringstream row(trim(line));
                    std::string key;
                    if (row >> key)
                    {
                        if (!key.empty() && key.back() == ':') key.pop_back();
                        if (upper(key) == "TYPE")
                        {
                            if (replaced) return false;
                            const bool crlf = end > begin && text[end - 1] == '\r';
                            const std::string replacement =
                                "TYPE: " + type + (crlf ? "\r" : "");
                            text.replace(begin, end - begin, replacement);
                            replaced = true;
                            begin += replacement.size();
                            if (end == text.size()) break;
                            begin++;
                            continue;
                        }
                    }
                    if (end == text.size()) break;
                    begin = end + 1;
                }
                return replaced;
            }

            ProblemTemplateCatalog failed(const std::string& error)
            {
                ProblemTemplateCatalog result;
                result.error = error;
                return result;
            }
        }

        const ProblemTemplateMaterial* ProblemTemplateCatalog::find(
            const std::string& type) const
        {
            for (const ProblemTemplateMaterial& material : materials)
                if (material.type == type) return &material;
            return nullptr;
        }

        ProblemTemplateCatalog discoverProblemTemplateCatalog(
            const std::string& rootText,
            const std::vector<std::string>& registeredTypes)
        {
            if (registeredTypes.empty())
                return failed("The problem-template registry is empty.");

            std::set<std::string> registered;
            for (const std::string& type : registeredTypes)
            {
                if (type.empty() || !registered.insert(type).second)
                    return failed("The problem-template registry contains an invalid "
                                  "or duplicate TYPE.");
            }

            std::error_code rootError;
            const fs::path suppliedRoot = fs::u8path(rootText);
            const fs::file_status rootStatus = fs::symlink_status(
                suppliedRoot, rootError);
            if (rootError || !fs::is_directory(rootStatus)
                || fs::is_symlink(rootStatus))
                return failed("Problem material root is unavailable or is a symlink: "
                              + rootText);
            const fs::path root = fs::weakly_canonical(suppliedRoot, rootError);
            if (rootError)
                return failed("Cannot resolve the problem material root: " + rootText);

            std::map<std::string, Readme> readmes;
            std::vector<Example> examples;
            std::size_t fileCount = 0;
            std::size_t directoryCount = 0;
            std::error_code iterateError;
            fs::recursive_directory_iterator it(root, iterateError);
            const fs::recursive_directory_iterator end;
            if (iterateError)
                return failed("Cannot scan the problem material root.");
            while (it != end)
            {
                const fs::directory_entry entry = *it;
                const fs::file_status status = entry.symlink_status(iterateError);
                if (iterateError)
                    return failed("Cannot inspect a problem material entry.");
                if (fs::is_symlink(status))
                {
                    if (fs::is_directory(status)) it.disable_recursion_pending();
                    it.increment(iterateError);
                    if (iterateError)
                        return failed("Cannot continue scanning problem materials.");
                    continue;
                }
                if (fs::is_directory(status))
                {
                    directoryCount++;
                    if (directoryCount > MAX_DISCOVERED_DIRECTORIES)
                        return failed("Problem material directory count exceeds 128.");
                    it.increment(iterateError);
                    if (iterateError)
                        return failed("Cannot continue scanning problem materials.");
                    continue;
                }
                if (!fs::is_regular_file(status))
                {
                    it.increment(iterateError);
                    if (iterateError)
                        return failed("Cannot continue scanning problem materials.");
                    continue;
                }
                fileCount++;
                if (fileCount > MAX_DISCOVERED_FILES)
                    return failed("Problem material file count exceeds 4096.");

                const fs::path canonical = fs::weakly_canonical(
                    entry.path(), iterateError);
                if (iterateError || !confinedTo(root, canonical))
                    return failed("A problem material path escapes the configured root.");
                const fs::path relative = fs::relative(canonical, root, iterateError);
                if (iterateError)
                    return failed("Cannot derive a problem material relative path.");
                const std::string parent = normalizedPath(relative.parent_path());

                if (entry.path().filename() == fs::path("README.md"))
                {
                    std::string content;
                    std::string readError;
                    if (!readBounded(canonical, content, readError))
                        return failed(readError);
                    Readme readme;
                    readme.family = parent;
                    readme.content = std::move(content);
                    if (!parseReadme(
                            readme.content, readme.title, readme.classification))
                        return failed("Problem README must contain an H1 title and "
                                      "'## 1.' introduction: "
                                      + relative.generic_u8string());
                    if (!readmes.emplace(parent, std::move(readme)).second)
                        return failed("Multiple README files map to one problem family.");
                }
                else
                {
                    std::error_code sizeError;
                    const std::uintmax_t size = fs::file_size(canonical, sizeError);
                    if (!sizeError && size > 0 && size <= MAX_MATERIAL_BYTES)
                    {
                        std::string content;
                        std::string readError;
                        if (!readCandidate(canonical, size, content, readError))
                            return failed(readError);
                        if (content.empty())
                        {
                            it.increment(iterateError);
                            if (iterateError)
                                return failed(
                                    "Cannot continue scanning problem materials.");
                            continue;
                        }
                        const std::string type = typeHeader(content);
                        if (!type.empty() && registered.find(type) != registered.end())
                        {
                            Example example;
                            example.type = type;
                            example.parent = parent;
                            example.relativePath = relative.generic_u8string();
                            example.content = std::move(content);
                            example.size = size;
                            examples.push_back(std::move(example));
                        }
                    }
                }
                it.increment(iterateError);
                if (iterateError)
                    return failed("Cannot continue scanning problem materials.");
            }

            std::map<std::string, std::vector<const Example*>> byType;
            for (const Example& example : examples)
                if (readmes.find(example.parent) != readmes.end())
                    byType[example.type].push_back(&example);

            ProblemTemplateCatalog result;
            for (const std::string& type : registered)
            {
                auto candidates = byType[type];
                if (candidates.empty() && type == "ATSP"
                    && registered.find("TSP") != registered.end())
                {
                    candidates = byType["TSP"];
                }
                if (candidates.empty())
                    return failed("No bounded problem example with an exact TYPE header "
                                  "and sibling README was found for " + type + ".");

                std::set<std::string> parents;
                for (const Example* candidate : candidates)
                    parents.insert(candidate->parent);
                if (parents.size() != 1)
                    return failed("Problem TYPE " + type
                                  + " maps to more than one material family.");

                std::sort(candidates.begin(), candidates.end(),
                          [](const Example* left, const Example* right) {
                              if (left->size != right->size)
                                  return left->size < right->size;
                              return left->relativePath < right->relativePath;
                          });
                const Example& chosen = *candidates.front();
                const Readme& readme = readmes.at(chosen.parent);
                ProblemTemplateMaterial material;
                material.type = type;
                material.family = readme.family;
                material.title = readme.title;
                material.classification = readme.classification;
                material.guide = readme.content;
                material.example = chosen.content;
                if (chosen.type != type
                    && !replaceTypeHeader(material.example, type))
                    return failed("Cannot adapt the shared TSP example for " + type + ".");
                result.materials.push_back(std::move(material));
            }

            std::sort(result.materials.begin(), result.materials.end(),
                      [](const ProblemTemplateMaterial& left,
                         const ProblemTemplateMaterial& right) {
                          return left.type < right.type;
                      });
            if (result.materials.size() != registered.size())
                return failed("Problem material catalog and template registry differ.");
            result.ok = true;
            return result;
        }

        const ProblemTemplateCatalog& runtimeProblemTemplateCatalog()
        {
            static const ProblemTemplateCatalog catalog =
                discoverProblemTemplateCatalog(
                    "_pdata", ProblemTemplateRegistry::instance().types());
            return catalog;
        }
    }
}
