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
    std::cout << "=== Home Automation System Starting ===" << std::endl << std::endl;

    // Initialize communication clients
    auto mqttClient = std::make_shared<MQTTClient>("localhost", 1883);
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

    // Setup Home Assistant MQTT Integration
    std::cout << "=== Configuring Home Assistant MQTT Integration ===" << std::endl;
    
    auto haIntegration = std::make_shared<HAIntegration>(mqttClient, "homeassistant");
    
    // Subscribe to HA temperature sensors
    haIntegration->subscribeToEntity("sensor.living_room_temperature", 
        [&](const std::string& entityId, const std::string& state, const std::string& attributes) {
            try {
                double temp = std::stod(state);
                indoorTempSensor->setTemperature(temp);
                indoorTempSensor->update();
                std::cout << "HA: Updated indoor temperature from " << entityId << ": " << temp << "°C" << std::endl;
            } catch (...) {
                std::cerr << "HA: Failed to parse temperature from " << entityId << std::endl;
            }
        });
    
    haIntegration->subscribeToEntity("sensor.outdoor_temperature",
        [&](const std::string& entityId, const std::string& state, const std::string& attributes) {
            try {
                double temp = std::stod(state);
                outdoorTempSensor->setTemperature(temp);
                outdoorTempSensor->update();
                std::cout << "HA: Updated outdoor temperature from " << entityId << ": " << temp << "°C" << std::endl;
            } catch (...) {
                std::cerr << "HA: Failed to parse temperature from " << entityId << std::endl;
            }
        });
    
    // Subscribe to HA solar production sensor
    haIntegration->subscribeToEntity("sensor.solar_production",
        [&](const std::string& entityId, const std::string& state, const std::string& attributes) {
            try {
                double production = std::stod(state);
                solarSensor->setProduction(production);
                solarSensor->update();
                std::cout << "HA: Updated solar production from " << entityId << ": " << production << " kW" << std::endl;
            } catch (...) {
                std::cerr << "HA: Failed to parse solar production from " << entityId << std::endl;
            }
        });
    
    // Subscribe to HA energy consumption sensor
    haIntegration->subscribeToEntity("sensor.energy_consumption",
        [&](const std::string& entityId, const std::string& state, const std::string& attributes) {
            try {
                double consumption = std::stod(state);
                energyMeter->setConsumption(consumption);
                energyMeter->update();
                std::cout << "HA: Updated energy consumption from " << entityId << ": " << consumption << " kW" << std::endl;
            } catch (...) {
                std::cerr << "HA: Failed to parse energy consumption from " << entityId << std::endl;
            }
        });
    
    // Subscribe to all switch domain entities for appliance control feedback
    haIntegration->subscribeToDomain("switch",
        [](const std::string& entityId, const std::string& state, const std::string& attributes) {
            std::cout << "HA: Switch " << entityId << " state changed to: " << state << std::endl;
        });
    
    std::cout << "Home Assistant integration configured" << std::endl << std::endl;

    // Setup MQTT subscriptions for sensors
    // In production, these would parse real MQTT payloads and update sensors
    mqttClient->subscribe("sensor/temperature/indoor", 
        [&](const std::string&, const std::string&) {});
    mqttClient->subscribe("sensor/temperature/outdoor",
        [&](const std::string&, const std::string&) {});
    mqttClient->subscribe("sensor/solar/production",
        [&](const std::string&, const std::string&) {});

    std::cout << "=== Starting simulation ===" << std::endl << std::endl;

    // Demonstrate HA integration with simulated messages
    std::cout << "--- Demonstrating Home Assistant Integration ---" << std::endl;
    std::cout << "Simulating HA sensor data updates via MQTT..." << std::endl << std::endl;
    
    // Simulate receiving temperature from HA
    mqttClient->simulateMessage("homeassistant/state/sensor.living_room_temperature", "21.5");
    mqttClient->simulateMessage("homeassistant/state/sensor.outdoor_temperature", "15.0");
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Publish commands to HA devices
    std::cout << "Sending commands to Home Assistant devices..." << std::endl;
    haIntegration->publishCommand("switch.heater", "ON");
    haIntegration->publishCommandWithData("light.living_room", "ON", "{\"brightness\": 80}");
    
    std::cout << std::endl;

    // Simulation loop
    for (int cycle = 0; cycle < 5; cycle++) {
        std::cout << "--- Cycle " << (cycle + 1) << " ---" << std::endl;

        // Simulate sensor readings changing over time
        if (cycle == 0) {
            // Initial state: cold inside, need heating
            // Simulate HA sending sensor data
            std::cout << "Fetching data from Home Assistant via MQTT..." << std::endl;
            mqttClient->simulateMessage("homeassistant/state/sensor.living_room_temperature", "18.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.outdoor_temperature", "10.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.solar_production", "0.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.energy_consumption", "2.5");
            
            evChargerSensor->setCharging(true, 11.0);
            evChargerSensor->update();
            
            // Send command to HA to turn on heater
            std::cout << "Executing command to HA: Turn on heater" << std::endl;
            haIntegration->publishCommand("switch.heater", "ON");
        } else if (cycle == 1) {
            // Energy cost becomes high
            std::cout << "Fetching data from Home Assistant via MQTT..." << std::endl;
            mqttClient->simulateMessage("homeassistant/state/sensor.living_room_temperature", "19.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.outdoor_temperature", "12.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.solar_production", "1.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.energy_consumption", "3.5");
            
            // Reduce light brightness due to high cost
            std::cout << "Executing command to HA: Dim lights" << std::endl;
            haIntegration->publishCommandWithData("light.living_room", "ON", "{\"brightness\": 30}");
        } else if (cycle == 2) {
            // High solar production
            std::cout << "Fetching data from Home Assistant via MQTT..." << std::endl;
            mqttClient->simulateMessage("homeassistant/state/sensor.living_room_temperature", "21.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.outdoor_temperature", "15.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.solar_production", "8.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.energy_consumption", "4.0");
            
            // Start EV charging with high solar
            std::cout << "Executing command to HA: Start EV charging" << std::endl;
            haIntegration->publishCommandWithData("switch.ev_charger", "ON", "{\"power\": 11.0}");
        } else if (cycle == 3) {
            // Temperature reached target, hot outside
            std::cout << "Fetching data from Home Assistant via MQTT..." << std::endl;
            mqttClient->simulateMessage("homeassistant/state/sensor.living_room_temperature", "25.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.outdoor_temperature", "30.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.solar_production", "5.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.energy_consumption", "2.0");
            
            // Turn off heater and turn on AC
            std::cout << "Executing commands to HA: Turn off heater, turn on AC" << std::endl;
            haIntegration->publishCommand("switch.heater", "OFF");
            haIntegration->publishCommandWithData("climate.ac", "cool", "{\"temperature\": 22.0}");
        } else if (cycle == 4) {
            // Low energy cost, normal conditions
            std::cout << "Fetching data from Home Assistant via MQTT..." << std::endl;
            mqttClient->simulateMessage("homeassistant/state/sensor.living_room_temperature", "22.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.outdoor_temperature", "20.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.solar_production", "3.0");
            mqttClient->simulateMessage("homeassistant/state/sensor.energy_consumption", "1.5");
            
            // Normal light brightness
            std::cout << "Executing command to HA: Normal light brightness" << std::endl;
            haIntegration->publishCommandWithData("light.living_room", "ON", "{\"brightness\": 100}");
        }

        // Update sensor readings (triggers events)
        // Note: Indoor/outdoor temp, solar, and energy are updated via HA MQTT callbacks
        // EV charger sensor is updated directly here as it's not fetched from HA in this demo
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
    std::cout << "✓ Home Assistant MQTT Integration (Bidirectional)" << std::endl;
    std::cout << "✓ Fetch Data from HA via MQTT Subscribe" << std::endl;
    std::cout << "✓ Execute Commands to HA via MQTT Publish" << std::endl;
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
