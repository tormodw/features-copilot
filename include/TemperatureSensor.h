#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include "Sensor.h"

class TemperatureSensor : public Sensor {
public:
    enum class Location {
        INDOOR,
        OUTDOOR
    };

    TemperatureSensor(const std::string& id, const std::string& name, Location loc);

    void update() override;

    void setTemperature(double temp);
    double getTemperature() const;
    Location getLocation() const;

private:
    Location location_;
    double currentTemp_;
};

#endif // TEMPERATURE_SENSOR_H
