// =================================================================================
// Filename:    framework/Panel.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral styled container panel.
// =================================================================================
#pragma once

#include "Container.h"

class Panel : public Container {
    public:
        Panel();
        virtual ~Panel();

        virtual void render(ComponentRenderer& renderer) const;
};
