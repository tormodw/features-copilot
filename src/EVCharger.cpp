#include "EVCharger.h"

EVCharger::EVCharger(const std::string& id, const std::string& name, double maxPower)
    : Appliance(id, name), isOn_(false), maxChargePower_(maxPower), currentChargePower_(0.0) {
    powerConsumption_ = maxPower;
}

void EVCharger::turnOn() {
    if (enabled_) {
        isOn_ = true;
        currentChargePower_ = maxChargePower_;
    }
}

void EVCharger::turnOff() {
    isOn_ = false;
    currentChargePower_ = 0.0;
}

bool EVCharger::isOn() const {
    return isOn_;
}

void EVCharger::setChargePower(double power) {
    if (power >= 0.0 && power <= maxChargePower_) {
        currentChargePower_ = power;
        powerConsumption_ = power;
    }
}

double EVCharger::getChargePower() const {
    return currentChargePower_;
}

double EVCharger::getMaxChargePower() const {
    return maxChargePower_;
}
