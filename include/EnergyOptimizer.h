#ifndef ENERGY_OPTIMIZER_H
#define ENERGY_OPTIMIZER_H

#include "EventManager.h"
#include "HTTPClient.h"
#include "Appliance.h"
#include "Heater.h"
#include "AirConditioner.h"
#include "Light.h"
#include "Curtain.h"
#include "EVCharger.h"
#include <memory>
#include <vector>
#include <iostream>

class EnergyOptimizer {
public:
    EnergyOptimizer(std::shared_ptr<HTTPClient> httpClient);

    void addAppliance(std::shared_ptr<Appliance> appliance);
    void setTargetTemperature(double temp);
    void updateEnergyCost();
    void optimizeEnergyUsage();

private:
    void subscribeToEvents();
    void optimizeEVCharging();
    void optimizeTemperatureControl();
    void optimizeLighting();
    void optimizeCurtains();

    std::shared_ptr<HTTPClient> httpClient_;
    std::vector<std::shared_ptr<Appliance>> appliances_;
    
    double currentEnergyCost_;
    double indoorTemp_;
    double outdoorTemp_;
    double solarProduction_;
    double energyConsumption_;
    double targetIndoorTemp_;
    double highCostThreshold_;
    double lowCostThreshold_;
};

#endif // ENERGY_OPTIMIZER_H
