#include "Event.h"

Event::Event(EventType t, const std::string& src) 
    : type(t), source(src), timestamp(0) {}

void Event::addData(const std::string& key, double value) {
    data[key] = value;
}

double Event::getData(const std::string& key, double defaultValue) const {
    auto it = data.find(key);
    return (it != data.end()) ? it->second : defaultValue;
}
