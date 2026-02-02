#include "Curtain.h"

Curtain::Curtain(const std::string& id, const std::string& name)
    : Appliance(id, name), isOpen_(true), position_(100) {
    powerConsumption_ = 0.01; // Very low power for motors
}

void Curtain::turnOn() {
    // For curtains, "on" means open
    open();
}

void Curtain::turnOff() {
    // For curtains, "off" means close
    close();
}

bool Curtain::isOn() const {
    return isOpen_;
}

void Curtain::open() {
    if (enabled_) {
        isOpen_ = true;
        position_ = 100;
    }
}

void Curtain::close() {
    if (enabled_) {
        isOpen_ = false;
        position_ = 0;
    }
}

void Curtain::setPosition(int pos) {
    if (enabled_ && pos >= 0 && pos <= 100) {
        position_ = pos;
        isOpen_ = (pos > 50);
    }
}

int Curtain::getPosition() const {
    return position_;
}
