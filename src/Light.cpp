#include "Light.h"

Light::Light(const std::string& id, const std::string& name, double power)
    : Appliance(id, name), isOn_(false), brightness_(100) {
    powerConsumption_ = power;
}

void Light::turnOn() {
    if (enabled_) {
        isOn_ = true;
    }
}

void Light::turnOff() {
    isOn_ = false;
}

bool Light::isOn() const {
    return isOn_;
}

void Light::setBrightness(int level) {
    if (level >= 0 && level <= 100) {
        brightness_ = level;
    }
}

int Light::getBrightness() const {
    return brightness_;
}
