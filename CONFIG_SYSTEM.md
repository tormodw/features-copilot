# Configuration System

The home automation system includes a comprehensive configuration management system with a web-based interface for easy setup and management.

## Features

### Config Class

The `Config` class provides centralized configuration management with the following capabilities:

#### Deferrable Loads Configuration
- Set the number of deferrable loads
- Manage list of deferrable load names (add/remove)
- Configure which appliances can be deferred during high energy prices

#### MQTT Configuration
- Enable/disable MQTT integration
- Configure MQTT broker address
- Set MQTT port

#### Sensor Configuration
- Manage list of sensors to include in the system
- Add/remove sensors dynamically
- Control which sensor data is collected and published

#### Web Interface Configuration
- Enable/disable web interface
- Configure web interface port

#### Persistence
- Save configuration to JSON file
- Load configuration from JSON file
- Automatic persistence on updates via web interface

### Web Interface

The system includes a modern, responsive web interface for configuration management accessible at `http://localhost:8080` (configurable port).

#### Features:
- **MQTT Settings**: Configure MQTT broker address, port, and enable/disable MQTT
- **Deferrable Loads**: Add/remove deferrable loads with intuitive list management
- **Sensor Values**: Manage which sensors are included in the system
- **Web Interface Settings**: Configure the web interface itself
- **Real-time Updates**: Configuration changes are immediately reflected in the system
- **Persistent Storage**: All changes are automatically saved to `config.json`

![Web Interface Screenshot](https://github.com/user-attachments/assets/bceebf87-02c1-4f71-a8b5-ae699fae1222)

## API Reference

### Config Class API

```cpp
#include "Config.h"

// Create configuration
Config config;

// Deferrable loads
config.addDeferrableLoad("ev_charger");
config.removeDeferrableLoad("pool_pump");
std::vector<std::string> loads = config.getDeferrableLoadNames();
int count = config.getDeferrableLoadCount();

// MQTT settings
config.setMqttEnabled(true);
config.setMqttBrokerAddress("192.168.1.100");
config.setMqttPort(1883);
bool enabled = config.isMqttEnabled();

// Sensor settings
config.addSensorValue("temperature_indoor");
config.removeSensorValue("humidity");
std::vector<std::string> sensors = config.getSensorValues();

// Persistence
config.saveToFile("config.json");
config.loadFromFile("config.json");

// JSON serialization
std::string json = config.toJson();
config.fromJson(jsonString);
```

### ConfigWebServer API

```cpp
#include "ConfigWebServer.h"

// Create web server with config
auto config = std::make_shared<Config>();
auto webServer = std::make_shared<ConfigWebServer>(config, 8080);

// Start server (non-blocking)
webServer->start();

// Check if running
bool running = webServer->isRunning();

// Get server URL
std::string url = webServer->getServerUrl(); // "http://localhost:8080"

// Stop server
webServer->stop();
```

## REST API Endpoints

### GET /api/config

Retrieves the current configuration.

**Response:**
```json
{
  "mqtt": {
    "enabled": true,
    "brokerAddress": "localhost",
    "port": 1883
  },
  "deferrableLoads": [
    "ev_charger",
    "decorative_lights",
    "pool_pump"
  ],
  "sensors": [
    "temperature_indoor",
    "temperature_outdoor",
    "energy_meter",
    "solar_production"
  ],
  "webInterface": {
    "enabled": true,
    "port": 8080
  }
}
```

### POST /api/config

Updates the configuration.

**Request Body:** Same JSON structure as GET response

**Response:**
```json
{
  "success": true,
  "message": "Configuration updated successfully"
}
```

### GET /

Returns the HTML configuration interface.

## Usage Examples

### Basic Configuration

```cpp
#include "Config.h"
#include "ConfigWebServer.h"

int main() {
    // Load or create default configuration
    auto config = std::make_shared<Config>(Config::getDefaultConfig());
    
    // Customize configuration
    config->addDeferrableLoad("ev_charger");
    config->addSensorValue("temperature_indoor");
    config->setMqttEnabled(true);
    
    // Save to file
    config->saveToFile("config.json");
    
    // Start web server for runtime configuration
    auto webServer = std::make_shared<ConfigWebServer>(config, 8080);
    webServer->start();
    
    std::cout << "Configuration available at: " << webServer->getServerUrl() << std::endl;
    
    // Your application code here...
    // The config can be accessed and modified at runtime via the web interface
    
    return 0;
}
```

### Loading Existing Configuration

```cpp
auto config = std::make_shared<Config>();

// Load from file
if (config->loadFromFile("config.json")) {
    std::cout << "Configuration loaded successfully" << std::endl;
    std::cout << "MQTT Enabled: " << config->isMqttEnabled() << std::endl;
    std::cout << "Deferrable Loads: " << config->getDeferrableLoadCount() << std::endl;
} else {
    std::cout << "Failed to load config, using defaults" << std::endl;
    *config = Config::getDefaultConfig();
}
```

### Using Configuration in Your Application

```cpp
auto config = std::make_shared<Config>();
config->loadFromFile("config.json");

// Configure MQTT based on config
if (config->isMqttEnabled()) {
    auto mqttClient = std::make_shared<MQTTClient>(
        config->getMqttBrokerAddress().c_str(),
        config->getMqttPort()
    );
    mqttClient->connect();
}

// Configure sensors based on config
for (const auto& sensorName : config->getSensorValues()) {
    // Create and configure sensor
    std::cout << "Initializing sensor: " << sensorName << std::endl;
}

// Configure deferrable loads
for (const auto& loadName : config->getDeferrableLoadNames()) {
    std::cout << "Registering deferrable load: " << loadName << std::endl;
}
```

## Testing

A test application is provided to demonstrate the configuration system:

```bash
# Build the test application
cd build
cmake ..
make test_config

# Run the test application
./test_config
```

The test application will:
1. Create a default configuration
2. Display the configuration
3. Save the configuration to `config.json`
4. Start the web server on port 8080
5. Allow you to access the web interface at http://localhost:8080

### Testing with curl

```bash
# Get current configuration
curl http://localhost:8080/api/config

# Update configuration
curl -X POST http://localhost:8080/api/config \
  -H "Content-Type: application/json" \
  -d '{"mqtt":{"enabled":true,"brokerAddress":"192.168.1.100","port":1883},"deferrableLoads":["ev_charger","pool_pump"],"sensors":["temp1","temp2"],"webInterface":{"enabled":true,"port":8080}}'

# Get HTML interface
curl http://localhost:8080/
```

## Configuration File Format

The configuration is stored in JSON format in `config.json`:

```json
{
  "mqtt": {
    "enabled": true,
    "brokerAddress": "localhost",
    "port": 1883
  },
  "deferrableLoads": [
    "ev_charger",
    "decorative_lights",
    "pool_pump"
  ],
  "sensors": [
    "temperature_indoor",
    "temperature_outdoor",
    "energy_meter",
    "solar_production",
    "ev_charger_power"
  ],
  "webInterface": {
    "enabled": true,
    "port": 8080
  }
}
```

## Default Configuration

The system provides sensible defaults:

- **MQTT**: Enabled, localhost:1883
- **Deferrable Loads**: ev_charger, decorative_lights, pool_pump
- **Sensors**: temperature_indoor, temperature_outdoor, energy_meter, solar_production, ev_charger_power
- **Web Interface**: Enabled, port 8080

Access defaults via:
```cpp
Config config = Config::getDefaultConfig();
```

## Integration with Existing System

The Config class is designed to integrate seamlessly with the existing home automation components:

### With MQTT Client
```cpp
if (config->isMqttEnabled()) {
    mqttClient = std::make_shared<MQTTClient>(
        config->getMqttBrokerAddress().c_str(),
        config->getMqttPort()
    );
}
```

### With Deferrable Load Controller
```cpp
auto deferrableController = std::make_shared<DeferrableLoadController>(mlPredictor);
for (const auto& loadName : config->getDeferrableLoadNames()) {
    // Look up appliance by name and add to controller
    auto appliance = findApplianceByName(loadName);
    if (appliance) {
        deferrableController->addDeferrableLoad(appliance);
    }
}
```

### With Sensors
```cpp
for (const auto& sensorName : config->getSensorValues()) {
    // Create appropriate sensor based on name
    if (sensorName.find("temperature") != std::string::npos) {
        sensors.push_back(std::make_shared<TemperatureSensor>(sensorName, sensorName));
    } else if (sensorName.find("energy") != std::string::npos) {
        sensors.push_back(std::make_shared<EnergyMeter>(sensorName, sensorName));
    }
    // etc.
}
```

## Security Considerations

For production deployment:

1. **Authentication**: Add authentication to the web interface (currently open)
2. **HTTPS**: Use TLS/SSL for web interface
3. **Access Control**: Restrict web interface to trusted networks
4. **Input Validation**: Already implemented in Config class
5. **CORS**: Configure CORS headers appropriately (currently allows all origins for development)

## Thread Safety

The ConfigWebServer runs in a separate thread and uses atomic operations for the running flag. The Config class itself is not thread-safe by default. For multi-threaded access:

```cpp
#include <mutex>

std::mutex configMutex;

// Reading config
{
    std::lock_guard<std::mutex> lock(configMutex);
    auto loads = config->getDeferrableLoadNames();
}

// Writing config
{
    std::lock_guard<std::mutex> lock(configMutex);
    config->addDeferrableLoad("new_load");
}
```

## Building

The configuration system is built automatically when building the home automation project:

```bash
mkdir build
cd build
cmake ..
make
```

This builds:
- `home_automation` - Main application with Config integration
- `test_config` - Configuration system test application

## Dependencies

- C++17 compiler
- CMake 3.10+
- pthread (for web server threading)
- No external dependencies for core functionality

## Future Enhancements

Potential improvements:

1. **Authentication**: Add user authentication for web interface
2. **HTTPS Support**: SSL/TLS for secure communication
3. **Advanced JSON Parsing**: Integration with nlohmann/json for better JSON handling
4. **Config Validation**: Schema validation for configuration
5. **Hot Reload**: Automatic reload of configuration without restart
6. **Backup/Restore**: Configuration backup and restore functionality
7. **Multiple Profiles**: Support for multiple configuration profiles
8. **Remote Configuration**: Support for fetching configuration from remote server

## License

This is part of the home automation system demonstration project.
