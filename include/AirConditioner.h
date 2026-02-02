#ifndef AIR_CONDITIONER_H
#define AIR_CONDITIONER_H

#include "Appliance.h"

class AirConditioner : public Appliance {
public:
    AirConditioner(const std::string& id, const std::string& name, double power);

    void turnOn() override;
    void turnOff() override;
    bool isOn() const override;

    void setTargetTemperature(double temp);
    double getTargetTemperature() const;

private:
    bool isOn_;
    double targetTemp_;
};

#endif // AIR_CONDITIONER_H
