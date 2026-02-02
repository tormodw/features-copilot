#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <memory>
#include <map>

enum class EventType {
    TEMPERATURE_CHANGE,
    ENERGY_COST_UPDATE,
    SOLAR_PRODUCTION_UPDATE,
    ENERGY_CONSUMPTION_UPDATE,
    EV_CHARGER_STATUS,
    APPLIANCE_CONTROL
};

class Event {
public:
    EventType type;
    std::string source;
    std::map<std::string, double> data;
    long timestamp;

    Event(EventType t, const std::string& src);
    
    void addData(const std::string& key, double value);
    double getData(const std::string& key, double defaultValue = 0.0) const;
};

#endif // EVENT_H
