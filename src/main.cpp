#include "EventManager.h"
#include "MQTTClient.h"
#include "HTTPClient.h"
#include "EnergyOptimizer.h"
#include "TemperatureSensor.h"
#include "EnergyMeter.h"
#include "SolarSensor.h"
#include "EVChargerSensor.h"
#include "Heater.h"
#include "AirConditioner.h"
#include "Light.h"
#include "Curtain.h"
#include "EVCharger.h"
#include "MLPredictor.h"
#include "DayAheadOptimizer.h"
#include "HistoricalDataGenerator.h"
#include "HAIntegration.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <ctime>

int main() {
    // Setup MQTT and HA Integration
    auto mqttClient = std::make_shared<MQTTClient>("10.0.0.59", 1883);
    mqttClient->connect();
    
    auto haIntegration = std::make_shared<HAIntegration>(mqttClient);
    
    // Local sensor
    auto indoorTempSensor = std::make_shared<TemperatureSensor>(
        "temp_indoor_1", "Living Room", TemperatureSensor::Location::INDOOR);

    auto energyMeter = std::make_shared<EnergyMeter>(
        "energy_meter_1", "Main Energy Meter");


    haIntegration->subscribeToDomain("status",
        [&] (const std::string& entityId, const std::string& state, const std::string& attributes){
            std::cout << "HA: updated sensor change"  << std::endl;
        });
    
    // Subscribe to HA energy meter
    haIntegration->subscribeToEntity("sensor.eva_meter_reader_summation_delivered",
        [&](const std::string& entityId, const std::string& state, const std::string& attributes) {
            (void) attributes;
            try {
                double consumption = std::stod(state);
                energyMeter->setConsumption(consumption);
                energyMeter->update();
                std::cout << "HA: Updated energy consumption from " << entityId << ": " << consumption << " kW" << std::endl;
            } catch (...) {
                std::cerr << "HA: Failed to parse energy consumption from " << entityId << std::endl;
            }
        });
    
    // Control logic example
    if (indoorTempSensor->getTemperature() < 20.0) {
        // Too cold - turn on heater via HA
        haIntegration->publishCommand("switch.heater", "ON");
        std::cout << "Turned on heater via Home Assistant" << std::endl;
    } else if (indoorTempSensor->getTemperature() > 25.0) {
        // Too hot - turn on AC via HA
        haIntegration->publishCommandWithData("climate.ac", "cool",
            "{\"temperature\": 22.0}");
        std::cout << "Turned on AC via Home Assistant" << std::endl;
    }
    
    // Keep running to receive updates
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}