#include "Heater.h"

Heater::Heater(const std::string& id, const std::string& name, double power)
    : Appliance(id, name), isOn_(false), targetTemp_(22.0) {
    powerConsumption_ = power;
}

void Heater::turnOn() {
    if (enabled_) {
        isOn_ = true;
    }
}

void Heater::turnOff() {
    isOn_ = false;
}

bool Heater::isOn() const {
    return isOn_;
}

void Heater::setTargetTemperature(double temp) {
    targetTemp_ = temp;
}

double Heater::getTargetTemperature() const {
    return targetTemp_;
}
