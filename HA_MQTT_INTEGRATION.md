# Home Assistant MQTT Integration Guide

This guide explains how to integrate the Home Automation System with Home Assistant using MQTT for bidirectional communication.

## Overview

The Home Assistant MQTT Integration (`HAIntegration` class) enables:
- **Fetching data FROM Home Assistant** via MQTT subscribe (sensor states, device status)
- **Executing commands TO Home Assistant** via MQTT publish (control switches, lights, climate devices)
- Support for Home Assistant's MQTT discovery protocol
- Bidirectional communication for real-time monitoring and control

## Key Features

### Data Fetching (Subscribe)
- Subscribe to individual HA entity state updates
- Subscribe to all entities in a domain (e.g., all sensors)
- Automatic parsing of HA state messages
- Real-time data synchronization with local sensors

### Command Execution (Publish)
- Send simple commands (ON/OFF for switches)
- Send commands with data (brightness for lights, temperature for climate)
- Control any HA device via MQTT
- Automatic topic generation following HA conventions

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│              Home Automation System                     │
│                                                          │
│  ┌──────────────┐         ┌─────────────────┐         │
│  │ HAIntegration│◄────────┤   MQTTClient    │         │
│  └──────┬───────┘         └─────────────────┘         │
│         │                                               │
│         │ Subscribe/Publish                             │
└─────────┼───────────────────────────────────────────────┘
          │
          │ MQTT Broker (e.g., Mosquitto)
          │
┌─────────▼───────────────────────────────────────────────┐
│              Home Assistant                              │
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │   Sensors    │  │   Switches   │  │    Lights    │ │
│  │ (Temp, etc.) │  │  (Heater)    │  │  (Bedroom)   │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
└──────────────────────────────────────────────────────────┘
```

## MQTT Topic Structure

### State Topics (Subscribe - Data FROM HA)
The system subscribes to these topics to receive sensor data from HA:

```
homeassistant/state/<entity_id>
```

Examples:
- `homeassistant/state/sensor.living_room_temperature` - Indoor temperature
- `homeassistant/state/sensor.outdoor_temperature` - Outdoor temperature
- `homeassistant/state/sensor.solar_production` - Solar panel output
- `homeassistant/state/sensor.energy_consumption` - Current power usage
- `homeassistant/state/switch.heater` - Heater state

### Command Topics (Publish - Commands TO HA)
The system publishes to these topics to control HA devices:

```
homeassistant/command/<entity_id>
```

Examples:
- `homeassistant/command/switch.heater` - Control heater
- `homeassistant/command/light.living_room` - Control lights
- `homeassistant/command/climate.ac` - Control air conditioning
- `homeassistant/command/switch.ev_charger` - Control EV charger

### Discovery Topics (Optional)
For Home Assistant MQTT Discovery:

```
homeassistant/<component>/<node_id>/<object_id>/config
```

## Message Formats

### State Messages (FROM HA)
Simple state (plain text):
```
22.5
```

JSON state with attributes:
```json
{
  "state": "22.5",
  "attributes": {
    "unit_of_measurement": "°C",
    "friendly_name": "Living Room Temperature",
    "device_class": "temperature"
  }
}
```

### Command Messages (TO HA)
Simple command:
```
ON
```

Command with data (JSON):
```json
{
  "command": "ON",
  "data": {"brightness": 80}
}
```

## Usage Examples

### Basic Setup

```cpp
#include "HAIntegration.h"
#include "MQTTClient.h"

// Create MQTT client
auto mqttClient = std::make_shared<MQTTClient>("localhost", 1883);
mqttClient->connect();

// Create HA integration
auto haIntegration = std::make_shared<HAIntegration>(mqttClient, "homeassistant");
```

### Fetching Data from HA (Subscribe)

#### Subscribe to a Specific Entity

```cpp
// Subscribe to temperature sensor
haIntegration->subscribeToEntity("sensor.living_room_temperature",
    [](const std::string& entityId, const std::string& state, const std::string& attributes) {
        double temp = std::stod(state);
        std::cout << "Temperature: " << temp << "°C" << std::endl;
        // Update your local sensor
        localTempSensor->setTemperature(temp);
    });
```

#### Subscribe to All Entities in a Domain

```cpp
// Subscribe to all sensors
haIntegration->subscribeToDomain("sensor",
    [](const std::string& entityId, const std::string& state, const std::string& attributes) {
        std::cout << "Sensor " << entityId << " updated: " << state << std::endl;
    });

// Subscribe to all switches
haIntegration->subscribeToDomain("switch",
    [](const std::string& entityId, const std::string& state, const std::string& attributes) {
        std::cout << "Switch " << entityId << " is now: " << state << std::endl;
    });
```

### Executing Commands to HA (Publish)

#### Simple ON/OFF Commands

```cpp
// Turn on a switch
haIntegration->publishCommand("switch.heater", "ON");

// Turn off a switch
haIntegration->publishCommand("switch.heater", "OFF");
```

#### Commands with Additional Data

```cpp
// Set light brightness
haIntegration->publishCommandWithData("light.living_room", "ON", 
    "{\"brightness\": 80}");

// Set thermostat temperature
haIntegration->publishCommandWithData("climate.ac", "cool",
    "{\"temperature\": 22.0}");

// Set EV charger power
haIntegration->publishCommandWithData("switch.ev_charger", "ON",
    "{\"power\": 11.0}");
```

### Request Current State

```cpp
// Request current state of an entity
haIntegration->requestState("sensor.living_room_temperature");
```

### Complete Integration Example

```cpp
#include "HAIntegration.h"
#include "MQTTClient.h"
#include "TemperatureSensor.h"

int main() {
    // Setup MQTT and HA Integration
    auto mqttClient = std::make_shared<MQTTClient>("localhost", 1883);
    mqttClient->connect();
    
    auto haIntegration = std::make_shared<HAIntegration>(mqttClient);
    
    // Local sensor
    auto indoorTempSensor = std::make_shared<TemperatureSensor>(
        "temp_indoor_1", "Living Room", TemperatureSensor::Location::INDOOR);
    
    // Subscribe to HA temperature sensor
    haIntegration->subscribeToEntity("sensor.living_room_temperature",
        [&indoorTempSensor](const std::string& entityId, 
                           const std::string& state, 
                           const std::string& attributes) {
            try {
                double temp = std::stod(state);
                indoorTempSensor->setTemperature(temp);
                indoorTempSensor->update();
                std::cout << "Updated from HA: " << temp << "°C" << std::endl;
            } catch (...) {
                std::cerr << "Failed to parse temperature" << std::endl;
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
```

## Home Assistant Configuration

### Configure MQTT in Home Assistant

Add to your `configuration.yaml`:

```yaml
mqtt:
  broker: localhost
  port: 1883
  username: homeassistant
  password: your_password
  discovery: true
  discovery_prefix: homeassistant
```

### Create Virtual Sensors for Testing

```yaml
# configuration.yaml
mqtt:
  sensor:
    - name: "Living Room Temperature"
      state_topic: "homeassistant/state/sensor.living_room_temperature"
      unit_of_measurement: "°C"
      device_class: temperature
      
    - name: "Outdoor Temperature"
      state_topic: "homeassistant/state/sensor.outdoor_temperature"
      unit_of_measurement: "°C"
      device_class: temperature
      
    - name: "Solar Production"
      state_topic: "homeassistant/state/sensor.solar_production"
      unit_of_measurement: "kW"
      device_class: power
      
    - name: "Energy Consumption"
      state_topic: "homeassistant/state/sensor.energy_consumption"
      unit_of_measurement: "kW"
      device_class: power
```

### Create Virtual Switches

```yaml
# configuration.yaml
mqtt:
  switch:
    - name: "Heater"
      command_topic: "homeassistant/command/switch.heater"
      state_topic: "homeassistant/state/switch.heater"
      
    - name: "EV Charger"
      command_topic: "homeassistant/command/switch.ev_charger"
      state_topic: "homeassistant/state/switch.ev_charger"
```

### Create Virtual Lights

```yaml
# configuration.yaml
mqtt:
  light:
    - name: "Living Room Light"
      command_topic: "homeassistant/command/light.living_room"
      state_topic: "homeassistant/state/light.living_room"
      brightness: true
      brightness_scale: 100
```

## Testing the Integration

### 1. Start MQTT Broker

```bash
# Start Mosquitto broker
sudo systemctl start mosquitto

# Or run in foreground for debugging
mosquitto -v
```

### 2. Monitor MQTT Traffic

```bash
# Subscribe to all Home Assistant topics
mosquitto_sub -h localhost -t "homeassistant/#" -v
```

### 3. Simulate HA Sensor Updates

```bash
# Publish temperature update
mosquitto_pub -h localhost \
  -t "homeassistant/state/sensor.living_room_temperature" \
  -m "22.5"

# Publish solar production update
mosquitto_pub -h localhost \
  -t "homeassistant/state/sensor.solar_production" \
  -m "5.5"
```

### 4. Monitor Commands to HA

```bash
# Subscribe to command topics
mosquitto_sub -h localhost -t "homeassistant/command/#" -v
```

### 5. Run the Home Automation System

```bash
cd build
./home_automation
```

You should see output like:
```
=== Configuring Home Assistant MQTT Integration ===
HAIntegration: Subscribed to entity sensor.living_room_temperature on topic: homeassistant/state/sensor.living_room_temperature
...
Fetching data from Home Assistant via MQTT...
HA: Updated indoor temperature from sensor.living_room_temperature: 18°C
Executing command to HA: Turn on heater
HAIntegration: Published command 'ON' to switch.heater
```

## API Reference

### HAIntegration Class

#### Constructor
```cpp
HAIntegration(std::shared_ptr<MQTTClient> mqttClient, 
              const std::string& haDiscoveryPrefix = "homeassistant");
```

#### Subscribe Methods
```cpp
// Subscribe to specific entity
void subscribeToEntity(const std::string& entityId, StateCallback callback);

// Subscribe to all entities in domain
void subscribeToDomain(const std::string& domain, StateCallback callback);

// Subscribe to discovery messages
void subscribeToDiscovery(DiscoveryCallback callback);
```

#### Publish Methods
```cpp
// Publish simple command
void publishCommand(const std::string& entityId, const std::string& command);

// Publish command with data
void publishCommandWithData(const std::string& entityId, 
                           const std::string& command, 
                           const std::string& data);

// Request current state
void requestState(const std::string& entityId);

// Publish discovery message
void publishDiscovery(const std::string& component, 
                     const std::string& nodeId,
                     const std::string& objectId, 
                     const std::string& config);
```

#### Helper Methods
```cpp
// Parse state message (static)
static bool parseStateMessage(const std::string& payload, 
                             std::string& state, 
                             std::string& attributes);

// Create command payload (static)
static std::string createCommandPayload(const std::string& command, 
                                       const std::string& data = "");
```

## Common Entity Types and Commands

### Sensors (Read-Only)
- `sensor.temperature` - Temperature readings
- `sensor.humidity` - Humidity readings
- `sensor.power` - Power consumption/production
- `sensor.energy` - Energy totals
- `sensor.battery` - Battery levels

### Switches (ON/OFF)
```cpp
haIntegration->publishCommand("switch.heater", "ON");
haIntegration->publishCommand("switch.heater", "OFF");
```

### Lights (ON/OFF with Brightness)
```cpp
haIntegration->publishCommandWithData("light.bedroom", "ON", 
    "{\"brightness\": 255}");
```

### Climate (Temperature Control)
```cpp
haIntegration->publishCommandWithData("climate.thermostat", "heat",
    "{\"temperature\": 22.0}");
```

### Covers (Curtains/Blinds)
```cpp
haIntegration->publishCommandWithData("cover.living_room", "open",
    "{\"position\": 100}");
```

## Troubleshooting

### No Data Received from HA

1. Check MQTT broker is running:
   ```bash
   sudo systemctl status mosquitto
   ```

2. Verify HA is publishing to correct topics:
   ```bash
   mosquitto_sub -h localhost -t "homeassistant/state/#" -v
   ```

3. Check HA MQTT configuration in `configuration.yaml`

### Commands Not Received by HA

1. Monitor command topics:
   ```bash
   mosquitto_sub -h localhost -t "homeassistant/command/#" -v
   ```

2. Verify HA entities are configured to listen on command topics

3. Check MQTT broker logs:
   ```bash
   sudo tail -f /var/log/mosquitto/mosquitto.log
   ```

### Connection Issues

1. Verify broker address and port in code
2. Check firewall settings
3. Test basic MQTT connectivity:
   ```bash
   mosquitto_pub -h localhost -t test -m "hello"
   mosquitto_sub -h localhost -t test
   ```

## Best Practices

1. **Error Handling**: Always wrap state parsing in try-catch blocks
2. **State Validation**: Validate state values before using them
3. **Rate Limiting**: Avoid publishing commands too frequently
4. **Reconnection**: Implement MQTT reconnection logic for production
5. **Logging**: Log all HA interactions for debugging
6. **Testing**: Test with simulated messages before using real HA
7. **Security**: Use TLS/SSL and authentication in production

## Production Deployment

For production use:

1. **Use Real MQTT Library**: Replace mock MQTTClient with Eclipse Paho or Mosquitto
2. **Add Authentication**: Configure MQTT username/password
3. **Enable TLS/SSL**: Secure MQTT communications
4. **Implement Persistence**: Store and recover subscriptions on restart
5. **Add Health Checks**: Monitor MQTT connection status
6. **Use JSON Library**: Use nlohmann/json for proper JSON parsing
7. **Add Message Queuing**: Buffer commands during disconnections

See [MQTT_MOSQUITTO_GUIDE.md](MQTT_MOSQUITTO_GUIDE.md) for integration with production MQTT libraries.

## Summary

The Home Assistant MQTT Integration provides:
- ✓ Bidirectional communication with Home Assistant
- ✓ Subscribe to HA sensors to fetch real-time data
- ✓ Publish commands to control HA devices
- ✓ Support for multiple entity types (sensors, switches, lights, climate)
- ✓ Flexible callback system for handling state changes
- ✓ Easy integration with existing event-driven architecture
- ✓ Compatible with HA MQTT discovery protocol

This enables seamless integration between the Home Automation System and Home Assistant for comprehensive home automation control.
