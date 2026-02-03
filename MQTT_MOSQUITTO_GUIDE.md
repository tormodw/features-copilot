# Using Mosquitto with the MQTT Client Interface

This guide explains how to integrate the Eclipse Mosquitto MQTT library with the Home Automation System's `MQTTClient` interface.

## Overview

The current `MQTTClient.h` provides a mock implementation for simulation purposes. For production deployment, you can integrate the Eclipse Mosquitto C/C++ library to enable real MQTT communication with sensors and appliances.

## Installation

### Installing Mosquitto Library

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libmosquitto-dev mosquitto mosquitto-clients
```

#### macOS
```bash
brew install mosquitto
```

#### Windows
Download and install from: https://mosquitto.org/download/

### Installing Mosquitto Broker (Optional)

To run a local MQTT broker for testing:

```bash
# Ubuntu/Debian
sudo apt-get install mosquitto

# Start the broker
sudo systemctl start mosquitto
sudo systemctl enable mosquitto

# Check status
sudo systemctl status mosquitto
```

The broker will listen on port 1883 by default.

## Integration Approaches

### Option 1: Wrapper Implementation (Recommended)

Replace the mock implementation in `MQTTClient.cpp` with real mosquitto calls while keeping the same interface:

```cpp
// MQTTClient.h - Interface remains unchanged
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <string>
#include <functional>
#include <map>

class MQTTClient {
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

    MQTTClient(const std::string& brokerAddress, int port = 1883);
    ~MQTTClient();

    bool connect();
    void disconnect();
    bool isConnected() const;
    void subscribe(const std::string& topic, MessageCallback callback);
    void publish(const std::string& topic, const std::string& payload);
    void processMessages();

private:
    std::string brokerAddress_;
    int port_;
    bool connected_;
    std::map<std::string, MessageCallback> subscriptions_;
    
    // Mosquitto-specific members
    struct mosquitto* mosq_;
    static void on_connect_callback(struct mosquitto* mosq, void* obj, int result);
    static void on_message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message);
};

#endif // MQTT_CLIENT_H
```

```cpp
// MQTTClient.cpp - Implementation with Mosquitto
#include "MQTTClient.h"
#include <mosquitto.h>
#include <iostream>
#include <cstring>

MQTTClient::MQTTClient(const std::string& brokerAddress, int port)
    : brokerAddress_(brokerAddress), port_(port), connected_(false), mosq_(nullptr) {
    
    // Initialize mosquitto library (call once per application)
    mosquitto_lib_init();
    
    // Create mosquitto client instance
    mosq_ = mosquitto_new(nullptr, true, this);
    
    if (mosq_) {
        // Set callbacks
        mosquitto_connect_callback_set(mosq_, on_connect_callback);
        mosquitto_message_callback_set(mosq_, on_message_callback);
    }
}

MQTTClient::~MQTTClient() {
    if (connected_) {
        disconnect();
    }
    if (mosq_) {
        mosquitto_destroy(mosq_);
    }
    mosquitto_lib_cleanup();
}

bool MQTTClient::connect() {
    if (!mosq_) {
        return false;
    }
    
    int rc = mosquitto_connect(mosq_, brokerAddress_.c_str(), port_, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to connect to MQTT broker: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    
    // Start the network loop in a separate thread
    rc = mosquitto_loop_start(mosq_);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to start MQTT loop: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    
    connected_ = true;
    return true;
}

void MQTTClient::disconnect() {
    if (mosq_ && connected_) {
        mosquitto_loop_stop(mosq_, false);
        mosquitto_disconnect(mosq_);
        connected_ = false;
        subscriptions_.clear();
    }
}

bool MQTTClient::isConnected() const {
    return connected_;
}

void MQTTClient::subscribe(const std::string& topic, MessageCallback callback) {
    if (connected_ && mosq_) {
        subscriptions_[topic] = callback;
        int rc = mosquitto_subscribe(mosq_, nullptr, topic.c_str(), 0);
        if (rc != MOSQ_ERR_SUCCESS) {
            std::cerr << "Failed to subscribe to " << topic << ": " << mosquitto_strerror(rc) << std::endl;
        }
    }
}

void MQTTClient::publish(const std::string& topic, const std::string& payload) {
    if (connected_ && mosq_) {
        int rc = mosquitto_publish(mosq_, nullptr, topic.c_str(), 
                                   payload.length(), payload.c_str(), 0, false);
        if (rc != MOSQ_ERR_SUCCESS) {
            std::cerr << "Failed to publish to " << topic << ": " << mosquitto_strerror(rc) << std::endl;
        }
    }
}

void MQTTClient::processMessages() {
    // Messages are processed automatically by mosquitto_loop_start()
    // This method is kept for interface compatibility
}

void MQTTClient::on_connect_callback(struct mosquitto* mosq, void* obj, int result) {
    if (result == 0) {
        std::cout << "Successfully connected to MQTT broker" << std::endl;
    } else {
        std::cerr << "Connection failed with code: " << result << std::endl;
    }
}

void MQTTClient::on_message_callback(struct mosquitto* mosq, void* obj, 
                                     const struct mosquitto_message* message) {
    MQTTClient* client = static_cast<MQTTClient*>(obj);
    
    std::string topic(message->topic);
    std::string payload(static_cast<char*>(message->payload), message->payloadlen);
    
    // Find and call the appropriate callback
    auto it = client->subscriptions_.find(topic);
    if (it != client->subscriptions_.end()) {
        it->second(topic, payload);
    }
}
```

### Option 2: Direct Mosquitto Usage

Alternatively, you can use the mosquitto library directly in your application code:

```cpp
// Example: DirectMQTTExample.cpp
#include <mosquitto.h>
#include <iostream>
#include <string>
#include <unistd.h>

void on_connect(struct mosquitto* mosq, void* obj, int result) {
    std::cout << "Connected with result code: " << result << std::endl;
    if (result == 0) {
        // Subscribe to topics
        mosquitto_subscribe(mosq, nullptr, "sensor/temperature/indoor", 0);
        mosquitto_subscribe(mosq, nullptr, "sensor/solar/production", 0);
    }
}

void on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message) {
    std::string topic(message->topic);
    std::string payload(static_cast<char*>(message->payload), message->payloadlen);
    
    std::cout << "Received message on " << topic << ": " << payload << std::endl;
    
    // Handle different sensor topics
    if (topic == "sensor/temperature/indoor") {
        // Parse temperature and update system
    } else if (topic == "sensor/solar/production") {
        // Parse solar production and update system
    }
}

int main() {
    mosquitto_lib_init();
    
    struct mosquitto* mosq = mosquitto_new("home_automation_client", true, nullptr);
    
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    
    int rc = mosquitto_connect(mosq, "localhost", 1883, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Could not connect to broker" << std::endl;
        return 1;
    }
    
    // Start the network loop
    mosquitto_loop_start(mosq);
    
    // Publish commands
    for (int i = 0; i < 5; i++) {
        mosquitto_publish(mosq, nullptr, "appliance/heater/command", 2, "ON", 0, false);
        sleep(2);
    }
    
    mosquitto_loop_stop(mosq, false);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    
    return 0;
}
```

## MQTT Topics Structure

The home automation system uses the following topic structure:

### Sensor Topics (Publish from sensors)

```
sensor/temperature/indoor    - Indoor temperature readings (JSON: {"temperature": 22.5, "unit": "C"})
sensor/temperature/outdoor   - Outdoor temperature readings
sensor/energy/consumption    - Energy consumption (JSON: {"consumption": 3.5, "unit": "kW"})
sensor/solar/production      - Solar production (JSON: {"production": 5.0, "unit": "kW"})
sensor/ev/charging           - EV charging status (JSON: {"charging": true, "power": 11.0})
```

### Appliance Command Topics (Subscribe on appliances)

```
appliance/heater/command     - Heater control (Payload: "ON" or "OFF")
appliance/ac/command         - Air conditioner control
appliance/light/command      - Light control (JSON: {"state": "ON", "brightness": 80})
appliance/curtain/command    - Curtain control (JSON: {"state": "OPEN", "position": 100})
appliance/ev/command         - EV charger control (JSON: {"state": "ON", "power": 11.0})
```

### Status Topics (Publish from appliances)

```
appliance/heater/status      - Heater status
appliance/ac/status          - AC status
appliance/light/status       - Light status
appliance/curtain/status     - Curtain status
appliance/ev/status          - EV charger status
```

## Example Message Formats

### Temperature Sensor Message
```json
{
  "sensor_id": "temp_indoor_1",
  "temperature": 22.5,
  "unit": "C",
  "timestamp": "2024-01-15T10:30:00Z"
}
```

### Energy Meter Message
```json
{
  "sensor_id": "energy_meter_1",
  "consumption": 3.5,
  "unit": "kW",
  "timestamp": "2024-01-15T10:30:00Z"
}
```

### Heater Command Message
```json
{
  "command": "ON",
  "target_temperature": 22.0,
  "timestamp": "2024-01-15T10:30:00Z"
}
```

### Light Command Message
```json
{
  "command": "ON",
  "brightness": 80,
  "timestamp": "2024-01-15T10:30:00Z"
}
```

## Building with Mosquitto

Update your `CMakeLists.txt` to link against the mosquitto library:

```cmake
cmake_minimum_required(VERSION 3.10)
project(home_automation)

set(CMAKE_CXX_STANDARD 17)

# Find mosquitto library
find_library(MOSQUITTO_LIB mosquitto REQUIRED)

# Source files
file(GLOB SOURCES "src/*.cpp")

add_executable(home_automation ${SOURCES})

# Link mosquitto library
target_link_libraries(home_automation ${MOSQUITTO_LIB})

# Include directories
target_include_directories(home_automation PRIVATE include)
```

## Testing with Mosquitto Clients

### Subscribe to sensor data
```bash
mosquitto_sub -h localhost -t "sensor/#" -v
```

### Publish sensor data
```bash
# Publish temperature
mosquitto_pub -h localhost -t "sensor/temperature/indoor" -m '{"temperature": 22.5, "unit": "C"}'

# Publish solar production
mosquitto_pub -h localhost -t "sensor/solar/production" -m '{"production": 5.0, "unit": "kW"}'
```

### Send appliance commands
```bash
# Turn on heater
mosquitto_pub -h localhost -t "appliance/heater/command" -m 'ON'

# Control light brightness
mosquitto_pub -h localhost -t "appliance/light/command" -m '{"state": "ON", "brightness": 80}'
```

### Monitor all topics
```bash
# Subscribe to all topics
mosquitto_sub -h localhost -t "#" -v
```

## Broker Configuration

### Basic mosquitto.conf

Create `/etc/mosquitto/mosquitto.conf`:

```conf
# Basic configuration
listener 1883
allow_anonymous true

# Logging
log_dest file /var/log/mosquitto/mosquitto.log
log_type all

# Persistence
persistence true
persistence_location /var/lib/mosquitto/
```

### Secure Configuration (Production)

```conf
# Secure configuration
listener 8883
cafile /etc/mosquitto/certs/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key

# Require authentication
allow_anonymous false
password_file /etc/mosquitto/passwd

# Logging
log_dest file /var/log/mosquitto/mosquitto.log
log_type error
log_type warning
log_type notice
log_type information

# Persistence
persistence true
persistence_location /var/lib/mosquitto/
```

### Creating Users

```bash
# Create password file
sudo mosquitto_passwd -c /etc/mosquitto/passwd admin

# Add more users
sudo mosquitto_passwd /etc/mosquitto/passwd sensor_user
sudo mosquitto_passwd /etc/mosquitto/passwd appliance_user

# Restart mosquitto
sudo systemctl restart mosquitto
```

## Authentication in Code

To connect with authentication:

```cpp
MQTTClient::MQTTClient(const std::string& brokerAddress, int port,
                       const std::string& username, const std::string& password)
    : brokerAddress_(brokerAddress), port_(port), connected_(false) {
    
    mosquitto_lib_init();
    mosq_ = mosquitto_new(nullptr, true, this);
    
    if (mosq_) {
        // Set username and password
        mosquitto_username_pw_set(mosq_, username.c_str(), password.c_str());
        
        mosquitto_connect_callback_set(mosq_, on_connect_callback);
        mosquitto_message_callback_set(mosq_, on_message_callback);
    }
}
```

## TLS/SSL Configuration

For secure connections:

```cpp
bool MQTTClient::connectSecure(const std::string& cafile) {
    if (!mosq_) {
        return false;
    }
    
    // Set TLS options
    int rc = mosquitto_tls_set(mosq_, cafile.c_str(), nullptr, nullptr, nullptr, nullptr);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to set TLS: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    
    // Connect to broker on secure port (usually 8883)
    rc = mosquitto_connect(mosq_, brokerAddress_.c_str(), 8883, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to connect: " << mosquitto_strerror(rc) << std::endl;
        return false;
    }
    
    mosquitto_loop_start(mosq_);
    connected_ = true;
    return true;
}
```

## Complete Integration Example

Here's a complete example showing how to integrate mosquitto with the home automation system:

```cpp
// main.cpp with real MQTT
#include "EventManager.h"
#include "MQTTClient.h"
#include "TemperatureSensor.h"
#include "EnergyOptimizer.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>  // For JSON parsing

using json = nlohmann::json;

int main() {
    std::cout << "=== Home Automation System Starting ===" << std::endl;

    // Connect to real MQTT broker
    auto mqttClient = std::make_shared<MQTTClient>("localhost", 1883);
    
    if (!mqttClient->connect()) {
        std::cerr << "Failed to connect to MQTT broker" << std::endl;
        return 1;
    }
    
    std::cout << "Connected to MQTT broker" << std::endl;

    // Create sensors
    auto indoorTempSensor = std::make_shared<TemperatureSensor>(
        "temp_indoor_1", "Living Room", TemperatureSensor::Location::INDOOR);

    // Subscribe to temperature sensor topic with real callback
    mqttClient->subscribe("sensor/temperature/indoor", 
        [&](const std::string& topic, const std::string& payload) {
            try {
                // Parse JSON payload
                auto data = json::parse(payload);
                double temp = data["temperature"];
                
                // Update sensor
                indoorTempSensor->setTemperature(temp);
                indoorTempSensor->update();
                
                std::cout << "Received temperature: " << temp << "°C" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse message: " << e.what() << std::endl;
            }
        });

    // Subscribe to solar production
    mqttClient->subscribe("sensor/solar/production",
        [&](const std::string& topic, const std::string& payload) {
            try {
                auto data = json::parse(payload);
                double production = data["production"];
                std::cout << "Solar production: " << production << " kW" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse message: " << e.what() << std::endl;
            }
        });

    std::cout << "MQTT subscriptions configured" << std::endl;

    // Run the system
    std::cout << "System running. Press Ctrl+C to exit." << std::endl;
    
    // Keep the application running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    mqttClient->disconnect();
    return 0;
}
```

## Troubleshooting

### Connection Issues

1. **Check if broker is running:**
   ```bash
   sudo systemctl status mosquitto
   ```

2. **Check broker logs:**
   ```bash
   sudo tail -f /var/log/mosquitto/mosquitto.log
   ```

3. **Test with mosquitto_sub:**
   ```bash
   mosquitto_sub -h localhost -t test
   ```

4. **Check firewall:**
   ```bash
   sudo ufw allow 1883/tcp
   ```

### Common Errors

- **Connection refused**: Broker not running or wrong address
- **Authentication failed**: Wrong username/password
- **Certificate errors**: TLS configuration issue
- **Timeout**: Network connectivity problem

## Additional Resources

- **Eclipse Mosquitto Documentation**: https://mosquitto.org/documentation/
- **Mosquitto C Library API**: https://mosquitto.org/api/files/mosquitto-h.html
- **MQTT Protocol Specification**: https://mqtt.org/
- **MQTT.org**: https://mqtt.org/ - MQTT protocol documentation and resources

## Next Steps

1. Install mosquitto library and broker
2. Test the broker with command-line clients
3. Implement the wrapper in `MQTTClient.cpp`
4. Update `CMakeLists.txt` to link mosquitto
5. Test with real sensor hardware or simulated MQTT messages
6. Configure security (TLS/SSL and authentication) for production
7. Add error handling and reconnection logic
8. Implement message queuing for reliability
