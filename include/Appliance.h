#ifndef APPLIANCE_H
#define APPLIANCE_H

#include <string>

class Appliance {
public:
    Appliance(const std::string& id, const std::string& name);

    virtual ~Appliance() = default;

    virtual void turnOn() = 0;
    virtual void turnOff() = 0;
    virtual bool isOn() const = 0;

    const std::string& getId() const;
    const std::string& getName() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);
    double getPowerConsumption() const;
    
    // Deferrable load management
    bool isDeferrable() const;
    void setDeferrable(bool deferrable);

protected:
    std::string id_;
    std::string name_;
    bool enabled_;
    double powerConsumption_; // in kW
    bool deferrable_;          // Can this load be deferred/switched off during high prices
};

#endif // APPLIANCE_H
