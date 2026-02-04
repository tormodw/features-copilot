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
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <ctime>

int main() {
    std::cout << "=== Home Automation System Starting ===" << std::endl << std::endl;

    // Initialize communication clients
    auto mqttClient = std::make_shared<MQTTClient>("10.0.0.59", 1883);
    auto httpClient = std::make_shared<HTTPClient>("http://api.energy.com/hourly");

    // Connect to MQTT broker
    if (mqttClient->connect()) {
        std::cout << "Connected to MQTT broker" << std::endl;
    } else {
        std::cerr << "Failed to connect to MQTT broker" << std::endl;
        return 1;
    }

    // Create sensors
    auto indoorTempSensor = std::make_shared<TemperatureSensor>(
        "temp_indoor_1", "Living Room Temperature", TemperatureSensor::Location::INDOOR);
    auto outdoorTempSensor = std::make_shared<TemperatureSensor>(
        "temp_outdoor_1", "Outdoor Temperature", TemperatureSensor::Location::OUTDOOR);
    auto energyMeter = std::make_shared<EnergyMeter>(
        "energy_meter_1", "Main Energy Meter");
    auto solarSensor = std::make_shared<SolarSensor>(
        "solar_1", "Rooftop Solar Panels");
    auto evChargerSensor = std::make_shared<EVChargerSensor>(
        "ev_charger_sensor_1", "EV Charger Status");

    std::cout << "Sensors initialized" << std::endl;

    // Create appliances
    auto heater = std::make_shared<Heater>("heater_1", "Living Room Heater", 2.0);
    auto ac = std::make_shared<AirConditioner>("ac_1", "Living Room AC", 2.5);
    auto light1 = std::make_shared<Light>("light_1", "Living Room Light", 0.1);
    auto light2 = std::make_shared<Light>("light_2", "Bedroom Light", 0.1);
    auto curtain = std::make_shared<Curtain>("curtain_1", "Living Room Curtain");
    auto evCharger = std::make_shared<EVCharger>("ev_charger_1", "Tesla Charger", 11.0);

    std::cout << "Appliances initialized" << std::endl;

    // Initialize energy optimizer
    auto optimizer = std::make_shared<EnergyOptimizer>(httpClient);
    optimizer->addAppliance(heater);
    optimizer->addAppliance(ac);
    optimizer->addAppliance(light1);
    optimizer->addAppliance(light2);
    optimizer->addAppliance(curtain);
    optimizer->addAppliance(evCharger);
    optimizer->setTargetTemperature(22.0);

    std::cout << "Energy optimizer configured" << std::endl << std::endl;

    // Setup ML-based day-ahead optimization
    std::cout << "=== Initializing ML Day-Ahead Optimizer ===" << std::endl;
    
    // Create ML predictor
    auto mlPredictor = std::make_shared<MLPredictor>();
    
    // Generate and train with historical data
    std::cout << "Generating historical data for training..." << std::endl;
    auto historicalData = HistoricalDataGenerator::generateSampleData(30);
    std::cout << "Training ML model with " << historicalData.size() << " data points..." << std::endl;
    mlPredictor->train(historicalData);
    std::cout << "ML model trained successfully" << std::endl;
    
    // Create day-ahead optimizer
    auto dayAheadOptimizer = std::make_shared<DayAheadOptimizer>(mlPredictor);
    dayAheadOptimizer->addAppliance(heater);
    dayAheadOptimizer->addAppliance(ac);
    dayAheadOptimizer->addAppliance(evCharger);
    dayAheadOptimizer->setTargetTemperature(22.0);
    dayAheadOptimizer->setEVChargingHoursNeeded(4);
    
    // Generate day-ahead schedule
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    int currentHour = localTime->tm_hour;
    int currentDayOfWeek = localTime->tm_wday;
    
    auto schedule = dayAheadOptimizer->generateSchedule(currentHour, currentDayOfWeek);
    dayAheadOptimizer->printSchedule(schedule);
    
    std::cout << "Day-ahead optimizer configured" << std::endl << std::endl;

    // Setup MQTT subscriptions for sensors
    // Parse MQTT payloads and update sensors
    mqttClient->subscribe("sensor/temperature/indoor", 
        [&](const std::string& topic, const std::string& payload) {
            try {
                if (payload.empty()) {
                    std::cerr << "Empty payload received from " << topic << std::endl;
                    return;
                }
                double temp = std::stod(payload);
                if (temp < -50.0 || temp > 100.0) {
                    std::cerr << "Temperature value out of range from " << topic << ": " << temp << std::endl;
                    return;
                }
                indoorTempSensor->setTemperature(temp);
                indoorTempSensor->update();
                std::cout << "Received MQTT message on " << topic << ": " << temp << "°C" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse temperature from " << topic << ": " << e.what() << std::endl;
            }
        });
    mqttClient->subscribe("sensor/temperature/outdoor",
        [&](const std::string& topic, const std::string& payload) {
            try {
                if (payload.empty()) {
                    std::cerr << "Empty payload received from " << topic << std::endl;
                    return;
                }
                double temp = std::stod(payload);
                if (temp < -50.0 || temp > 100.0) {
                    std::cerr << "Temperature value out of range from " << topic << ": " << temp << std::endl;
                    return;
                }
                outdoorTempSensor->setTemperature(temp);
                outdoorTempSensor->update();
                std::cout << "Received MQTT message on " << topic << ": " << temp << "°C" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse temperature from " << topic << ": " << e.what() << std::endl;
            }
        });
    mqttClient->subscribe("sensor/solar/production",
        [&](const std::string& topic, const std::string& payload) {
            try {
                if (payload.empty()) {
                    std::cerr << "Empty payload received from " << topic << std::endl;
                    return;
                }
                double production = std::stod(payload);
                if (production < 0.0 || production > 100.0) {
                    std::cerr << "Solar production value out of range from " << topic << ": " << production << std::endl;
                    return;
                }
                solarSensor->setProduction(production);
                solarSensor->update();
                std::cout << "Received MQTT message on " << topic << ": " << production << " kW" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse solar production from " << topic << ": " << e.what() << std::endl;
            }
        });
    mqttClient->subscribe("homeassistant/sensor/eva_meter_reader_summation_delivered/state",
        [&](const std::string& topic, const std::string& payload) {
            try {
                if (payload.empty()) {
                    std::cerr << "Empty payload received from " << topic << std::endl;
                    return;
                }
                double consumption = std::stod(payload);
                if (consumption < 0.0 || consumption > 1000.0) {
                    std::cerr << "Energy consumption value out of range from " << topic << ": " << consumption << std::endl;
                    return;
                }
                energyMeter->setConsumption(consumption);
                energyMeter->update();
                std::cout << "Received MQTT message on " << topic << ": " << consumption << " kW" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse energy consumption from " << topic << ": " << e.what() << std::endl;
            }
        });

    std::cout << "=== Starting simulation ===" << std::endl << std::endl;

    // Simulation loop
    for (int cycle = 0; cycle < 5; cycle++) {
        std::cout << "--- Cycle " << (cycle + 1) << " ---" << std::endl;

        // Simulate sensor readings changing over time
        if (cycle == 0) {
            // Initial state: cold inside, need heating
            indoorTempSensor->setTemperature(18.0);
            outdoorTempSensor->setTemperature(10.0);
            solarSensor->setProduction(0.0);
            energyMeter->setConsumption(2.5);
            evChargerSensor->setCharging(true, 11.0);
        } else if (cycle == 1) {
            // Energy cost becomes high
            indoorTempSensor->setTemperature(19.0);
            outdoorTempSensor->setTemperature(12.0);
            solarSensor->setProduction(1.0);
            energyMeter->setConsumption(3.5);
        } else if (cycle == 2) {
            // High solar production
            indoorTempSensor->setTemperature(21.0);
            outdoorTempSensor->setTemperature(15.0);
            solarSensor->setProduction(8.0);
            energyMeter->setConsumption(4.0);
        } else if (cycle == 3) {
            // Temperature reached target, hot outside
            indoorTempSensor->setTemperature(25.0);
            outdoorTempSensor->setTemperature(30.0);
            solarSensor->setProduction(5.0);
            energyMeter->setConsumption(2.0);
        } else if (cycle == 4) {
            // Low energy cost, normal conditions
            indoorTempSensor->setTemperature(22.0);
            outdoorTempSensor->setTemperature(20.0);
            solarSensor->setProduction(3.0);
            energyMeter->setConsumption(1.5);
        }

        // Update sensor readings (triggers events)
        indoorTempSensor->update();
        outdoorTempSensor->update();
        solarSensor->update();
        energyMeter->update();
        evChargerSensor->update();

        // Update energy cost from API
        optimizer->updateEnergyCost();

        // Wait before next cycle
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << "=== Simulation completed ===" << std::endl;
    std::cout << std::endl << "Home Automation System features:" << std::endl;
    std::cout << "✓ Sensor Integration (Energy, Temperature, Solar, EV)" << std::endl;
    std::cout << "✓ MQTT Communication Protocol" << std::endl;
    std::cout << "✓ HTTP API for Energy Cost Retrieval" << std::endl;
    std::cout << "✓ Appliance Control (Heater, AC, Lights, Curtains, EV Charger)" << std::endl;
    std::cout << "✓ Event-Driven Architecture" << std::endl;
    std::cout << "✓ Energy Cost Optimization" << std::endl;
    std::cout << "✓ Temperature Maintenance" << std::endl;
    std::cout << "✓ Modular and Extensible Design" << std::endl;
    std::cout << "✓ Machine Learning-Based Day-Ahead Optimization" << std::endl;
    std::cout << "✓ Predictive Scheduling for Cost Reduction" << std::endl;

    // Cleanup
    mqttClient->disconnect();
    
    return 0;
}
