#ifndef HEATER_H
#define HEATER_H

#include "Appliance.h"

class Heater : public Appliance {
public:
    Heater(const std::string& id, const std::string& name, double power)
        : Appliance(id, name), isOn_(false), targetTemp_(22.0) {
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

#endif // HEATER_H
