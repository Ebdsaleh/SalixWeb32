// =================================================================================
// Filename:    runtime/Service.h
// Author:      Ebdsaleh
// Description: Declares the common lifecycle contract for runtime services.
// =================================================================================
#pragma once

class Service {
    public:
        virtual ~Service() {}

        virtual bool start() = 0;
        virtual void update() = 0;
        virtual void stop() = 0;
};
