#ifndef EV_CHARGER_SENSOR_H
#define EV_CHARGER_SENSOR_H

#include "Sensor.h"

class EVChargerSensor : public Sensor {
public:
    EVChargerSensor(const std::string& id, const std::string& name);

    void update() override;

    void setCharging(bool charging, double power = 0.0);
    bool isCharging() const;
    double getChargePower() const;

private:
    bool isCharging_;
    double chargePower_;
};

#endif // EV_CHARGER_SENSOR_H
