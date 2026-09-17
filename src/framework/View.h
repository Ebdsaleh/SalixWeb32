// =================================================================================
// Filename:    framework/View.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral application view contract.
// =================================================================================
#pragma once

#include <string>

class ComponentRenderer;
class NativeControlHost;
class TextMetrics;
class UIEvent;

class View {
    public:
        virtual ~View() {}

        virtual void attach_native_control_host(NativeControlHost* control_host) {
            (void)control_host;
        }

        virtual void detach_native_control_host() {
        }

        virtual bool build_diagnostic_report(std::string& report) const {
            report.clear();
            return false;
        }

        virtual void layout(
            int width,
            int height,
            TextMetrics* text_metrics = 0
        ) = 0;
        virtual bool handle_event(const UIEvent& event) = 0;
        virtual void render(ComponentRenderer& renderer) = 0;
};
