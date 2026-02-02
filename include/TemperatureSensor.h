#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include "Sensor.h"

class TemperatureSensor : public Sensor {
public:
    enum class Location {
        INDOOR,
        OUTDOOR
    };

    TemperatureSensor(const std::string& id, const std::string& name, Location loc)
        : Sensor(id, name), location_(loc), currentTemp_(20.0) {}

    void update() override {
        // Simulate reading from MQTT or actual sensor
        Event event(EventType::TEMPERATURE_CHANGE, id_);
        event.addData("temperature", currentTemp_);
        event.addData("location", static_cast<double>(location_));
        publishEvent(event);
    }

    void setTemperature(double temp) {
        currentTemp_ = temp;
    }

    double getTemperature() const {
        return currentTemp_;
    }

    Location getLocation() const {
        return location_;
    }

private:
    Location location_;
    double currentTemp_;
};

#endif // TEMPERATURE_SENSOR_H
