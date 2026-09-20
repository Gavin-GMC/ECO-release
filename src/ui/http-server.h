//------------------------Description------------------------
// 最小本地 HTTP/1.1 服务：仅供 EvoFlow 浏览器 UI 的静态资源与 JSON 路由。

//-------------------------Reference-------------------------
// HTTP/1.1 message syntax (RFC 9112 subset).

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace EvoFlow
{
    namespace UI
    {
        struct HttpRequest
        {
            std::string method;
            std::string target;
            std::string path;
            std::string query;
            std::map<std::string, std::string> headers;
            std::string body;

            std::string header(const std::string& lowercaseName) const
            {
                auto it = headers.find(lowercaseName);
                return it == headers.end() ? std::string() : it->second;
            }
        };

        struct HttpResponse
        {
            int status = 200;
            std::string contentType = "text/plain; charset=utf-8";
            std::string body;
            std::vector<std::pair<std::string, std::string>> headers;
            bool stopServerAfterResponse = false;
        };

        class LocalHttpServer
        {
        public:
            using Handler = std::function<HttpResponse(const HttpRequest&)>;

            LocalHttpServer();
            ~LocalHttpServer();
            LocalHttpServer(const LocalHttpServer&) = delete;
            LocalHttpServer& operator=(const LocalHttpServer&) = delete;

            bool start(const Handler& handler, std::string& error,
                       std::size_t maxHeaderBytes = 16 * 1024,
                       std::size_t maxBodyBytes = 64 * 1024 * 1024);
            void run();
            void stop();
            std::uint16_t port() const;

        private:
            struct Impl;
            std::unique_ptr<Impl> _impl;
        };
    }
}
