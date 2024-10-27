#pragma once

namespace zyn {

class Observer {
public:
    virtual void update() = 0;
};


class Sync {
public:
    void attach(Observer* observer) {
        observers.push_back(observer);
    }
//    void detach(Observer* observer) {
//        observers.erase(std::remove(observers.begin(),  
// observers.end(), observer), observers.end());
//    }
    void notify() {
        for (Observer* obs : observers) {
            obs->update();  

        }
    }

private:
    std::vector<Observer*> observers;
};

}
