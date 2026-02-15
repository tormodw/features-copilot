#include "HARestClient.h"
#include <iostream>
#include <sstream>
#include <ctime>

// Callback function for curl to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

HARestClient::HARestClient(const std::string& baseUrl, const std::string& token)
    : baseUrl_(baseUrl), token_(token) {
    // Initialize curl globally (once per program)
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Remove trailing slash from base URL if present
    if (!baseUrl_.empty() && baseUrl_.back() == '/') {
        baseUrl_.pop_back();
    }
}

HARestClient::~HARestClient() {
        // Cleanup curl globally
        curl_global_cleanup();
}


HASensorData HARestClient::getSensorState(const std::string& entityId) {
    std::cout << "HARestClient: Fetching state for " << entityId << std::endl;
    
    std::string endpoint = baseUrl_ + "/api/states/" + entityId;
    std::string response = httpGet(endpoint);
    
    return parseSensorData(response);
}

std::vector<HASensorData> HARestClient::getAllSensors() {
    std::cout << "HARestClient: Fetching all sensors" << std::endl;
    
    std::string response = httpGet(baseUrl_ + "/api/states");
    std::vector<HASensorData> allStates = parseMultipleSensors(response);
    
    // Filter for sensors only
    std::vector<HASensorData> sensors;
    for (const auto& state : allStates) {
        if (state.entityId.find("sensor.") == 0) {
            sensors.push_back(state);
        }
    }
    
    return sensors;
}

std::vector<HASensorData> HARestClient::getAllStates() {
    std::cout << "HARestClient: Fetching all entity states" << std::endl;
    
    std::string response = httpGet(baseUrl_ + "/api/states");
    return parseMultipleSensors(response);
}

std::vector<HAHistoricalData> HARestClient::getHistory(const std::string& entityId, long startTimestamp) {
    std::cout << "HARestClient: Fetching history for " << entityId << std::endl;
    
    // Convert timestamp to ISO format
    char timeStr[100];
    std::time_t t = startTimestamp;
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%S+00:00", std::gmtime(&t));
    
    std::string endpoint = baseUrl_ + "/api/history/period/" + std::string(timeStr) + 
                          "?filter_entity_id=" + entityId;
    std::string response = httpGet(endpoint);
    
    return parseHistoricalData(response);
}

bool HARestClient::callService(const std::string& domain, const std::string& service,
                               const std::string& entityId, const std::string& data) {
    std::cout << "HARestClient: Calling service " << domain << "." << service 
              << " on " << entityId << std::endl;
    
    std::string endpoint = baseUrl_ + "/api/services/" + domain + "/" + service;
    
    // Build JSON payload
    std::string payload = "{\"entity_id\": \"" + entityId + "\"";
    if (!data.empty()) {
        payload += ", " + data;
    }
    payload += "}";
    
    std::string response = httpPost(endpoint, payload);
    
    // Check if response contains success indicators
    return !response.empty() && response.find("error") == std::string::npos;
}

bool HARestClient::testConnection() {
    std::cout << "HARestClient: Testing connection to Home Assistant" << std::endl;
    
    try {
        std::string url = baseUrl_+"/api/";
        std::string response = httpGet(url);
        return !response.empty() && 
               (response.find("API running") != std::string::npos || 
               response.find("message") != std::string::npos);
    } catch (...) {
        return false;
    }
}

// Private helper methods

/**
 * Perform HTTP GET request
 * 
 * @param url Full URL to request
 * @return Response body as string
 */
std::string HARestClient::httpGet(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    long httpCode = 0;
    
    curl = curl_easy_init();
    
    if(curl) {
        // Prepare authorization header
        std::string authHeader = "Authorization: Bearer " + token_;
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        // Set timeouts
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);           // 10 second timeout
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);     // 5 second connect timeout
        
        // For HTTPS, verify SSL certificate (set to 0 to disable for self-signed certs)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        
        // Perform the request
        res = curl_easy_perform(curl);
        
        // Check for errors
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            // Get HTTP response code
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            
            if (httpCode == 200) {
                // Success
            } else if (httpCode == 401) {
                std::cerr << "Authentication failed (401): Invalid token" << std::endl;
            } else if (httpCode == 404) {
                std::cerr << "Not found (404): Entity may not exist" << std::endl;
            } else {
                std::cerr << "HTTP error: " << httpCode << std::endl;
            }
        }
        
        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    return readBuffer;
}
    
/**
 * Perform HTTP POST request
 * 
 * @param url Full URL to request
 * @param data POST data (JSON string)
 * @return Response body as string
 */
std::string HARestClient::httpPost(const std::string& url, const std::string& data) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    long httpCode = 0;
    
    curl = curl_easy_init();
    
    if(curl) {
        // Prepare authorization header
        std::string authHeader = "Authorization: Bearer " + token_;
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        // Set POST data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        
        // Set timeouts
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        
        // For HTTPS
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        
        // Perform the request
        res = curl_easy_perform(curl);
        
        // Check for errors
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            
            if (httpCode == 200 || httpCode == 201) {
                // Success
            } else if (httpCode == 401) {
                std::cerr << "Authentication failed (401): Invalid token" << std::endl;
            } else {
                std::cerr << "HTTP error: " << httpCode << std::endl;
            }
        }
        
        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    return readBuffer;
}


#if 0
std::string HARestClient::httpGet(const std::string& endpoint) {
    // In production: Use libcurl to make actual HTTP GET request
    // For simulation: Return mock data based on endpoint
    
    std::cout << "  HTTP GET: " << baseUrl_ << endpoint << std::endl;
    std::cout << "  Authorization: Bearer " << token_.substr(0, 10) << "..." << std::endl;
    
    // Mock response based on endpoint
    if (endpoint == "/api/") {
        return R"({"message": "API running."})";
    }
    
    if (endpoint == "/api/states") {
        // Mock: Return multiple sensor states
        return R"([
            {
                "entity_id": "sensor.living_room_temperature",
                "state": "22.5",
                "attributes": {
                    "unit_of_measurement": "°C",
                    "friendly_name": "Living Room Temperature",
                    "device_class": "temperature"
                },
                "last_changed": "2024-01-15T10:30:00+00:00",
                "last_updated": "2024-01-15T10:30:00+00:00"
            },
            {
                "entity_id": "sensor.energy_consumption",
                "state": "1250",
                "attributes": {
                    "unit_of_measurement": "W",
                    "friendly_name": "Energy Consumption",
                    "device_class": "power"
                },
                "last_changed": "2024-01-15T10:25:00+00:00",
                "last_updated": "2024-01-15T10:25:00+00:00"
            },
            {
                "entity_id": "sensor.solar_production",
                "state": "3200",
                "attributes": {
                    "unit_of_measurement": "W",
                    "friendly_name": "Solar Production",
                    "device_class": "power"
                },
                "last_changed": "2024-01-15T10:28:00+00:00",
                "last_updated": "2024-01-15T10:28:00+00:00"
            },
            {
                "entity_id": "switch.heater",
                "state": "off",
                "attributes": {
                    "friendly_name": "Heater Switch"
                },
                "last_changed": "2024-01-15T09:00:00+00:00",
                "last_updated": "2024-01-15T09:00:00+00:00"
            }
        ])";
    }
    
    if (endpoint.find("/api/states/") == 0) {
        // Extract entity_id from endpoint
        std::string entityId = endpoint.substr(12); // After "/api/states/"
        
        if (entityId == "sensor.living_room_temperature") {
            return R"({
                "entity_id": "sensor.living_room_temperature",
                "state": "22.5",
                "attributes": {
                    "unit_of_measurement": "°C",
                    "friendly_name": "Living Room Temperature",
                    "device_class": "temperature"
                },
                "last_changed": "2024-01-15T10:30:00+00:00",
                "last_updated": "2024-01-15T10:30:00+00:00"
            })";
        } else if (entityId == "sensor.energy_consumption") {
            return R"({
                "entity_id": "sensor.energy_consumption",
                "state": "1250",
                "attributes": {
                    "unit_of_measurement": "W",
                    "friendly_name": "Energy Consumption",
                    "device_class": "power"
                },
                "last_changed": "2024-01-15T10:25:00+00:00",
                "last_updated": "2024-01-15T10:25:00+00:00"
            })";
        } else if (entityId == "sensor.solar_production") {
            return R"({
                "entity_id": "sensor.solar_production",
                "state": "3200",
                "attributes": {
                    "unit_of_measurement": "W",
                    "friendly_name": "Solar Production",
                    "device_class": "power"
                },
                "last_changed": "2024-01-15T10:28:00+00:00",
                "last_updated": "2024-01-15T10:28:00+00:00"
            })";
        }
    }
    
    if (endpoint.find("/api/history/period/") == 0) {
        // Mock historical data
        return R"([[
            {
                "entity_id": "sensor.energy_consumption",
                "state": "1100",
                "last_changed": "2024-01-15T08:00:00+00:00",
                "attributes": {
                    "unit_of_measurement": "W"
                }
            },
            {
                "entity_id": "sensor.energy_consumption",
                "state": "1250",
                "last_changed": "2024-01-15T09:00:00+00:00",
                "attributes": {
                    "unit_of_measurement": "W"
                }
            },
            {
                "entity_id": "sensor.energy_consumption",
                "state": "1350",
                "last_changed": "2024-01-15T10:00:00+00:00",
                "attributes": {
                    "unit_of_measurement": "W"
                }
            }
        ]])";
    }
    
    return "{}";
}

std::string HARestClient::httpPost(const std::string& endpoint, const std::string& data) {
    // In production: Use libcurl to make actual HTTP POST request
    // For simulation: Return success response
    
    std::cout << "  HTTP POST: " << baseUrl_ << endpoint << std::endl;
    std::cout << "  Authorization: Bearer " << token_.substr(0, 10) << "..." << std::endl;
    std::cout << "  Data: " << data << std::endl;
    
    // Mock: Return success for service calls
    return R"([{"success": true}])";
}

#endif

HASensorData HARestClient::parseSensorData(const std::string& jsonResponse) {
    // In production: Use proper JSON library like nlohmann/json
    // For simulation: Simple string parsing
    
    HASensorData data;
    data.entityId = extractJsonValue(jsonResponse, "entity_id");
    data.state = extractJsonValue(jsonResponse, "state");
    
    // Extract from attributes object
    size_t attrStart = jsonResponse.find("\"attributes\"");
    if (attrStart != std::string::npos) {
        size_t attrEnd = jsonResponse.find("}", attrStart);
        std::string attributes = jsonResponse.substr(attrStart, attrEnd - attrStart);
        
        data.unitOfMeasurement = extractJsonValue(attributes, "unit_of_measurement");
        data.friendlyName = extractJsonValue(attributes, "friendly_name");
        data.deviceClass = extractJsonValue(attributes, "device_class");
    }
    
    // For simulation, use current time
    data.lastChanged = std::time(nullptr);
    data.lastUpdated = std::time(nullptr);
    
    return data;
}

std::vector<HASensorData> HARestClient::parseMultipleSensors(const std::string& jsonResponse) {
    // In production: Use proper JSON library
    // For simulation: Parse array of sensor objects
    
    std::vector<HASensorData> sensors;
    
    // Find each object in the array
    size_t pos = 0;
    while ((pos = jsonResponse.find("{", pos)) != std::string::npos) {
        size_t endPos = jsonResponse.find("}", pos);
        if (endPos == std::string::npos) break;
        
        // Find the complete object (handle nested braces)
        int braceCount = 1;
        size_t searchPos = pos + 1;
        while (braceCount > 0 && searchPos < jsonResponse.length()) {
            if (jsonResponse[searchPos] == '{') braceCount++;
            else if (jsonResponse[searchPos] == '}') braceCount--;
            searchPos++;
        }
        
        if (braceCount == 0) {
            std::string objectStr = jsonResponse.substr(pos, searchPos - pos);
            HASensorData data = parseSensorData(objectStr);
            if (!data.entityId.empty()) {
                sensors.push_back(data);
            }
            pos = searchPos;
        } else {
            break;
        }
    }
    
    return sensors;
}

std::vector<HAHistoricalData> HARestClient::parseHistoricalData(const std::string& jsonResponse) {
    // In production: Use proper JSON library
    // For simulation: Parse historical data array
    
    std::vector<HAHistoricalData> history;
    
    // Find each object in the nested array
    size_t pos = 0;
    while ((pos = jsonResponse.find("{", pos)) != std::string::npos) {
        size_t endPos = jsonResponse.find("}", pos);
        if (endPos == std::string::npos) break;
        
        // Find complete object
        int braceCount = 1;
        size_t searchPos = pos + 1;
        while (braceCount > 0 && searchPos < jsonResponse.length()) {
            if (jsonResponse[searchPos] == '{') braceCount++;
            else if (jsonResponse[searchPos] == '}') braceCount--;
            searchPos++;
        }
        
        if (braceCount == 0) {
            std::string objectStr = jsonResponse.substr(pos, searchPos - pos);
            
            HAHistoricalData data;
            data.entityId = extractJsonValue(objectStr, "entity_id");
            data.state = extractJsonValue(objectStr, "state");
            data.timestamp = std::time(nullptr); // Simplified
            
            if (!data.entityId.empty()) {
                history.push_back(data);
            }
            pos = searchPos;
        } else {
            break;
        }
    }
    
    return history;
}

std::string HARestClient::extractJsonValue(const std::string& json, const std::string& key) {
    // Simple JSON value extractor for simulation
    // In production: Use proper JSON library
    
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    
    if (keyPos == std::string::npos) {
        return "";
    }
    
    // Find the colon after the key
    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos) {
        return "";
    }
    
    // Skip whitespace after colon
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() && 
           (json[valueStart] == ' ' || json[valueStart] == '\t' || json[valueStart] == '\n')) {
        valueStart++;
    }
    
    if (valueStart >= json.length()) {
        return "";
    }
    
    // Check if value is a string (starts with ")
    if (json[valueStart] == '"') {
        size_t valueEnd = json.find('"', valueStart + 1);
        if (valueEnd != std::string::npos) {
            return json.substr(valueStart + 1, valueEnd - valueStart - 1);
        }
    } else {
        // Number or other value
        size_t valueEnd = valueStart;
        while (valueEnd < json.length() && 
               json[valueEnd] != ',' && json[valueEnd] != '}' && 
               json[valueEnd] != ']' && json[valueEnd] != '\n') {
            valueEnd++;
        }
        
        std::string value = json.substr(valueStart, valueEnd - valueStart);
        // Trim trailing whitespace
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }
        return value;
    }
    
    return "";
}
