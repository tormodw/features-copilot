#ifndef LIGHT_H
#define LIGHT_H

#include "Appliance.h"

class Light : public Appliance {
public:
    Light(const std::string& id, const std::string& name, double power)
        : Appliance(id, name), isOn_(false), brightness_(100) {
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

    void setBrightness(int level) {
        if (level >= 0 && level <= 100) {
            brightness_ = level;
        }
    }

    int getBrightness() const {
        return brightness_;
    }

private:
    bool isOn_;
    int brightness_;
};

#endif // LIGHT_H
