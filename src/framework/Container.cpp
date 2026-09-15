// =================================================================================
// Filename:    framework/Container.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral component container.
// =================================================================================

#include "Container.h"

Container::Container() {
}

Container::~Container() {
    clear_children();
}

bool Container::add_child(Component* child) {
    if (child == 0) {
        return false;
    }

    for (int index = 0; index < (int)children.size(); ++index) {
        if (children[index] == child) {
            return false;
        }
    }

    children.push_back(child);
    return true;
}

bool Container::remove_child(Component* child) {
    if (child == 0) {
        return false;
    }

    for (std::vector<Component*>::iterator iterator = children.begin(); iterator != children.end(); ++iterator) {
        if (*iterator == child) {
            children.erase(iterator);
            return true;
        }
    }

    return false;
}

void Container::clear_children() {
    children.clear();
}

int Container::get_child_count() const {
    return (int)children.size();
}

Component* Container::get_child(int index) {
    if (index < 0 || index >= (int)children.size()) {
        return 0;
    }

    return children[index];
}

const Component* Container::get_child(int index) const {
    if (index < 0 || index >= (int)children.size()) {
        return 0;
    }

    return children[index];
}

void Container::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    for (int index = 0; index < (int)children.size(); ++index) {
        if (children[index] != 0) {
            children[index]->render(renderer);
        }
    }
}
