// =================================================================================
// Filename:    runtime/RuntimeStatusService.h
// Author:      Ebdsaleh
// Description: Declares lightweight runtime lifecycle/status instrumentation.
// =================================================================================
#pragma once

#include "Service.h"

class RuntimeStatusService : public Service {
    public:
        RuntimeStatusService();

        virtual bool start();
        virtual void update();
        virtual void stop();

        bool get_is_started() const;
        unsigned long get_update_count() const;

    private:
        bool is_started;
        unsigned long update_count;
};
