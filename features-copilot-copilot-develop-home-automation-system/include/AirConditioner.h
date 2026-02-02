#ifndef AIR_CONDITIONER_H
#define AIR_CONDITIONER_H

#include "Appliance.h"

class AirConditioner : public Appliance {
public:
    AirConditioner(const std::string& id, const std::string& name, double power)
        : Appliance(id, name), isOn_(false), targetTemp_(24.0) {
        powerConsumption_ = power;
    }

    void turnOn() override {
        if (enabled_) {
            isOn_ = true;
        }
    }

    void turnOff() override {
        isOn_ = false;
    }

    bool isOn() const override {
        return isOn_;
    }

    void setTargetTemperature(double temp) {
        targetTemp_ = temp;
    }

    double getTargetTemperature() const {
        return targetTemp_;
    }

private:
    bool isOn_;
    double targetTemp_;
};

#endif // AIR_CONDITIONER_H
