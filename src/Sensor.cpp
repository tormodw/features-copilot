#include "Sensor.h"

Sensor::Sensor(const std::string& id, const std::string& name)
    : id_(id), name_(name), enabled_(true) {}

const std::string& Sensor::getId() const { 
    return id_; 
}

const std::string& Sensor::getName() const { 
    return name_; 
}

bool Sensor::isEnabled() const { 
    return enabled_; 
}

void Sensor::setEnabled(bool enabled) { 
    enabled_ = enabled; 
}

void Sensor::publishEvent(const Event& event) {
    if (enabled_) {
        EventManager::getInstance().publish(event);
    }
}
