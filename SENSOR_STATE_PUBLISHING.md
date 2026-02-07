# Publishing Home Assistant Sensor States to MQTT

This guide demonstrates how to automatically publish all your local sensor states to MQTT so they are available in Home Assistant and other MQTT-connected systems.

## Overview

The Home Automation System can publish sensor states TO Home Assistant (in addition to subscribing FROM Home Assistant), enabling bidirectional synchronization of sensor data. This is useful for:

- **Making local sensors visible in Home Assistant** - Sensors created in this system can appear in HA
- **Sharing sensor data** - Other MQTT clients can subscribe to your sensor states
- **Creating a unified sensor network** - Combine sensors from multiple sources in one place
- **Real-time monitoring** - Sensor updates are immediately published to MQTT

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│         Home Automation System (This Application)      │
│                                                         │
│  ┌──────────────┐         ┌─────────────────┐         │
│  │Local Sensors │────────►│  HAIntegration  │         │
│  │              │         │                 │         │
│  │- Temperature │         │  publishState() │         │
│  │- Energy      │         │                 │         │
│  │- Solar       │         └────────┬────────┘         │
│  │- EV Charger  │                  │                   │
│  └──────────────┘                  │ MQTT Publish      │
└────────────────────────────────────┼───────────────────┘
                                     │
                                     ▼
                              MQTT Broker
                            (e.g., Mosquitto)
                                     │
                                     ▼
┌────────────────────────────────────┼───────────────────┐
│              Home Assistant        │                   │
│                                    ▼                   │
│         Automatically appears as sensors               │
│  ┌──────────────────────────────────────────────┐    │
│  │ sensor.local_temp_indoor        22.5°C       │    │
│  │ sensor.local_temp_outdoor       15.0°C       │    │
│  │ sensor.local_energy_consumption  3.5 kW      │    │
│  │ sensor.local_solar_production    5.2 kW      │    │
│  │ sensor.local_ev_charger_power   11.0 kW      │    │
│  └──────────────────────────────────────────────┘    │
└───────────────────────────────────────────────────────┘
```

## Quick Start Example

Here's a complete example showing how to publish all sensor states:

```cpp
#include "HAIntegration.h"
#include "MQTTClient.h"
#include "TemperatureSensor.h"
#include "EnergyMeter.h"
#include "SolarSensor.h"

int main() {
    // Setup MQTT and HA Integration
    auto mqttClient = std::make_shared<MQTTClient>("localhost", 1883);
    mqttClient->connect();
    
    auto haIntegration = std::make_shared<HAIntegration>(mqttClient);
    
    // Create local sensors
    auto tempSensor = std::make_shared<TemperatureSensor>(
        "temp_1", "Living Room", TemperatureSensor::Location::INDOOR);
    
    auto energyMeter = std::make_shared<EnergyMeter>(
        "energy_1", "Main Meter");
    
    auto solarSensor = std::make_shared<SolarSensor>(
        "solar_1", "Solar Panels");
    
    // Set sensor values
    tempSensor->setTemperature(22.5);
    energyMeter->setConsumption(3.5);
    solarSensor->setProduction(5.2);
    
    // Publish ALL sensor states to MQTT/HA
    haIntegration->publishState("sensor.local_temperature", 
                                std::to_string(tempSensor->getTemperature()));
    
    haIntegration->publishState("sensor.local_energy", 
                                std::to_string(energyMeter->getConsumption()));
    
    haIntegration->publishState("sensor.local_solar", 
                                std::to_string(solarSensor->getProduction()));
    
    std::cout << "All sensor states published to MQTT!" << std::endl;
    
    return 0;
}
```

## Publishing Sensor States

### Basic State Publishing

The simplest way to publish a sensor state:

```cpp
// Publish a simple state value
haIntegration->publishState("sensor.my_temperature", "22.5");
```

This publishes the value to the MQTT topic: `homeassistant/state/sensor.my_temperature`

### State Publishing with Attributes

For richer information, include attributes:

```cpp
// Publish state with attributes (JSON format)
std::string attributes = "{\"unit_of_measurement\": \"°C\", "
                        "\"friendly_name\": \"Living Room Temperature\", "
                        "\"device_class\": \"temperature\"}";

haIntegration->publishState("sensor.local_temp_indoor", "22.5", attributes);
```

This creates a JSON payload:
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

### Publishing Different Sensor Types

#### Temperature Sensors
```cpp
auto tempSensor = std::make_shared<TemperatureSensor>(
    "temp_1", "Bedroom", TemperatureSensor::Location::INDOOR);
tempSensor->setTemperature(21.0);

std::string attrs = "{\"unit_of_measurement\": \"°C\", "
                   "\"device_class\": \"temperature\"}";
haIntegration->publishState("sensor.bedroom_temp", 
                            std::to_string(tempSensor->getTemperature()),
                            attrs);
```

#### Energy Meters
```cpp
auto energyMeter = std::make_shared<EnergyMeter>(
    "energy_1", "Main Meter");
energyMeter->setConsumption(4.2);

std::string attrs = "{\"unit_of_measurement\": \"kW\", "
                   "\"device_class\": \"power\"}";
haIntegration->publishState("sensor.main_power", 
                            std::to_string(energyMeter->getConsumption()),
                            attrs);
```

#### Solar Production Sensors
```cpp
auto solarSensor = std::make_shared<SolarSensor>(
    "solar_1", "Rooftop Solar");
solarSensor->setProduction(6.5);

std::string attrs = "{\"unit_of_measurement\": \"kW\", "
                   "\"device_class\": \"power\"}";
haIntegration->publishState("sensor.solar_power", 
                            std::to_string(solarSensor->getProduction()),
                            attrs);
```

#### EV Charger Sensors
```cpp
auto evCharger = std::make_shared<EVChargerSensor>(
    "ev_1", "EV Charger");
evCharger->setChargingPower(11.0);
evCharger->setIsCharging(true);

std::ostringstream attrs;
attrs << "{\"unit_of_measurement\": \"kW\", "
      << "\"device_class\": \"power\", "
      << "\"charging\": " << (evCharger->isCharging() ? "true" : "false") << "}";

haIntegration->publishState("sensor.ev_charger_power", 
                            std::to_string(evCharger->getChargingPower()),
                            attrs.str());
```

## Automatic Publishing on Sensor Updates

To automatically publish sensor states whenever they change, integrate with the event system:

```cpp
// Subscribe to sensor events
auto eventManager = EventManager::getInstance();

eventManager->subscribe(EventType::TEMPERATURE_CHANGED, 
    [&](const Event& event) {
        // Get sensor data from event
        double temperature = event.temperature;
        std::string sensorId = event.sensorId;
        
        // Publish to MQTT
        std::string entityId = "sensor.local_" + sensorId;
        haIntegration->publishState(entityId, std::to_string(temperature));
        
        std::cout << "Auto-published temperature: " << temperature << "°C" << std::endl;
    });

eventManager->subscribe(EventType::ENERGY_CHANGED,
    [&](const Event& event) {
        double consumption = event.currentEnergy;
        std::string entityId = "sensor.local_energy";
        haIntegration->publishState(entityId, std::to_string(consumption));
        
        std::cout << "Auto-published energy: " << consumption << " kW" << std::endl;
    });

// Now whenever sensors update and publish events, states are auto-published to MQTT
tempSensor->setTemperature(23.0);
tempSensor->update(); // Triggers event -> auto publishes to MQTT
```

## MQTT Discovery Protocol

Make sensors automatically appear in Home Assistant using the discovery protocol:

### Publishing Discovery Configs

```cpp
// Publish discovery for temperature sensor
haIntegration->publishDiscovery("sensor", "home_automation", "local_temp_indoor",
    "{\"name\": \"Local Indoor Temperature\", "
    "\"state_topic\": \"homeassistant/state/sensor.local_temp_indoor\", "
    "\"unit_of_measurement\": \"°C\", "
    "\"device_class\": \"temperature\"}");

// Publish discovery for energy sensor
haIntegration->publishDiscovery("sensor", "home_automation", "local_energy",
    "{\"name\": \"Local Energy Consumption\", "
    "\"state_topic\": \"homeassistant/state/sensor.local_energy\", "
    "\"unit_of_measurement\": \"kW\", "
    "\"device_class\": \"power\"}");

// Publish discovery for solar sensor
haIntegration->publishDiscovery("sensor", "home_automation", "local_solar",
    "{\"name\": \"Local Solar Production\", "
    "\"state_topic\": \"homeassistant/state/sensor.local_solar\", "
    "\"unit_of_measurement\": \"kW\", "
    "\"device_class\": \"power\"}");
```

After publishing discovery configs, these sensors will automatically appear in Home Assistant!

### Discovery Topic Structure

Discovery messages are published to:
```
homeassistant/<component>/<node_id>/<object_id>/config
```

Example: `homeassistant/sensor/home_automation/local_temp_indoor/config`

## Complete Working Example

See `src/main.cpp` for a complete demonstration that:
1. Creates multiple local sensors
2. Publishes discovery configs for automatic HA detection
3. Publishes initial sensor states with attributes
4. Simulates automatic updates and re-publishes states

Run it:
```bash
cd build
cmake ..
make
./home_automation
```

Output shows:
```
=== Home Assistant Sensor State Publishing Demo ===

=== Step 1: Creating Local Sensors ===
Created 5 local sensors

=== Step 2: Publishing Discovery Configs to HA ===
HAIntegration: Published discovery for sensor.local_temp_indoor
HAIntegration: Published discovery for sensor.local_temp_outdoor
...

=== Step 3: Setting Initial Sensor Values ===
Set initial values for all sensors

=== Step 4: Publishing ALL Sensor States to MQTT ===
HAIntegration: Published state for sensor.local_temp_indoor: 22.5
HAIntegration: Published state for sensor.local_temp_outdoor: 15.0
...

All sensor states published!
  - Indoor Temperature: 22.5 °C
  - Outdoor Temperature: 15.0 °C
  - Energy Consumption: 3.5 kW
  - Solar Production: 5.2 kW
  - EV Charger Power: 11.0 kW

=== Step 5: Automatic Updates ===
Publishing sensor updates every 5 seconds...
```

## MQTT Topics Used

### State Topics
All sensor states are published to:
```
homeassistant/state/<entity_id>
```

Examples:
- `homeassistant/state/sensor.local_temp_indoor`
- `homeassistant/state/sensor.local_temp_outdoor`
- `homeassistant/state/sensor.local_energy_consumption`
- `homeassistant/state/sensor.local_solar_production`
- `homeassistant/state/sensor.local_ev_charger_power`

### Discovery Topics
Discovery configs are published to:
```
homeassistant/<component>/<node_id>/<object_id>/config
```

Examples:
- `homeassistant/sensor/home_automation/local_temp_indoor/config`
- `homeassistant/sensor/home_automation/local_energy/config`

## Testing with MQTT Tools

### Monitor Published States

```bash
# Subscribe to all sensor states
mosquitto_sub -h localhost -t "homeassistant/state/#" -v
```

You'll see:
```
homeassistant/state/sensor.local_temp_indoor {"state": "22.5", "attributes": {...}}
homeassistant/state/sensor.local_energy_consumption {"state": "3.5", "attributes": {...}}
homeassistant/state/sensor.local_solar_production {"state": "5.2", "attributes": {...}}
```

### Monitor Discovery Messages

```bash
# Subscribe to discovery topics
mosquitto_sub -h localhost -t "homeassistant/sensor/+/+/config" -v
```

### Manually Publish State (for testing)

```bash
# Publish a sensor state
mosquitto_pub -h localhost \
  -t "homeassistant/state/sensor.test_sensor" \
  -m "42.0"

# Publish state with JSON attributes
mosquitto_pub -h localhost \
  -t "homeassistant/state/sensor.test_sensor" \
  -m '{"state": "42.0", "attributes": {"unit": "°C"}}'
```

## Home Assistant Configuration

### Enable MQTT Discovery

In your `configuration.yaml`:

```yaml
mqtt:
  broker: localhost
  port: 1883
  discovery: true
  discovery_prefix: homeassistant
```

After restarting HA and running this application, all published sensors will automatically appear in Home Assistant!

### View Published Sensors in HA

1. Go to **Developer Tools** → **States**
2. Search for `sensor.local_*`
3. You'll see all your published sensors with their current values

### Manual Sensor Configuration (Alternative to Discovery)

If not using discovery, configure sensors manually:

```yaml
# configuration.yaml
mqtt:
  sensor:
    - name: "Local Indoor Temperature"
      state_topic: "homeassistant/state/sensor.local_temp_indoor"
      unit_of_measurement: "°C"
      device_class: temperature
      value_template: >
        {% if value_json is defined %}
          {{ value_json.state }}
        {% else %}
          {{ value }}
        {% endif %}
      
    - name: "Local Energy Consumption"
      state_topic: "homeassistant/state/sensor.local_energy_consumption"
      unit_of_measurement: "kW"
      device_class: power
      value_template: >
        {% if value_json is defined %}
          {{ value_json.state }}
        {% else %}
          {{ value }}
        {% endif %}
```

## API Reference

### HAIntegration::publishState()

```cpp
void publishState(const std::string& entityId, 
                 const std::string& state, 
                 const std::string& attributes = "");
```

**Parameters:**
- `entityId` - HA entity ID (e.g., "sensor.local_temperature")
- `state` - State value to publish (e.g., "22.5")
- `attributes` - Optional JSON attributes (e.g., {"unit": "°C", "friendly_name": "..."})

**Example:**
```cpp
// Simple state
haIntegration->publishState("sensor.my_temp", "22.5");

// State with attributes
haIntegration->publishState("sensor.my_temp", "22.5", 
    "{\"unit_of_measurement\": \"°C\"}");
```

### HAIntegration::publishDiscovery()

```cpp
void publishDiscovery(const std::string& component, 
                     const std::string& nodeId,
                     const std::string& objectId, 
                     const std::string& config);
```

**Parameters:**
- `component` - HA component type (e.g., "sensor", "switch", "light")
- `nodeId` - Node identifier (e.g., "home_automation")
- `objectId` - Object identifier (e.g., "local_temp_indoor")
- `config` - JSON configuration for HA discovery

**Example:**
```cpp
haIntegration->publishDiscovery("sensor", "home_automation", "my_sensor",
    "{\"name\": \"My Sensor\", "
    "\"state_topic\": \"homeassistant/state/sensor.my_sensor\", "
    "\"unit_of_measurement\": \"°C\"}");
```

## Best Practices

1. **Use Discovery Protocol** - Automatically register sensors in HA using discovery
2. **Include Attributes** - Add unit, friendly name, and device class for better HA integration
3. **Publish on Change** - Only publish when sensor values actually change to reduce MQTT traffic
4. **Use Standard Units** - Follow HA conventions (°C, kW, %, etc.)
5. **Consistent Entity IDs** - Use descriptive entity IDs like `sensor.local_temp_indoor`
6. **Handle Errors** - Check if MQTT client is connected before publishing
7. **Rate Limiting** - Don't publish too frequently (e.g., max once per second)

## Troubleshooting

### States Not Appearing in HA

1. **Check MQTT broker is running:**
   ```bash
   sudo systemctl status mosquitto
   ```

2. **Monitor MQTT traffic:**
   ```bash
   mosquitto_sub -h localhost -t "homeassistant/#" -v
   ```

3. **Verify HA MQTT configuration:**
   - Check `configuration.yaml` has MQTT enabled
   - Verify broker address and port
   - Ensure discovery is enabled

4. **Check MQTT client connection:**
   ```cpp
   if (mqttClient->isConnected()) {
       std::cout << "MQTT connected" << std::endl;
   }
   ```

### Discovery Not Working

1. **Verify discovery prefix matches HA:**
   - Default is "homeassistant"
   - Must match HA configuration

2. **Check discovery topic format:**
   - Should be: `homeassistant/<component>/<node_id>/<object_id>/config`

3. **Ensure state_topic matches:**
   - Discovery config must reference correct state topic

### Values Not Updating

1. **Ensure states are being published:**
   - Check console output for "Published state" messages

2. **Monitor MQTT broker:**
   ```bash
   mosquitto_sub -h localhost -t "homeassistant/state/#" -v
   ```

3. **Verify HA is subscribed:**
   - Check HA logs for MQTT messages
   - Look for connection errors

## Summary

This guide shows how to:

✅ Publish all local sensor states to MQTT  
✅ Make sensors automatically appear in Home Assistant  
✅ Include attributes for richer sensor information  
✅ Use HA discovery protocol for automatic registration  
✅ Implement automatic publishing on sensor updates  
✅ Test with MQTT command-line tools  

Now all your Home Automation System sensors are available in Home Assistant via MQTT!
