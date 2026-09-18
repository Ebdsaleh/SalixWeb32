// =================================================================================
// Filename:    engine/platform/win32/Win32SecureTransportProvider.h
// Author:      Ebdsaleh
// Description: Loads the optional Salix secure transport DLL through a flat C ABI.
// =================================================================================
#pragma once

#include <windows.h>
#include <string>

#include "security/SecureTransportProvider.h"
#include "security/SalixSecureTransportAbi.h"

class Win32SecureTransportProvider : public SecureTransportProvider {
    public:
        Win32SecureTransportProvider();
        virtual ~Win32SecureTransportProvider();

        void initialize(const char* executable_directory);
        void shutdown();

        virtual const char* get_name() const;
        virtual const char* get_status_text() const;
        virtual bool get_is_ready() const;
        virtual unsigned long get_capabilities() const;

    private:
        void set_unavailable(const char* status);
        void set_incompatible(const char* status);

        HMODULE module_handle;
        bool is_ready;
        unsigned long capabilities;
        std::string provider_name;
        std::string status_text;
};
