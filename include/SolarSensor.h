#ifndef SOLAR_SENSOR_H
#define SOLAR_SENSOR_H

#include "Sensor.h"

class SolarSensor : public Sensor {
public:
    SolarSensor(const std::string& id, const std::string& name);

    void update() override;

    void setProduction(double kw);
    double getProduction() const;

private:
    double currentProduction_;
};

#endif // SOLAR_SENSOR_H
