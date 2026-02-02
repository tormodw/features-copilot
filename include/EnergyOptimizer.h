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
    EnergyOptimizer(std::shared_ptr<HTTPClient> httpClient)
        : httpClient_(httpClient), 
          currentEnergyCost_(0.0),
          indoorTemp_(20.0),
          outdoorTemp_(15.0),
          solarProduction_(0.0),
          energyConsumption_(0.0),
          targetIndoorTemp_(22.0),
          highCostThreshold_(0.15),
          lowCostThreshold_(0.10) {
        
        subscribeToEvents();
    }

    void addAppliance(std::shared_ptr<Appliance> appliance) {
        appliances_.push_back(appliance);
    }

    void setTargetTemperature(double temp) {
        targetIndoorTemp_ = temp;
    }

    void updateEnergyCost() {
        currentEnergyCost_ = httpClient_->getCurrentEnergyCost();
        Event event(EventType::ENERGY_COST_UPDATE, "energy_optimizer");
        event.addData("cost_per_kwh", currentEnergyCost_);
        EventManager::getInstance().publish(event);
        
        // Re-evaluate decisions based on new cost
        optimizeEnergyUsage();
    }

    void optimizeEnergyUsage() {
        std::cout << "=== Energy Optimization Cycle ===" << std::endl;
        std::cout << "Current Energy Cost: $" << currentEnergyCost_ << "/kWh" << std::endl;
        std::cout << "Indoor Temperature: " << indoorTemp_ << "°C" << std::endl;
        std::cout << "Outdoor Temperature: " << outdoorTemp_ << "°C" << std::endl;
        std::cout << "Solar Production: " << solarProduction_ << " kW" << std::endl;
        std::cout << "Energy Consumption: " << energyConsumption_ << " kW" << std::endl;

        // Decision 1: EV Charging based on energy cost
        optimizeEVCharging();

        // Decision 2: Temperature control
        optimizeTemperatureControl();

        // Decision 3: Lighting optimization
        optimizeLighting();

        // Decision 4: Curtain control for passive temperature management
        optimizeCurtains();

        std::cout << "=================================" << std::endl << std::endl;
    }

private:
    void subscribeToEvents() {
        auto& eventMgr = EventManager::getInstance();

        eventMgr.subscribe(EventType::TEMPERATURE_CHANGE, 
            [this](const Event& e) {
                double temp = e.getData("temperature");
                int location = static_cast<int>(e.getData("location"));
                if (location == 0) { // Indoor
                    indoorTemp_ = temp;
                } else { // Outdoor
                    outdoorTemp_ = temp;
                }
                optimizeEnergyUsage();
            });

        eventMgr.subscribe(EventType::SOLAR_PRODUCTION_UPDATE,
            [this](const Event& e) {
                solarProduction_ = e.getData("production_kw");
                optimizeEnergyUsage();
            });

        eventMgr.subscribe(EventType::ENERGY_CONSUMPTION_UPDATE,
            [this](const Event& e) {
                energyConsumption_ = e.getData("consumption_kw");
            });
    }

    void optimizeEVCharging() {
        for (auto& appliance : appliances_) {
            auto evCharger = std::dynamic_pointer_cast<EVCharger>(appliance);
            if (evCharger) {
                // Stop charging if cost is high and solar production is low
                if (currentEnergyCost_ > highCostThreshold_ && 
                    solarProduction_ < evCharger->getChargePower()) {
                    if (evCharger->isOn()) {
                        std::cout << "Stopping EV charging: High energy cost ($" 
                                  << currentEnergyCost_ << "/kWh)" << std::endl;
                        evCharger->turnOff();
                    }
                } 
                // Resume charging if cost is low or solar production is sufficient
                else if (currentEnergyCost_ <= lowCostThreshold_ || 
                         solarProduction_ >= evCharger->getChargePower()) {
                    if (!evCharger->isOn()) {
                        std::cout << "Resuming EV charging: Favorable conditions" << std::endl;
                        evCharger->turnOn();
                    }
                }
            }
        }
    }

    void optimizeTemperatureControl() {
        double tempDiff = targetIndoorTemp_ - indoorTemp_;
        
        for (auto& appliance : appliances_) {
            auto heater = std::dynamic_pointer_cast<Heater>(appliance);
            if (heater) {
                // Turn on heater if too cold
                if (tempDiff > 2.0) {
                    if (!heater->isOn()) {
                        std::cout << "Turning on heater: Temperature " << tempDiff 
                                  << "°C below target" << std::endl;
                        heater->turnOn();
                    }
                } 
                // Turn off heater if temperature is acceptable or cost is too high
                else if (tempDiff < 0.5 || 
                         (currentEnergyCost_ > highCostThreshold_ && tempDiff < 1.5)) {
                    if (heater->isOn()) {
                        std::cout << "Turning off heater: Target reached or high cost" << std::endl;
                        heater->turnOff();
                    }
                }
            }

            auto ac = std::dynamic_pointer_cast<AirConditioner>(appliance);
            if (ac) {
                // Turn on AC if too hot
                if (tempDiff < -2.0) {
                    if (!ac->isOn()) {
                        std::cout << "Turning on AC: Temperature " << (-tempDiff) 
                                  << "°C above target" << std::endl;
                        ac->turnOn();
                    }
                }
                // Turn off AC if temperature is acceptable or cost is too high
                else if (tempDiff > -0.5 || 
                         (currentEnergyCost_ > highCostThreshold_ && tempDiff > -1.5)) {
                    if (ac->isOn()) {
                        std::cout << "Turning off AC: Target reached or high cost" << std::endl;
                        ac->turnOff();
                    }
                }
            }
        }
    }

    void optimizeLighting() {
        // Simple optimization: dim lights when solar production is low and cost is high
        for (auto& appliance : appliances_) {
            auto light = std::dynamic_pointer_cast<Light>(appliance);
            if (light && light->isOn()) {
                if (currentEnergyCost_ > highCostThreshold_ && solarProduction_ < 1.0) {
                    if (light->getBrightness() > 70) {
                        std::cout << "Reducing light brightness to save energy" << std::endl;
                        light->setBrightness(70);
                    }
                }
            }
        }
    }

    void optimizeCurtains() {
        // Passive temperature control with curtains
        for (auto& appliance : appliances_) {
            auto curtain = std::dynamic_pointer_cast<Curtain>(appliance);
            if (curtain) {
                // Close curtains if it's hot outside and need cooling
                if (outdoorTemp_ > indoorTemp_ + 5.0 && indoorTemp_ > targetIndoorTemp_) {
                    if (curtain->getPosition() > 20) {
                        std::cout << "Closing curtains to block heat" << std::endl;
                        curtain->setPosition(20);
                    }
                }
                // Open curtains if it's cold outside and we have solar production
                else if (outdoorTemp_ < indoorTemp_ && solarProduction_ > 0.5) {
                    if (curtain->getPosition() < 80) {
                        std::cout << "Opening curtains to utilize solar heat" << std::endl;
                        curtain->setPosition(80);
                    }
                }
            }
        }
    }

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
