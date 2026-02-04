cmake_minimum_required(VERSION 3.10)
project(HomeAutomation VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

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
    src/MQTTClient.cpp
    src/HTTPClient.cpp
    src/EnergyOptimizer.cpp
    src/MLPredictor.cpp
    src/DayAheadOptimizer.cpp
    src/HistoricalDataGenerator.cpp
    src/HAIntegration.cpp
)

# Enable warnings
if(MSVC)
    target_compile_options(home_automation PRIVATE /W4)
else()
    target_compile_options(home_automation PRIVATE -Wall -Wextra -pedantic)
endif()

# Installation rules
install(TARGETS home_automation DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/home_automation)
