// =================================================================================
// Filename:    engine/platform/win32/Win32NetworkRequestExecutor.h
// Author:      Ebdsaleh
// Description: Runs blocking NetworkTransport work away from the Win32 UI thread.
// =================================================================================
#pragma once

#include <windows.h>
#include <string>

#include "web/network/NetworkRequestExecutor.h"
#include "web/network/NetworkRequest.h"
#include "web/network/NetworkResponse.h"

class NetworkTransport;

class Win32NetworkRequestExecutor : public NetworkRequestExecutor {
    public:
        explicit Win32NetworkRequestExecutor(NetworkTransport* transport);
        virtual ~Win32NetworkRequestExecutor();

        virtual bool initialize();
        virtual void shutdown();
        virtual bool get_is_initialized() const;

        virtual bool submit(
            const char* host,
            unsigned short port,
            const NetworkRequest& request
        );

        virtual bool get_is_busy() const;

        virtual bool take_result(
            NetworkResponse& response,
            std::string& error_text,
            bool& succeeded
        );

        virtual const char* get_last_error() const;

    private:
        static unsigned __stdcall worker_entry(void* context);
        void run_worker();
        void set_last_error(const char* text);

        NetworkTransport* transport;
        bool is_initialized;
        mutable CRITICAL_SECTION critical_section;
        bool critical_section_initialized;
        HANDLE thread_handle;

        bool busy;
        bool completed;
        bool request_succeeded;

        std::string request_host;
        unsigned short request_port;
        NetworkRequest request;
        NetworkResponse response;
        std::string result_error;
        std::string last_error;
};
