// =================================================================================
// Filename:    web/platform/WebSurfaceSnapshot.h
// Author:      Ebdsaleh
// Description: Declares the first backend-neutral WebView surface snapshot.
// =================================================================================
#pragma once

#include <string>

class WebSurfaceSnapshot {
    public:
        WebSurfaceSnapshot();

        void clear();

        std::string title;
        std::string address;
        std::string status;
        std::string content;
};
