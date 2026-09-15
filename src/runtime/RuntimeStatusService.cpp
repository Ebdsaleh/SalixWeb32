// =================================================================================
// Filename:    runtime/RuntimeStatusService.cpp
// Author:      Ebdsaleh
// Description: Implements lightweight runtime lifecycle/status instrumentation.
// =================================================================================

#include "RuntimeStatusService.h"
#include "Diagnostics.h"

RuntimeStatusService::RuntimeStatusService()
    : is_started(false),
      update_count(0) {
}

bool RuntimeStatusService::start() {
    if (is_started) {
        return true;
    }

    update_count = 0;
    is_started = true;
    Diagnostics::write_line("RuntimeStatusService started.");
    return true;
}

void RuntimeStatusService::update() {
    if (!is_started) {
        return;
    }

    ++update_count;
}

void RuntimeStatusService::stop() {
    if (!is_started) {
        return;
    }

    is_started = false;
    Diagnostics::write_line("RuntimeStatusService stopped.");
}

bool RuntimeStatusService::get_is_started() const {
    return is_started;
}

unsigned long RuntimeStatusService::get_update_count() const {
    return update_count;
}
