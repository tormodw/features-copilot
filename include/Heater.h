#ifndef HEATER_H
#define HEATER_H

#include "Appliance.h"

class Heater : public Appliance {
public:
    Heater(const std::string& id, const std::string& name, double power);

    void turnOn() override;
    void turnOff() override;
    bool isOn() const override;

    void setTargetTemperature(double temp);
    double getTargetTemperature() const;

private:
    bool isOn_;
    double targetTemp_;
};

#endif // HEATER_H
