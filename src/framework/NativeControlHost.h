// =================================================================================
// Filename:    framework/NativeControlHost.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral host contract for native UI peers.
// =================================================================================
#pragma once

class ComboBox;

class NativeControlHost {
    public:
        virtual ~NativeControlHost() {}

        virtual bool attach_combo_box(ComboBox* combo_box) = 0;
        virtual void detach_combo_box(ComboBox* combo_box) = 0;
        virtual void sync_combo_box(ComboBox* combo_box) = 0;
};
