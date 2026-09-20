//------------------------Description------------------------
// EvoFlow 浏览器 UI 的路由与启动实现。

//-------------------------Reference-------------------------
// Windows BCrypt RNG / ShellExecute (Windows SDK).

// SPDX-License-Identifier: BSD-3-Clause

#include "ui-app.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <cmath>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>
#include "app-facade.h"
#include "component-catalog.h"
#include "config-editor.h"
#include "config-validator.h"
#include "http-server.h"
#include "json-reader.h"
#include "json-writer.h"
#include "llm-client.h"
#include "llm-config-recommender.h"
#include "llm-problem-builder.h"
#include "run-reporter.h"
#include "evoflow-version.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <shellapi.h>
#include "ui-assets.generated.h"
#endif

namespace EvoFlow
{
    namespace UI
    {
        namespace fs = std::filesystem;
        using Clock = std::chrono::steady_clock;

        namespace
        {
            constexpr const char* UI_HOST_SWITCH = "--ui-host";

            long long nowMilliseconds()
            {
                return std::chrono::duration_cast<std::chrono::milliseconds>(
                    Clock::now().time_since_epoch()).count();
            }

#ifdef _WIN32
            constexpr DWORD UI_HANDSHAKE_MAGIC = 0x45465549; // "EFUI"
            constexpr DWORD UI_HANDSHAKE_VERSION = 1;
            constexpr DWORD UI_HANDSHAKE_READY = 1;
            constexpr DWORD UI_HANDSHAKE_ERROR = 2;
            constexpr DWORD UI_HANDSHAKE_MAX_TEXT = 1024 * 1024;

            struct UiHandshake
            {
                DWORD magic = UI_HANDSHAKE_MAGIC;
                DWORD version = UI_HANDSHAKE_VERSION;
                DWORD status = 0;
                DWORD port = 0;
                DWORD flags = 0; // bit 0: host has a console
                DWORD tokenSize = 0;
                DWORD errorSize = 0;
            };

            bool writeAll(HANDLE pipe, const void* data, DWORD size)
            {
                const char* cursor = static_cast<const char*>(data);
                DWORD writtenTotal = 0;
                while (writtenTotal < size)
                {
                    DWORD written = 0;
                    if (!WriteFile(pipe, cursor + writtenTotal, size - writtenTotal,
                                   &written, nullptr) || written == 0)
                        return false;
                    writtenTotal += written;
                }
                return true;
            }

            bool readAll(HANDLE pipe, void* data, DWORD size)
            {
                char* cursor = static_cast<char*>(data);
                DWORD readTotal = 0;
                while (readTotal < size)
                {
                    DWORD count = 0;
                    if (!ReadFile(pipe, cursor + readTotal, size - readTotal,
                                  &count, nullptr) || count == 0)
                        return false;
                    readTotal += count;
                }
                return true;
            }

            void sendHandshake(HANDLE pipe, DWORD status, const UiApplication* app,
                               const std::string& error)
            {
                UiHandshake message;
                message.status = status;
                message.flags = GetConsoleWindow() != nullptr ? 1u : 0u;
                const std::string token = app ? app->token() : std::string();
                message.port = app ? app->port() : 0;
                message.tokenSize = static_cast<DWORD>((std::min<std::size_t>)(
                    token.size(), UI_HANDSHAKE_MAX_TEXT));
                message.errorSize = static_cast<DWORD>((std::min<std::size_t>)(
                    error.size(), UI_HANDSHAKE_MAX_TEXT));
                writeAll(pipe, &message, sizeof(message));
                if (message.tokenSize != 0)
                    writeAll(pipe, token.data(), message.tokenSize);
                if (message.errorSize != 0)
                    writeAll(pipe, error.data(), message.errorSize);
            }

            std::wstring currentExecutablePath()
            {
                std::vector<wchar_t> buffer(512);
                for (;;)
                {
                    DWORD count = GetModuleFileNameW(nullptr, buffer.data(),
                                                     static_cast<DWORD>(buffer.size()));
                    if (count == 0) return std::wstring();
                    if (count < buffer.size() - 1)
                        return std::wstring(buffer.data(), count);
                    if (buffer.size() >= 32768) return std::wstring();
                    buffer.resize(buffer.size() * 2);
                }
            }

            std::wstring utf8ToWide(const std::string& value)
            {
                if (value.empty()) return std::wstring();
                int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                               value.data(),
                                               static_cast<int>(value.size()),
                                               nullptr, 0);
                if (size <= 0) return std::wstring();
                std::wstring result(static_cast<std::size_t>(size), L'\0');
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    value.data(), static_cast<int>(value.size()),
                                    &result[0], size);
                return result;
            }

            std::wstring quoteWindowsArgument(const std::wstring& value)
            {
                std::wstring result = L"\"";
                std::size_t slashes = 0;
                for (wchar_t ch : value)
                {
                    if (ch == L'\\')
                    {
                        slashes++;
                        continue;
                    }
                    if (ch == L'"')
                    {
                        result.append(slashes * 2 + 1, L'\\');
                        result.push_back(L'"');
                    }
                    else
                    {
                        result.append(slashes, L'\\');
                        result.push_back(ch);
                    }
                    slashes = 0;
                }
                result.append(slashes * 2, L'\\');
                result.push_back(L'"');
                return result;
            }

            bool parseUnsigned(const char* text, unsigned long long& value)
            {
                if (!text || !*text) return false;
                char* end = nullptr;
                errno = 0;
                unsigned long long parsed = std::strtoull(text, &end, 10);
                if (errno != 0 || !end || *end != '\0') return false;
                value = parsed;
                return true;
            }
#endif

            std::string percentDecode(const std::string& value)
            {
                auto hex = [](char ch) -> int {
                    if (ch >= '0' && ch <= '9') return ch - '0';
                    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
                    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
                    return -1;
                };
                std::string out;
                out.reserve(value.size());
                for (std::size_t i = 0; i < value.size(); i++)
                {
                    if (value[i] == '%' && i + 2 < value.size())
                    {
                        int hi = hex(value[i + 1]), lo = hex(value[i + 2]);
                        if (hi >= 0 && lo >= 0)
                        {
                            out.push_back(static_cast<char>((hi << 4) | lo));
                            i += 2;
                            continue;
                        }
                    }
                    out.push_back(value[i] == '+' ? ' ' : value[i]);
                }
                return out;
            }

            std::string queryValue(const std::string& query, const std::string& key)
            {
                std::size_t begin = 0;
                while (begin <= query.size())
                {
                    std::size_t end = query.find('&', begin);
                    if (end == std::string::npos) end = query.size();
                    std::size_t equal = query.find('=', begin);
                    if (equal != std::string::npos && equal < end
                        && query.substr(begin, equal - begin) == key)
                        return percentDecode(query.substr(equal + 1, end - equal - 1));
                    if (end == query.size()) break;
                    begin = end + 1;
                }
                return "";
            }

            std::vector<std::string> queryValues(const std::string& query, const std::string& key)
            {
                std::vector<std::string> values;
                std::size_t begin = 0;
                while (begin <= query.size())
                {
                    std::size_t end = query.find('&', begin);
                    if (end == std::string::npos) end = query.size();
                    std::size_t equal = query.find('=', begin);
                    if (equal != std::string::npos && equal < end
                        && query.substr(begin, equal - begin) == key)
                        values.push_back(percentDecode(query.substr(equal + 1, end - equal - 1)));
                    if (end == query.size()) break;
                    begin = end + 1;
                }
                return values;
            }

            std::string displayFileName(const std::string& name)
            {
                std::size_t slash = name.find_last_of("/\\");
                std::string clean = slash == std::string::npos ? name : name.substr(slash + 1);
                if (clean.empty()) clean = "problem.data";
                if (clean.size() > 512) clean.resize(512);
                return clean;
            }

            std::string safeLogicalProblemName(const std::string& name,
                                               const std::string& fallback)
            {
                std::string clean;
                clean.reserve((std::min<size_t>)(name.size(), 160));
                for (size_t i = 0; i < name.size() && clean.size() < 160; i++)
                {
                    const unsigned char ch = static_cast<unsigned char>(name[i]);
                    if (ch < 32 || ch == '<' || ch == '>' || ch == ':'
                        || ch == '"' || ch == '/' || ch == '\\' || ch == '|'
                        || ch == '?' || ch == '*')
                        clean.push_back('_');
                    else
                        clean.push_back(static_cast<char>(ch));
                }
                while (!clean.empty()
                       && (clean.back() == ' ' || clean.back() == '.'))
                    clean.pop_back();
                if (clean.empty() || clean == "." || clean == "..")
                    clean = fallback;
                return clean;
            }

            std::string problemNameFromFileName(const std::string& fileName)
            {
                std::string name = displayFileName(fileName);
                const std::size_t dot = name.find_last_of('.');
                if (dot != std::string::npos && dot > 0)
                    name.resize(dot);
                return safeLogicalProblemName(name, "problem");
            }

            std::string generatedProblemName(const std::string& data)
            {
                std::size_t begin = 0;
                for (int line = 0; line < 128 && begin < data.size(); line++)
                {
                    std::size_t end = data.find('\n', begin);
                    if (end == std::string::npos) end = data.size();
                    std::string text = data.substr(begin, end - begin);
                    while (!text.empty()
                           && (text.back() == '\r' || text.back() == ' '
                               || text.back() == '\t'))
                        text.pop_back();
                    const std::size_t first = text.find_first_not_of(" \t");
                    if (first != std::string::npos
                        && text.compare(first, 5, "NAME:") == 0)
                    {
                        const std::size_t valueAt =
                            text.find_first_not_of(" \t", first + 5);
                        if (valueAt != std::string::npos)
                            return safeLogicalProblemName(
                                text.substr(valueAt), std::string());
                    }
                    if (end == data.size()) break;
                    begin = end + 1;
                }
                return std::string();
            }

            bool constantTimeEqual(const std::string& a, const std::string& b)
            {
                std::size_t count = std::max(a.size(), b.size());
                unsigned int different = static_cast<unsigned int>(a.size() ^ b.size());
                for (std::size_t i = 0; i < count; i++)
                {
                    unsigned char left = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
                    unsigned char right = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
                    different |= left ^ right;
                }
                return different == 0;
            }

            std::string randomToken()
            {
#ifdef _WIN32
                unsigned char bytes[32]{};
                NTSTATUS status = BCryptGenRandom(nullptr, bytes, sizeof(bytes),
                                                   BCRYPT_USE_SYSTEM_PREFERRED_RNG);
                if (status < 0) return "";
                static const char* hex = "0123456789abcdef";
                std::string token(64, '0');
                for (std::size_t i = 0; i < sizeof(bytes); i++)
                {
                    token[i * 2] = hex[(bytes[i] >> 4) & 0x0f];
                    token[i * 2 + 1] = hex[bytes[i] & 0x0f];
                }
                return token;
#else
                return "";
#endif
            }

            void addSecurityHeaders(HttpResponse& response);

            HttpResponse jsonError(int status, const std::string& message)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject().key("ok").value(false).key("error").value(message).endObject();
                HttpResponse response;
                response.status = status;
                response.contentType = "application/json; charset=utf-8";
                response.body = out.str();
                response.headers.emplace_back("Cache-Control", "no-store");
                response.headers.emplace_back("X-Content-Type-Options", "nosniff");
                response.headers.emplace_back("Content-Security-Policy",
                                              "default-src 'none'; frame-ancestors 'none'");
                response.headers.emplace_back("Referrer-Policy", "no-referrer");
                return response;
            }

            HttpResponse jsonResponse(const std::string& body, int status = 200)
            {
                HttpResponse response;
                response.status = status;
                response.contentType = "application/json; charset=utf-8";
                response.body = body;
                addSecurityHeaders(response);
                return response;
            }

            bool parseLlmSettings(const std::string& body, App::LlmSettings& settings,
                                  std::string& error)
            {
                const App::JsonParseResult parsed = App::parseJson(body);
                if (!parsed.ok)
                {
                    error = parsed.error;
                    return false;
                }
                if (!parsed.value.isObject())
                {
                    error = "request body must be a JSON object";
                    return false;
                }
                const App::JsonValue* endpoint = parsed.value.find("endpoint");
                const App::JsonValue* model = parsed.value.find("model");
                const App::JsonValue* apiKey = parsed.value.find("apiKey");
                if (!endpoint || !endpoint->isString())
                {
                    error = "endpoint must be a string";
                    return false;
                }
                if (!model || !model->isString())
                {
                    error = "model must be a string";
                    return false;
                }
                if (apiKey && !apiKey->isString())
                {
                    error = "apiKey must be a string";
                    return false;
                }
                if (endpoint->stringValue.size() > 8192)
                {
                    error = "endpoint is too long";
                    return false;
                }
                if (model->stringValue.size() > 512)
                {
                    error = "model is too long";
                    return false;
                }
                if (apiKey && apiKey->stringValue.size() > 16384)
                {
                    error = "apiKey is too long";
                    return false;
                }
                settings.endpoint = endpoint->stringValue;
                settings.model = model->stringValue;
                settings.apiKey = apiKey ? apiKey->stringValue : std::string();
                return true;
            }

            bool parseLlmProblemFields(const std::string& body, std::string& description,
                                       std::string& type, bool requireType,
                                       App::LlmProblemAttachment& attachment,
                                       std::string& error)
            {
                const App::JsonParseResult parsed = App::parseJson(body);
                if (!parsed.ok || !parsed.value.isObject())
                {
                    error = parsed.ok ? "request body must be a JSON object" : parsed.error;
                    return false;
                }
                const App::JsonValue* descriptionValue =
                    parsed.value.find("description");
                if (!descriptionValue || !descriptionValue->isString())
                {
                    error = "description must be a string";
                    return false;
                }
                const App::JsonValue* typeValue = parsed.value.find("type");
                if (requireType && (!typeValue || !typeValue->isString()))
                {
                    error = "type must be a string";
                    return false;
                }
                description = descriptionValue->stringValue;
                type = typeValue && typeValue->isString()
                    ? typeValue->stringValue : std::string();
                const App::JsonValue* attachmentName =
                    parsed.value.find("attachmentName");
                const App::JsonValue* attachmentText =
                    parsed.value.find("attachmentText");
                if ((attachmentName && !attachmentName->isString())
                    || (attachmentText && !attachmentText->isString()))
                {
                    error = "attachmentName and attachmentText must be strings";
                    return false;
                }
                attachment.fileName = attachmentName
                    ? attachmentName->stringValue : std::string();
                attachment.text = attachmentText
                    ? attachmentText->stringValue : std::string();
                return true;
            }

            std::string llmResultJson(const App::LlmResult& result)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(result.ok)
                    .key("errorCode").value(result.errorCode)
                    .key("error").value(result.error)
                    .key("httpStatus").value(result.httpStatus)
                    .key("attempts").value(result.attempts)
                .endObject();
                return out.str();
            }

            std::string llmProblemResultJson(const App::LlmProblemResult& result)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(result.ok)
                    .key("errorCode").value(result.errorCode)
                    .key("error").value(result.error)
                    .key("httpStatus").value(result.httpStatus)
                    .key("type").value(result.type)
                    .key("attempts").value(result.attempts)
                .endObject();
                return out.str();
            }

            std::string llmConfigRecommendationJson(
                const App::LlmConfigRecommendationResult& result)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(result.ok)
                    .key("errorCode").value(result.errorCode)
                    .key("error").value(result.error)
                    .key("httpStatus").value(result.httpStatus)
                    .key("configId").value(result.configId)
                    .key("sourceVersion").value(result.sourceVersion)
                    .key("canonicalJson").value(result.canonicalJson)
                    .key("candidateCount").value(result.candidateCount)
                    .key("attempts").value(result.attempts)
                    .endObject();
                return out.str();
            }

            int llmErrorStatus(const std::string& errorCode)
            {
                if (errorCode == "invalid_settings"
                    || errorCode == "unsupported_endpoint"
                    || errorCode == "invalid_description"
                    || errorCode == "invalid_attachment"
                    || errorCode == "attachment_too_large")
                    return 400;
                if (errorCode == "no_current_problem") return 409;
                if (errorCode == "classification_invalid"
                    || errorCode == "unsupported_problem_type"
                    || errorCode == "generation_invalid"
                    || errorCode == "generated_data_too_large"
                    || errorCode == "generated_data_invalid"
                    || errorCode == "problem_summary_too_large"
                    || errorCode == "recommendation_invalid"
                    || errorCode == "recommended_config_not_found")
                    return 422;
                if (errorCode == "template_catalog_mismatch"
                    || errorCode == "template_material_unavailable"
                    || errorCode == "config_catalog_unavailable"
                    || errorCode == "temporary_problem_unavailable")
                    return 500;
                if (errorCode == "timeout") return 504;
                return 502;
            }

            int llmResponseStatus(const App::LlmResult& result)
            {
                if (result.ok) return 200;
                if (result.errorCode == "invalid_settings"
                    || result.errorCode == "unsupported_endpoint")
                    return 400;
                if (result.errorCode == "timeout") return 504;
                return 502;
            }

            void addSecurityHeaders(HttpResponse& response)
            {
                response.headers.emplace_back("Cache-Control", "no-store");
                response.headers.emplace_back("X-Content-Type-Options", "nosniff");
                response.headers.emplace_back(
                    "Content-Security-Policy",
                    "default-src 'self'; script-src 'self'; style-src 'self'; "
                    "img-src 'self' data:; connect-src 'self'; object-src 'none'; base-uri 'none'");
                response.headers.emplace_back("Referrer-Policy", "no-referrer");
            }

            bool loadBrandLogo(std::string& bytes)
            {
#ifdef _WIN32
                const std::wstring executable = currentExecutablePath();
                if (executable.empty()) return false;
                const fs::path path = fs::path(executable).parent_path()
                    / "figs" / "Logo.png";
                std::error_code error;
                const std::uintmax_t size = fs::file_size(path, error);
                if (error || size < 8 || size > 8u * 1024u * 1024u)
                    return false;
                std::ifstream input(path, std::ios::binary);
                if (!input) return false;
                std::ostringstream content;
                content << input.rdbuf();
                bytes = content.str();
                static const char signature[] = {
                    static_cast<char>(0x89), 'P', 'N', 'G', '\r', '\n',
                    static_cast<char>(0x1a), '\n'
                };
                return bytes.size() == size
                    && bytes.compare(0, sizeof(signature), signature,
                                     sizeof(signature)) == 0;
#else
                (void)bytes;
                return false;
#endif
            }

            std::string descriptionJson(const std::string& fileName,
                                        const App::ProblemDescription& description,
                                        const std::string& errorCode = std::string(),
                                        bool generated = false,
                                        const std::string& classifiedType = std::string(),
                                        int attempts = 0)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(description.ok)
                    .key("errorCode").value(errorCode)
                    .key("error").value(description.error)
                    .key("fileName").value(fileName)
                    .key("source").value(generated ? "llm" : "file")
                    .key("classifiedType").value(classifiedType)
                    .key("attempts").value(attempts)
                    .key("type").value(description.type)
                    .key("name").value(description.name)
                    .key("dimension").value(description.dimension)
                    .key("variableCount").value(description.variables.size())
                    .key("objectiveCount").value(description.objectives.size())
                    .key("constraintCount").value(description.constraints.size())
                    .key("variables").beginArray();
                for (const App::VariableInfo& variable : description.variables)
                {
                    json.beginObject()
                        .key("name").value(variable.name)
                        .key("kind").value(variable.kind)
                        .key("valueType").value(variable.valueType)
                        .key("length").value(variable.length)
                        .key("lowerBound");
                    if (variable.hasBounds) json.value(variable.lowerBound); else json.nullValue();
                    json.key("upperBound");
                    if (variable.hasBounds) json.value(variable.upperBound); else json.nullValue();
                    json.endObject();
                }
                json.endArray().key("objectives").beginArray();
                for (const App::ObjectiveInfo& objective : description.objectives)
                {
                    json.beginObject()
                        .key("name").value(objective.name)
                        .key("variables").beginArray();
                    for (const std::string& variable : objective.variables) json.value(variable);
                    json.endArray()
                        .key("minimize").value(objective.minimize)
                        .key("priority").value(objective.priority)
                        .endObject();
                }
                json.endArray().key("constraints").beginArray();
                for (const App::ConstraintInfo& constraint : description.constraints)
                {
                    json.beginObject()
                        .key("type").value(constraint.type)
                        .key("variables").beginArray();
                    for (const std::string& variable : constraint.variables) json.value(variable);
                    json.endArray()
                        .key("penalty").value(constraint.penalty)
                        .key("objectives").beginArray();
                    for (const std::string& objective : constraint.objectives) json.value(objective);
                    json.endArray().endObject();
                }
                json.endArray().endObject();
                return out.str();
            }

            void writeRunSolutions(
                App::JsonWriter& json,
                const std::vector<App::RunSolution>& solutions)
            {
                json.beginArray();
                for (const App::RunSolution& solution : solutions)
                {
                    json.beginObject()
                        .key("fitness").value(solution.fitness)
                        .key("objectives").beginArray();
                    for (double objective : solution.objectives)
                        json.value(objective);
                    json.endArray()
                        .key("text").value(solution.text)
                        .endObject();
                }
                json.endArray();
            }

            void writeRunResultFields(App::JsonWriter& json,
                                      const App::RunResult& result,
                                      unsigned seed, int runs)
            {
                json.key("error").value(result.error)
                    .key("cancelled").value(result.cancelled)
                    .key("tag").value(result.tag)
                    .key("optimizerName").value(result.optimizerName)
                    .key("runStart").value(result.runStart)
                    .key("seed").value(static_cast<unsigned long long>(seed))
                    .key("runs").value(runs)
                    .key("bestFitness");
                if (result.ok && !result.best.empty())
                    json.value(result.bestFitness);
                else
                    json.nullValue();
                json.key("best");
                writeRunSolutions(json, result.best);
                json.key("rounds").beginArray();
                for (const App::RunRoundResult& round : result.rounds)
                {
                    json.beginObject()
                        .key("run").value(round.run)
                        .key("resultIndex").value(round.resultIndex)
                        .key("bestFitness");
                    if (!round.best.empty())
                        json.value(round.bestFitness);
                    else
                        json.nullValue();
                    json.key("best");
                    writeRunSolutions(json, round.best);
                    json.endObject();
                }
                json.endArray();
            }

            std::string runResultJson(const App::RunResult& result,
                                      unsigned seed, int runs)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(result.ok);
                writeRunResultFields(json, result, seed, runs);
                json.endObject();
                return out.str();
            }

            struct RunJobSnapshot
            {
                bool found = false;
                bool running = false;
                bool finished = false;
                std::string id;
                int completedRuns = 0;
                int totalRuns = 0;
                unsigned seed = 1;
                App::RunResult result;
            };

            std::string runJobStartJson(const std::string& id, int totalRuns)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(true)
                    .key("jobId").value(id)
                    .key("totalRuns").value(totalRuns)
                    .endObject();
                return out.str();
            }

            std::string runJobStatusJson(const RunJobSnapshot& job)
            {
                const std::string state = job.running ? "running"
                    : (job.result.ok ? "completed"
                                     : (job.result.cancelled ? "cancelled" : "failed"));
                const int currentRun = (job.running || job.result.cancelled)
                    ? (std::min)(job.completedRuns + 1, job.totalRuns)
                    : job.totalRuns;
                const double progress = job.totalRuns > 0
                    ? static_cast<double>(job.completedRuns) / job.totalRuns : 0.0;

                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(true)
                    .key("jobId").value(job.id)
                    .key("state").value(state)
                    .key("completedRuns").value(job.completedRuns)
                    .key("currentRun").value(currentRun)
                    .key("totalRuns").value(job.totalRuns)
                    .key("progress").value(progress);
                writeRunResultFields(json, job.result, job.seed, job.totalRuns);
                json.endObject();
                return out.str();
            }

            void writeParamMeta(App::JsonWriter& json, const App::ParamMeta& parameter)
            {
                json.beginObject()
                    .key("name").value(parameter.name)
                    .key("kind").value(parameter.kind)
                    .key("low").value(parameter.low)
                    .key("high").value(parameter.high)
                    .key("recLow").value(parameter.recLow)
                    .key("recHigh").value(parameter.recHigh)
                    .key("allowEmpty").value(parameter.allowEmpty)
                    .endObject();
            }

            void writeComponentMeta(App::JsonWriter& json, const App::ComponentMeta& component)
            {
                json.beginObject()
                    .key("tag").value(component.tag)
                    .key("category").value(component.categoryKey)
                    .key("params").beginArray();
                for (const App::ParamMeta& parameter : component.params)
                    writeParamMeta(json, parameter);
                json.endArray().endObject();
            }

            std::string configCatalogJson()
            {
                const std::vector<std::string> categories = App::categoryKeys();
                const std::vector<App::ComponentMeta> components = App::listComponents();
                const std::vector<App::ComponentMeta> initializers = App::listInitializers();
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject().key("ok").value(true)
                    .key("categories").beginArray();
                for (const std::string& category : categories) json.value(category);
                json.endArray().key("components").beginArray();
                for (const App::ComponentMeta& component : components)
                    writeComponentMeta(json, component);
                json.endArray().key("initializers").beginArray();
                for (const App::ComponentMeta& initializer : initializers)
                    writeComponentMeta(json, initializer);
                json.endArray().endObject();
                return out.str();
            }

            std::string configDraftJson(const App::ConfigDraftResult& draft)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(draft.ok)
                    .key("error").value(draft.error)
                    .key("sourceVersion").value(draft.sourceVersion)
                    .key("canonicalJson").value(draft.canonicalJson)
                    .endObject();
                return out.str();
            }

            std::string configSchemaJson(const App::ResolvedParameterSchema& schema)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(schema.ok)
                    .key("error").value(schema.error)
                    .key("params").beginArray();
                for (const App::ParamMeta& parameter : schema.params)
                    writeParamMeta(json, parameter);
                json.endArray().key("defaults").beginArray();
                for (double value : schema.defaults) json.value(value);
                json.endArray().endObject();
                return out.str();
            }

            void writeRequirementItem(App::JsonWriter& json,
                                      const App::RequirementItem& item)
            {
                json.beginObject()
                    .key("item").value(item.item)
                    .key("number").value(item.number)
                    .key("owner").value(item.owner)
                    .key("detail").value(item.detail)
                    .endObject();
            }

            std::string configContractJson(
                const std::vector<App::RequirementItem>& requirements,
                const std::vector<App::RequirementItem>& provisions)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject().key("ok").value(true)
                    .key("requirements").beginArray();
                for (const App::RequirementItem& item : requirements)
                    writeRequirementItem(json, item);
                json.endArray().key("provisions").beginArray();
                for (const App::RequirementItem& item : provisions)
                    writeRequirementItem(json, item);
                json.endArray().endObject();
                return out.str();
            }

            std::string configFlowCheckJson(const App::ConfigFlowCheckResult& check)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(check.ok)
                    .key("allowed").value(check.allowed)
                    .key("error").value(check.error)
                    .key("state").value(check.state)
                    .key("reason").value(check.reason)
                    .key("message").value(check.message)
                    .key("errors").value(check.errors)
                    .key("warnings").value(check.warnings)
                    .key("infos").value(check.infos)
                    .endObject();
                return out.str();
            }

            std::string configValidationJson(const App::ValidateReport& report)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(true)
                    .key("scanned").value(report.scanned)
                    .key("passed").value(report.passed)
                    .key("failed").value(report.failed)
                    .key("status").value(report.status)
                    .key("passFiles").value(report.passFiles)
                    .key("infoFiles").value(report.infoFiles)
                    .key("warningFiles").value(report.warningFiles)
                    .key("errorFiles").value(report.errorFiles)
                    .key("deepBuild").value(report.deepBuild)
                    .key("fatal").value(report.fatal)
                    .key("issues").beginArray();
                for (const App::ConfigIssue& issue : report.issues)
                {
                    json.beginObject()
                        .key("file").value(issue.file)
                        .key("scope").value(issue.scope)
                        .key("scopeType").value(issue.scopeType)
                        .key("scopeIndex").value(issue.scopeIndex)
                        .key("component").value(issue.component)
                        .key("category").value(issue.category)
                        .key("componentIndex").value(issue.componentIndex)
                        .key("reason").value(issue.reason)
                        .key("message").value(issue.message)
                        .key("severity").value(issue.severity)
                        .endObject();
                }
                json.endArray().endObject();
                return out.str();
            }

            bool parseNonnegativeIndex(const std::string& text, const char* label,
                                       int& value, std::string& error)
            {
                if (text.empty())
                {
                    error = std::string(label) + " is required";
                    return false;
                }
                char* tail = nullptr;
                errno = 0;
                const long parsed = std::strtol(text.c_str(), &tail, 10);
                if (errno != 0 || !tail || *tail != '\0' || parsed < 0
                    || parsed > (std::numeric_limits<int>::max)())
                {
                    error = std::string(label) + " must be a non-negative integer";
                    return false;
                }
                value = static_cast<int>(parsed);
                return true;
            }

            bool parseRunSeed(const std::string& text, unsigned& value,
                              std::string& error)
            {
                if (text.empty()) return true;
                char* tail = nullptr;
                errno = 0;
                const unsigned long long parsed = std::strtoull(text.c_str(), &tail, 10);
                if (errno != 0 || !tail || *tail != '\0'
                    || parsed > (std::numeric_limits<unsigned>::max)())
                {
                    error = "seed must be an integer from 0 to 4294967295";
                    return false;
                }
                value = static_cast<unsigned>(parsed);
                return true;
            }

            bool parseRunTag(const std::string& text, std::string& error)
            {
                if (text.size() > 128)
                {
                    error = "run tag must not exceed 128 characters";
                    return false;
                }
                for (unsigned char ch : text)
                {
                    if (!std::isalnum(ch) && ch != '_' && ch != '.'
                        && ch != '-')
                    {
                        error = "run tag contains unsupported characters";
                        return false;
                    }
                }
                return true;
            }

            bool parseRunCount(const std::string& text, int& value,
                               std::string& error)
            {
                if (text.empty()) return true;
                char* tail = nullptr;
                errno = 0;
                const long parsed = std::strtol(text.c_str(), &tail, 10);
                if (errno != 0 || !tail || *tail != '\0'
                    || parsed < 1 || parsed > 10000)
                {
                    error = "runs must be an integer from 1 to 10000";
                    return false;
                }
                value = static_cast<int>(parsed);
                return true;
            }

            bool parseRunArtifactOptions(const std::string& startText,
                                         const std::string& replaceText,
                                         int& runStart, bool& replaceExisting,
                                         std::string& error)
            {
                runStart = -1;
                replaceExisting = false;
                if (!startText.empty()
                    && !parseNonnegativeIndex(startText, "run start", runStart, error))
                    return false;
                if (!replaceText.empty())
                {
                    if (replaceText == "1" || replaceText == "true") replaceExisting = true;
                    else if (replaceText != "0" && replaceText != "false")
                    {
                        error = "replaceExisting must be true or false";
                        return false;
                    }
                }
                if (replaceExisting && runStart < 0)
                {
                    error = "replaceExisting requires an explicit run start";
                    return false;
                }
                return true;
            }

            std::string configEncodeJson(const App::ConfigEncodeResult& encoded)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(encoded.ok)
                    .key("error").value(encoded.error)
                    .key("version").value(encoded.version)
                    .key("text").value(encoded.text)
                    .endObject();
                return out.str();
            }

            void writeExperimentBatch(App::JsonWriter& json, const App::ExperimentBatch& batch)
            {
                json.beginObject()
                    .key("tag").value(batch.tag)
                    .key("optimizers").beginArray();
                for (const std::string& optimizer : batch.optimizers) json.value(optimizer);
                json.endArray().key("problems").beginArray();
                for (const std::string& problem : batch.problems) json.value(problem);
                json.endArray()
                    .key("repeats").value(batch.repeats)
                    .key("runNumbers").beginArray();
                for (int run : batch.runNumbers) json.value(run);
                json.endArray().key("runDiagnostics").beginArray();
                for (const std::string& diagnostic : batch.runDiagnostics)
                    json.value(diagnostic);
                json.endArray()
                    .key("rsltCount").value(batch.rsltCount)
                    .key("ragged").value(batch.ragged)
                    .endObject();
            }

            std::string reportDiscoveryJson(const App::ReportResult& report)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(report.ok)
                    .key("error").value(report.error)
                    .key("batches").beginArray();
                for (const App::ExperimentBatch& batch : report.batches)
                    writeExperimentBatch(json, batch);
                json.endArray().endObject();
                return out.str();
            }

            std::string reportResultJson(const App::ReportResult& report,
                                         const std::vector<std::string>& chartSvg)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject()
                    .key("ok").value(report.ok)
                    .key("error").value(report.error)
                    .key("warnings").beginArray();
                for (const std::string& warning : report.warnings) json.value(warning);
                json.endArray().key("chosen");
                writeExperimentBatch(json, report.chosen);
                json.key("header").beginArray();
                for (const std::string& value : report.header) json.value(value);
                json.endArray().key("subheader").beginArray();
                for (const std::string& value : report.subheader) json.value(value);
                json.endArray().key("rows").beginArray();
                for (size_t i = 0; i < report.rows.size(); i++)
                {
                    json.beginObject().key("name").value(report.rows[i]).key("values").beginArray();
                    if (i < report.cells.size())
                        for (const std::string& value : report.cells[i]) json.value(value);
                    json.endArray().endObject();
                }
                json.endArray().key("charts").beginArray();
                for (size_t i = 0;
                     i < chartSvg.size() && i < report.chartArtifacts.size(); i++)
                    json.beginObject()
                        .key("id").value(report.chartArtifacts[i].id)
                        .key("kind").value(report.chartArtifacts[i].id)
                        .key("type").value(report.chartArtifacts[i].kind)
                        .key("title").value(report.chartArtifacts[i].title)
                        .key("svg").value(chartSvg[i])
                        .endObject();
                json.endArray().endObject();
                return out.str();
            }

            std::vector<std::string> splitReportField(const std::string& value, char delimiter)
            {
                std::vector<std::string> parts;
                size_t begin = 0;
                while (begin <= value.size())
                {
                    const size_t end = value.find(delimiter, begin);
                    if (end == std::string::npos)
                    {
                        parts.push_back(value.substr(begin));
                        break;
                    }
                    parts.push_back(value.substr(begin, end - begin));
                    begin = end + 1;
                }
                return parts;
            }

            bool parseReportInteger(const std::string& text, int minimum, int maximum,
                                    const char* label, int& value, std::string& error)
            {
                char* tail = nullptr;
                errno = 0;
                const long parsed = std::strtol(text.c_str(), &tail, 10);
                if (text.empty() || errno != 0 || !tail || *tail != '\0'
                    || parsed < minimum || parsed > maximum)
                {
                    error = std::string(label) + " is outside its supported integer range";
                    return false;
                }
                value = static_cast<int>(parsed);
                return true;
            }

            bool parseReportNumber(const std::string& text, double minimum, double maximum,
                                   const char* label, double& value, std::string& error)
            {
                char* tail = nullptr;
                errno = 0;
                const double parsed = std::strtod(text.c_str(), &tail);
                if (text.empty() || errno != 0 || !tail || *tail != '\0'
                    || !std::isfinite(parsed) || parsed < minimum || parsed > maximum)
                {
                    error = std::string(label) + " is outside its supported numeric range";
                    return false;
                }
                value = parsed;
                return true;
            }

            bool parseReferenceValues(const std::string& text, std::vector<double>& values,
                                      std::string& error)
            {
                std::string normalized = text;
                for (char& ch : normalized)
                    if (ch == ',' || ch == ';' || ch == '\t'
                        || ch == '\r' || ch == '\n') ch = ' ';
                std::istringstream input(normalized);
                std::string token;
                while (input >> token)
                {
                    if (values.size() >= 100000)
                    {
                        error = "indicator reference contains too many values";
                        return false;
                    }
                    char* tail = nullptr;
                    errno = 0;
                    const double value = std::strtod(token.c_str(), &tail);
                    if (errno != 0 || !tail || *tail != '\0' || !std::isfinite(value))
                    {
                        error = "indicator reference contains a non-numeric value";
                        return false;
                    }
                    values.push_back(value);
                }
                return true;
            }

            bool parseReportOptions(const std::string& form, App::ReportOptions& options,
                                    std::string& error)
            {
                options.logPath = queryValue(form, "path");
                if (options.logPath.empty()) options.logPath = "_log";
                if (options.logPath.size() > 32767)
                {
                    error = "log path is too long";
                    return false;
                }
                options.tag = queryValue(form, "tag");
                options.optimizers = queryValues(form, "optimizer");
                options.problems = queryValues(form, "problem");
                options.pngLatinFont = queryValue(form, "latinFont");
                options.pngCjkFont = queryValue(form, "cjkFont");
                options.baselineOptimizer = queryValue(form, "baseline");

                const std::string indexText = queryValue(form, "cjkIndex");
                if (!indexText.empty())
                {
                    char* tail = nullptr;
                    errno = 0;
                    const long value = std::strtol(indexText.c_str(), &tail, 10);
                    if (errno != 0 || !tail || *tail != '\0' || value < 0
                        || value > (std::numeric_limits<int>::max)())
                    {
                        error = "CJK font index must be a non-negative integer";
                        return false;
                    }
                    options.pngCjkIndex = static_cast<int>(value);
                }

                const std::string scaleText = queryValue(form, "scale");
                if (!scaleText.empty())
                {
                    char* tail = nullptr;
                    errno = 0;
                    const double value = std::strtod(scaleText.c_str(), &tail);
                    if (errno != 0 || !tail || *tail != '\0' || !std::isfinite(value)
                        || value < 0.25 || value > 8.0)
                    {
                        error = "PNG scale must be between 0.25 and 8";
                        return false;
                    }
                    options.pngScale = value;
                }

                options.customAnalysis = queryValue(form, "analysis") == "custom";
                if (!options.customAnalysis) return true;

                options.reducer.kind = queryValue(form, "reducerKind");
                options.reducer.metric = queryValue(form, "reducerMetric");
                if (options.reducer.kind.empty()) options.reducer.kind = "statistic";
                if (options.reducer.metric.empty()) options.reducer.metric = "smallest";

                const std::string objectiveText = queryValue(form, "reducerObjective");
                if (!objectiveText.empty()
                    && !parseReportInteger(objectiveText, 0, 1000000,
                                           "reducer objective", options.reducer.objective, error))
                    return false;
                const std::string reducerPercentile = queryValue(form, "reducerPercentile");
                if (!reducerPercentile.empty()
                    && !parseReportNumber(reducerPercentile, 0.0, 1.0,
                                          "reducer percentile",
                                          options.reducer.percentile, error))
                    return false;

                const std::vector<std::string> referenceFields =
                    queryValues(form, "reference");
                for (const std::string& reference : referenceFields)
                {
                    std::vector<double> values;
                    if (!parseReferenceValues(reference, values, error)) return false;
                    options.reducer.references.push_back(values);
                }

                const std::vector<std::string> aggregateFields =
                    queryValues(form, "aggregate");
                if (aggregateFields.size() > 32)
                {
                    error = "too many aggregation statistics";
                    return false;
                }
                for (const std::string& field : aggregateFields)
                {
                    App::ReportStatisticSpec statistic;
                    const std::vector<std::string> parts = splitReportField(field, ':');
                    statistic.metric = parts.empty() ? std::string() : parts[0];
                    if (statistic.metric == "percentile")
                    {
                        if (parts.size() != 2
                            || !parseReportNumber(parts[1], 0.0, 1.0,
                                                  "aggregation percentile",
                                                  statistic.percentile, error))
                            return false;
                    }
                    else if (parts.size() != 1)
                    {
                        error = "invalid aggregation statistic";
                        return false;
                    }
                    options.statistics.push_back(statistic);
                }

                const std::vector<std::string> comparisonFields =
                    queryValues(form, "comparison");
                if (comparisonFields.size() > 16)
                {
                    error = "too many comparison metrics";
                    return false;
                }
                for (const std::string& field : comparisonFields)
                {
                    const std::vector<std::string> parts = splitReportField(field, '|');
                    if (parts.size() != 3 || (parts[2] != "min" && parts[2] != "max"))
                    {
                        error = "invalid comparison metric";
                        return false;
                    }
                    App::ReportComparisonSpec comparison;
                    comparison.metric = parts[0];
                    comparison.on = parts[1];
                    comparison.minimize = parts[2] == "min";
                    options.comparisons.push_back(comparison);
                }

                const std::vector<std::string> chartFields = queryValues(form, "chart");
                if (chartFields.size() > 32)
                {
                    error = "too many charts";
                    return false;
                }
                for (const std::string& field : chartFields)
                {
                    const std::vector<std::string> parts = splitReportField(field, '|');
                    if (parts.size() != 13)
                    {
                        error = "invalid chart specification";
                        return false;
                    }
                    App::ReportChartSpec chart;
                    chart.id = parts[0];
                    chart.kind = parts[1];
                    chart.statistic = parts[3];
                    chart.normalization = parts[4];
                    chart.bandLow = parts[5];
                    chart.bandHigh = parts[6];
                    int jitterValue = 0;
                    if (!parseReportInteger(parts[2], 0, 1000000,
                                            "chart problem", chart.problem, error)
                        || !parseReportInteger(parts[7], 0, 1,
                                               "chart jitter", jitterValue, error))
                        return false;
                    chart.jitter = jitterValue == 1;
                    if (!parseReportInteger(parts[8], 2, 10000,
                                            "chart grid", chart.grid, error)
                        || !parseReportInteger(parts[9], 0, 1000000,
                                               "chart objective X", chart.objectiveX, error)
                        || !parseReportInteger(parts[10], 0, 1000000,
                                               "chart objective Y", chart.objectiveY, error)
                        || !parseReportInteger(parts[11], -1, 1000000,
                                               "chart run", chart.run, error))
                        return false;
                    if (parts[12] != "0" && parts[12] != "1")
                    {
                        error = "chart connect must be 0 or 1";
                        return false;
                    }
                    chart.connect = parts[12] == "1";
                    options.charts.push_back(chart);
                }
                return true;
            }

            std::string readBinaryFile(const fs::path& path)
            {
                std::ifstream file(path, std::ios::binary);
                std::ostringstream out;
                out << file.rdbuf();
                return out.str();
            }

            std::string safeDownloadName(const std::string& tag, const std::string& kind,
                                         const std::string& extension)
            {
                std::string clean;
                clean.reserve((std::min<size_t>)(tag.size(), 80));
                for (size_t i = 0; i < tag.size() && clean.size() < 80; i++)
                {
                    const unsigned char ch = static_cast<unsigned char>(tag[i]);
                    clean.push_back(std::isalnum(ch) || ch == '-' || ch == '_' ? static_cast<char>(ch) : '_');
                }
                if (clean.empty()) clean = "report";
                return "evoflow-" + clean + "-" + kind + "." + extension;
            }

            std::string safeProblemDownloadName(const std::string& name)
            {
                std::string clean;
                clean.reserve((std::min<size_t>)(name.size(), 160));
                for (size_t i = 0; i < name.size() && clean.size() < 160; i++)
                {
                    const unsigned char ch = static_cast<unsigned char>(name[i]);
                    clean.push_back(std::isalnum(ch) || ch == '-' || ch == '_'
                        || ch == '.' ? static_cast<char>(ch) : '_');
                }
                if (clean.empty() || clean == "." || clean == "..")
                    clean = "problem.data";
                return clean;
            }

            bool parseParameterValues(const std::string& text, std::vector<double>& values,
                                      std::string& error)
            {
                if (text.empty()) return true;
                std::size_t begin = 0;
                while (begin <= text.size())
                {
                    std::size_t end = text.find(',', begin);
                    if (end == std::string::npos) end = text.size();
                    std::string token = text.substr(begin, end - begin);
                    const std::size_t first = token.find_first_not_of(" \t\r\n");
                    const std::size_t last = token.find_last_not_of(" \t\r\n");
                    token = first == std::string::npos
                        ? std::string() : token.substr(first, last - first + 1);
                    if (token == "null")
                    {
                        values.push_back(std::numeric_limits<double>::quiet_NaN());
                    }
                    else
                    {
                        char* tail = nullptr;
                        errno = 0;
                        const double value = std::strtod(token.c_str(), &tail);
                        if (token.empty() || errno != 0 || !tail || *tail != '\0'
                            || !std::isfinite(value))
                        {
                            error = "parameter values must be comma-separated finite numbers or null";
                            return false;
                        }
                        values.push_back(value);
                    }
                    if (end == text.size()) break;
                    begin = end + 1;
                }
                return true;
            }

            class TempProblemFile
            {
            public:
                fs::path path;
                fs::path release()
                {
                    fs::path result = std::move(path);
                    path.clear();
                    return result;
                }
                ~TempProblemFile()
                {
                    std::error_code ignored;
                    if (!path.empty()) fs::remove(path, ignored);
                }
            };

            bool writeTempProblem(const std::string& data, TempProblemFile& temp,
                                  std::string& error)
            {
                const std::string suffix = randomToken();
                if (suffix.empty())
                {
                    error = "cannot generate a temporary file name";
                    return false;
                }
                temp.path = fs::temp_directory_path()
                    / ("evoflow-ui-" + suffix.substr(0, 24) + ".problem");
                std::ofstream file(temp.path, std::ios::binary);
                if (!file)
                {
                    error = "cannot create temporary problem file";
                    return false;
                }
                file.write(data.data(), static_cast<std::streamsize>(data.size()));
                file.close();
                if (!file)
                {
                    error = "cannot write temporary problem file";
                    return false;
                }
                return true;
            }

            class TempConfigFile
            {
            public:
                fs::path path;
                ~TempConfigFile()
                {
                    std::error_code ignored;
                    if (!path.empty()) fs::remove(path, ignored);
                }
            };

            bool writeTempConfig(const std::string& data, TempConfigFile& temp,
                                 std::string& error)
            {
                const std::string suffix = randomToken();
                if (suffix.empty())
                {
                    error = "cannot generate a temporary config file name";
                    return false;
                }
                temp.path = fs::temp_directory_path()
                    / ("evoflow-ui-config-" + suffix.substr(0, 24) + ".cfg");
                std::ofstream file(temp.path, std::ios::binary);
                if (!file)
                {
                    error = "cannot create temporary config file";
                    return false;
                }
                file.write(data.data(), static_cast<std::streamsize>(data.size()));
                file.close();
                if (!file)
                {
                    error = "cannot write temporary config file";
                    return false;
                }
                return true;
            }

            class TempReportDirectory
            {
            public:
                fs::path path;
                ~TempReportDirectory()
                {
                    std::error_code ignored;
                    if (!path.empty()) fs::remove_all(path, ignored);
                }
            };
        }

        struct UiApplication::Impl
        {
            LocalHttpServer server;
            UiOptions options;
            std::string token;
            std::atomic<long long> lastActivity{ 0 };
            std::atomic<long long> closingRequestedAt{ 0 };
            std::atomic<int> activeRequests{ 0 };
            std::atomic<bool> stopping{ false };
            fs::path currentProblemPath;
            App::ProblemDescription currentProblem;
            std::string currentProblemFileName;
            bool currentProblemGenerated = false;
            std::mutex runMutex;
            std::thread runThread;
            std::string runJobId;
            bool runJobRunning = false;
            bool runJobFinished = false;
            int runCompletedRuns = 0;
            int runTotalRuns = 0;
            unsigned runSeed = 1;
            App::RunResult runJobResult;
            std::atomic<bool> runCancelRequested{ false };

            ~Impl()
            {
                requestRunCancellation();
                joinRunThread();
                clearCurrentProblem();
            }

            void requestRunCancellation()
            {
                runCancelRequested = true;
            }

            void joinRunThread()
            {
                if (runThread.joinable()) runThread.join();
            }

            bool runInProgress()
            {
                std::lock_guard<std::mutex> lock(runMutex);
                return runJobRunning;
            }

            bool startRunJob(std::string problemData, std::string configData,
                              const std::string& problemName,
                              const std::string& runTag, unsigned seed, int runs,
                              int runStart, bool replaceExisting,
                              std::string& jobId, std::string& error)
            {
                if (runThread.joinable())
                {
                    bool completed = false;
                    {
                        std::lock_guard<std::mutex> lock(runMutex);
                        completed = runJobFinished;
                    }
                    if (!completed)
                    {
                        error = "another optimization run is already active";
                        return false;
                    }
                    runThread.join();
                }

                jobId = randomToken();
                if (jobId.empty())
                {
                    error = "cannot generate a run job identifier";
                    return false;
                }
                {
                    std::lock_guard<std::mutex> lock(runMutex);
                    runJobId = jobId;
                    runJobRunning = true;
                    runJobFinished = false;
                    runCompletedRuns = 0;
                    runTotalRuns = runs;
                    runSeed = seed;
                    runJobResult = App::RunResult{};
                    runCancelRequested = false;
                }

                try
                {
                    runThread = std::thread(
                        [this, jobId, problemData = std::move(problemData),
                         configData = std::move(configData), problemName,
                         runTag, seed, runs, runStart, replaceExisting]() {
                        App::RunResult result;
                        try
                        {
                            TempProblemFile problemTemp;
                            TempConfigFile configTemp;
                            std::string tempError;
                            if (!writeTempProblem(problemData, problemTemp, tempError)
                                || !writeTempConfig(configData, configTemp, tempError))
                            {
                                result.error = tempError;
                            }
                            else
                            {
                                App::RunOptions options;
                                options.problemPath = problemTemp.path.u8string();
                                options.problemName = problemName;
                                options.configPath = configTemp.path.u8string();
                                options.tag = runTag;
                                 options.seed = seed;
                                 options.runs = runs;
                                 options.runStart = runStart;
                                 options.replaceExisting = replaceExisting;
                                result = App::runOptimization(
                                    options,
                                    [this, jobId](int completed, int total) {
                                        std::lock_guard<std::mutex> lock(runMutex);
                                        if (runJobId == jobId && runJobRunning)
                                        {
                                            runCompletedRuns = completed;
                                            runTotalRuns = total;
                                        }
                                    },
                                    [this]() {
                                        return runCancelRequested.load();
                                    });
                            }
                        }
                        catch (const std::exception& ex)
                        {
                            result.error = ex.what();
                        }
                        catch (...)
                        {
                            result.error = "unexpected error while running optimization";
                        }

                        std::lock_guard<std::mutex> lock(runMutex);
                        if (runJobId == jobId)
                        {
                            runJobResult = std::move(result);
                            if (runJobResult.ok) runCompletedRuns = runTotalRuns;
                            runJobRunning = false;
                            runJobFinished = true;
                        }
                    });
                }
                catch (const std::exception& ex)
                {
                    std::lock_guard<std::mutex> lock(runMutex);
                    runJobRunning = false;
                    runJobFinished = true;
                    runJobResult.error = ex.what();
                    error = ex.what();
                    return false;
                }
                return true;
            }

            RunJobSnapshot runSnapshot(const std::string& id)
            {
                std::lock_guard<std::mutex> lock(runMutex);
                RunJobSnapshot snapshot;
                if (id.empty() || id != runJobId) return snapshot;
                snapshot.found = true;
                snapshot.running = runJobRunning;
                snapshot.finished = runJobFinished;
                snapshot.id = runJobId;
                snapshot.completedRuns = runCompletedRuns;
                snapshot.totalRuns = runTotalRuns;
                snapshot.seed = runSeed;
                snapshot.result = runJobResult;
                return snapshot;
            }

            void clearCurrentProblem()
            {
                std::error_code ignored;
                if (!currentProblemPath.empty()) fs::remove(currentProblemPath, ignored);
                currentProblemPath.clear();
                currentProblem = App::ProblemDescription{};
                currentProblemFileName.clear();
                currentProblemGenerated = false;
            }

            void acceptCurrentProblem(TempProblemFile& temp,
                                      const App::ProblemDescription& description,
                                      const std::string& fileName, bool generated)
            {
                clearCurrentProblem();
                currentProblemPath = temp.release();
                currentProblem = description;
                currentProblemFileName = fileName;
                currentProblemGenerated = generated;
            }

            std::string origin() const
            {
                return "http://127.0.0.1:" + std::to_string(server.port());
            }

            bool authorized(const HttpRequest& request) const
            {
                if (!constantTimeEqual(request.header("x-evoflow-token"), token)) return false;
                const std::string host = request.header("host");
                const std::string expectedHost = "127.0.0.1:" + std::to_string(server.port());
                if (host != expectedHost) return false;
                const std::string requestOrigin = request.header("origin");
                return requestOrigin.empty() || requestOrigin == origin();
            }

            HttpResponse handle(const HttpRequest& request)
            {
                if (request.path == "/" || request.path == "/index.html")
                {
                    if (request.method != "GET") return jsonError(405, "method not allowed");
                    HttpResponse response;
                    response.contentType = "text/html; charset=utf-8";
#ifdef _WIN32
                    response.body = Assets::indexHtml;
#endif
                    addSecurityHeaders(response);
                    return response;
                }
                if (request.path == "/brand-logo.png")
                {
                    if (request.method != "GET") return jsonError(405, "method not allowed");
                    HttpResponse response;
                    response.contentType = "image/png";
                    if (!loadBrandLogo(response.body))
                        return jsonError(404, "brand logo is unavailable");
                    addSecurityHeaders(response);
                    return response;
                }
                if (request.path == "/app.css" || request.path == "/i18n.js"
                    || request.path == "/app.js"
                    || request.path == "/llm-settings.js"
                    || request.path == "/problem-builder.js"
                    || request.path == "/config-editor.js"
                    || request.path == "/config-recommender.js"
                    || request.path == "/flow-editor.js"
                    || request.path == "/report-viewer.js")
                {
                    if (request.method != "GET") return jsonError(405, "method not allowed");
                    HttpResponse response;
#ifdef _WIN32
                    if (request.path == "/app.css")
                    {
                        response.contentType = "text/css; charset=utf-8";
                        response.body = Assets::appCss;
                    }
                    else if (request.path == "/i18n.js")
                    {
                        response.contentType = "text/javascript; charset=utf-8";
                        response.body = Assets::i18nJs;
                    }
                    else if (request.path == "/app.js")
                    {
                        response.contentType = "text/javascript; charset=utf-8";
                        response.body = Assets::appJs;
                    }
                    else if (request.path == "/llm-settings.js")
                    {
                        response.contentType = "text/javascript; charset=utf-8";
                        response.body = Assets::llmSettingsJs;
                    }
                    else if (request.path == "/problem-builder.js")
                    {
                        response.contentType = "text/javascript; charset=utf-8";
                        response.body = Assets::problemBuilderJs;
                    }
                    else if (request.path == "/config-editor.js")
                    {
                        response.contentType = "text/javascript; charset=utf-8";
                        response.body = Assets::configEditorJs;
                    }
                    else if (request.path == "/config-recommender.js")
                    {
                        response.contentType = "text/javascript; charset=utf-8";
                        response.body = Assets::configRecommenderJs;
                    }
                    else if (request.path == "/flow-editor.js")
                    {
                        response.contentType = "text/javascript; charset=utf-8";
                        response.body = Assets::flowEditorJs;
                    }
                    else
                    {
                        response.contentType = "text/javascript; charset=utf-8";
                        response.body = Assets::reportViewerJs;
                    }
#endif
                    addSecurityHeaders(response);
                    return response;
                }

                if (request.path.rfind("/api/", 0) != 0) return jsonError(404, "route not found");
                if (!authorized(request)) return jsonError(403, "invalid UI session");

                lastActivity = nowMilliseconds();
                if (request.path != "/api/session/closing")
                    closingRequestedAt = 0;
                struct ActiveGuard
                {
                    std::atomic<int>& count;
                    explicit ActiveGuard(std::atomic<int>& value) : count(value) { count++; }
                    ~ActiveGuard() { count--; }
                } active(activeRequests);

                if (request.path == "/api/health")
                {
                    if (request.method != "GET") return jsonError(405, "method not allowed");
                    HttpResponse response;
                    response.contentType = "application/json; charset=utf-8";
                    response.body = "{\"ok\":true,\"version\":\""
                                  EVOFLOW_VERSION_STRING "\"}";
                    addSecurityHeaders(response);
                    return response;
                }
                if (request.path == "/api/heartbeat")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    HttpResponse response;
                    response.contentType = "application/json; charset=utf-8";
                    response.body = "{\"ok\":true}";
                    addSecurityHeaders(response);
                    return response;
                }
                if (request.path == "/api/session/closing")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    closingRequestedAt = nowMilliseconds();
                    HttpResponse response;
                    response.contentType = "application/json; charset=utf-8";
                    response.body = "{\"ok\":true}";
                    addSecurityHeaders(response);
                    return response;
                }
                if (request.path == "/api/llm/test")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    App::LlmSettings settings;
                    std::string parseError;
                    if (!parseLlmSettings(request.body, settings, parseError))
                    {
                        App::LlmResult result;
                        result.errorCode = "invalid_settings";
                        result.error = parseError;
                        return jsonResponse(llmResultJson(result), 400);
                    }
                    const App::LlmResult result = App::testLlmConnection(settings);
                    return jsonResponse(llmResultJson(result), llmResponseStatus(result));
                }
                if (request.path == "/api/llm/problem/classify")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    App::LlmSettings settings;
                    std::string description;
                    std::string ignoredType;
                    App::LlmProblemAttachment attachment;
                    std::string parseError;
                    if (!parseLlmSettings(request.body, settings, parseError)
                        || !parseLlmProblemFields(
                            request.body, description, ignoredType, false,
                            attachment, parseError))
                    {
                        App::LlmProblemResult result;
                        result.errorCode = "invalid_settings";
                        result.error = parseError;
                        return jsonResponse(llmProblemResultJson(result), 400);
                    }
                    const App::LlmProblemResult result =
                        App::classifyProblemDescription(
                            settings, description, attachment);
                    return jsonResponse(
                        llmProblemResultJson(result),
                        result.ok ? 200 : llmErrorStatus(result.errorCode));
                }
                if (request.path == "/api/llm/problem/generate")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    App::LlmSettings settings;
                    std::string description;
                    std::string type;
                    App::LlmProblemAttachment attachment;
                    std::string parseError;
                    if (!parseLlmSettings(request.body, settings, parseError)
                        || !parseLlmProblemFields(
                            request.body, description, type, true,
                            attachment, parseError))
                    {
                        App::LlmProblemResult result;
                        result.errorCode = "invalid_settings";
                        result.error = parseError;
                        return jsonResponse(llmProblemResultJson(result), 400);
                    }

                    TempProblemFile acceptedTemp;
                    App::ProblemDescription acceptedProblem;
                    std::string acceptedFileName;
                    const App::LlmProblemValidator validator =
                        [&](const std::string& checkedType,
                            const std::string& problemData) {
                            TempProblemFile candidate;
                            std::string tempError;
                            if (!writeTempProblem(
                                    problemData, candidate, tempError))
                            {
                                App::LlmProblemValidationResult checked;
                                checked.errorCode =
                                    "temporary_problem_unavailable";
                                checked.error = tempError;
                                checked.repairable = false;
                                return checked;
                            }
                            const std::string logicalName =
                                generatedProblemName(problemData);
                            if (logicalName.empty())
                            {
                                App::LlmProblemValidationResult checked;
                                checked.errorCode =
                                    "problem_validation_failed";
                                checked.error =
                                    "Generated data must declare a non-empty NAME field.";
                                return checked;
                            }
                            App::ProblemDescription problem =
                                App::compileAndDescribeProblem(
                                    candidate.path.string(), checkedType,
                                    logicalName);
                            if (!problem.ok)
                            {
                                App::LlmProblemValidationResult checked;
                                checked.errorCode =
                                    "problem_validation_failed";
                                checked.error = problem.error;
                                return checked;
                            }
                            acceptedTemp.path = candidate.release();
                            acceptedProblem = std::move(problem);
                            acceptedFileName =
                                safeProblemDownloadName(logicalName) + ".problem";
                            App::LlmProblemValidationResult checked;
                            checked.ok = true;
                            return checked;
                        };
                    App::LlmProblemResult generated =
                        App::generateProblemData(
                            settings, description, type, validator, attachment);
                    if (!generated.ok)
                        return jsonResponse(
                            llmProblemResultJson(generated),
                            llmErrorStatus(generated.errorCode));

                    const std::string fileName = acceptedFileName.empty()
                        ? "generated-" + type + ".problem" : acceptedFileName;
                    acceptCurrentProblem(
                        acceptedTemp, acceptedProblem, fileName, true);
                    return jsonResponse(
                        descriptionJson(
                            fileName, acceptedProblem, std::string(),
                            true, type, generated.attempts),
                        200);
                }
                if (request.path == "/api/llm/config/recommend")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    App::LlmSettings settings;
                    std::string parseError;
                    if (!parseLlmSettings(request.body, settings, parseError))
                    {
                        App::LlmConfigRecommendationResult result;
                        result.errorCode = "invalid_settings";
                        result.error = parseError;
                        return jsonResponse(llmConfigRecommendationJson(result), 400);
                    }
                    const App::LlmConfigRecommendationResult result =
                        App::recommendConfig(settings, currentProblem);
                    return jsonResponse(
                        llmConfigRecommendationJson(result),
                        result.ok ? 200 : llmErrorStatus(result.errorCode));
                }
                if (request.path == "/api/shutdown")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    HttpResponse response;
                    response.contentType = "application/json; charset=utf-8";
                    response.body = "{\"ok\":true}";
                    addSecurityHeaders(response);
                    requestRunCancellation();
                    clearCurrentProblem();
                    stopping = true;
                    response.stopServerAfterResponse = true;
                    return response;
                }
                if (request.path == "/api/report/discover")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    App::ReportOptions options;
                    std::string parseError;
                    if (!parseReportOptions(request.body, options, parseError))
                        return jsonError(400, parseError);
                    const App::ReportResult report = App::discoverBatches(options.logPath);
                    return jsonResponse(reportDiscoveryJson(report), report.ok ? 200 : 422);
                }
                if (request.path == "/api/report/build")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    App::ReportOptions options;
                    std::string parseError;
                    if (!parseReportOptions(request.body, options, parseError))
                        return jsonError(400, parseError);

                    const std::string suffix = randomToken();
                    if (suffix.empty())
                        return jsonError(500, "cannot generate a temporary report directory");
                    TempReportDirectory temp;
                    temp.path = fs::temp_directory_path()
                        / ("evoflow-ui-report-" + suffix.substr(0, 24));
                    options.chartsDir = temp.path.u8string();
                    options.chartsPng = false;
                    const App::ReportResult report = App::buildReport(options);
                    std::vector<std::string> charts;
                    if (report.ok)
                    {
                        for (const std::string& file : report.chartFiles)
                        {
                            const std::string svg = readBinaryFile(fs::u8path(file));
                            if (svg.empty())
                                return jsonError(500, "generated SVG chart is empty");
                            charts.push_back(svg);
                        }
                    }
                    return jsonResponse(reportResultJson(report, charts), report.ok ? 200 : 422);
                }
                if (request.path == "/api/report/download")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    App::ReportOptions options;
                    std::string parseError;
                    if (!parseReportOptions(request.body, options, parseError))
                        return jsonError(400, parseError);
                    std::string chartId = queryValue(request.body, "chartId");
                    if (chartId.empty()) chartId = queryValue(request.body, "kind");
                    const std::string format = queryValue(request.body, "format");
                    if (chartId.empty()) return jsonError(400, "chart id is required");
                    if (format != "svg" && format != "png")
                        return jsonError(400, "chart format must be svg or png");

                    const std::string suffix = randomToken();
                    if (suffix.empty())
                        return jsonError(500, "cannot generate a temporary report directory");
                    TempReportDirectory temp;
                    temp.path = fs::temp_directory_path()
                        / ("evoflow-ui-report-" + suffix.substr(0, 24));
                    options.chartsDir = temp.path.u8string();
                    options.chartsPng = format == "png";
                    const App::ReportResult report = App::buildReport(options);
                    if (!report.ok)
                        return jsonResponse(reportResultJson(report, {}), 422);
                    const App::ReportChartArtifact* artifact = nullptr;
                    for (const App::ReportChartArtifact& candidate : report.chartArtifacts)
                        if (candidate.id == chartId) { artifact = &candidate; break; }
                    if (!artifact) return jsonError(400, "unknown chart id");

                    HttpResponse response;
                    response.contentType = format == "png"
                        ? "image/png" : "image/svg+xml; charset=utf-8";
                    response.body = readBinaryFile(fs::u8path(artifact->file));
                    if (response.body.empty()) return jsonError(500, "generated chart is empty");
                    response.headers.emplace_back(
                        "Content-Disposition",
                        "attachment; filename=\""
                            + safeDownloadName(options.tag, chartId, format) + "\"");
                    addSecurityHeaders(response);
                    return response;
                }
                if (request.path == "/api/config/catalog")
                {
                    if (request.method != "GET") return jsonError(405, "method not allowed");
                    return jsonResponse(configCatalogJson());
                }
                if (request.path == "/api/config/new")
                {
                    if (request.method != "GET") return jsonError(405, "method not allowed");
                    const App::ConfigDraftResult draft = App::createConfigDraft();
                    return jsonResponse(configDraftJson(draft), draft.ok ? 200 : 500);
                }
                if (request.path == "/api/config/open")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    if (request.body.empty()) return jsonError(400, "config file is empty");
                    const App::ConfigDraftResult draft = App::loadConfigDraft(request.body);
                    return jsonResponse(configDraftJson(draft), draft.ok ? 200 : 422);
                }
                if (request.path == "/api/config/schema")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    const std::string category = queryValue(request.query, "category");
                    const std::string tag = queryValue(request.query, "tag");
                    if (category.empty() || tag.empty())
                        return jsonError(400, "category and tag are required");
                    std::vector<double> values;
                    std::string parseError;
                    if (!parseParameterValues(request.body, values, parseError))
                        return jsonError(400, parseError);
                    const App::ResolvedParameterSchema schema =
                        App::resolveParameterSchema(category, tag, values);
                    return jsonResponse(configSchemaJson(schema), schema.ok ? 200 : 404);
                }
                if (request.path == "/api/config/contract")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    const std::string category = queryValue(request.query, "category");
                    const std::string tag = queryValue(request.query, "tag");
                    if (category.empty() || tag.empty())
                        return jsonError(400, "category and tag are required");
                    std::vector<double> values;
                    std::string parseError;
                    if (!parseParameterValues(request.body, values, parseError))
                        return jsonError(400, parseError);
                    const App::ResolvedParameterSchema schema =
                        App::resolveParameterSchema(category, tag, values);
                    if (!schema.ok) return jsonError(404, schema.error);
                    const ModuleType type = App::categoryOfKey(category);
                    return jsonResponse(configContractJson(
                        App::queryRequirementsAt(type, tag, values),
                        App::queryProvisionsAt(type, tag, values)));
                }
                if (request.path == "/api/config/flow/check")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    if (request.body.empty()) return jsonError(400, "config draft is empty");
                    int workflowIndex = -1;
                    int componentIndex = -1;
                    std::string parseError;
                    if (!parseNonnegativeIndex(queryValue(request.query, "workflow"),
                                               "workflow index", workflowIndex, parseError)
                        || !parseNonnegativeIndex(queryValue(request.query, "component"),
                                                  "component index", componentIndex,
                                                  parseError))
                        return jsonError(400, parseError);
                    const App::ConfigFlowCheckResult check =
                        App::checkConfigFlowConnection(
                            request.body, workflowIndex, componentIndex);
                    return jsonResponse(configFlowCheckJson(check), check.ok ? 200 : 422);
                }
                if (request.path == "/api/config/validate")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    if (request.body.empty()) return jsonError(400, "config draft is empty");
                    std::string name = displayFileName(queryValue(request.query, "name"));
                    if (name == "problem.data") name = "draft.cfg";
                    const bool deepBuild = queryValue(request.query, "deep") == "1";
                    const App::ValidateReport report =
                        App::validateConfigText(request.body, deepBuild, name);
                    return jsonResponse(configValidationJson(report));
                }
                if (request.path == "/api/config/encode")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    if (request.body.empty()) return jsonError(400, "config draft is empty");
                    const std::string version = queryValue(request.query, "version");
                    const App::ConfigEncodeResult encoded =
                        App::encodeConfigDraft(request.body, version);
                    return jsonResponse(configEncodeJson(encoded), encoded.ok ? 200 : 422);
                }
                if (request.path == "/api/run/status")
                {
                    if (request.method != "GET") return jsonError(405, "method not allowed");
                    const RunJobSnapshot job =
                        runSnapshot(queryValue(request.query, "id"));
                    if (!job.found) return jsonError(404, "run job not found");
                    return jsonResponse(runJobStatusJson(job));
                }
                if (request.path == "/api/run/start")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    if (currentProblemPath.empty() || !currentProblem.ok)
                        return jsonError(409, "load or build a problem before running");
                    if (request.body.empty()) return jsonError(400, "config draft is empty");

                    unsigned seed = 1;
                    int runs = 1;
                    int runStart = -1;
                    bool replaceExisting = false;
                    const std::string runTag = queryValue(request.query, "tag");
                    std::string parseError;
                    if (!parseRunTag(runTag, parseError)
                        || !parseRunSeed(queryValue(request.query, "seed"), seed, parseError)
                        || !parseRunCount(queryValue(request.query, "runs"), runs, parseError)
                        || !parseRunArtifactOptions(queryValue(request.query, "runStart"),
                            queryValue(request.query, "replaceExisting"), runStart,
                            replaceExisting, parseError))
                        return jsonError(400, parseError);

                    const App::ConfigEncodeResult encoded =
                        App::encodeConfigDraft(request.body, "v3.1");
                    if (!encoded.ok) return jsonError(422, encoded.error);
                    const std::string problemData = readBinaryFile(currentProblemPath);
                    if (problemData.empty())
                        return jsonError(500, "current problem data is unavailable");

                    std::string jobId;
                    std::string startError;
                    if (!startRunJob(problemData, encoded.text, currentProblem.name,
                                     runTag, seed, runs, runStart, replaceExisting,
                                     jobId, startError))
                        return jsonError(409, startError);
                    return jsonResponse(runJobStartJson(jobId, runs));
                }
                if (request.path == "/api/run/current")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    if (runInProgress())
                        return jsonError(409, "another optimization run is already active");
                    if (currentProblemPath.empty() || !currentProblem.ok)
                        return jsonError(409, "load or build a problem before running");
                    if (request.body.empty()) return jsonError(400, "config draft is empty");

                    unsigned seed = 1;
                    int runs = 1;
                    int runStart = -1;
                    bool replaceExisting = false;
                    const std::string runTag = queryValue(request.query, "tag");
                    std::string parseError;
                    if (!parseRunTag(runTag, parseError)
                        || !parseRunSeed(queryValue(request.query, "seed"), seed, parseError)
                        || !parseRunCount(queryValue(request.query, "runs"), runs, parseError)
                        || !parseRunArtifactOptions(queryValue(request.query, "runStart"),
                            queryValue(request.query, "replaceExisting"), runStart,
                            replaceExisting, parseError))
                        return jsonError(400, parseError);

                    const App::ConfigEncodeResult encoded =
                        App::encodeConfigDraft(request.body, "v3.1");
                    if (!encoded.ok) return jsonError(422, encoded.error);

                    TempConfigFile temp;
                    std::string tempError;
                    if (!writeTempConfig(encoded.text, temp, tempError))
                        return jsonError(500, tempError);

                    App::RunOptions options;
                    options.problemPath = currentProblemPath.u8string();
                    options.problemName = currentProblem.name;
                    options.configPath = temp.path.u8string();
                    options.tag = runTag;
                    options.seed = seed;
                    options.runs = runs;
                    options.runStart = runStart;
                    options.replaceExisting = replaceExisting;
                    const App::RunResult result = App::runOptimization(options);
                    return jsonResponse(
                        runResultJson(result, seed, runs), result.ok ? 200 : 422);
                }
                if (request.path == "/api/problem/current")
                {
                    if (request.method != "GET") return jsonError(405, "method not allowed");
                    if (currentProblemPath.empty() || !currentProblem.ok)
                        return jsonError(409, "no current problem to save");
                    HttpResponse response;
                    response.contentType = "application/octet-stream";
                    response.body = readBinaryFile(currentProblemPath);
                    if (response.body.empty())
                        return jsonError(500, "current problem data is unavailable");
                    response.headers.emplace_back(
                        "Content-Disposition",
                        "attachment; filename=\"" +
                            safeProblemDownloadName(currentProblemFileName) + "\"");
                    addSecurityHeaders(response);
                    return response;
                }
                if (request.path == "/api/problem/describe")
                {
                    if (request.method != "POST") return jsonError(405, "method not allowed");
                    if (request.body.empty()) return jsonError(400, "problem file is empty");

                    std::string fileName = displayFileName(queryValue(request.query, "name"));
                    TempProblemFile temp;
                    try
                    {
                        std::string tempError;
                        if (!writeTempProblem(request.body, temp, tempError))
                            return jsonError(500, tempError);

                        const std::string logicalName =
                            problemNameFromFileName(fileName);
                        App::ProblemDescription description =
                            App::describeProblem(temp.path.string(), logicalName);
                        if (description.ok)
                            acceptCurrentProblem(temp, description, fileName, false);
                        HttpResponse response;
                        response.status = description.ok ? 200 : 422;
                        response.contentType = "application/json; charset=utf-8";
                        response.body = descriptionJson(fileName, description);
                        addSecurityHeaders(response);
                        return response;
                    }
                    catch (const std::exception& ex)
                    {
                        return jsonError(500, ex.what());
                    }
                }
                return jsonError(404, "route not found");
            }
        };

        UiApplication::UiApplication() : _impl(new Impl) {}
        UiApplication::~UiApplication() { stop(); }

        bool UiApplication::start(const UiOptions& options, std::string& error)
        {
            _impl->options = options;
            _impl->token = options.token.empty() ? randomToken() : options.token;
            if (_impl->token.empty())
            {
                error = "failed to generate the one-time UI token";
                return false;
            }
            _impl->lastActivity = nowMilliseconds();
            _impl->stopping = false;
            if (!_impl->server.start(
                    [this](const HttpRequest& request) { return _impl->handle(request); }, error))
                return false;

#ifdef _WIN32
            if (options.openBrowser)
            {
                HINSTANCE opened = ShellExecuteA(nullptr, "open", url().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                if (reinterpret_cast<std::intptr_t>(opened) <= 32)
                {
                    error = "failed to open the default browser";
                    stop();
                    return false;
                }
            }
#else
            if (options.openBrowser)
            {
                error = "the graphical interface is currently supported on Windows only";
                stop();
                return false;
            }
#endif
            return true;
        }

        void UiApplication::run()
        {
            std::thread watchdog([this]() {
                const long long timeout = static_cast<long long>(
                    _impl->options.heartbeatTimeoutSeconds) * 1000;
                // pagehide also fires during a reload. Keep a short grace period
                // so the replacement page's immediate heartbeat can cancel exit.
                constexpr long long CLOSE_GRACE_MS = 2000;
                while (!_impl->stopping)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    const long long now = nowMilliseconds();
                    const long long closingAt = _impl->closingRequestedAt.load();
                    if (_impl->activeRequests == 0
                        && ((closingAt > 0 && now - closingAt >= CLOSE_GRACE_MS)
                            || (timeout > 0
                                && now - _impl->lastActivity.load() > timeout)))
                    {
                        stop();
                        break;
                    }
                }
            });
            _impl->server.run();
            _impl->stopping = true;
            _impl->requestRunCancellation();
            _impl->joinRunThread();
            watchdog.join();
        }

        void UiApplication::stop()
        {
            if (!_impl) return;
            _impl->stopping = true;
            _impl->requestRunCancellation();
            _impl->server.stop();
            _impl->clearCurrentProblem();
        }

        std::uint16_t UiApplication::port() const { return _impl->server.port(); }
        std::string UiApplication::token() const { return _impl->token; }
        std::string UiApplication::url() const
        {
            return _impl->origin() + "/#token=" + _impl->token;
        }

        UiLaunchResult launchUi()
        {
            UiLaunchResult result;
            UiApplication app;
            UiOptions options;
            std::string error;
            if (!app.start(options, error))
            {
                result.error = error;
                return result;
            }
            result.port = app.port();
            result.token = app.token();
#ifdef _WIN32
            result.childHasConsole = GetConsoleWindow() != nullptr;
            result.processId = GetCurrentProcessId();
#endif
            app.run();
            result.ok = true;
            return result;
        }

        bool isUiHostInvocation(int argc, char** argv)
        {
            return argc >= 2 && argv && argv[1]
                && std::string(argv[1]) == UI_HOST_SWITCH;
        }

        int runUiHostInvocation(int argc, char** argv)
        {
#ifdef _WIN32
            if (!isUiHostInvocation(argc, argv) || argc != 6)
                return 2;

            unsigned long long pipeValue = 0;
            unsigned long long openBrowserValue = 0;
            unsigned long long heartbeatValue = 0;
            if (!parseUnsigned(argv[2], pipeValue)
                || !parseUnsigned(argv[3], openBrowserValue)
                || !parseUnsigned(argv[4], heartbeatValue)
                || pipeValue == 0
                || openBrowserValue > 1
                || heartbeatValue > static_cast<unsigned long long>(
                    (std::numeric_limits<int>::max)()))
                return 2;

            HANDLE readyPipe = reinterpret_cast<HANDLE>(
                static_cast<std::uintptr_t>(pipeValue));
            UiOptions options;
            options.openBrowser = openBrowserValue != 0;
            options.heartbeatTimeoutSeconds = static_cast<int>(heartbeatValue);
            options.token = argv[5];

            UiApplication app;
            std::string error;
            if (!app.start(options, error))
            {
                sendHandshake(readyPipe, UI_HANDSHAKE_ERROR, nullptr, error);
                CloseHandle(readyPipe);
                return 1;
            }

            sendHandshake(readyPipe, UI_HANDSHAKE_READY, &app, std::string());
            CloseHandle(readyPipe);
            app.run();
            return 0;
#else
            (void)argc;
            (void)argv;
            return 2;
#endif
        }

        UiLaunchResult launchUiDetached(const UiOptions& options)
        {
            UiLaunchResult result;
#ifdef _WIN32
            if (options.heartbeatTimeoutSeconds < 0)
            {
                result.error = "the UI heartbeat timeout cannot be negative";
                return result;
            }

            SECURITY_ATTRIBUTES security{};
            security.nLength = sizeof(security);
            security.bInheritHandle = TRUE;
            HANDLE readyRead = INVALID_HANDLE_VALUE;
            HANDLE readyWrite = INVALID_HANDLE_VALUE;
            if (!CreatePipe(&readyRead, &readyWrite, &security, 0))
            {
                result.error = "failed to create the UI readiness pipe";
                return result;
            }
            if (!SetHandleInformation(readyRead, HANDLE_FLAG_INHERIT, 0))
            {
                CloseHandle(readyRead);
                CloseHandle(readyWrite);
                result.error = "failed to protect the UI readiness pipe";
                return result;
            }

            const std::wstring executable = currentExecutablePath();
            if (executable.empty())
            {
                CloseHandle(readyRead);
                CloseHandle(readyWrite);
                result.error = "failed to locate the current executable";
                return result;
            }

            const std::wstring token = utf8ToWide(options.token);
            if (!options.token.empty() && token.empty())
            {
                CloseHandle(readyRead);
                CloseHandle(readyWrite);
                result.error = "the UI token is not valid UTF-8";
                return result;
            }

            std::wostringstream command;
            command << quoteWindowsArgument(executable)
                    << L" " << UI_HOST_SWITCH
                    << L" " << reinterpret_cast<std::uintptr_t>(readyWrite)
                    << L" " << (options.openBrowser ? 1 : 0)
                    << L" " << options.heartbeatTimeoutSeconds
                    << L" " << quoteWindowsArgument(token);
            std::wstring commandText = command.str();
            std::vector<wchar_t> mutableCommand(commandText.begin(), commandText.end());
            mutableCommand.push_back(L'\0');

            SIZE_T attributeSize = 0;
            InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeSize);
            std::vector<unsigned char> attributeStorage(attributeSize);
            STARTUPINFOEXW startup{};
            startup.StartupInfo.cb = sizeof(startup);
            startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
                attributeStorage.data());
            bool attributeListInitialized = InitializeProcThreadAttributeList(
                startup.lpAttributeList, 1, 0, &attributeSize) != FALSE;
            bool attributesReady = attributeListInitialized;
            HANDLE inheritedHandles[] = { readyWrite };
            if (attributesReady)
            {
                attributesReady = UpdateProcThreadAttribute(
                    startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                    inheritedHandles, sizeof(inheritedHandles), nullptr, nullptr) != FALSE;
            }

            PROCESS_INFORMATION process{};
            BOOL created = FALSE;
            if (attributesReady)
            {
                created = CreateProcessW(
                    executable.c_str(), mutableCommand.data(), nullptr, nullptr, TRUE,
                    CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT,
                    nullptr, nullptr, &startup.StartupInfo, &process);
            }
            if (attributeListInitialized)
                DeleteProcThreadAttributeList(startup.lpAttributeList);
            CloseHandle(readyWrite);

            if (!created)
            {
                CloseHandle(readyRead);
                result.error = "failed to start the hidden UI host";
                return result;
            }
            result.processId = static_cast<std::uint32_t>(process.dwProcessId);
            CloseHandle(process.hThread);

            const auto deadline = Clock::now() + std::chrono::seconds(20);
            UiHandshake message;
            bool received = false;
            while (Clock::now() < deadline)
            {
                DWORD available = 0;
                if (!PeekNamedPipe(readyRead, nullptr, 0, nullptr, &available, nullptr))
                    break;
                if (available >= sizeof(message))
                {
                    received = readAll(readyRead, &message, sizeof(message));
                    break;
                }
                if (WaitForSingleObject(process.hProcess, 25) == WAIT_OBJECT_0)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }

            if (!received)
            {
                if (WaitForSingleObject(process.hProcess, 0) == WAIT_TIMEOUT)
                    TerminateProcess(process.hProcess, 1);
                result.error = "the hidden UI host did not report readiness";
            }
            else if (message.magic != UI_HANDSHAKE_MAGIC
                     || message.version != UI_HANDSHAKE_VERSION
                     || message.tokenSize > UI_HANDSHAKE_MAX_TEXT
                     || message.errorSize > UI_HANDSHAKE_MAX_TEXT)
            {
                TerminateProcess(process.hProcess, 1);
                result.error = "the hidden UI host returned an invalid readiness message";
            }
            else
            {
                std::string tokenText(message.tokenSize, '\0');
                std::string errorText(message.errorSize, '\0');
                bool payloadOk = (message.tokenSize == 0
                                  || readAll(readyRead, &tokenText[0], message.tokenSize))
                              && (message.errorSize == 0
                                  || readAll(readyRead, &errorText[0], message.errorSize));
                if (!payloadOk)
                {
                    TerminateProcess(process.hProcess, 1);
                    result.error = "the hidden UI host readiness message was truncated";
                }
                else if (message.status == UI_HANDSHAKE_READY && message.port != 0)
                {
                    result.ok = true;
                    result.port = static_cast<std::uint16_t>(message.port);
                    result.token = std::move(tokenText);
                    result.childHasConsole = (message.flags & 1u) != 0;
                }
                else
                {
                    result.error = errorText.empty()
                        ? "the hidden UI host failed to start"
                        : std::move(errorText);
                }
            }

            CloseHandle(readyRead);
            CloseHandle(process.hProcess);
            return result;
#else
            (void)options;
            result.error = "detached graphical launch is currently supported on Windows only";
            return result;
#endif
        }
    }
}
