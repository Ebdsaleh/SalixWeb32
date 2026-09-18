// =================================================================================
// Filename:    security/SecureTransportProvider.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral secure transport provider discovery state.
// =================================================================================
#pragma once

class SecureTransportProvider {
    public:
        virtual ~SecureTransportProvider() {}

        virtual const char* get_name() const = 0;
        virtual const char* get_status_text() const = 0;
        virtual bool get_is_ready() const = 0;
        virtual unsigned long get_capabilities() const = 0;
};
