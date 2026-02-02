#ifndef APPLIANCE_H
#define APPLIANCE_H

#include <string>

class Appliance {
public:
    Appliance(const std::string& id, const std::string& name)
        : id_(id), name_(name), enabled_(true), powerConsumption_(0.0) {}

    virtual ~Appliance() = default;

    virtual void turnOn() = 0;
    virtual void turnOff() = 0;
    virtual bool isOn() const = 0;

    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    double getPowerConsumption() const { return powerConsumption_; }

protected:
    std::string id_;
    std::string name_;
    bool enabled_;
    double powerConsumption_; // in kW
};

#endif // APPLIANCE_H
