#include "EVChargerSensor.h"

EVChargerSensor::EVChargerSensor(const std::string& id, const std::string& name)
    : Sensor(id, name), isCharging_(false), chargePower_(0.0) {}

void EVChargerSensor::update() {
    Event event(EventType::EV_CHARGER_STATUS, id_);
    event.addData("is_charging", isCharging_ ? 1.0 : 0.0);
    event.addData("charge_power_kw", chargePower_);
    publishEvent(event);
}

void EVChargerSensor::setCharging(bool charging, double power) {
    isCharging_ = charging;
    chargePower_ = charging ? power : 0.0;
}

bool EVChargerSensor::isCharging() const {
    return isCharging_;
}

double EVChargerSensor::getChargePower() const {
    return chargePower_;
}
