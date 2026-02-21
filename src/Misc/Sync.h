/*
  ZynAddSubFX - a software synthesizer

  Sync.h - allow sync callback using observer pattern
  Copyright (C) 2024 Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <algorithm> // for std::find

namespace zyn {

#define MAX_OBSERVERS 8

class Observer {
public:
    virtual void update() = 0;
};


class Sync {
public:
    Sync() {observerCount = 0;}
    void attach(Observer* observer) {
        if (observerCount >= MAX_OBSERVERS) {
            return; // No space left to attach a new observer
        }
        // Check if already attached
        if (std::find(observers, observers + observerCount, observer) != observers + observerCount) {
            return; // Observer already attached
        }
        observers[observerCount++] = observer;
    }
    void detach(const Observer* observer) {
        // Find the observer
        auto it = std::find(observers, observers + observerCount, observer);
        if (it == observers + observerCount) {
            return; // Observer not found
        }
        // Remove the observer by shifting the rest
        std::move(it + 1, observers + observerCount, it);
        --observerCount;
    }
    void notify() {
        for (int i = 0; i < observerCount; ++i) {
            observers[i]->update();
        }
    }

private:
    Observer* observers[MAX_OBSERVERS];
    int observerCount;              // Current number of observers
};

}
