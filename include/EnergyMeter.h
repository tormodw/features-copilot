#ifndef ENERGY_METER_H
#define ENERGY_METER_H

#include "Sensor.h"

class EnergyMeter : public Sensor {
public:
    EnergyMeter(const std::string& id, const std::string& name);

    void update() override;

    void setConsumption(double kw);
    double getConsumption() const;

private:
    double currentConsumption_;
};

#endif // ENERGY_METER_H
