#ifndef CURTAIN_H
#define CURTAIN_H

#include "Appliance.h"

class Curtain : public Appliance {
public:
    Curtain(const std::string& id, const std::string& name)
        : Appliance(id, name), isOpen_(true), position_(100) {
        powerConsumption_ = 0.01; // Very low power for motors
    }

    void turnOn() override {
        // For curtains, "on" means open
        open();
    }

    void turnOff() override {
        // For curtains, "off" means close
        close();
    }

    bool isOn() const override {
        return isOpen_;
    }

    void open() {
        if (enabled_) {
            isOpen_ = true;
            position_ = 100;
        }
    }

    void close() {
        if (enabled_) {
            isOpen_ = false;
            position_ = 0;
        }
    }

    void setPosition(int pos) {
        if (enabled_ && pos >= 0 && pos <= 100) {
            position_ = pos;
            isOpen_ = (pos > 50);
        }
    }

    int getPosition() const {
        return position_;
    }

private:
    bool isOpen_;
    int position_;
};

#endif // CURTAIN_H
