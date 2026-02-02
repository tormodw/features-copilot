#include "AirConditioner.h"

AirConditioner::AirConditioner(const std::string& id, const std::string& name, double power)
    : Appliance(id, name), isOn_(false), targetTemp_(24.0) {
    powerConsumption_ = power;
}

void AirConditioner::turnOn() {
    if (enabled_) {
        isOn_ = true;
    }
}

void AirConditioner::turnOff() {
    isOn_ = false;
}

bool AirConditioner::isOn() const {
    return isOn_;
}

void AirConditioner::setTargetTemperature(double temp) {
    targetTemp_ = temp;
}

double AirConditioner::getTargetTemperature() const {
    return targetTemp_;
}
