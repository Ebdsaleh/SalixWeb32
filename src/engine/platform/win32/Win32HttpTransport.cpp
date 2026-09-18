// =================================================================================
// Filename:    engine/platform/win32/Win32HttpTransport.cpp
// Author:      Ebdsaleh
// Description: Implements a small plain-HTTP transport over legacy Winsock2.
// =================================================================================

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "Win32HttpTransport.h"
#include "web/network/NetworkRequest.h"
#include "web/network/NetworkResponse.h"

namespace {
    const int maximum_response_bytes = 1024 * 1024;

    bool resolve_ipv4_address(
        const char* host,
        unsigned long& address
    ) {
        if (host == 0 || host[0] == '\0') {
            return false;
        }

        address = inet_addr(host);
        if (address != INADDR_NONE) {
            return true;
        }

        hostent* host_entry = gethostbyname(host);
        if (
            host_entry == 0 ||
            host_entry->h_addr_list == 0 ||
            host_entry->h_addr_list[0] == 0
        ) {
            return false;
        }

        CopyMemory(
            &address,
            host_entry->h_addr_list[0],
            sizeof(address)
        );
        return true;
    }

    bool socket_is_in_set(
        const fd_set& socket_set,
        SOCKET socket_handle
    ) {
        u_int index = 0;

        for (index = 0; index < socket_set.fd_count; ++index) {
            if (socket_set.fd_array[index] == socket_handle) {
                return true;
            }
        }

        return false;
    }

    bool wait_for_connect(
        SOCKET socket_handle,
        int timeout_milliseconds
    ) {
        fd_set write_set;
        fd_set error_set;

        // Winsock fd_set is an explicit count + socket array.  Populate it
        // directly instead of using FD_ZERO/FD_SET: the VC7.1 SDK macros emit
        // C4127 at /W4 even though the code is correct.
        write_set.fd_count = 1;
        write_set.fd_array[0] = socket_handle;
        error_set.fd_count = 1;
        error_set.fd_array[0] = socket_handle;

        timeval timeout;
        timeout.tv_sec = timeout_milliseconds / 1000;
        timeout.tv_usec =
            (timeout_milliseconds % 1000) * 1000;

        int result = select(
            0,
            0,
            &write_set,
            &error_set,
            &timeout
        );

        if (
            result <= 0 ||
            socket_is_in_set(error_set, socket_handle)
        ) {
            return false;
        }

        int socket_error = 0;
        int socket_error_size = sizeof(socket_error);
        if (getsockopt(
                socket_handle,
                SOL_SOCKET,
                SO_ERROR,
                (char*)&socket_error,
                &socket_error_size
            ) == SOCKET_ERROR) {
            return false;
        }

        return socket_error == 0;
    }

    bool send_all(
        SOCKET socket_handle,
        const std::string& data
    ) {
        int sent_total = 0;
        int data_size = (int)data.size();

        while (sent_total < data_size) {
            int sent = ::send(
                socket_handle,
                data.c_str() + sent_total,
                data_size - sent_total,
                0
            );

            if (sent == SOCKET_ERROR || sent <= 0) {
                return false;
            }

            sent_total += sent;
        }

        return true;
    }

    bool parse_http_response(
        const std::string& raw_response,
        NetworkResponse& response
    ) {
        std::string::size_type first_line_end =
            raw_response.find("\r\n");
        std::string::size_type header_end =
            raw_response.find("\r\n\r\n");

        if (
            first_line_end == std::string::npos ||
            header_end == std::string::npos
        ) {
            return false;
        }

        std::string status_line = raw_response.substr(
            0,
            first_line_end
        );

        int status_code = 0;
        if (sscanf(status_line.c_str(), "HTTP/%*s %d", &status_code) != 1) {
            return false;
        }

        response.set_status_code(status_code);

        std::string::size_type first_space = status_line.find(' ');
        std::string::size_type second_space = first_space == std::string::npos
            ? std::string::npos
            : status_line.find(' ', first_space + 1);

        if (
            second_space != std::string::npos &&
            second_space + 1 < status_line.size()
        ) {
            response.set_status_text(
                status_line.substr(second_space + 1).c_str()
            );
        }

        response.set_body(
            raw_response.substr(header_end + 4)
        );
        return true;
    }
}

Win32HttpTransport::Win32HttpTransport()
    : is_initialized(false),
      timeout_milliseconds(1500) {
}

Win32HttpTransport::~Win32HttpTransport() {
    shutdown();
}

bool Win32HttpTransport::initialize() {
    if (is_initialized) {
        return true;
    }

    WSADATA winsock_data;
    ZeroMemory(&winsock_data, sizeof(winsock_data));

    if (WSAStartup(MAKEWORD(2, 2), &winsock_data) != 0) {
        set_error("WSAStartup failed.");
        return false;
    }

    is_initialized = true;
    last_error.clear();
    return true;
}

void Win32HttpTransport::shutdown() {
    if (!is_initialized) {
        return;
    }

    WSACleanup();
    is_initialized = false;
}

bool Win32HttpTransport::get_is_initialized() const {
    return is_initialized;
}

bool Win32HttpTransport::send(
    const char* host,
    unsigned short port,
    const NetworkRequest& request,
    NetworkResponse& response
) {
    response.clear();
    last_error.clear();

    if (!is_initialized) {
        set_error("Transport is not initialized.");
        return false;
    }

    unsigned long address = 0;
    if (!resolve_ipv4_address(host, address)) {
        set_error("Unable to resolve bridge host.");
        return false;
    }

    SOCKET socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_handle == INVALID_SOCKET) {
        set_error("Unable to create bridge socket.");
        return false;
    }

    u_long nonblocking = 1;
    ioctlsocket(socket_handle, FIONBIO, &nonblocking);

    sockaddr_in socket_address;
    ZeroMemory(&socket_address, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);
    socket_address.sin_addr.s_addr = address;

    int connect_result = connect(
        socket_handle,
        (sockaddr*)&socket_address,
        sizeof(socket_address)
    );

    if (connect_result == SOCKET_ERROR) {
        int connect_error = WSAGetLastError();
        if (
            connect_error != WSAEWOULDBLOCK &&
            connect_error != WSAEINPROGRESS
        ) {
            closesocket(socket_handle);
            set_error("Bridge connection failed.");
            return false;
        }

        if (!wait_for_connect(socket_handle, timeout_milliseconds)) {
            closesocket(socket_handle);
            set_error("Bridge connection timed out or was refused.");
            return false;
        }
    }

    nonblocking = 0;
    ioctlsocket(socket_handle, FIONBIO, &nonblocking);

    int socket_timeout = timeout_milliseconds;
    setsockopt(
        socket_handle,
        SOL_SOCKET,
        SO_RCVTIMEO,
        (const char*)&socket_timeout,
        sizeof(socket_timeout)
    );
    setsockopt(
        socket_handle,
        SOL_SOCKET,
        SO_SNDTIMEO,
        (const char*)&socket_timeout,
        sizeof(socket_timeout)
    );

    char port_text[16];
    char content_length_text[32];
    sprintf(port_text, "%u", (unsigned int)port);
    sprintf(
        content_length_text,
        "%u",
        (unsigned int)request.get_body().size()
    );

    std::string wire_request;
    wire_request += request.get_method();
    wire_request += " ";
    wire_request += request.get_path();
    wire_request += " HTTP/1.0\r\nHost: ";
    wire_request += host == 0 ? "" : host;
    wire_request += ":";
    wire_request += port_text;
    wire_request += "\r\nConnection: close\r\n";

    if (!request.get_content_type().empty()) {
        wire_request += "Content-Type: ";
        wire_request += request.get_content_type();
        wire_request += "\r\n";
    }

    wire_request += "Content-Length: ";
    wire_request += content_length_text;
    wire_request += "\r\n\r\n";
    wire_request += request.get_body();

    if (!send_all(socket_handle, wire_request)) {
        closesocket(socket_handle);
        set_error("Bridge request send failed.");
        return false;
    }

    std::string raw_response;
    char receive_buffer[2048];

    while ((int)raw_response.size() < maximum_response_bytes) {
        int received = recv(
            socket_handle,
            receive_buffer,
            sizeof(receive_buffer),
            0
        );

        if (received == 0) {
            break;
        }

        if (received == SOCKET_ERROR) {
            int receive_error = WSAGetLastError();
            closesocket(socket_handle);

            if (
                receive_error == WSAETIMEDOUT ||
                receive_error == WSAEWOULDBLOCK
            ) {
                set_error("Bridge response timed out.");
            } else {
                set_error("Bridge response receive failed.");
            }
            return false;
        }

        raw_response.append(receive_buffer, received);
    }

    closesocket(socket_handle);

    if ((int)raw_response.size() >= maximum_response_bytes) {
        set_error("Bridge response exceeded the first-pass size limit.");
        return false;
    }

    if (!parse_http_response(raw_response, response)) {
        set_error("Bridge returned an invalid HTTP response.");
        return false;
    }

    return true;
}

const char* Win32HttpTransport::get_last_error() const {
    return last_error.c_str();
}

void Win32HttpTransport::set_timeout_milliseconds(
    int new_timeout_milliseconds
) {
    if (new_timeout_milliseconds < 1) {
        new_timeout_milliseconds = 1;
    }

    timeout_milliseconds = new_timeout_milliseconds;
}

void Win32HttpTransport::set_error(const char* text) {
    last_error = text == 0 ? "Unknown transport error." : text;
}
