// =================================================================================
// Filename:    engine/platform/win32/Win32NetworkRequestExecutor.cpp
// Author:      Ebdsaleh
// Description: Implements a single-flight Win32 background network executor.
// =================================================================================

#include <process.h>

#include "Win32NetworkRequestExecutor.h"
#include "web/network/NetworkTransport.h"

Win32NetworkRequestExecutor::Win32NetworkRequestExecutor(
    NetworkTransport* new_transport
) : transport(new_transport),
    is_initialized(false),
    critical_section_initialized(false),
    thread_handle(NULL),
    busy(false),
    completed(false),
    request_succeeded(false),
    request_port(0) {
}

Win32NetworkRequestExecutor::~Win32NetworkRequestExecutor() {
    shutdown();
}

bool Win32NetworkRequestExecutor::initialize() {
    if (is_initialized) {
        return true;
    }

    if (transport == 0) {
        set_last_error("Network executor has no transport.");
        return false;
    }

    InitializeCriticalSection(&critical_section);
    critical_section_initialized = true;

    if (!transport->initialize()) {
        set_last_error(transport->get_last_error());
        DeleteCriticalSection(&critical_section);
        critical_section_initialized = false;
        return false;
    }

    busy = false;
    completed = false;
    request_succeeded = false;
    last_error.clear();
    result_error.clear();
    is_initialized = true;
    return true;
}

void Win32NetworkRequestExecutor::shutdown() {
    if (!is_initialized && !critical_section_initialized) {
        return;
    }

    HANDLE active_thread = NULL;

    if (critical_section_initialized) {
        EnterCriticalSection(&critical_section);
        active_thread = thread_handle;
        LeaveCriticalSection(&critical_section);
    }

    if (active_thread != NULL) {
        // NetworkTransport requests are already bounded by their configured
        // timeout.  Wait for the worker before destroying transport state.
        WaitForSingleObject(active_thread, INFINITE);
        CloseHandle(active_thread);
    }

    if (critical_section_initialized) {
        EnterCriticalSection(&critical_section);
        thread_handle = NULL;
        busy = false;
        completed = false;
        request_succeeded = false;
        LeaveCriticalSection(&critical_section);
    }

    if (transport != 0 && transport->get_is_initialized()) {
        transport->shutdown();
    }

    is_initialized = false;

    if (critical_section_initialized) {
        DeleteCriticalSection(&critical_section);
        critical_section_initialized = false;
    }
}

bool Win32NetworkRequestExecutor::get_is_initialized() const {
    return is_initialized;
}

bool Win32NetworkRequestExecutor::submit(
    const char* host,
    unsigned short port,
    const NetworkRequest& new_request
) {
    if (
        !is_initialized ||
        !critical_section_initialized ||
        host == 0 ||
        host[0] == '\0' ||
        port == 0
    ) {
        set_last_error("Network executor request is invalid.");
        return false;
    }

    EnterCriticalSection(&critical_section);

    if (busy) {
        last_error = "A network request is already in progress.";
        LeaveCriticalSection(&critical_section);
        return false;
    }

    request_host = host;
    request_port = port;
    request = new_request;
    response.clear();
    result_error.clear();
    request_succeeded = false;
    completed = false;
    busy = true;
    last_error.clear();

    unsigned thread_id = 0;
    HANDLE new_thread_handle = (HANDLE)_beginthreadex(
        NULL,
        0,
        Win32NetworkRequestExecutor::worker_entry,
        this,
        0,
        &thread_id
    );

    if (new_thread_handle == NULL) {
        busy = false;
        last_error = "Unable to start background network worker.";
        LeaveCriticalSection(&critical_section);
        return false;
    }

    thread_handle = new_thread_handle;
    LeaveCriticalSection(&critical_section);
    return true;
}

bool Win32NetworkRequestExecutor::get_is_busy() const {
    if (!critical_section_initialized) {
        return false;
    }

    EnterCriticalSection(&critical_section);
    bool value = busy;
    LeaveCriticalSection(&critical_section);
    return value;
}

bool Win32NetworkRequestExecutor::take_result(
    NetworkResponse& output_response,
    std::string& error_text,
    bool& succeeded
) {
    if (!critical_section_initialized) {
        return false;
    }

    HANDLE completed_thread = NULL;

    EnterCriticalSection(&critical_section);

    if (!completed) {
        LeaveCriticalSection(&critical_section);
        return false;
    }

    output_response = response;
    error_text = result_error;
    succeeded = request_succeeded;

    completed = false;
    busy = false;
    completed_thread = thread_handle;
    thread_handle = NULL;

    LeaveCriticalSection(&critical_section);

    if (completed_thread != NULL) {
        WaitForSingleObject(completed_thread, INFINITE);
        CloseHandle(completed_thread);
    }

    return true;
}

const char* Win32NetworkRequestExecutor::get_last_error() const {
    return last_error.c_str();
}

unsigned __stdcall Win32NetworkRequestExecutor::worker_entry(void* context) {
    Win32NetworkRequestExecutor* executor =
        (Win32NetworkRequestExecutor*)context;

    if (executor != 0) {
        executor->run_worker();
    }

    return 0;
}

void Win32NetworkRequestExecutor::run_worker() {
    NetworkResponse worker_response;
    bool succeeded = false;
    std::string worker_error;

    if (transport != 0) {
        succeeded = transport->send(
            request_host.c_str(),
            request_port,
            request,
            worker_response
        );

        if (!succeeded && transport->get_last_error() != 0) {
            worker_error = transport->get_last_error();
        }
    } else {
        worker_error = "Network transport is unavailable.";
    }

    EnterCriticalSection(&critical_section);
    response = worker_response;
    result_error = worker_error;
    request_succeeded = succeeded;
    completed = true;
    LeaveCriticalSection(&critical_section);
}

void Win32NetworkRequestExecutor::set_last_error(const char* text) {
    last_error = text == 0 ? "Unknown network executor error." : text;
}
