# Home Assistant REST API - Sensor Data Extraction Guide

## Overview

This guide provides comprehensive examples for extracting sensor data from Home Assistant using its RESTful API. While MQTT provides real-time event-driven communication, the REST API is perfect for:

- **One-time data queries** when you need current state
- **Historical data retrieval** from the database
- **Polling-based integrations** where MQTT isn't available
- **Initial state synchronization** on startup
- **Administrative operations** like listing all entities

## Table of Contents

1. [Authentication Setup](#authentication-setup)
2. [API Endpoints](#api-endpoints)
3. [Extracting Sensor Data](#extracting-sensor-data)
4. [Code Examples](#code-examples)
5. [Best Practices](#best-practices)
6. [Error Handling](#error-handling)
7. [Production Deployment](#production-deployment)

## Authentication Setup

### Creating a Long-Lived Access Token

Home Assistant REST API requires authentication using a long-lived access token:

1. **Log into Home Assistant** web interface
2. **Navigate to your profile**: Click your username in the bottom left
3. **Scroll to "Long-Lived Access Tokens"**
4. **Click "Create Token"**
5. **Give it a name** (e.g., "C++ Automation System")
6. **Copy the token** - You won't see it again!

### Using the Token

Include the token in the `Authorization` header of all API requests:

```
Authorization: Bearer YOUR_LONG_LIVED_ACCESS_TOKEN
```

**Important Security Notes:**
- Never commit tokens to version control
- Store tokens in environment variables or secure configuration files
- Use file permissions to protect configuration files (chmod 600)
- Rotate tokens periodically
- Revoke tokens when no longer needed

## API Endpoints

### Base URL Structure

```
http://YOUR_HOME_ASSISTANT_IP:8123/api/
```

For example:
```
http://192.168.1.100:8123/api/states
http://homeassistant.local:8123/api/states/sensor.temperature
```

### Key Endpoints for Sensor Data

#### 1. Get All States
```
GET /api/states
```
Returns the state of all entities in Home Assistant.

**Response:** Array of state objects

#### 2. Get Specific Entity State
```
GET /api/states/<entity_id>
```
Returns the state of a specific entity.

**Example:** `GET /api/states/sensor.living_room_temperature`

**Response:**
```json
{
  "entity_id": "sensor.living_room_temperature",
  "state": "22.5",
  "attributes": {
    "unit_of_measurement": "°C",
    "friendly_name": "Living Room Temperature",
    "device_class": "temperature"
  },
  "last_changed": "2024-01-15T10:30:00.000000+00:00",
  "last_updated": "2024-01-15T10:30:00.000000+00:00"
}
```

#### 3. Get History Data
```
GET /api/history/period/<timestamp>?filter_entity_id=<entity_id>
```
Returns historical state changes for entities.

**Example:** `GET /api/history/period/2024-01-15T00:00:00+00:00?filter_entity_id=sensor.energy_consumption`

#### 4. Get Service List
```
GET /api/services
```
Returns all available services (useful for discovering what you can control).

#### 5. Call a Service
```
POST /api/services/<domain>/<service>
```
Execute a service (control devices, trigger automations).

**Example:** Turn on a switch
```
POST /api/services/switch/turn_on
Content-Type: application/json

{
  "entity_id": "switch.heater"
}
```

## Extracting Sensor Data

### Common Sensor Types and Entity IDs

Home Assistant uses the format: `<domain>.<entity_name>`

**Sensors (`sensor.`):**
- `sensor.living_room_temperature` - Temperature sensor
- `sensor.energy_consumption` - Energy meter
- `sensor.solar_production` - Solar panel output
- `sensor.outdoor_humidity` - Humidity sensor
- `sensor.cpu_temperature` - System monitoring

**Binary Sensors (`binary_sensor.`):**
- `binary_sensor.motion_detector` - Motion detection
- `binary_sensor.door_contact` - Door/window sensor
- `binary_sensor.smoke_detector` - Smoke alarm

**Other Domains:**
- `switch.*` - Switches (on/off devices)
- `light.*` - Lights
- `climate.*` - Thermostats and HVAC
- `cover.*` - Blinds, curtains, garage doors

### Example: Getting Temperature Data

**Request:**
```bash
curl -X GET \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  http://192.168.1.100:8123/api/states/sensor.living_room_temperature
```

**Response:**
```json
{
  "entity_id": "sensor.living_room_temperature",
  "state": "22.5",
  "attributes": {
    "unit_of_measurement": "°C",
    "friendly_name": "Living Room Temperature",
    "device_class": "temperature"
  },
  "last_changed": "2024-01-15T10:30:00.000000+00:00",
  "last_updated": "2024-01-15T10:30:00.000000+00:00",
  "context": {
    "id": "01HGZX...",
    "parent_id": null,
    "user_id": null
  }
}
```

### Example: Getting All Sensors

**Request:**
```bash
curl -X GET \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  http://192.168.1.100:8123/api/states
```

This returns an array of all entity states. You can filter on the client side:

```python
# Example in Python
import requests

response = requests.get(
    "http://192.168.1.100:8123/api/states",
    headers={"Authorization": "Bearer YOUR_TOKEN"}
)

all_states = response.json()

# Filter for sensors only
sensors = [entity for entity in all_states if entity['entity_id'].startswith('sensor.')]

for sensor in sensors:
    print(f"{sensor['entity_id']}: {sensor['state']} {sensor['attributes'].get('unit_of_measurement', '')}")
```

### Example: Historical Data

Get the last 24 hours of temperature data:

**Request:**
```bash
curl -X GET \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  "http://192.168.1.100:8123/api/history/period/2024-01-15T00:00:00+00:00?filter_entity_id=sensor.living_room_temperature"
```

**Response:**
```json
[
  [
    {
      "entity_id": "sensor.living_room_temperature",
      "state": "21.0",
      "last_changed": "2024-01-15T00:15:00.000000+00:00",
      "last_updated": "2024-01-15T00:15:00.000000+00:00",
      "attributes": {
        "unit_of_measurement": "°C",
        "friendly_name": "Living Room Temperature"
      }
    },
    {
      "entity_id": "sensor.living_room_temperature",
      "state": "21.5",
      "last_changed": "2024-01-15T01:30:00.000000+00:00",
      "last_updated": "2024-01-15T01:30:00.000000+00:00",
      "attributes": {
        "unit_of_measurement": "°C",
        "friendly_name": "Living Room Temperature"
      }
    }
  ]
]
```

## Code Examples

### C++ Example with libcurl

> **💡 Working Example Available:** A complete, ready-to-compile example is available in [`examples/ha_rest_curl_example.cpp`](examples/ha_rest_curl_example.cpp). See [`examples/README.md`](examples/README.md) for build instructions and usage guide.

**Basic Example:**

```cpp
#include <curl/curl.h>
#include <string>
#include <iostream>

// Callback function for curl to write response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string getSensorData(const std::string& haUrl, const std::string& token, const std::string& entityId) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if(curl) {
        std::string url = haUrl + "/api/states/" + entityId;
        std::string authHeader = "Authorization: Bearer " + token;
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    curl_global_cleanup();
    
    return readBuffer;
}

int main() {
    std::string haUrl = "http://192.168.1.100:8123";
    std::string token = "YOUR_LONG_LIVED_ACCESS_TOKEN";
    std::string entityId = "sensor.living_room_temperature";
    
    std::string response = getSensorData(haUrl, token, entityId);
    std::cout << "Response: " << response << std::endl;
    
    return 0;
}
```

**Compile:**
```bash
g++ -o ha_client ha_client.cpp -lcurl
```

### Python Example

```python
import requests
import json

class HARestClient:
    def __init__(self, base_url, token):
        self.base_url = base_url.rstrip('/')
        self.token = token
        self.headers = {
            'Authorization': f'Bearer {token}',
            'Content-Type': 'application/json'
        }
    
    def get_sensor_state(self, entity_id):
        """Get current state of a sensor."""
        url = f"{self.base_url}/api/states/{entity_id}"
        response = requests.get(url, headers=self.headers)
        response.raise_for_status()
        return response.json()
    
    def get_all_sensors(self):
        """Get all sensor states."""
        url = f"{self.base_url}/api/states"
        response = requests.get(url, headers=self.headers)
        response.raise_for_status()
        
        all_states = response.json()
        return [s for s in all_states if s['entity_id'].startswith('sensor.')]
    
    def get_history(self, entity_id, start_time):
        """Get historical data for a sensor."""
        url = f"{self.base_url}/api/history/period/{start_time}"
        params = {'filter_entity_id': entity_id}
        response = requests.get(url, headers=self.headers, params=params)
        response.raise_for_status()
        return response.json()

# Usage example
if __name__ == "__main__":
    client = HARestClient(
        base_url="http://192.168.1.100:8123",
        token="YOUR_LONG_LIVED_ACCESS_TOKEN"
    )
    
    # Get single sensor
    temp = client.get_sensor_state("sensor.living_room_temperature")
    print(f"Temperature: {temp['state']} {temp['attributes']['unit_of_measurement']}")
    
    # Get all sensors
    sensors = client.get_all_sensors()
    for sensor in sensors:
        entity_id = sensor['entity_id']
        state = sensor['state']
        unit = sensor['attributes'].get('unit_of_measurement', '')
        print(f"{entity_id}: {state} {unit}")
    
    # Get historical data
    from datetime import datetime, timedelta
    start_time = (datetime.now() - timedelta(hours=24)).isoformat()
    history = client.get_history("sensor.energy_consumption", start_time)
    print(f"Found {len(history[0])} historical records")
```

### JavaScript/Node.js Example

```javascript
const axios = require('axios');

class HARestClient {
    constructor(baseUrl, token) {
        this.baseUrl = baseUrl.replace(/\/$/, '');
        this.headers = {
            'Authorization': `Bearer ${token}`,
            'Content-Type': 'application/json'
        };
    }

    async getSensorState(entityId) {
        const url = `${this.baseUrl}/api/states/${entityId}`;
        const response = await axios.get(url, { headers: this.headers });
        return response.data;
    }

    async getAllSensors() {
        const url = `${this.baseUrl}/api/states`;
        const response = await axios.get(url, { headers: this.headers });
        return response.data.filter(entity => entity.entity_id.startsWith('sensor.'));
    }

    async getHistory(entityId, startTime) {
        const url = `${this.baseUrl}/api/history/period/${startTime}`;
        const response = await axios.get(url, {
            headers: this.headers,
            params: { filter_entity_id: entityId }
        });
        return response.data;
    }
}

// Usage example
(async () => {
    const client = new HARestClient(
        'http://192.168.1.100:8123',
        'YOUR_LONG_LIVED_ACCESS_TOKEN'
    );

    try {
        // Get single sensor
        const temp = await client.getSensorState('sensor.living_room_temperature');
        console.log(`Temperature: ${temp.state} ${temp.attributes.unit_of_measurement}`);

        // Get all sensors
        const sensors = await client.getAllSensors();
        sensors.forEach(sensor => {
            const unit = sensor.attributes.unit_of_measurement || '';
            console.log(`${sensor.entity_id}: ${sensor.state} ${unit}`);
        });

        // Get historical data
        const yesterday = new Date(Date.now() - 24 * 60 * 60 * 1000).toISOString();
        const history = await client.getHistory('sensor.energy_consumption', yesterday);
        console.log(`Found ${history[0].length} historical records`);
    } catch (error) {
        console.error('Error:', error.message);
    }
})();
```

### Bash/curl Example

```bash
#!/bin/bash

# Configuration
HA_URL="http://192.168.1.100:8123"
TOKEN="YOUR_LONG_LIVED_ACCESS_TOKEN"

# Function to get sensor state
get_sensor() {
    local entity_id=$1
    curl -s -X GET \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        "$HA_URL/api/states/$entity_id"
}

# Function to get all sensors
get_all_sensors() {
    curl -s -X GET \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        "$HA_URL/api/states" | jq '.[] | select(.entity_id | startswith("sensor."))'
}

# Function to get historical data
get_history() {
    local entity_id=$1
    local start_time=$2
    curl -s -X GET \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        "$HA_URL/api/history/period/$start_time?filter_entity_id=$entity_id"
}

# Usage examples
echo "=== Single Sensor ==="
get_sensor "sensor.living_room_temperature" | jq '.'

echo ""
echo "=== All Sensors ==="
get_all_sensors | jq -r '"\(.entity_id): \(.state) \(.attributes.unit_of_measurement // "")"'

echo ""
echo "=== Historical Data (last 24 hours) ==="
START_TIME=$(date -u -d '24 hours ago' '+%Y-%m-%dT%H:%M:%S+00:00')
get_history "sensor.energy_consumption" "$START_TIME" | jq '.[0] | length'
```

## Best Practices

### 1. Error Handling

Always check HTTP response codes:
- **200 OK** - Success
- **401 Unauthorized** - Invalid or expired token
- **404 Not Found** - Entity doesn't exist
- **500 Internal Server Error** - HA server error

```cpp
// Example error handling in C++
if (httpCode == 200) {
    // Parse and use data
} else if (httpCode == 401) {
    std::cerr << "Authentication failed - check your token" << std::endl;
} else if (httpCode == 404) {
    std::cerr << "Entity not found" << std::endl;
} else {
    std::cerr << "HTTP error: " << httpCode << std::endl;
}
```

### 2. Rate Limiting

Be respectful of Home Assistant's resources:
- **Don't poll faster than once per second**
- **Use MQTT for real-time updates** instead of polling
- **Batch requests** when possible (get all states instead of individual queries)
- **Cache results** for frequently accessed data

### 3. JSON Parsing

Always validate JSON responses:
```cpp
// Check if required fields exist
if (json.contains("state") && json.contains("entity_id")) {
    // Safe to use
} else {
    // Handle missing fields
}
```

### 4. Timeout Configuration

Set appropriate timeouts to avoid hanging:
```cpp
curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);      // 10 second timeout
curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // 5 second connect timeout
```

### 5. HTTPS in Production

Always use HTTPS in production:
```cpp
std::string haUrl = "https://your-domain.duckdns.org:8123";

// Verify SSL certificate
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
```

### 6. Configuration Management

Store credentials securely:
```cpp
// Read from environment variable
const char* token = std::getenv("HA_TOKEN");
if (!token) {
    std::cerr << "HA_TOKEN environment variable not set" << std::endl;
    return 1;
}

// Or from config file with restricted permissions
// chmod 600 config.ini
```

## Error Handling

### Common Errors and Solutions

#### 1. 401 Unauthorized
```
{"message": "Invalid token"}
```
**Solution:** 
- Check token is correct
- Ensure no extra spaces in Authorization header
- Token may have been revoked - create a new one

#### 2. 404 Not Found
```
{"message": "Entity not found: sensor.unknown"}
```
**Solution:**
- Verify entity_id is correct (check in HA UI)
- Entity may have been renamed or removed
- Check for typos in entity name

#### 3. Connection Refused
```
curl: (7) Failed to connect to 192.168.1.100 port 8123
```
**Solution:**
- Verify Home Assistant is running
- Check IP address and port
- Ensure firewall allows connections
- If using Docker, check network settings

#### 4. SSL Certificate Errors
```
curl: (60) SSL certificate problem: unable to get local issuer certificate
```
**Solution:**
- For self-signed certificates, use `-k` flag (insecure) for testing
- In production, install proper certificates
- Or disable SSL verification (not recommended for production)

### Robust Error Handling Example

```cpp
class HARestClient {
public:
    struct Response {
        bool success;
        int httpCode;
        std::string data;
        std::string error;
    };
    
    Response getSensorState(const std::string& entityId) {
        Response resp;
        resp.success = false;
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            resp.error = "Failed to initialize CURL";
            return resp;
        }
        
        std::string url = baseUrl_ + "/api/states/" + entityId;
        std::string authHeader = "Authorization: Bearer " + token_;
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            resp.error = curl_easy_strerror(res);
        } else {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.httpCode);
            
            if (resp.httpCode == 200) {
                resp.success = true;
            } else if (resp.httpCode == 401) {
                resp.error = "Authentication failed - invalid token";
            } else if (resp.httpCode == 404) {
                resp.error = "Entity not found: " + entityId;
            } else {
                resp.error = "HTTP error " + std::to_string(resp.httpCode);
            }
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return resp;
    }
    
private:
    std::string baseUrl_;
    std::string token_;
};
```

## Production Deployment

### 1. Dependencies Installation

**C++ with libcurl:**
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# Fedora/RHEL
sudo dnf install libcurl-devel

# macOS
brew install curl
```

**Python:**
```bash
pip install requests
```

**Node.js:**
```bash
npm install axios
```

### 2. Security Checklist

- [ ] Use HTTPS for Home Assistant
- [ ] Store tokens in environment variables or encrypted config
- [ ] Set appropriate file permissions (chmod 600 for config files)
- [ ] Enable SSL certificate verification
- [ ] Use token rotation policy
- [ ] Monitor for unauthorized access
- [ ] Log API requests for audit trail

### 3. Performance Optimization

**Use Connection Pooling:**
```cpp
// Reuse CURL handle for multiple requests
CURL* curl = curl_easy_init();
// Make multiple requests...
curl_easy_cleanup(curl); // Cleanup once at the end
```

**Implement Caching:**
```cpp
struct CachedState {
    std::string state;
    std::time_t timestamp;
    int cacheSeconds = 5; // Cache for 5 seconds
};

std::map<std::string, CachedState> cache;
```

**Batch Requests:**
```cpp
// Instead of multiple individual requests:
// getSensorState("sensor.temp1");
// getSensorState("sensor.temp2");
// getSensorState("sensor.temp3");

// Do one request and filter:
auto allStates = getAllStates();
// Filter client-side for needed sensors
```

### 4. Monitoring and Logging

```cpp
#include <fstream>
#include <ctime>

void logRequest(const std::string& method, const std::string& endpoint, int httpCode) {
    std::ofstream logFile("ha_api.log", std::ios::app);
    std::time_t now = std::time(nullptr);
    char timestamp[100];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    
    logFile << timestamp << " " << method << " " << endpoint 
            << " HTTP " << httpCode << std::endl;
}
```

## Integration with This Project

### Adding REST API Support

The existing `HTTPClient` class can be extended for Home Assistant REST API:

1. **Extend HTTPClient** or create `HARestClient` inheriting from it
2. **Add authentication** header support
3. **Implement sensor data methods**
4. **Parse JSON responses** (consider using nlohmann/json library)
5. **Update EnergyOptimizer** to use REST API as alternative to MQTT

### Comparison: REST API vs MQTT

| Feature | REST API | MQTT |
|---------|----------|------|
| **Use Case** | Polling, one-time queries | Real-time updates |
| **Latency** | Higher (request/response) | Lower (push notifications) |
| **Efficiency** | Less efficient for frequent updates | Highly efficient |
| **Simplicity** | Simpler (HTTP requests) | More complex (broker, subscriptions) |
| **Historical Data** | Excellent (history endpoint) | Not available |
| **State Queries** | Excellent | Requires waiting for update |

**Recommendation:** Use both!
- **REST API** for initial state synchronization and historical data
- **MQTT** for real-time updates during operation

## Troubleshooting

### Testing Your Connection

```bash
# Test 1: Can you reach Home Assistant?
ping 192.168.1.100

# Test 2: Is the API responding?
curl http://192.168.1.100:8123/api/

# Test 3: Is authentication working?
curl -H "Authorization: Bearer YOUR_TOKEN" \
     http://192.168.1.100:8123/api/states

# Test 4: Can you get a specific sensor?
curl -H "Authorization: Bearer YOUR_TOKEN" \
     http://192.168.1.100:8123/api/states/sensor.living_room_temperature
```

### Enable Debug Logging

**Python:**
```python
import logging
logging.basicConfig(level=logging.DEBUG)
```

**curl:**
```bash
curl -v -H "Authorization: Bearer YOUR_TOKEN" http://192.168.1.100:8123/api/states
```

**C++ with libcurl:**
```cpp
curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
```

### Common Integration Issues

1. **"Connection refused"**
   - Check Home Assistant is running: `systemctl status home-assistant`
   - Verify port 8123 is open

2. **"Invalid token"**
   - Token may contain hidden characters - copy carefully
   - Try creating a new token

3. **Empty response**
   - Check entity_id exists in HA
   - Use `/api/states` to list all entities

4. **Slow responses**
   - HA database may be large - consider cleanup
   - Check network latency
   - Consider using MQTT instead

## Additional Resources

### Official Documentation
- [Home Assistant REST API](https://developers.home-assistant.io/docs/api/rest/)
- [Authentication](https://developers.home-assistant.io/docs/auth_api/)
- [Entity Registry](https://developers.home-assistant.io/docs/entity_registry_index/)

### Libraries
- **C++:** [libcurl](https://curl.se/libcurl/), [cpp-httplib](https://github.com/yhirose/cpp-httplib), [nlohmann/json](https://github.com/nlohmann/json)
- **Python:** [requests](https://requests.readthedocs.io/), [aiohttp](https://docs.aiohttp.org/)
- **Node.js:** [axios](https://axios-http.com/), [node-fetch](https://www.npmjs.com/package/node-fetch)

### Related Guides in This Repository
- [HA_MQTT_INTEGRATION.md](HA_MQTT_INTEGRATION.md) - MQTT integration guide
- [MQTT_MOSQUITTO_GUIDE.md](MQTT_MOSQUITTO_GUIDE.md) - MQTT broker setup
- [SENSOR_STATE_PUBLISHING.md](SENSOR_STATE_PUBLISHING.md) - Publishing sensor data

## Summary

The Home Assistant REST API provides a powerful way to extract sensor data programmatically. Key takeaways:

✅ **Simple HTTP requests** with Bearer token authentication  
✅ **Get current state** of any entity with `/api/states/<entity_id>`  
✅ **Historical data** via `/api/history/period` endpoint  
✅ **List all entities** with `/api/states`  
✅ **Execute commands** with service calls  
✅ **Works with any HTTP client** - curl, Python requests, C++ libcurl, etc.  

For this home automation project:
- Use **REST API** for initial state and historical data
- Use **MQTT** for real-time updates (see HA_MQTT_INTEGRATION.md)
- Consider implementing both for a robust system

Happy automating! 🏡⚡
