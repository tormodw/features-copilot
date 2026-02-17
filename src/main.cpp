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
#include "HARestClient.h"
#include "DeferrableLoadController.h"
#include "Config.h"
#include "ConfigWebServer.h"
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
    std::cout << "\n=== Home Automation System with Configuration ===" << std::endl;
    std::cout << "This demonstrates the complete home automation system with configuration management\n" << std::endl;
    
    // Load or create configuration
    auto config = std::make_shared<Config>();
    std::cout << "=== Loading Configuration ===" << std::endl;
    if (config->loadFromFile("config.json")) {
        std::cout << "✓ Configuration loaded from config.json" << std::endl;
    } else {
        std::cout << "⚠ Config file not found, using defaults" << std::endl;
        *config = Config::getDefaultConfig();
        config->saveToFile("config.json");
        std::cout << "✓ Default configuration saved to config.json" << std::endl;
    }
    
    std::cout << "  MQTT Enabled: " << (config->isMqttEnabled() ? "Yes" : "No") << std::endl;
    std::cout << "  MQTT Broker: " << config->getMqttBrokerAddress() << ":" << config->getMqttPort() << std::endl;
    std::cout << "  Deferrable Loads: " << config->getDeferrableLoadCount() << std::endl;
    std::cout << "  Sensors: " << config->getSensorValues().size() << std::endl;
    std::cout << std::endl;
    
    // Start web interface for runtime configuration
    std::shared_ptr<ConfigWebServer> webServer;
    if (config->isWebInterfaceEnabled()) {
        std::cout << "=== Starting Configuration Web Interface ===" << std::endl;
        webServer = std::make_shared<ConfigWebServer>(config, config->getWebInterfacePort());
        if (webServer->start()) {
            std::cout << "✓ Web interface started at: " << webServer->getServerUrl() << std::endl;
            std::cout << "  You can configure the system at any time via the web interface" << std::endl;
        } else {
            std::cout << "⚠ Failed to start web interface (port may be in use)" << std::endl;
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n=== Home Assistant Sensor State Publishing Demo ===" << std::endl;
    std::cout << "This demonstrates automatic publishing of ALL local sensor states to MQTT/Home Assistant\n" << std::endl;
    
    // Setup MQTT and HA Integration (only if MQTT is enabled in config)
    std::shared_ptr<MQTTClient> mqttClient;
    std::shared_ptr<HAIntegration> haIntegration;
    
    if (config->isMqttEnabled()) {
        std::cout << "MQTT is enabled in configuration" << std::endl;
        mqttClient = std::make_shared<MQTTClient>(
            config->getMqttBrokerAddress().c_str(), 
            config->getMqttPort()
        );
        mqttClient->connect();
        haIntegration = std::make_shared<HAIntegration>(mqttClient);
    } else {
        std::cout << "⚠ MQTT is disabled in configuration - skipping MQTT integration" << std::endl;
        std::cout << "  Enable MQTT via the web interface at " << webServer->getServerUrl() << std::endl;
        // Use mock client for demo purposes when MQTT is disabled
        mqttClient = std::make_shared<MQTTClient>("localhost", 1883);
        mqttClient->connect();
        haIntegration = std::make_shared<HAIntegration>(mqttClient);
    }
    
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
    
    // ==================== REST API DEMONSTRATION ====================
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "=== Home Assistant REST API Demo ===" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nThis demonstrates extracting sensor data from Home Assistant using REST API" << std::endl;
    std::cout << "See HA_REST_API_GUIDE.md for detailed documentation and usage examples\n" << std::endl;
    
    // Create REST API client
    // For production: Use environment variables for credentials
    // Set HA_URL and HA_TOKEN environment variables before running
    const char* haUrlEnv = std::getenv("HA_URL");
    const char* haTokenEnv = std::getenv("HA_TOKEN");
    
    std::string haUrl = haUrlEnv ? haUrlEnv : "http://192.168.1.100:8123";
    std::string haToken = haTokenEnv ? haTokenEnv : "DEMO_MODE_NO_REAL_TOKEN";
    
    if (!haTokenEnv) {
        std::cout << "\n⚠️  Note: Using demo mode without real Home Assistant credentials." << std::endl;
        std::cout << "   For production, set environment variables:" << std::endl;
        std::cout << "   export HA_URL='http://your-ha-ip:8123'" << std::endl;
        std::cout << "   export HA_TOKEN='your_long_lived_access_token'\n" << std::endl;
    }
    
    auto haRestClient = std::make_shared<HARestClient>(haUrl, haToken);
    
    std::cout << "=== Step 1: Testing Connection ===" << std::endl;
    bool connected = haRestClient->testConnection();
    if (connected) {
        std::cout << "✓ Successfully connected to Home Assistant REST API\n" << std::endl;
    } else {
        std::cout << "✗ Failed to connect to Home Assistant REST API" << std::endl;
        std::cout << "  (This is expected in simulation mode)\n" << std::endl;
    }
    
    std::cout << "=== Step 2: Getting Single Sensor State ===" << std::endl;
    std::cout << "Fetching living room temperature sensor...\n" << std::endl;
    
    HASensorData tempData = haRestClient->getSensorState("sensor.shellyhtg3_e4b3232d5348_temperature");
    std::cout << "Sensor Data Retrieved:" << std::endl;
    std::cout << "  Entity ID: " << tempData.entityId << std::endl;
    std::cout << "  State: " << tempData.state << " " << tempData.unitOfMeasurement << std::endl;
    std::cout << "  Friendly Name: " << tempData.friendlyName << std::endl;
    std::cout << "  Device Class: " << tempData.deviceClass << "\n" << std::endl;
    
    std::cout << "=== Step 3: Getting All Sensors ===" << std::endl;
    std::cout << "Fetching all sensors from Home Assistant...\n" << std::endl;
    
    auto allSensors = haRestClient->getAllSensors();
    std::cout << "Found " << allSensors.size() << " sensors:" << std::endl;
    for (const auto& sensor : allSensors) {
        std::cout << "  - " << sensor.entityId << ": " 
                  << sensor.state << " " << sensor.unitOfMeasurement << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== Step 4: Getting Historical Data ===" << std::endl;
    std::cout << "Fetching 24-hour history for energy consumption...\n" << std::endl;
    
    // Get data from 24 hours ago
    long startTime = std::time(nullptr) - (24 * 3600);
    auto history = haRestClient->getHistory("sensor.eva_meter_reader_summation_delivered", startTime);
    
    std::cout << "Historical Data Points: " << history.size() << std::endl;
    if (!history.empty()) {
        std::cout << "Sample data points:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(3), history.size()); i++) {
            std::cout << "  - " << history[i].entityId << ": " 
                      << history[i].state << std::endl;
        }
    }
    std::cout << std::endl;
    
    std::cout << "=== Step 5: Calling a Service (Control a Device) ===" << std::endl;
    std::cout << "Turning on the heater switch...\n" << std::endl;
    
    bool serviceSuccess = haRestClient->callService("switch", "turn_on", "switch.heater");
    if (serviceSuccess) {
        std::cout << "✓ Service call successful - Heater turned on" << std::endl;
    } else {
        std::cout << "✗ Service call failed" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== Step 6: Advanced Service Call with Data ===" << std::endl;
    std::cout << "Turning on living room light with brightness...\n" << std::endl;
    
    std::string lightData = "\"brightness\": 200";
    bool lightSuccess = haRestClient->callService("light", "turn_on", "light.living_room", lightData);
    if (lightSuccess) {
        std::cout << "✓ Light turned on with brightness 200" << std::endl;
    } else {
        std::cout << "✗ Failed to control light" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== REST API Demo Summary ===" << std::endl;
    std::cout << "\nThis demonstration showed how to:" << std::endl;
    std::cout << "  1. ✓ Connect to Home Assistant REST API" << std::endl;
    std::cout << "  2. ✓ Extract single sensor data (temperature)" << std::endl;
    std::cout << "  3. ✓ Get all sensors at once" << std::endl;
    std::cout << "  4. ✓ Retrieve historical data (24-hour history)" << std::endl;
    std::cout << "  5. ✓ Control devices (turn on switch)" << std::endl;
    std::cout << "  6. ✓ Send complex commands with data (light brightness)" << std::endl;
    
    std::cout << "\n📚 For complete documentation and production examples, see:" << std::endl;
    std::cout << "   - HA_REST_API_GUIDE.md (Comprehensive REST API guide)" << std::endl;
    std::cout << "   - HA_MQTT_INTEGRATION.md (Real-time MQTT integration)" << std::endl;
    
    std::cout << "\n💡 Production Deployment Notes:" << std::endl;
    std::cout << "   - Replace mock HTTP client with libcurl or cpp-httplib" << std::endl;
    std::cout << "   - Store access token in environment variables" << std::endl;
    std::cout << "   - Use HTTPS for secure communication" << std::endl;
    std::cout << "   - Implement proper JSON parsing (nlohmann/json)" << std::endl;
    std::cout << "   - Add error handling and retry logic" << std::endl;
    std::cout << "   - Consider using MQTT for real-time updates" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "=== All Demos Complete ===" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // ==================== DEFERRABLE LOAD CONTROL DEMONSTRATION ====================
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "=== Deferrable Load Control Demo ===" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nDemonstrating control of deferrable loads during busy hours" << std::endl;
    std::cout << "and when energy prices exceed threshold\n" << std::endl;
    
    // Setup ML predictor and historical data
    std::cout << "=== Step 1: Training ML Model ===" << std::endl;
    auto mlPredictor = std::make_shared<MLPredictor>();
    auto historicalData = HistoricalDataGenerator::generateSampleData(
        DeferrableLoadDefaults::DEFAULT_TRAINING_DATA_DAYS);
    mlPredictor->train(historicalData);
    std::cout << "ML model trained with " << historicalData.size() << " data points\n" << std::endl;
    
    // Create deferrable load controller
    std::cout << "=== Step 2: Setting Up Deferrable Load Controller ===" << std::endl;
    auto deferrableController = std::make_shared<DeferrableLoadController>(mlPredictor);
    
    // Configure thresholds
    deferrableController->setPriceThreshold(0.15);     // $0.15/kWh - switch off above this
    deferrableController->setBusyHourThreshold(0.13);  // $0.13/kWh - identify busy hours
    std::cout << "Price threshold: $0.15/kWh (switch off deferrable loads above)" << std::endl;
    std::cout << "Busy hour threshold: $0.13/kWh\n" << std::endl;
    
    // Create appliances and mark some as deferrable
    std::cout << "=== Step 3: Creating Appliances ===" << std::endl;
    auto heater = std::make_shared<Heater>("heater_1", "Living Room Heater", 2.5);
    auto ac = std::make_shared<AirConditioner>("ac_1", "Living Room AC", 3.0);
    auto evCharger = std::make_shared<EVCharger>("ev_1", "EV Charger", 11.0);
    auto light1 = std::make_shared<Light>("light_1", "Decorative Lights", 0.3);
    auto light2 = std::make_shared<Light>("light_2", "Essential Lights", 0.2);
    
    // Mark deferrable loads
    heater->setDeferrable(false);     // Critical for comfort
    ac->setDeferrable(false);         // Critical for comfort
    evCharger->setDeferrable(true);   // Can be deferred
    light1->setDeferrable(true);      // Decorative, can be deferred
    light2->setDeferrable(false);     // Essential lighting
    
    // Note: In production, deferrable status would be determined by the configuration
    // Example integration with Config class:
    // auto deferrableLoadNames = config->getDeferrableLoadNames();
    // evCharger->setDeferrable(std::find(deferrableLoadNames.begin(), deferrableLoadNames.end(), "ev_charger") != deferrableLoadNames.end());
    // light1->setDeferrable(std::find(deferrableLoadNames.begin(), deferrableLoadNames.end(), "decorative_lights") != deferrableLoadNames.end());
    
    std::cout << "Created 5 appliances:" << std::endl;
    std::cout << "  - " << heater->getName() << " (NOT deferrable - critical)" << std::endl;
    std::cout << "  - " << ac->getName() << " (NOT deferrable - critical)" << std::endl;
    std::cout << "  - " << evCharger->getName() << " (DEFERRABLE)" << std::endl;
    std::cout << "  - " << light1->getName() << " (DEFERRABLE)" << std::endl;
    std::cout << "  - " << light2->getName() << " (NOT deferrable - essential)" << std::endl;
    std::cout << "  Configuration defines " << config->getDeferrableLoadCount() 
              << " deferrable loads: ";
    for (const auto& loadName : config->getDeferrableLoadNames()) {
        std::cout << loadName << " ";
    }
    std::cout << "\n" << std::endl;
    
    // Add deferrable loads to controller
    deferrableController->addDeferrableLoad(evCharger);
    deferrableController->addDeferrableLoad(light1);
    
    // Turn on all appliances initially
    heater->turnOn();
    ac->turnOn();
    evCharger->turnOn();
    light1->turnOn();
    light2->turnOn();
    
    std::cout << "\n=== Step 4: Analyzing Busy Hours from Historical Data ===" << std::endl;
    auto busyHourAnalysis = deferrableController->analyzeBusyHours(historicalData);
    
    std::cout << "\nBusy Hours: ";
    for (int hour : busyHourAnalysis.busyHours) {
        std::cout << hour << ":00 ";
    }
    std::cout << std::endl;
    
    std::cout << "Optimal Hours: ";
    for (int hour : busyHourAnalysis.optimalHours) {
        std::cout << hour << ":00 ";
    }
    std::cout << "\n" << std::endl;
    
    // Test price-based control
    std::cout << "=== Step 5: Testing Price-Based Control ===" << std::endl;
    
    std::cout << "\nScenario 1: Low price period ($0.10/kWh)" << std::endl;
    deferrableController->controlLoadsByPrice(0.10);
    std::cout << "  EV Charger status: " << (evCharger->isOn() ? "ON" : "OFF") << std::endl;
    std::cout << "  Decorative Lights status: " << (light1->isOn() ? "ON" : "OFF") << std::endl;
    
    std::cout << "\nScenario 2: High price period ($0.18/kWh)" << std::endl;
    deferrableController->controlLoadsByPrice(0.18);
    std::cout << "  EV Charger status: " << (evCharger->isOn() ? "ON" : "OFF") << std::endl;
    std::cout << "  Decorative Lights status: " << (light1->isOn() ? "ON" : "OFF") << std::endl;
    std::cout << "  Essential Lights status: " << (light2->isOn() ? "ON" : "OFF") << " (not affected - not deferrable)" << std::endl;
    std::cout << "  Heater status: " << (heater->isOn() ? "ON" : "OFF") << " (not affected - not deferrable)" << std::endl;
    
    // Day-ahead recommendations
    std::cout << "\n=== Step 6: Day-Ahead Recommendations ===" << std::endl;
    int currentHour = 8;  // 8 AM
    int currentDayOfWeek = 2;  // Tuesday
    
    auto recommendations = deferrableController->getDayAheadRecommendations(currentHour, currentDayOfWeek);
    
    std::cout << "\nSample recommendations for key hours:" << std::endl;
    for (int hour : {8, 12, 18, 22}) {
        if (recommendations.find(hour) != recommendations.end()) {
            std::cout << "\nHour " << hour << ":00" << std::endl;
            for (const auto& rec : recommendations[hour]) {
                std::cout << "  - " << rec << std::endl;
            }
        }
    }
    
    // Integration with DayAheadOptimizer
    std::cout << "\n=== Step 7: Integration with Day-Ahead Optimizer ===" << std::endl;
    auto dayAheadOptimizer = std::make_shared<DayAheadOptimizer>(mlPredictor);
    dayAheadOptimizer->setDeferrableLoadController(deferrableController);
    dayAheadOptimizer->addAppliance(heater);
    dayAheadOptimizer->addAppliance(ac);
    dayAheadOptimizer->addAppliance(evCharger);
    
    auto schedule = dayAheadOptimizer->generateSchedule(currentHour, currentDayOfWeek);
    
    std::cout << "\n=== Generated Day-Ahead Schedule (with Deferrable Load Control) ===" << std::endl;
    std::cout << "Total estimated cost: $" << schedule.estimatedCost << std::endl;
    std::cout << "Total estimated consumption: " << schedule.estimatedConsumption << " kWh\n" << std::endl;
    
    // Show sample hours from schedule
    std::cout << "Sample schedule for key hours:" << std::endl;
    for (int hour : {8, 12, 18, 22}) {
        auto actions = schedule.getActionsForHour(hour);
        if (!actions.empty()) {
            std::cout << "\nHour " << hour << ":00" << std::endl;
            for (const auto& action : actions) {
                std::cout << "  - " << action.applianceId << ": " << action.action;
                if (action.value != 0.0) {
                    std::cout << " (" << action.value << ")";
                }
                std::cout << " - " << action.reason << std::endl;
            }
        }
    }
    
    std::cout << "\n=== Deferrable Load Control Demo Summary ===" << std::endl;
    std::cout << "\nThis demonstration showed how to:" << std::endl;
    std::cout << "  1. ✓ Mark appliances as deferrable or non-deferrable" << std::endl;
    std::cout << "  2. ✓ Analyze historical data to identify busy hours" << std::endl;
    std::cout << "  3. ✓ Switch off deferrable loads when price exceeds threshold" << std::endl;
    std::cout << "  4. ✓ Resume deferrable loads when price drops" << std::endl;
    std::cout << "  5. ✓ Generate day-ahead recommendations for deferrable loads" << std::endl;
    std::cout << "  6. ✓ Integrate with day-ahead optimizer for complete scheduling" << std::endl;
    
    std::cout << "\n💡 Key Benefits:" << std::endl;
    std::cout << "   - Automatic load shedding during high-price periods" << std::endl;
    std::cout << "   - Protection of critical loads (heating, cooling, essential lighting)" << std::endl;
    std::cout << "   - Historical data analysis for pattern recognition" << std::endl;
    std::cout << "   - Day-ahead planning for optimal energy usage" << std::endl;
    std::cout << "   - Significant cost savings without compromising comfort" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "=== All Demonstrations Complete ===" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // ==================== CONFIGURATION SYSTEM SUMMARY ====================
    std::cout << "\n========================================" << std::endl;
    std::cout << "=== Configuration System ===" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::cout << "The system is now running with the following configuration:\n" << std::endl;
    
    std::cout << "📋 Current Configuration:" << std::endl;
    std::cout << "  • MQTT: " << (config->isMqttEnabled() ? "Enabled" : "Disabled") << std::endl;
    if (config->isMqttEnabled()) {
        std::cout << "    - Broker: " << config->getMqttBrokerAddress() << ":" << config->getMqttPort() << std::endl;
    }
    std::cout << "  • Deferrable Loads (" << config->getDeferrableLoadCount() << " configured):" << std::endl;
    for (const auto& load : config->getDeferrableLoadNames()) {
        std::cout << "    - " << load << std::endl;
    }
    std::cout << "  • Sensors (" << config->getSensorValues().size() << " configured):" << std::endl;
    for (const auto& sensor : config->getSensorValues()) {
        std::cout << "    - " << sensor << std::endl;
    }
    
    if (webServer && webServer->isRunning()) {
        std::cout << "\n🌐 Web Configuration Interface:" << std::endl;
        std::cout << "  • URL: " << webServer->getServerUrl() << std::endl;
        std::cout << "  • Features:" << std::endl;
        std::cout << "    - Real-time configuration updates" << std::endl;
        std::cout << "    - Add/remove deferrable loads" << std::endl;
        std::cout << "    - Add/remove sensors" << std::endl;
        std::cout << "    - Configure MQTT settings" << std::endl;
        std::cout << "    - All changes automatically saved to config.json" << std::endl;
        std::cout << "\n  📖 Documentation: See CONFIG_SYSTEM.md for complete guide" << std::endl;
    }
    
    std::cout << "\n✅ System Ready!" << std::endl;
    std::cout << "   All features are operational and can be configured via the web interface." << std::endl;
    std::cout << "\n========================================\n" << std::endl;
    
    return 0;
}