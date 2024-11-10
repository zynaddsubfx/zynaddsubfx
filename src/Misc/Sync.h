#pragma once

#include <algorithm> // for std::find
#include <vector>

namespace zyn {

class Observer {
public:
    virtual void update() = 0;
};


class Sync {
public:
    Sync() {}
    void attach(Observer* observer) {
        observers.push_back(observer);
    }
    void detach(Observer* observer) {
        // Prüfen, ob observer im Vektor existiert; falls nicht, einfach return
        auto it = std::find(observers.begin(), observers.end(), observer);
        if (it == observers.end()) {
            return;
        }
        // Wenn gefunden, entfernen
        observers.erase(it);
    }
    void notify() {
        for (Observer* obs : observers) {
            obs->update();  

        }
    }

private:
    std::vector<Observer*> observers;
};

}
