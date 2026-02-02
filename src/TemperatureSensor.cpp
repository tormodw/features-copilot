#include "TemperatureSensor.h"

TemperatureSensor::TemperatureSensor(const std::string& id, const std::string& name, Location loc)
    : Sensor(id, name), location_(loc), currentTemp_(20.0) {}

void TemperatureSensor::update() {
    // Simulate reading from MQTT or actual sensor
    Event event(EventType::TEMPERATURE_CHANGE, id_);
    event.addData("temperature", currentTemp_);
    event.addData("location", static_cast<double>(location_));
    publishEvent(event);
}

void TemperatureSensor::setTemperature(double temp) {
    currentTemp_ = temp;
}

double TemperatureSensor::getTemperature() const {
    return currentTemp_;
}

TemperatureSensor::Location TemperatureSensor::getLocation() const {
    return location_;
}
