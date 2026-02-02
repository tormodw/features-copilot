#ifndef EV_CHARGER_H
#define EV_CHARGER_H

#include "Appliance.h"

class EVCharger : public Appliance {
public:
    EVCharger(const std::string& id, const std::string& name, double maxPower)
        : Appliance(id, name), isOn_(false), maxChargePower_(maxPower), currentChargePower_(0.0) {
        powerConsumption_ = maxPower;
    }

    void turnOn() override {
        if (enabled_) {
            isOn_ = true;
            currentChargePower_ = maxChargePower_;
        }
    }

    void turnOff() override {
        isOn_ = false;
        currentChargePower_ = 0.0;
    }

    bool isOn() const override {
        return isOn_;
    }

    void setChargePower(double power) {
        if (power >= 0.0 && power <= maxChargePower_) {
            currentChargePower_ = power;
            powerConsumption_ = power;
        }
    }

    double getChargePower() const {
        return currentChargePower_;
    }

    double getMaxChargePower() const {
        return maxChargePower_;
    }

private:
    bool isOn_;
    double maxChargePower_;
    double currentChargePower_;
};

#endif // EV_CHARGER_H
