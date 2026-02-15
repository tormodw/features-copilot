#include "Appliance.h"

Appliance::Appliance(const std::string& id, const std::string& name)
    : id_(id), name_(name), enabled_(true), powerConsumption_(0.0), deferrable_(false) {}

const std::string& Appliance::getId() const { 
    return id_; 
}

const std::string& Appliance::getName() const { 
    return name_; 
}

bool Appliance::isEnabled() const { 
    return enabled_; 
}

void Appliance::setEnabled(bool enabled) { 
    enabled_ = enabled; 
}

double Appliance::getPowerConsumption() const { 
    return powerConsumption_; 
}

bool Appliance::isDeferrable() const {
    return deferrable_;
}

void Appliance::setDeferrable(bool deferrable) {
    deferrable_ = deferrable;
}
