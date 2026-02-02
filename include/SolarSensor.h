#ifndef SOLAR_SENSOR_H
#define SOLAR_SENSOR_H

#include "Sensor.h"

class SolarSensor : public Sensor {
public:
    SolarSensor(const std::string& id, const std::string& name)
        : Sensor(id, name), currentProduction_(0.0) {}

    void update() override {
        Event event(EventType::SOLAR_PRODUCTION_UPDATE, id_);
        event.addData("production_kw", currentProduction_);
        publishEvent(event);
    }

    void setProduction(double kw) {
        currentProduction_ = kw;
    }

    double getProduction() const {
        return currentProduction_;
    }

private:
    double currentProduction_;
};

#endif // SOLAR_SENSOR_H
