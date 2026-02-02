#ifndef CURTAIN_H
#define CURTAIN_H

#include "Appliance.h"

class Curtain : public Appliance {
public:
    Curtain(const std::string& id, const std::string& name);

    void turnOn() override;
    void turnOff() override;
    bool isOn() const override;

    void open();
    void close();
    void setPosition(int pos);
    int getPosition() const;

private:
    bool isOpen_;
    int position_;
};

#endif // CURTAIN_H
