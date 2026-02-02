#ifndef EV_CHARGER_H
#define EV_CHARGER_H

#include "Appliance.h"

class EVCharger : public Appliance {
public:
    EVCharger(const std::string& id, const std::string& name, double maxPower);

    void turnOn() override;
    void turnOff() override;
    bool isOn() const override;

    void setChargePower(double power);
    double getChargePower() const;
    double getMaxChargePower() const;

private:
    bool isOn_;
    double maxChargePower_;
    double currentChargePower_;
};

#endif // EV_CHARGER_H
