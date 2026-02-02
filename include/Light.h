#ifndef LIGHT_H
#define LIGHT_H

#include "Appliance.h"

class Light : public Appliance {
public:
    Light(const std::string& id, const std::string& name, double power);

    void turnOn() override;
    void turnOff() override;
    bool isOn() const override;

    void setBrightness(int level);
    int getBrightness() const;

private:
    bool isOn_;
    int brightness_;
};

#endif // LIGHT_H
