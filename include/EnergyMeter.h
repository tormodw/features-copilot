#ifndef ENERGY_METER_H
#define ENERGY_METER_H

#include "Sensor.h"

class EnergyMeter : public Sensor {
public:
    EnergyMeter(const std::string& id, const std::string& name)
        : Sensor(id, name), currentConsumption_(0.0) {}

    void update() override {
        Event event(EventType::ENERGY_CONSUMPTION_UPDATE, id_);
        event.addData("consumption_kw", currentConsumption_);
        publishEvent(event);
    }

    void setConsumption(double kw) {
        currentConsumption_ = kw;
    }

    double getConsumption() const {
        return currentConsumption_;
    }

private:
    double currentConsumption_;
};

#endif // ENERGY_METER_H
