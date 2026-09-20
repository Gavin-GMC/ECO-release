//------------------------Description------------------------
// EvoFlow 浏览器 UI 应用：token、静态资源、问题上传路由、心跳与浏览器启动。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cstdint>
#include <memory>
#include <string>

namespace EvoFlow
{
    namespace UI
    {
        struct UiOptions
        {
            bool openBrowser = true;
            std::string token;              // empty = generate a secure one-time token
            int heartbeatTimeoutSeconds = 600; // 0 = disabled (tests)
        };

        class UiApplication
        {
        public:
            UiApplication();
            ~UiApplication();
            UiApplication(const UiApplication&) = delete;
            UiApplication& operator=(const UiApplication&) = delete;

            bool start(const UiOptions& options, std::string& error);
            void run();
            void stop();
            std::uint16_t port() const;
            std::string token() const;
            std::string url() const;

        private:
            struct Impl;
            std::unique_ptr<Impl> _impl;
        };

        struct UiLaunchResult
        {
            bool ok = false;
            std::string error;
            std::uint16_t port = 0;
            std::string token;
            bool childHasConsole = false;
            std::uint32_t processId = 0;
        };

        // Runs the UI in the current process until shutdown. This is used by the
        // hidden host process; normal no-argument launches use launchUiDetached().
        UiLaunchResult launchUi();

        // Windows no-argument launcher: starts a console-less copy of the current
        // executable, waits until its HTTP server/browser launch is ready, then
        // returns so the caller's console can close or its shell prompt can resume.
        UiLaunchResult launchUiDetached(const UiOptions& options = UiOptions{});

        // Internal command-line dispatch shared by evoflow.exe and the launcher
        // smoke test. The host switch is intentionally omitted from public usage.
        bool isUiHostInvocation(int argc, char** argv);
        int runUiHostInvocation(int argc, char** argv);
    }
}
