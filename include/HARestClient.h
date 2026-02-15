#ifndef HA_REST_CLIENT_H
#define HA_REST_CLIENT_H

#include <curl/curl.h>
#include <string>
#include <vector>
#include <functional>
#include <map>

// Structure to hold sensor data from Home Assistant
struct HASensorData {
    std::string entityId;
    std::string state;
    std::string unitOfMeasurement;
    std::string friendlyName;
    std::string deviceClass;
    long lastChanged;
    long lastUpdated;
};

// Structure to hold historical data point
struct HAHistoricalData {
    std::string entityId;
    std::string state;
    long timestamp;
    std::map<std::string, std::string> attributes;
};

// Home Assistant REST API Client
// Provides methods to extract sensor data from Home Assistant using RESTful API
// In production, this would use libcurl or similar HTTP library
class HARestClient {
public:
    HARestClient(const std::string& baseUrl, const std::string& token);

    ~HARestClient();
    
    // Get current state of a specific sensor
    HASensorData getSensorState(const std::string& entityId);
    
    // Get all sensors from Home Assistant
    std::vector<HASensorData> getAllSensors();
    
    // Get historical data for a sensor
    std::vector<HAHistoricalData> getHistory(const std::string& entityId, long startTimestamp);
    
    // Get all entities (sensors, switches, lights, etc.)
    std::vector<HASensorData> getAllStates();
    
    // Call a service (e.g., turn on a switch)
    bool callService(const std::string& domain, const std::string& service, 
                    const std::string& entityId, const std::string& data = "");
    
    // Check if API is accessible
    bool testConnection();

private:
    std::string baseUrl_;
    std::string token_;
    
    // Helper methods for HTTP operations
    std::string httpGet(const std::string& endpoint);
    std::string httpPost(const std::string& endpoint, const std::string& data);
    
    // Parse JSON response into sensor data
    HASensorData parseSensorData(const std::string& jsonResponse);
    std::vector<HASensorData> parseMultipleSensors(const std::string& jsonResponse);
    std::vector<HAHistoricalData> parseHistoricalData(const std::string& jsonResponse);
    
    // Simple JSON value extractor (in production, use proper JSON library)
    std::string extractJsonValue(const std::string& json, const std::string& key);
};

#endif // HA_REST_CLIENT_H
