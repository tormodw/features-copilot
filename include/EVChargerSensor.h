#ifndef EV_CHARGER_SENSOR_H
#define EV_CHARGER_SENSOR_H

#include "Sensor.h"

class EVChargerSensor : public Sensor {
public:
    EVChargerSensor(const std::string& id, const std::string& name)
        : Sensor(id, name), isCharging_(false), chargePower_(0.0) {}

    void update() override {
        Event event(EventType::EV_CHARGER_STATUS, id_);
        event.addData("is_charging", isCharging_ ? 1.0 : 0.0);
        event.addData("charge_power_kw", chargePower_);
        publishEvent(event);
    }

    void setCharging(bool charging, double power = 0.0) {
        isCharging_ = charging;
        chargePower_ = charging ? power : 0.0;
    }

    bool isCharging() const {
        return isCharging_;
    }

    double getChargePower() const {
        return chargePower_;
    }

private:
    bool isCharging_;
    double chargePower_;
};

#endif // EV_CHARGER_SENSOR_H
