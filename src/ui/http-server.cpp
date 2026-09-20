//------------------------Description------------------------
// LocalHttpServer 的 Winsock2 实现；非 Windows 构建保留明确失败的兼容桩。

//-------------------------Reference-------------------------
// Winsock2 (Windows SDK).

// SPDX-License-Identifier: BSD-3-Clause

#include "http-server.h"
#include "json-writer.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <limits>
#include <mutex>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace EvoFlow
{
    namespace UI
    {
        namespace
        {
            std::string lower(std::string value)
            {
                for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                return value;
            }

            std::string trim(const std::string& value)
            {
                std::size_t first = 0;
                while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) first++;
                std::size_t last = value.size();
                while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) last--;
                return value.substr(first, last - first);
            }

            const char* reasonPhrase(int status)
            {
                switch (status)
                {
                case 200: return "OK";
                case 400: return "Bad Request";
                case 403: return "Forbidden";
                case 404: return "Not Found";
                case 405: return "Method Not Allowed";
                case 409: return "Conflict";
                case 413: return "Payload Too Large";
                case 422: return "Unprocessable Content";
                case 500: return "Internal Server Error";
                case 503: return "Service Unavailable";
                default:  return "Error";
                }
            }
        }

        struct LocalHttpServer::Impl
        {
            Handler handler;
            std::size_t maxHeaderBytes = 16 * 1024;
            std::size_t maxBodyBytes = 64 * 1024 * 1024;
            std::atomic<bool> stopping{ false };
            std::atomic<bool> running{ false };
            std::atomic<std::uint16_t> boundPort{ 0 };
#ifdef _WIN32
            std::atomic<SOCKET> listener{ INVALID_SOCKET };
            bool wsaReady = false;
            std::mutex cleanupMutex;
#endif
        };

        LocalHttpServer::LocalHttpServer() : _impl(new Impl) {}
        LocalHttpServer::~LocalHttpServer() { stop(); }

        bool LocalHttpServer::start(const Handler& handler, std::string& error,
                                    std::size_t maxHeaderBytes, std::size_t maxBodyBytes)
        {
#ifndef _WIN32
            (void)handler; (void)maxHeaderBytes; (void)maxBodyBytes;
            error = "the graphical interface is currently supported on Windows only";
            return false;
#else
            if (_impl->listener.load() != INVALID_SOCKET)
            {
                error = "HTTP server already started";
                return false;
            }

            WSADATA data{};
            int rc = WSAStartup(MAKEWORD(2, 2), &data);
            if (rc != 0)
            {
                error = "WSAStartup failed: " + std::to_string(rc);
                return false;
            }
            _impl->wsaReady = true;

            SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (listener == INVALID_SOCKET)
            {
                error = "socket failed: " + std::to_string(WSAGetLastError());
                stop();
                return false;
            }

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = htons(0);
            if (bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
            {
                error = "bind(127.0.0.1) failed: " + std::to_string(WSAGetLastError());
                closesocket(listener);
                stop();
                return false;
            }
            if (listen(listener, SOMAXCONN) == SOCKET_ERROR)
            {
                error = "listen failed: " + std::to_string(WSAGetLastError());
                closesocket(listener);
                stop();
                return false;
            }

            int length = sizeof(address);
            if (getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) == SOCKET_ERROR)
            {
                error = "getsockname failed: " + std::to_string(WSAGetLastError());
                closesocket(listener);
                stop();
                return false;
            }

            _impl->handler = handler;
            _impl->maxHeaderBytes = maxHeaderBytes;
            _impl->maxBodyBytes = maxBodyBytes;
            _impl->boundPort = ntohs(address.sin_port);
            _impl->stopping = false;
            _impl->listener = listener;
            return true;
#endif
        }

#ifdef _WIN32
        namespace
        {
            bool sendAll(SOCKET socketHandle, const std::string& data)
            {
                std::size_t sent = 0;
                while (sent < data.size())
                {
                    int count = send(socketHandle, data.data() + sent,
                                     static_cast<int>(std::min<std::size_t>(data.size() - sent, 1 << 20)), 0);
                    if (count == SOCKET_ERROR || count == 0) return false;
                    sent += static_cast<std::size_t>(count);
                }
                return true;
            }

            std::string responseBytes(const HttpResponse& response)
            {
                std::ostringstream out;
                out << "HTTP/1.1 " << response.status << " " << reasonPhrase(response.status) << "\r\n"
                    << "Content-Type: " << response.contentType << "\r\n"
                    << "Content-Length: " << response.body.size() << "\r\n"
                    << "Connection: close\r\n";
                for (const auto& header : response.headers)
                    out << header.first << ": " << header.second << "\r\n";
                out << "\r\n" << response.body;
                return out.str();
            }

            HttpResponse parseError(int status, const std::string& message)
            {
                std::ostringstream out;
                App::JsonWriter json(out);
                json.beginObject().key("ok").value(false).key("error").value(message).endObject();
                HttpResponse response;
                response.status = status;
                response.contentType = "application/json; charset=utf-8";
                response.body = out.str();
                return response;
            }

            bool receiveRequest(SOCKET socketHandle, std::size_t maxHeaderBytes,
                                std::size_t maxBodyBytes, HttpRequest& request,
                                HttpResponse& errorResponse)
            {
                DWORD timeout = 5000;
                setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO,
                           reinterpret_cast<const char*>(&timeout), sizeof(timeout));
                setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO,
                           reinterpret_cast<const char*>(&timeout), sizeof(timeout));

                std::string bytes;
                std::size_t headerEnd = std::string::npos;
                char buffer[8192];
                while (headerEnd == std::string::npos)
                {
                    int count = recv(socketHandle, buffer, sizeof(buffer), 0);
                    if (count <= 0)
                    {
                        errorResponse = parseError(400, "incomplete request headers");
                        return false;
                    }
                    bytes.append(buffer, static_cast<std::size_t>(count));
                    if (bytes.size() > maxHeaderBytes)
                    {
                        errorResponse = parseError(400, "request headers too large");
                        return false;
                    }
                    headerEnd = bytes.find("\r\n\r\n");
                }

                std::string headerText = bytes.substr(0, headerEnd);
                std::istringstream headers(headerText);
                std::string line;
                if (!std::getline(headers, line))
                {
                    errorResponse = parseError(400, "missing request line");
                    return false;
                }
                if (!line.empty() && line.back() == '\r') line.pop_back();
                std::istringstream requestLine(line);
                std::string version;
                if (!(requestLine >> request.method >> request.target >> version)
                    || version.rfind("HTTP/1.", 0) != 0)
                {
                    errorResponse = parseError(400, "invalid request line");
                    return false;
                }

                while (std::getline(headers, line))
                {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    std::size_t colon = line.find(':');
                    if (colon == std::string::npos) continue;
                    request.headers[lower(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
                }

                std::size_t queryAt = request.target.find('?');
                request.path = request.target.substr(0, queryAt);
                request.query = queryAt == std::string::npos ? std::string() : request.target.substr(queryAt + 1);

                if (!request.header("transfer-encoding").empty())
                {
                    errorResponse = parseError(400, "chunked requests are not supported");
                    return false;
                }

                std::size_t contentLength = 0;
                const std::string lengthText = request.header("content-length");
                if (!lengthText.empty())
                {
                    try
                    {
                        std::size_t used = 0;
                        unsigned long long parsed = std::stoull(lengthText, &used);
                        if (used != lengthText.size()) throw std::invalid_argument("trailing");
                        if (parsed > static_cast<unsigned long long>(
                                std::numeric_limits<std::size_t>::max()))
                            throw std::out_of_range("Content-Length");
                        contentLength = static_cast<std::size_t>(parsed);
                    }
                    catch (...)
                    {
                        errorResponse = parseError(400, "invalid Content-Length");
                        return false;
                    }
                }
                if (contentLength > maxBodyBytes)
                {
                    errorResponse = parseError(413, "request body exceeds 64 MiB");
                    return false;
                }

                request.body = bytes.substr(headerEnd + 4);
                while (request.body.size() < contentLength)
                {
                    int count = recv(socketHandle, buffer, sizeof(buffer), 0);
                    if (count <= 0)
                    {
                        errorResponse = parseError(400, "incomplete request body");
                        return false;
                    }
                    request.body.append(buffer, static_cast<std::size_t>(count));
                }
                if (request.body.size() > contentLength) request.body.resize(contentLength);
                return true;
            }
        }
#endif

        void LocalHttpServer::run()
        {
#ifdef _WIN32
            _impl->running = true;
            while (!_impl->stopping)
            {
                SOCKET listener = _impl->listener.load();
                if (listener == INVALID_SOCKET) break;

                fd_set readable;
                FD_ZERO(&readable);
                FD_SET(listener, &readable);
                timeval wait{ 0, 250000 };
                int ready = select(0, &readable, nullptr, nullptr, &wait);
                if (_impl->stopping) break;
                if (ready == SOCKET_ERROR)
                {
                    if (WSAGetLastError() == WSAENOTSOCK) break;
                    continue;
                }
                if (ready == 0) continue;

                SOCKET client = accept(listener, nullptr, nullptr);
                if (client == INVALID_SOCKET) continue;

                HttpRequest request;
                HttpResponse response;
                if (receiveRequest(client, _impl->maxHeaderBytes, _impl->maxBodyBytes,
                                   request, response))
                {
                    try { response = _impl->handler(request); }
                    catch (const std::exception& ex) { response = parseError(500, ex.what()); }
                    catch (...) { response = parseError(500, "unknown server error"); }
                }
                sendAll(client, responseBytes(response));
                shutdown(client, SD_BOTH);
                closesocket(client);
                if (response.stopServerAfterResponse)
                    _impl->stopping = true;
            }
            _impl->running = false;
            stop();
#endif
        }

        void LocalHttpServer::stop()
        {
            if (!_impl) return;
            _impl->stopping = true;
#ifdef _WIN32
            SOCKET listener = _impl->listener.exchange(INVALID_SOCKET);
            if (listener != INVALID_SOCKET)
            {
                shutdown(listener, SD_BOTH);
                closesocket(listener);
            }
            if (!_impl->running)
            {
                std::lock_guard<std::mutex> lock(_impl->cleanupMutex);
                if (_impl->wsaReady)
                {
                    WSACleanup();
                    _impl->wsaReady = false;
                }
            }
#endif
            _impl->boundPort = 0;
        }

        std::uint16_t LocalHttpServer::port() const { return _impl->boundPort.load(); }
    }
}
