// =================================================================================
// Filename:    framework/Clipboard.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral clipboard contract using MIME-tagged data.
// =================================================================================
#pragma once

class MimeData;

class Clipboard {
    public:
        virtual ~Clipboard() {}

        virtual bool set_data(const MimeData& data) = 0;
        virtual bool get_data(MimeData& data) = 0;
};
