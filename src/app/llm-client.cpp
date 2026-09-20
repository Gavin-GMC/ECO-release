// SPDX-License-Identifier: BSD-3-Clause

#include "llm-client.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <chrono>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "json-reader.h"
#include "json-writer.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#endif

namespace EvoFlow
{
    namespace App
    {
        namespace
        {
            constexpr std::size_t MAX_RESPONSE_BYTES = 64ull * 1024ull * 1024ull;

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

            bool containsLineBreak(const std::string& value)
            {
                return value.find('\r') != std::string::npos
                    || value.find('\n') != std::string::npos;
            }

            bool containsHeaderControl(const std::string& value)
            {
                for (unsigned char ch : value)
                    if (ch < 0x20 || ch == 0x7f) return true;
                return false;
            }

            LlmResult failure(const std::string& code, const std::string& message,
                              int status = 0)
            {
                LlmResult result;
                result.errorCode = code;
                result.error = message;
                result.httpStatus = status;
                return result;
            }

            bool retryableFailure(const LlmResult& result)
            {
                if (result.errorCode == "network_error"
                    || result.errorCode == "timeout"
                    || result.errorCode == "rate_limited")
                    return true;
                return result.errorCode == "provider_error"
                    && (result.httpStatus == 408
                        || result.httpStatus == 425
                        || result.httpStatus >= 500);
            }

            std::string scrubSecret(std::string message, const std::string& secret)
            {
                if (secret.empty()) return message;
                std::size_t position = 0;
                while ((position = message.find(secret, position)) != std::string::npos)
                {
                    message.replace(position, secret.size(), "[redacted]");
                    position += 10;
                }
                return message;
            }

            std::string requestBody(const LlmSettings& settings, const LlmPrompt& prompt)
            {
                std::ostringstream out;
                JsonWriter json(out);
                json.beginObject()
                    .key("model").value(trim(settings.model))
                    .key("messages").beginArray()
                        .beginObject()
                            .key("role").value("system")
                            .key("content").value(prompt.system)
                        .endObject()
                        .beginObject()
                            .key("role").value("user")
                            .key("content").value(prompt.user)
                        .endObject()
                    .endArray()
                .endObject();
                return out.str();
            }

            std::string providerError(const JsonValue& root)
            {
                const JsonValue* error = root.find("error");
                if (!error || !error->isObject()) return std::string();
                const JsonValue* message = error->find("message");
                return message && message->isString() ? message->stringValue : std::string();
            }

            LlmResult parseProviderResponse(const std::string& body, int status,
                                            const std::string& secret)
            {
                const JsonParseResult parsed = parseJson(body);
                if (status < 200 || status >= 300)
                {
                    std::string message = parsed.ok ? providerError(parsed.value) : std::string();
                    if (message.empty()) message = "The model provider rejected the request.";
                    message = scrubSecret(message, secret);
                    if (message.size() > 2048)
                        message.resize(2048);
                    if (status == 401 || status == 403)
                        return failure("authentication_failed", message, status);
                    if (status == 429)
                        return failure("rate_limited", message, status);
                    return failure("provider_error", message, status);
                }
                if (!parsed.ok)
                    return failure("invalid_response",
                                   "The model provider returned invalid JSON.", status);
                if (!parsed.value.isObject())
                    return failure("invalid_response",
                                   "The model response root is not an object.", status);
                const JsonValue* choices = parsed.value.find("choices");
                if (!choices || !choices->isArray() || choices->arrayValue.empty())
                    return failure("invalid_response",
                                   "The model response has no choices.", status);
                const JsonValue& first = choices->arrayValue.front();
                const JsonValue* message = first.find("message");
                const JsonValue* content =
                    message && message->isObject() ? message->find("content") : nullptr;
                if (!content || !content->isString() || content->stringValue.empty())
                    return failure("invalid_response",
                                   "The model response has no text content.", status);

                LlmResult result;
                result.ok = true;
                result.httpStatus = status;
                result.content = content->stringValue;
                return result;
            }

#ifdef _WIN32
            class InternetHandle
            {
            public:
                InternetHandle() = default;
                explicit InternetHandle(HINTERNET value) : _value(value) {}
                ~InternetHandle() { if (_value) WinHttpCloseHandle(_value); }
                InternetHandle(const InternetHandle&) = delete;
                InternetHandle& operator=(const InternetHandle&) = delete;
                HINTERNET get() const { return _value; }
                explicit operator bool() const { return _value != nullptr; }

            private:
                HINTERNET _value = nullptr;
            };

            std::wstring utf8ToWide(const std::string& value)
            {
                if (value.empty()) return std::wstring();
                const int size = MultiByteToWideChar(
                    CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                    static_cast<int>(value.size()), nullptr, 0);
                if (size <= 0) return std::wstring();
                std::wstring result(static_cast<std::size_t>(size), L'\0');
                if (MultiByteToWideChar(
                        CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), &result[0], size) != size)
                    return std::wstring();
                return result;
            }

            struct ParsedEndpoint
            {
                bool ok = false;
                bool secure = false;
                bool local = false;
                std::wstring host;
                std::wstring path;
                INTERNET_PORT port = 0;
                std::string error;
            };

            ParsedEndpoint parseEndpoint(const std::string& endpoint)
            {
                ParsedEndpoint result;
                const std::string clean = trim(endpoint);
                if (clean.empty())
                {
                    result.error = "Endpoint is required.";
                    return result;
                }
                if (containsLineBreak(clean) || clean.find('#') != std::string::npos)
                {
                    result.error = "Endpoint contains unsupported characters.";
                    return result;
                }
                const std::wstring wide = utf8ToWide(clean);
                if (wide.empty())
                {
                    result.error = "Endpoint is not valid UTF-8.";
                    return result;
                }

                URL_COMPONENTS parts{};
                parts.dwStructSize = sizeof(parts);
                parts.dwSchemeLength = static_cast<DWORD>(-1);
                parts.dwHostNameLength = static_cast<DWORD>(-1);
                parts.dwUserNameLength = static_cast<DWORD>(-1);
                parts.dwPasswordLength = static_cast<DWORD>(-1);
                parts.dwUrlPathLength = static_cast<DWORD>(-1);
                parts.dwExtraInfoLength = static_cast<DWORD>(-1);
                if (!WinHttpCrackUrl(wide.c_str(), static_cast<DWORD>(wide.size()), 0, &parts))
                {
                    result.error = "Endpoint is not a valid HTTP URL.";
                    return result;
                }
                if (parts.nScheme != INTERNET_SCHEME_HTTP
                    && parts.nScheme != INTERNET_SCHEME_HTTPS)
                {
                    result.error = "Endpoint must use HTTP or HTTPS.";
                    return result;
                }
                if (parts.dwHostNameLength == 0)
                {
                    result.error = "Endpoint host is required.";
                    return result;
                }
                if (parts.dwUserNameLength != 0 || parts.dwPasswordLength != 0)
                {
                    result.error = "Endpoint credentials are not supported.";
                    return result;
                }

                result.host.assign(parts.lpszHostName, parts.dwHostNameLength);
                std::wstring lowerHost = result.host;
                std::transform(lowerHost.begin(), lowerHost.end(), lowerHost.begin(),
                               [](wchar_t ch) {
                                   return static_cast<wchar_t>(std::towlower(ch));
                               });
                result.secure = parts.nScheme == INTERNET_SCHEME_HTTPS;
                result.local = lowerHost == L"127.0.0.1" || lowerHost == L"localhost";
                if (!result.secure && !result.local)
                {
                    result.error =
                        "HTTP is allowed only for 127.0.0.1 or localhost endpoints.";
                    return result;
                }
                result.path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
                if (parts.dwExtraInfoLength != 0)
                    result.path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
                if (result.path.empty()) result.path = L"/";
                result.port = parts.nPort;
                result.ok = true;
                return result;
            }

            bool isTimeoutError(DWORD code)
            {
                return code == ERROR_WINHTTP_TIMEOUT
                    || code == WAIT_TIMEOUT;
            }

            LlmResult networkFailure(DWORD code)
            {
                if (isTimeoutError(code))
                    return failure("timeout", "The model request timed out.");
                return failure("network_error",
                               "The model endpoint could not be reached (WinHTTP "
                                   + std::to_string(code) + ").");
            }

            LlmResult sendRequest(const LlmSettings& settings, const ParsedEndpoint& endpoint,
                                  const std::string& body)
            {
                InternetHandle session(WinHttpOpen(
                    L"EvoFlow/2.4.1",
                    endpoint.local ? WINHTTP_ACCESS_TYPE_NO_PROXY
                                   : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
                if (!session) return networkFailure(GetLastError());
                if (!WinHttpSetTimeouts(session.get(), 10000, 10000, 30000, 90000))
                    return networkFailure(GetLastError());

                InternetHandle connection(
                    WinHttpConnect(session.get(), endpoint.host.c_str(), endpoint.port, 0));
                if (!connection) return networkFailure(GetLastError());

                const DWORD flags = endpoint.secure ? WINHTTP_FLAG_SECURE : 0;
                InternetHandle request(WinHttpOpenRequest(
                    connection.get(), L"POST", endpoint.path.c_str(), nullptr,
                    WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
                if (!request) return networkFailure(GetLastError());

                DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
                if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY,
                                      &redirectPolicy, sizeof(redirectPolicy)))
                    return networkFailure(GetLastError());

                std::wstring headers =
                    L"Content-Type: application/json\r\nAccept: application/json\r\n";
                if (!settings.apiKey.empty())
                {
                    const std::wstring key = utf8ToWide(settings.apiKey);
                    if (key.empty())
                        return failure("invalid_settings", "API key is not valid UTF-8.");
                    headers += L"Authorization: Bearer " + key + L"\r\n";
                }
                if (!WinHttpAddRequestHeaders(
                        request.get(), headers.c_str(), static_cast<DWORD>(headers.size()),
                        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE))
                    return networkFailure(GetLastError());

                if (!WinHttpSendRequest(
                        request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                        body.empty() ? WINHTTP_NO_REQUEST_DATA
                                     : const_cast<char*>(body.data()),
                        static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0))
                    return networkFailure(GetLastError());
                if (!WinHttpReceiveResponse(request.get(), nullptr))
                    return networkFailure(GetLastError());

                DWORD status = 0;
                DWORD statusSize = sizeof(status);
                if (!WinHttpQueryHeaders(
                        request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                        WINHTTP_NO_HEADER_INDEX))
                    return networkFailure(GetLastError());

                std::string response;
                for (;;)
                {
                    DWORD available = 0;
                    if (!WinHttpQueryDataAvailable(request.get(), &available))
                        return networkFailure(GetLastError());
                    if (available == 0) break;
                    if (response.size() > MAX_RESPONSE_BYTES
                        || available > MAX_RESPONSE_BYTES - response.size())
                        return failure("invalid_response",
                                       "The model response is larger than 64 MiB.",
                                       static_cast<int>(status));
                    const std::size_t offset = response.size();
                    response.resize(offset + available);
                    DWORD read = 0;
                    if (!WinHttpReadData(request.get(), &response[offset], available, &read))
                        return networkFailure(GetLastError());
                    response.resize(offset + read);
                    if (read == 0) break;
                }
                return parseProviderResponse(
                    response, static_cast<int>(status), settings.apiKey);
            }
#endif
        }

        LlmResult complete(const LlmSettings& settings, const LlmPrompt& prompt,
                           int maxAttempts)
        {
            if (maxAttempts < 1 || maxAttempts > LLM_MAX_STAGE_ATTEMPTS)
                return failure("invalid_settings",
                               "Model request attempt budget must be between 1 and 3.");
            if (trim(settings.endpoint).empty())
                return failure("invalid_settings", "Endpoint is required.");
            if (trim(settings.model).empty())
                return failure("invalid_settings", "Model is required.");
            if (containsLineBreak(settings.model))
                return failure("invalid_settings", "Model contains a line break.");
            if (containsHeaderControl(settings.apiKey))
                return failure("invalid_settings", "API key contains a control character.");
#ifdef _WIN32
            const ParsedEndpoint endpoint = parseEndpoint(settings.endpoint);
            if (!endpoint.ok)
                return failure("unsupported_endpoint", endpoint.error);
            const std::string body = requestBody(settings, prompt);
            if (body.size() > static_cast<std::size_t>((std::numeric_limits<DWORD>::max)()))
                return failure("invalid_settings", "Model request is too large.");
            LlmResult result;
            for (int attempt = 1; attempt <= maxAttempts; attempt++)
            {
                result = sendRequest(settings, endpoint, body);
                result.attempts = attempt;
                if (result.ok || !retryableFailure(result)
                    || attempt == maxAttempts)
                    return result;
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(500 * attempt));
            }
            return result;
#else
            (void)prompt;
            (void)maxAttempts;
            return failure("unsupported_platform",
                           "Model access is currently supported on Windows only.");
#endif
        }

        LlmResult testLlmConnection(const LlmSettings& settings)
        {
            LlmPrompt prompt;
            prompt.system =
                "You are responding to an EvoFlow connection check. Reply with OK only.";
            prompt.user = "Connection check.";
            return complete(settings, prompt);
        }
    }
}
