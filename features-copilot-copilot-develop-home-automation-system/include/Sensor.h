#ifndef SENSOR_H
#define SENSOR_H

#include "Event.h"
#include "EventManager.h"
#include <string>
#include <memory>

class Sensor {
public:
    Sensor(const std::string& id, const std::string& name)
        : id_(id), name_(name), enabled_(true) {}

    virtual ~Sensor() = default;

    virtual void update() = 0;
    
    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

protected:
    void publishEvent(const Event& event) {
        if (enabled_) {
            EventManager::getInstance().publish(event);
        }
    }

    std::string id_;
    std::string name_;
    bool enabled_;
};

#endif // SENSOR_H
