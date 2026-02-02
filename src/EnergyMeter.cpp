#include "EnergyMeter.h"

EnergyMeter::EnergyMeter(const std::string& id, const std::string& name)
    : Sensor(id, name), currentConsumption_(0.0) {}

void EnergyMeter::update() {
    Event event(EventType::ENERGY_CONSUMPTION_UPDATE, id_);
    event.addData("consumption_kw", currentConsumption_);
    publishEvent(event);
}

void EnergyMeter::setConsumption(double kw) {
    currentConsumption_ = kw;
}

double EnergyMeter::getConsumption() const {
    return currentConsumption_;
}
