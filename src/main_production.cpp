/**
 * Production Main File for Home Automation System
 * 
 * Features:
 * - Hot reloadable configuration
 * - Continuous training with historical data
 * - Day-ahead optimization scheduling
 * - Graceful shutdown handling
 * - Real-time sensor monitoring and control
 */

#include "Config.h"
#include "ConfigWebServer.h"
#include "SystemWebService.h"
#include "MQTTClient.h"
#include "HAIntegration.h"
#include "HistoricalDataCollector.h"
#include "MLPredictor.h"
#include "MLTrainingScheduler.h"
#include "DayAheadOptimizer.h"
#include "DeferrableLoadController.h"
#include "EnergyOptimizer.h"
#include "TemperatureSensor.h"
#include "EnergyMeter.h"
#include "SolarSensor.h"
#include "EVChargerSensor.h"
#include "Heater.h"
#include "AirConditioner.h"
#include "Light.h"
#include "EVCharger.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <ctime>
#include <sys/stat.h>

// Global flag for graceful shutdown
std::atomic<bool> g_running(true);

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n\n🛑 Shutdown signal received. Shutting down gracefully..." << std::endl;
        g_running = false;
    } else if (signal == SIGUSR1) {
        // Hot reload signal (will be handled in main loop)
        std::cout << "\n📝 Configuration reload signal received (SIGUSR1)" << std::endl;
    }
}

// Helper to get current time info
void getCurrentTimeInfo(int& hour, int& dayOfWeek) {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    hour = localTime->tm_hour;
    dayOfWeek = localTime->tm_wday;
}

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     Home Automation System - Production Mode              ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // Register signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGUSR1, signalHandler);
    
    std::cout << "ℹ️  Signal handlers registered (SIGINT, SIGTERM, SIGUSR1)" << std::endl;
    std::cout << "   Use Ctrl+C or kill -TERM <pid> to shutdown gracefully" << std::endl;
    std::cout << "   Use kill -USR1 <pid> to reload configuration\n" << std::endl;
    
    // ========== STEP 1: Load Configuration ==========
    std::cout << "📋 [1/7] Loading Configuration..." << std::endl;
    auto config = std::make_shared<Config>();
    
    if (config->loadFromFile("config.json")) {
        std::cout << "   ✓ Configuration loaded from config.json" << std::endl;
    } else {
        std::cout << "   ⚠️  Config file not found, creating default configuration" << std::endl;
        *config = Config::getDefaultConfig();
        config->saveToFile("config.json");
        std::cout << "   ✓ Default configuration saved to config.json" << std::endl;
    }
    
    std::cout << "   → MQTT: " << (config->isMqttEnabled() ? "Enabled" : "Disabled");
    if (config->isMqttEnabled()) {
        std::cout << " (" << config->getMqttBrokerAddress() << ":" << config->getMqttPort() << ")";
    }
    std::cout << std::endl;
    std::cout << "   → Deferrable Loads: " << config->getDeferrableLoadCount() << std::endl;
    std::cout << "   → Sensors: " << config->getSensorValues().size() << "\n" << std::endl;
    
    // ========== STEP 2: Start Web Interfaces ==========
    std::cout << "🌐 [2/8] Starting Web Interfaces..." << std::endl;
    
    // Configuration Web Interface
    std::shared_ptr<ConfigWebServer> configWebServer;
    if (config->isWebInterfaceEnabled()) {
        configWebServer = std::make_shared<ConfigWebServer>(config, config->getWebInterfacePort());
        if (configWebServer->start()) {
            std::cout << "   ✓ Configuration interface: " << configWebServer->getServerUrl() << std::endl;
        } else {
            std::cout << "   ✗ Failed to start configuration interface\n" << std::endl;
        }
    }
    
    // System Monitoring and Control Web Service (will be initialized after components are ready)
    std::shared_ptr<SystemWebService> systemWebService;
    std::cout << "   ℹ️  System web service will start after initialization...\n" << std::endl;
    
    // ========== STEP 3: Initialize MQTT and Home Assistant Integration ==========
    std::cout << "📡 [3/8] Initializing MQTT and Home Assistant Integration..." << std::endl;
    std::shared_ptr<MQTTClient> mqttClient;
    std::shared_ptr<HAIntegration> haIntegration;
    
    if (config->isMqttEnabled()) {
        mqttClient = std::make_shared<MQTTClient>(
            config->getMqttBrokerAddress().c_str(),
            config->getMqttPort()
        );
        
        if (mqttClient->connect()) {
            std::cout << "   ✓ Connected to MQTT broker" << std::endl;
            haIntegration = std::make_shared<HAIntegration>(mqttClient);
            std::cout << "   ✓ Home Assistant integration ready\n" << std::endl;
        } else {
            std::cout << "   ✗ Failed to connect to MQTT broker\n" << std::endl;
        }
    } else {
        std::cout << "   ⚠️  MQTT disabled in configuration\n" << std::endl;
    }
    
    // ========== STEP 4: Create Sensors and Appliances ==========
    std::cout << "🔌 [4/8] Initializing Sensors and Appliances..." << std::endl;
    
    // Create sensors
    auto indoorTempSensor = std::make_shared<TemperatureSensor>(
        "temp_indoor", "Indoor Temperature", TemperatureSensor::Location::INDOOR);
    auto outdoorTempSensor = std::make_shared<TemperatureSensor>(
        "temp_outdoor", "Outdoor Temperature", TemperatureSensor::Location::OUTDOOR);
    auto energyMeter = std::make_shared<EnergyMeter>("energy_meter", "Main Energy Meter");
    auto solarSensor = std::make_shared<SolarSensor>("solar_panel", "Solar Production");
    auto evChargerSensor = std::make_shared<EVChargerSensor>("ev_charger_sensor", "EV Charger Status");
    
    // Create appliances
    auto heater = std::make_shared<Heater>("heater_main", "Main Heater", 2.5);
    auto ac = std::make_shared<AirConditioner>("ac_main", "Main AC", 3.0);
    auto evCharger = std::make_shared<EVCharger>("ev_charger", "EV Charger", 11.0);
    auto decorativeLights = std::make_shared<Light>("lights_decorative", "Decorative Lights", 0.3);
    
    // Mark deferrable loads based on configuration
    auto deferrableLoadNames = config->getDeferrableLoadNames();
    evCharger->setDeferrable(
        std::find(deferrableLoadNames.begin(), deferrableLoadNames.end(), "ev_charger") != deferrableLoadNames.end()
    );
    decorativeLights->setDeferrable(
        std::find(deferrableLoadNames.begin(), deferrableLoadNames.end(), "decorative_lights") != deferrableLoadNames.end()
    );
    
    std::cout << "   ✓ Created 5 sensors" << std::endl;
    std::cout << "   ✓ Created 4 appliances (" << config->getDeferrableLoadCount() << " deferrable)\n" << std::endl;
    
    // ========== STEP 5: Setup Historical Data Collection and ML Training ==========
    std::cout << "🧠 [5/8] Setting up ML Training and Historical Data Collection..." << std::endl;
    
    // Configure historical data collection
    DataCollectionConfig dataConfig;
    dataConfig.maxDaysToRetain = 90;
    dataConfig.enablePersistence = true;
    dataConfig.persistenceFile = "historical_data.csv";
    dataConfig.collectionIntervalMinutes = 60;
    dataConfig.verboseLogging = false;  // Production: minimal logging
    
    auto dataCollector = std::make_shared<HistoricalDataCollector>(dataConfig);
    
    // Load existing historical data
    if (dataCollector->loadFromFile()) {
        std::cout << "   ✓ Loaded " << dataCollector->getDataPointCount() << " historical data points" << std::endl;
    } else {
        std::cout << "   ℹ️  No existing historical data, starting fresh collection" << std::endl;
    }
    
    // Initialize ML predictor
    auto mlPredictor = std::make_shared<MLPredictor>();
    
    // Train initial model if we have sufficient data
    if (dataCollector->getDataPointCount() >= 168) {  // At least 7 days
        std::cout << "   → Training initial ML model..." << std::endl;
        auto historicalData = dataCollector->getAllData();
        mlPredictor->train(historicalData);
        std::cout << "   ✓ Initial ML model trained with " << historicalData.size() << " data points" << std::endl;
    } else {
        std::cout << "   ⚠️  Insufficient data for ML training (need at least 168 points)" << std::endl;
        std::cout << "   → Model will train automatically once sufficient data is collected" << std::endl;
    }
    
    // Setup continuous training scheduler
    TrainingScheduleConfig trainingConfig;
    trainingConfig.retrainingIntervalHours = 24;  // Retrain daily
    trainingConfig.minDataPointsForTraining = 168;
    trainingConfig.autoRetrain = true;
    trainingConfig.verboseLogging = false;  // Production: minimal logging
    
    auto trainingScheduler = std::make_shared<MLTrainingScheduler>(
        mlPredictor, dataCollector, trainingConfig
    );
    
    // Set training callback for notifications
    trainingScheduler->setTrainingCallback([](bool success, size_t dataPoints) {
        if (success) {
            std::cout << "   ✓ ML model retrained successfully with " << dataPoints << " data points" << std::endl;
        } else {
            std::cout << "   ✗ ML model retraining failed" << std::endl;
        }
    });
    
    // Start automatic training
    trainingScheduler->startAutoTraining();
    std::cout << "   ✓ Continuous training scheduler started (retrains every " 
              << trainingConfig.retrainingIntervalHours << " hours)\n" << std::endl;
    
    // ========== STEP 6: Setup Day-Ahead Optimization ==========
    std::cout << "📅 [6/8] Setting up Day-Ahead Optimization..." << std::endl;
    
    auto dayAheadOptimizer = std::make_shared<DayAheadOptimizer>(mlPredictor);
    dayAheadOptimizer->addAppliance(heater);
    dayAheadOptimizer->addAppliance(ac);
    dayAheadOptimizer->addAppliance(evCharger);
    dayAheadOptimizer->setTargetTemperature(21.0);
    dayAheadOptimizer->setEVChargingHoursNeeded(4);
    
    // Setup deferrable load controller
    auto deferrableController = std::make_shared<DeferrableLoadController>(mlPredictor);
    deferrableController->setPriceThreshold(0.15);
    deferrableController->setBusyHourThreshold(0.13);
    deferrableController->addDeferrableLoad(evCharger);
    deferrableController->addDeferrableLoad(decorativeLights);
    
    dayAheadOptimizer->setDeferrableLoadController(deferrableController);
    
    std::cout << "   ✓ Day-ahead optimizer configured" << std::endl;
    std::cout << "   → Target temperature: 21°C" << std::endl;
    std::cout << "   → EV charging hours needed: 4" << std::endl;
    std::cout << "   → Price threshold: $0.15/kWh\n" << std::endl;
    
    // Generate initial schedule
    int currentHour, currentDayOfWeek;
    getCurrentTimeInfo(currentHour, currentDayOfWeek);
    
    std::cout << "   → Generating initial 24-hour schedule..." << std::endl;
    auto schedule = dayAheadOptimizer->generateSchedule(currentHour, currentDayOfWeek);
    std::cout << "   ✓ Schedule generated (Estimated cost: $" << schedule.estimatedCost 
              << ", Consumption: " << schedule.estimatedConsumption << " kWh)\n" << std::endl;
    
    // ========== STEP 7: Start System Monitoring Web Service ==========
    std::cout << "🌐 [7/8] Starting System Monitoring & Control Web Service..." << std::endl;
    
    // Get REST API port from configuration
    int restApiPort = config->isRestApiEnabled() ? config->getRestApiPort() : 8081;
    
    systemWebService = std::make_shared<SystemWebService>(
        config, mlPredictor, dayAheadOptimizer, dataCollector, restApiPort
    );
    
    // Register all sensors and appliances with the web service
    systemWebService->registerSensor(indoorTempSensor);
    systemWebService->registerSensor(outdoorTempSensor);
    systemWebService->registerSensor(energyMeter);
    systemWebService->registerSensor(solarSensor);
    systemWebService->registerSensor(evChargerSensor);
    
    systemWebService->registerAppliance(heater);
    systemWebService->registerAppliance(ac);
    systemWebService->registerAppliance(evCharger);
    systemWebService->registerAppliance(decorativeLights);
    
    // Update initial status
    SystemStatus initialStatus;
    initialStatus.running = true;
    initialStatus.version = "1.0.0";
    initialStatus.uptimeSeconds = 0;
    initialStatus.dataPointsCollected = dataCollector->getDataPointCount();
    initialStatus.lastMLTraining = std::time(nullptr);
    initialStatus.lastScheduleGeneration = std::time(nullptr);
    initialStatus.mqttConnected = (mqttClient && config->isMqttEnabled());
    initialStatus.webServerRunning = (configWebServer && configWebServer->isRunning());
    systemWebService->updateSystemStatus(initialStatus);
    systemWebService->updateSchedule(schedule);
    
    if (systemWebService->start()) {
        std::cout << "   ✓ System web service: " << systemWebService->getServiceUrl() << std::endl;
        std::cout << "   → Dashboard available at: " << systemWebService->getServiceUrl() << "/dashboard" << std::endl;
        std::cout << "   → API endpoints available at: " << systemWebService->getServiceUrl() << "/api/*" << std::endl;
    } else {
        std::cout << "   ✗ Failed to start system web service (port may be in use)" << std::endl;
    }
    std::cout << std::endl;
    
    // ========== STEP 8: Start Main Event Loop ==========
    std::cout << "▶️  [8/8] Starting Main Event Loop..." << std::endl;
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                   System Running                           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // Print summary of available interfaces
    std::cout << "📊 Available Web Interfaces:" << std::endl;
    if (configWebServer && configWebServer->isRunning()) {
        std::cout << "   🔧 Configuration: " << configWebServer->getServerUrl() << std::endl;
        std::cout << "      - Manage deferrable loads" << std::endl;
        std::cout << "      - Configure MQTT settings" << std::endl;
        std::cout << "      - Add/remove sensors" << std::endl;
        std::cout << "      - Hot reload configuration" << std::endl;
    }
    if (systemWebService && systemWebService->isRunning()) {
        std::cout << "   📈 Monitoring Dashboard: " << systemWebService->getServiceUrl() << "/dashboard" << std::endl;
        std::cout << "      - Real-time system status" << std::endl;
        std::cout << "      - Sensor readings" << std::endl;
        std::cout << "      - Appliance status" << std::endl;
        std::cout << "      - Day-ahead schedule" << std::endl;
        std::cout << "   🔌 REST API: " << systemWebService->getServiceUrl() << "/api/" << std::endl;
        std::cout << "      - GET /api/status - System status" << std::endl;
        std::cout << "      - GET /api/sensors - All sensor data" << std::endl;
        std::cout << "      - GET /api/appliances - All appliance status" << std::endl;
        std::cout << "      - GET /api/schedule - Day-ahead schedule" << std::endl;
        std::cout << "      - GET /api/historical?days=7 - Historical data" << std::endl;
        std::cout << "      - GET /api/predictions - ML predictions" << std::endl;
    }
    std::cout << std::endl;
    
    // Tracking variables for scheduled tasks
    auto lastDataCollection = std::chrono::steady_clock::now();
    auto lastScheduleGeneration = std::chrono::steady_clock::now();
    auto lastConfigCheck = std::chrono::steady_clock::now();
    auto lastStatusUpdate = std::chrono::steady_clock::now();
    std::time_t lastConfigModTime = 0;
    auto startTime = std::chrono::steady_clock::now();
    
    const auto dataCollectionInterval = std::chrono::minutes(dataConfig.collectionIntervalMinutes);
    const auto scheduleGenerationInterval = std::chrono::hours(1);  // Regenerate every hour
    const auto configCheckInterval = std::chrono::seconds(30);  // Check config file every 30 seconds
    const auto statusUpdateInterval = std::chrono::seconds(10);  // Update web service status every 10 seconds
    
    int loopCount = 0;
    
    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        loopCount++;
        
        // Update system status for web service
        if (now - lastStatusUpdate >= statusUpdateInterval) {
            lastStatusUpdate = now;
            
            auto uptimeDuration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
            
            SystemStatus status;
            status.running = true;
            status.version = "1.0.0";
            status.uptimeSeconds = uptimeDuration.count();
            status.dataPointsCollected = dataCollector->getDataPointCount();
            status.lastMLTraining = trainingScheduler->getLastTrainingTime().time_since_epoch().count() / 1000000000;
            status.lastScheduleGeneration = std::time(nullptr);
            status.mqttConnected = (mqttClient && config->isMqttEnabled());
            status.webServerRunning = (configWebServer && configWebServer->isRunning());
            
            if (systemWebService) {
                systemWebService->updateSystemStatus(status);
            }
        }
        
        // Hot reload: Check if config file has been modified
        if (now - lastConfigCheck >= configCheckInterval) {
            lastConfigCheck = now;
            
            // Check config file modification time
            struct stat configStat;
            if (stat("config.json", &configStat) == 0) {
                if (lastConfigModTime == 0) {
                    lastConfigModTime = configStat.st_mtime;
                } else if (configStat.st_mtime > lastConfigModTime) {
                    std::cout << "\n🔄 Configuration file changed, reloading..." << std::endl;
                    
                    auto newConfig = std::make_shared<Config>();
                    if (newConfig->loadFromFile("config.json")) {
                        // Apply new configuration
                        *config = *newConfig;
                        lastConfigModTime = configStat.st_mtime;
                        
                        std::cout << "   ✓ Configuration reloaded successfully" << std::endl;
                        std::cout << "   → MQTT: " << (config->isMqttEnabled() ? "Enabled" : "Disabled") << std::endl;
                        std::cout << "   → Deferrable Loads: " << config->getDeferrableLoadCount() << std::endl;
                        std::cout << "   → Sensors: " << config->getSensorValues().size() << "\n" << std::endl;
                        
                        // Note: In a full production system, you would also:
                        // - Restart MQTT connection if broker settings changed
                        // - Update sensor subscriptions if sensor list changed
                        // - Update deferrable load configurations
                    }
                }
            }
        }
        
        // Collect historical data periodically
        if (now - lastDataCollection >= dataCollectionInterval) {
            lastDataCollection = now;
            
            // Simulate reading current sensor values
            double outdoorTemp = outdoorTempSensor->getTemperature();
            double solarProduction = solarSensor->getProduction();
            double energyCost = 0.12;  // In production, fetch from API
            
            dataCollector->recordCurrentState(outdoorTemp, solarProduction, energyCost);
            dataCollector->saveToFile();  // Persist immediately
            
            if (loopCount % 10 == 0) {  // Print every 10th collection
                std::cout << "📊 Data collected: " << dataCollector->getDataPointCount() 
                          << " points (Temp: " << outdoorTemp << "°C, Solar: " 
                          << solarProduction << " kW, Cost: $" << energyCost << "/kWh)" << std::endl;
            }
        }
        
        // Regenerate day-ahead schedule periodically
        if (now - lastScheduleGeneration >= scheduleGenerationInterval) {
            lastScheduleGeneration = now;
            
            getCurrentTimeInfo(currentHour, currentDayOfWeek);
            
            std::cout << "\n📅 Regenerating day-ahead schedule for hour " << currentHour << "..." << std::endl;
            schedule = dayAheadOptimizer->generateSchedule(currentHour, currentDayOfWeek);
            std::cout << "   ✓ Schedule updated (Cost: $" << schedule.estimatedCost 
                      << ", Consumption: " << schedule.estimatedConsumption << " kWh)" << std::endl;
            
            // Update web service with new schedule
            if (systemWebService) {
                systemWebService->updateSchedule(schedule);
            }
            
            // Execute actions for current hour
            auto currentActions = schedule.getActionsForHour(currentHour);
            if (!currentActions.empty()) {
                std::cout << "   → Executing " << currentActions.size() << " actions for current hour:" << std::endl;
                for (const auto& action : currentActions) {
                    std::cout << "      • " << action.applianceId << ": " << action.action 
                              << " - " << action.reason << std::endl;
                }
            }
            std::cout << std::endl;
        }
        
        // Sleep for a short interval (main loop runs every 5 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    // ========== Graceful Shutdown ==========
    std::cout << "\n\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                  Shutting Down...                          ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // Stop training scheduler
    std::cout << "🛑 Stopping continuous training scheduler..." << std::endl;
    trainingScheduler->stopAutoTraining();
    
    // Save final historical data
    std::cout << "💾 Saving historical data..." << std::endl;
    dataCollector->saveToFile();
    std::cout << "   ✓ Saved " << dataCollector->getDataPointCount() << " data points" << std::endl;
    
    // Stop system web service
    if (systemWebService && systemWebService->isRunning()) {
        std::cout << "🌐 Stopping system web service..." << std::endl;
        systemWebService->stop();
    }
    
    // Stop configuration web server
    if (configWebServer && configWebServer->isRunning()) {
        std::cout << "🔧 Stopping configuration interface..." << std::endl;
        configWebServer->stop();
    }
    
    // Disconnect MQTT
    if (mqttClient) {
        std::cout << "📡 Disconnecting from MQTT broker..." << std::endl;
        mqttClient->disconnect();
    }
    
    std::cout << "\n✅ Shutdown complete. Goodbye!\n" << std::endl;
    
    return 0;
}
