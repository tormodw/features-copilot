#include "SolarSensor.h"

SolarSensor::SolarSensor(const std::string& id, const std::string& name)
    : Sensor(id, name), currentProduction_(0.0) {}

void SolarSensor::update() {
    Event event(EventType::SOLAR_PRODUCTION_UPDATE, id_);
    event.addData("production_kw", currentProduction_);
    publishEvent(event);
}

void SolarSensor::setProduction(double kw) {
    currentProduction_ = kw;
}

double SolarSensor::getProduction() const {
    return currentProduction_;
}
