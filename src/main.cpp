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
#include <sstream>
#include <iomanip>

// Helper function to create sensor attributes JSON
// Uses HAIntegration::escapeJsonString for proper escaping
std::string createSensorAttributes(const std::string& name, const std::string& unit, const std::string& deviceClass) {
    std::ostringstream attrs;
    attrs << "{\"unit_of_measurement\": \"" << HAIntegration::escapeJsonString(unit) << "\", "
          << "\"friendly_name\": \"" << HAIntegration::escapeJsonString(name) << "\", "
          << "\"device_class\": \"" << HAIntegration::escapeJsonString(deviceClass) << "\"}";
    return attrs.str();
}

int main() {
    std::cout << "\n=== Home Assistant Sensor State Publishing Demo ===" << std::endl;
    std::cout << "This demonstrates automatic publishing of ALL local sensor states to MQTT/Home Assistant\n" << std::endl;
    
    // Setup MQTT and HA Integration
    auto mqttClient = std::make_shared<MQTTClient>("10.0.0.59", 1883);
    mqttClient->connect();
    
    auto haIntegration = std::make_shared<HAIntegration>(mqttClient);
    
    std::cout << "=== Step 1: Creating Local Sensors ===" << std::endl;
    
    // Create multiple local sensors
    auto indoorTempSensor = std::make_shared<TemperatureSensor>(
        "temp_indoor_1", "Living Room Temperature", TemperatureSensor::Location::INDOOR);
    
    auto outdoorTempSensor = std::make_shared<TemperatureSensor>(
        "temp_outdoor_1", "Outdoor Temperature", TemperatureSensor::Location::OUTDOOR);
    
    auto energyMeter = std::make_shared<EnergyMeter>(
        "energy_meter_1", "Main Energy Meter");
    
    auto solarSensor = std::make_shared<SolarSensor>(
        "solar_1", "Solar Production");
    
    auto evChargerSensor = std::make_shared<EVChargerSensor>(
        "ev_charger_1", "EV Charger Status");
    
    std::cout << "Created 5 local sensors\n" << std::endl;
    
    std::cout << "=== Step 2: Publishing Discovery Configs to HA ===" << std::endl;
    
    // Publish discovery configs for all sensors so they appear in HA automatically
    haIntegration->publishDiscovery("sensor", "home_automation", "local_temp_indoor",
        "{\"name\": \"Local Indoor Temperature\", \"state_topic\": \"homeassistant/state/sensor.local_temp_indoor\", "
        "\"unit_of_measurement\": \"°C\", \"device_class\": \"temperature\"}");
    
    haIntegration->publishDiscovery("sensor", "home_automation", "local_temp_outdoor",
        "{\"name\": \"Local Outdoor Temperature\", \"state_topic\": \"homeassistant/state/sensor.local_temp_outdoor\", "
        "\"unit_of_measurement\": \"°C\", \"device_class\": \"temperature\"}");
    
    haIntegration->publishDiscovery("sensor", "home_automation", "local_energy_consumption",
        "{\"name\": \"Local Energy Consumption\", \"state_topic\": \"homeassistant/state/sensor.local_energy_consumption\", "
        "\"unit_of_measurement\": \"kW\", \"device_class\": \"power\"}");
    
    haIntegration->publishDiscovery("sensor", "home_automation", "local_solar_production",
        "{\"name\": \"Local Solar Production\", \"state_topic\": \"homeassistant/state/sensor.local_solar_production\", "
        "\"unit_of_measurement\": \"kW\", \"device_class\": \"power\"}");
    
    haIntegration->publishDiscovery("sensor", "home_automation", "local_ev_charger_power",
        "{\"name\": \"Local EV Charger Power\", \"state_topic\": \"homeassistant/state/sensor.local_ev_charger_power\", "
        "\"unit_of_measurement\": \"kW\", \"device_class\": \"power\"}");
    
    std::cout << "Published 5 discovery configs\n" << std::endl;
    
    std::cout << "=== Step 3: Setting Initial Sensor Values ===" << std::endl;
    
    // Set initial sensor values
    indoorTempSensor->setTemperature(22.5);
    outdoorTempSensor->setTemperature(15.0);
    energyMeter->setConsumption(3.5);
    solarSensor->setProduction(5.2);
    evChargerSensor->setCharging(true, 11.0);
    
    std::cout << "Set initial values for all sensors\n" << std::endl;
    
    std::cout << "=== Step 4: Publishing ALL Sensor States to MQTT ===" << std::endl;
    
    // Publish all sensor states to MQTT/HA
    // This shows how to publish sensor data WITH attributes for richer information
    
    // Indoor Temperature with attributes
    std::string indoorAttrs = createSensorAttributes(indoorTempSensor->getName(), "°C", "temperature");
    haIntegration->publishState("sensor.local_temp_indoor", 
                                std::to_string(indoorTempSensor->getTemperature()),
                                indoorAttrs);
    
    // Outdoor Temperature with attributes
    std::string outdoorAttrs = createSensorAttributes(outdoorTempSensor->getName(), "°C", "temperature");
    haIntegration->publishState("sensor.local_temp_outdoor", 
                                std::to_string(outdoorTempSensor->getTemperature()),
                                outdoorAttrs);
    
    // Energy Consumption with attributes
    std::string energyAttrs = createSensorAttributes(energyMeter->getName(), "kW", "power");
    haIntegration->publishState("sensor.local_energy_consumption", 
                                std::to_string(energyMeter->getConsumption()),
                                energyAttrs);
    
    // Solar Production with attributes
    std::string solarAttrs = createSensorAttributes(solarSensor->getName(), "kW", "power");
    haIntegration->publishState("sensor.local_solar_production", 
                                std::to_string(solarSensor->getProduction()),
                                solarAttrs);
    
    // EV Charger Power with attributes and additional info
    std::ostringstream evAttrs;
    evAttrs << "{\"unit_of_measurement\": \"" << HAIntegration::escapeJsonString("kW") << "\", "
            << "\"friendly_name\": \"" << HAIntegration::escapeJsonString(evChargerSensor->getName()) << "\", "
            << "\"device_class\": \"" << HAIntegration::escapeJsonString("power") << "\", "
            << "\"charging\": " << (evChargerSensor->isCharging() ? "true" : "false") << "}";
    haIntegration->publishState("sensor.local_ev_charger_power", 
                                std::to_string(evChargerSensor->getChargePower()),
                                evAttrs.str());
    
    std::cout << "\nAll sensor states published!" << std::endl;
    std::cout << "  - Indoor Temperature: " << indoorTempSensor->getTemperature() << " °C" << std::endl;
    std::cout << "  - Outdoor Temperature: " << outdoorTempSensor->getTemperature() << " °C" << std::endl;
    std::cout << "  - Energy Consumption: " << energyMeter->getConsumption() << " kW" << std::endl;
    std::cout << "  - Solar Production: " << solarSensor->getProduction() << " kW" << std::endl;
    std::cout << "  - EV Charger Power: " << evChargerSensor->getChargePower() << " kW" << std::endl;
    
    std::cout << "\n=== Step 5: Automatic Updates - Publishing Changes ===" << std::endl;
    std::cout << "Simulating sensor updates and automatically publishing to MQTT...\n" << std::endl;
    
    // Simulate automatic updates every 5 seconds
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        std::cout << "\n--- Update #" << (i+1) << " ---" << std::endl;
        
        // Update sensor values
        double newIndoorTemp = 22.5 + (i * 0.5);
        double newOutdoorTemp = 15.0 - (i * 0.3);
        double newEnergy = 3.5 + (i * 0.2);
        double newSolar = 5.2 + (i * 0.5);
        
        indoorTempSensor->setTemperature(newIndoorTemp);
        outdoorTempSensor->setTemperature(newOutdoorTemp);
        energyMeter->setConsumption(newEnergy);
        solarSensor->setProduction(newSolar);
        
        // Publish updated states to MQTT/HA
        haIntegration->publishState("sensor.local_temp_indoor", 
                                    std::to_string(newIndoorTemp),
                                    indoorAttrs);
        
        haIntegration->publishState("sensor.local_temp_outdoor", 
                                    std::to_string(newOutdoorTemp),
                                    outdoorAttrs);
        
        haIntegration->publishState("sensor.local_energy_consumption", 
                                    std::to_string(newEnergy),
                                    energyAttrs);
        
        haIntegration->publishState("sensor.local_solar_production", 
                                    std::to_string(newSolar),
                                    solarAttrs);
        
        std::cout << "Published updated states:" << std::endl;
        std::cout << "  - Indoor Temperature: " << newIndoorTemp << " °C" << std::endl;
        std::cout << "  - Outdoor Temperature: " << newOutdoorTemp << " °C" << std::endl;
        std::cout << "  - Energy Consumption: " << newEnergy << " kW" << std::endl;
        std::cout << "  - Solar Production: " << newSolar << " kW" << std::endl;
    }
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
    std::cout << "\nThis demonstrates how to:" << std::endl;
    std::cout << "  1. Create local sensors" << std::endl;
    std::cout << "  2. Publish discovery configs to HA (sensors auto-appear in HA)" << std::endl;
    std::cout << "  3. Publish ALL sensor states to MQTT with attributes" << std::endl;
    std::cout << "  4. Automatically publish sensor updates" << std::endl;
    std::cout << "\nAll sensor states are now available in Home Assistant via MQTT!" << std::endl;
    
    return 0;
}