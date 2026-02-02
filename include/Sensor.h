#ifndef SENSOR_H
#define SENSOR_H

#include "Event.h"
#include "EventManager.h"
#include <string>
#include <memory>

class Sensor {
public:
    Sensor(const std::string& id, const std::string& name);

    virtual ~Sensor() = default;

    virtual void update() = 0;
    
    const std::string& getId() const;
    const std::string& getName() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);

protected:
    void publishEvent(const Event& event);

    std::string id_;
    std::string name_;
    bool enabled_;
};

#endif // SENSOR_H
