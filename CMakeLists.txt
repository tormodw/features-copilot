cmake_minimum_required(VERSION 3.10)
project(HomeAutomation VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add production executable
add_executable(home_automation_production
    src/main_production.cpp
    src/Appliance.cpp
    src/Light.cpp
    src/Heater.cpp
    src/AirConditioner.cpp
    src/Curtain.cpp
    src/EVCharger.cpp
    src/Sensor.cpp
    src/TemperatureSensor.cpp
    src/EnergyMeter.cpp
    src/SolarSensor.cpp
    src/EVChargerSensor.cpp
    src/Event.cpp
    src/EventManager.cpp
    src/RestApiClient.cpp
    src/HTTPClient.cpp
    src/EnergyOptimizer.cpp
    src/MLPredictor.cpp
    src/DayAheadOptimizer.cpp
    src/HistoricalDataGenerator.cpp
    src/HAIntegration.cpp
    src/DeferrableLoadController.cpp
    src/HistoricalDataCollector.cpp
    src/MLTrainingScheduler.cpp
    src/Config.cpp
    src/ConfigWebServer.cpp
    src/SystemWebService.cpp
)

# Add executable
add_executable(home_automation
    src/main.cpp
    src/Appliance.cpp
    src/Light.cpp
    src/Heater.cpp
    src/AirConditioner.cpp
    src/Curtain.cpp
    src/EVCharger.cpp
    src/Sensor.cpp
    src/TemperatureSensor.cpp
    src/EnergyMeter.cpp
    src/SolarSensor.cpp
    src/EVChargerSensor.cpp
    src/Event.cpp
    src/EventManager.cpp
    src/RestApiClient.cpp
    src/HTTPClient.cpp
    src/EnergyOptimizer.cpp
    src/MLPredictor.cpp
    src/DayAheadOptimizer.cpp
    src/HistoricalDataGenerator.cpp
    src/HAIntegration.cpp
    src/HARestClient.cpp
    src/DeferrableLoadController.cpp
    src/HistoricalDataCollector.cpp
    src/MLTrainingScheduler.cpp
    src/Config.cpp
    src/ConfigWebServer.cpp
)

# Add test executable for deferrable loads
add_executable(test_deferrable_loads
    src/test_deferrable_loads.cpp
    src/Appliance.cpp
    src/Light.cpp
    src/Heater.cpp
    src/AirConditioner.cpp
    src/EVCharger.cpp
    src/MLPredictor.cpp
    src/DayAheadOptimizer.cpp
    src/HistoricalDataGenerator.cpp
    src/DeferrableLoadController.cpp
)

# Add test executable for continuous ML training
add_executable(test_continuous_training
    src/test_continuous_training.cpp
    src/MLPredictor.cpp
    src/HistoricalDataGenerator.cpp
    src/HistoricalDataCollector.cpp
    src/MLTrainingScheduler.cpp
)

# Add test executable for configuration system
add_executable(test_config
    src/test_config.cpp
    src/Config.cpp
    src/ConfigWebServer.cpp
)

# Link pthread for test_config
target_link_libraries(test_config Threads::Threads)

# Find mosquitto library (optional - using mock implementation)
find_library(MOSQUITTO_LIB mosquitto)

# Include directories
target_include_directories(home_automation PRIVATE include)
target_include_directories(home_automation_production PRIVATE include)

# Find and link libcurl (required for REST API)
find_library(CURL_LIB curl)
if(CURL_LIB)
    target_link_libraries(home_automation ${CURL_LIB})
    target_link_libraries(home_automation_production ${CURL_LIB})
    message(STATUS "Curl library found: ${CURL_LIB}")
else()
    message(FATAL_ERROR "Curl library not found - required for REST API client")
endif()

# Link pthread for web server threading
find_package(Threads REQUIRED)
target_link_libraries(home_automation Threads::Threads)
target_link_libraries(home_automation_production Threads::Threads)

# Enable warnings
if(MSVC)
    target_compile_options(home_automation PRIVATE /W4)
else()
    target_compile_options(home_automation PRIVATE -Wall -Wextra -pedantic)
endif()

# Installation rules
install(TARGETS home_automation home_automation_production DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/home_automation)
