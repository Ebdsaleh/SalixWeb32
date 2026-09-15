// =================================================================================
// Filename:    framework/Container.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral component container.
// =================================================================================
#pragma once

#include <vector>

#include "Component.h"

class Container : public Component {
    public:
        Container();
        virtual ~Container();

        bool add_child(Component* child);
        bool remove_child(Component* child);
        void clear_children();

        int get_child_count() const;
        Component* get_child(int index);
        const Component* get_child(int index) const;

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        std::vector<Component*> children;
};
