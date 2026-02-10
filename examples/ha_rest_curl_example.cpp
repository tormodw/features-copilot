/**
 * Home Assistant REST API - libcurl Example
 * 
 * This example demonstrates how to use libcurl to interact with the Home Assistant
 * REST API to extract sensor data and control devices.
 * 
 * Features:
 * - Get single sensor state
 * - Get all sensors
 * - Get historical data
 * - Call services (control devices)
 * - Proper error handling
 * - JSON response parsing (simplified)
 * 
 * Compilation:
 *   g++ -o ha_rest_example ha_rest_curl_example.cpp -lcurl
 * 
 * Usage:
 *   export HA_TOKEN="your_long_lived_access_token"
 *   export HA_URL="http://192.168.1.100:8123"
 *   ./ha_rest_example
 */

#include <curl/curl.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sstream>

// Callback function for curl to write response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/**
 * Home Assistant REST API Client using libcurl
 */
class HARestClient {
public:
    HARestClient(const std::string& baseUrl, const std::string& token)
        : baseUrl_(baseUrl), token_(token) {
        // Initialize curl globally (once per program)
        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        // Remove trailing slash from base URL
        if (!baseUrl_.empty() && baseUrl_.back() == '/') {
            baseUrl_.pop_back();
        }
    }
    
    ~HARestClient() {
        // Cleanup curl globally
        curl_global_cleanup();
    }
    
    /**
     * Get the current state of a specific sensor
     * 
     * @param entityId The entity ID (e.g., "sensor.living_room_temperature")
     * @return JSON response as string
     */
    std::string getSensorState(const std::string& entityId) {
        std::string url = baseUrl_ + "/api/states/" + entityId;
        return httpGet(url);
    }
    
    /**
     * Get all entity states from Home Assistant
     * 
     * @return JSON array of all entity states
     */
    std::string getAllStates() {
        std::string url = baseUrl_ + "/api/states";
        return httpGet(url);
    }
    
    /**
     * Get historical data for a sensor
     * 
     * @param entityId The entity ID
     * @param startTime ISO 8601 timestamp (e.g., "2024-01-15T00:00:00+00:00")
     * @return JSON array of historical states
     */
    std::string getHistory(const std::string& entityId, const std::string& startTime) {
        std::string url = baseUrl_ + "/api/history/period/" + startTime + 
                         "?filter_entity_id=" + entityId;
        return httpGet(url);
    }
    
    /**
     * Call a Home Assistant service (control a device)
     * 
     * @param domain Service domain (e.g., "switch", "light")
     * @param service Service name (e.g., "turn_on", "turn_off")
     * @param entityId Target entity ID
     * @param extraData Additional JSON data (optional)
     * @return JSON response from the service call
     */
    std::string callService(const std::string& domain, const std::string& service,
                           const std::string& entityId, const std::string& extraData = "") {
        std::string url = baseUrl_ + "/api/services/" + domain + "/" + service;
        
        // Build JSON payload
        std::string payload = "{\"entity_id\": \"" + entityId + "\"";
        if (!extraData.empty()) {
            payload += ", " + extraData;
        }
        payload += "}";
        
        return httpPost(url, payload);
    }
    
    /**
     * Test connection to Home Assistant API
     * 
     * @return true if API is accessible
     */
    bool testConnection() {
        try {
            std::string url = baseUrl_ + "/api/";
            std::string response = httpGet(url);
            return !response.empty() && 
                   (response.find("API running") != std::string::npos ||
                    response.find("message") != std::string::npos);
        } catch (...) {
            return false;
        }
    }

private:
    std::string baseUrl_;
    std::string token_;
    
    /**
     * Perform HTTP GET request
     * 
     * @param url Full URL to request
     * @return Response body as string
     */
    std::string httpGet(const std::string& url) {
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
    std::string httpPost(const std::string& url, const std::string& data) {
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
};

/**
 * Helper function to extract a simple JSON value
 * Note: This is a simplified parser. In production, use a proper JSON library
 * like nlohmann/json (https://github.com/nlohmann/json)
 */
std::string extractJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    
    if (keyPos == std::string::npos) {
        return "";
    }
    
    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos) {
        return "";
    }
    
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() && 
           (json[valueStart] == ' ' || json[valueStart] == '\t' || json[valueStart] == '\n')) {
        valueStart++;
    }
    
    if (valueStart >= json.length()) {
        return "";
    }
    
    if (json[valueStart] == '"') {
        size_t valueEnd = json.find('"', valueStart + 1);
        if (valueEnd != std::string::npos) {
            return json.substr(valueStart + 1, valueEnd - valueStart - 1);
        }
    } else {
        size_t valueEnd = valueStart;
        while (valueEnd < json.length() && 
               json[valueEnd] != ',' && json[valueEnd] != '}' && 
               json[valueEnd] != ']' && json[valueEnd] != '\n') {
            valueEnd++;
        }
        
        std::string value = json.substr(valueStart, valueEnd - valueStart);
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }
        return value;
    }
    
    return "";
}

/**
 * Print a formatted sensor state from JSON response
 */
void printSensorState(const std::string& json) {
    std::string entityId = extractJsonValue(json, "entity_id");
    std::string state = extractJsonValue(json, "state");
    std::string unit = extractJsonValue(json, "unit_of_measurement");
    std::string friendlyName = extractJsonValue(json, "friendly_name");
    
    std::cout << "Entity: " << entityId << std::endl;
    if (!friendlyName.empty()) {
        std::cout << "Name: " << friendlyName << std::endl;
    }
    std::cout << "State: " << state;
    if (!unit.empty()) {
        std::cout << " " << unit;
    }
    std::cout << std::endl;
}

/**
 * Main function demonstrating various API calls
 */
int main() {
    // Read configuration from environment variables
    const char* haUrl = std::getenv("HA_URL");
    const char* haToken = std::getenv("HA_TOKEN");
    
    // Default values for demonstration (replace with your own)
    std::string baseUrl = haUrl ? haUrl : "http://192.168.1.100:8123";
    std::string token = haToken ? haToken : "YOUR_LONG_LIVED_ACCESS_TOKEN";
    
    if (token == "YOUR_LONG_LIVED_ACCESS_TOKEN") {
        std::cerr << "WARNING: Using default token. Set HA_TOKEN environment variable." << std::endl;
        std::cerr << "Example: export HA_TOKEN=\"your_actual_token_here\"" << std::endl;
        std::cerr << std::endl;
    }
    
    // Create REST client
    HARestClient client(baseUrl, token);
    
    std::cout << "=== Home Assistant REST API Example ===" << std::endl;
    std::cout << "Connecting to: " << baseUrl << std::endl;
    std::cout << std::endl;
    
    // Example 1: Test connection
    std::cout << "1. Testing connection..." << std::endl;
    if (client.testConnection()) {
        std::cout << "   ✓ Connected successfully!" << std::endl;
    } else {
        std::cout << "   ✗ Connection failed!" << std::endl;
        std::cout << "   Make sure Home Assistant is running and accessible." << std::endl;
        return 1;
    }
    std::cout << std::endl;
    
    // Example 2: Get single sensor state
    std::cout << "2. Getting temperature sensor state..." << std::endl;
    std::string tempResponse = client.getSensorState("sensor.shellyhtg3_e4b3232d5348_temperature");
    if (!tempResponse.empty() && tempResponse.find("entity_id") != std::string::npos) {
        std::cout << "   Response: " << std::endl;
        printSensorState(tempResponse);
    } else {
        std::cout << "   Entity not found or error occurred" << std::endl;
    }
    std::cout << std::endl;
    
    // Example 3: Get another sensor
    std::cout << "3. Getting energy consumption sensor..." << std::endl;
    std::string energyResponse = client.getSensorState("sensor.eva_meter_reader_summation_delivered");
    if (!energyResponse.empty() && energyResponse.find("entity_id") != std::string::npos) {
        std::cout << "   Response: " << std::endl;
        printSensorState(energyResponse);
    } else {
        std::cout << "   Entity not found or error occurred" << std::endl;
    }
    std::cout << std::endl;
    
    // Example 4: Get all states
    std::cout << "4. Getting all entity states..." << std::endl;
    std::string allStates = client.getAllStates();
    if (!allStates.empty() && allStates.find("[") != std::string::npos) {
        // Count sensors (simplified)
        size_t sensorCount = 0;
        size_t pos = 0;
        while ((pos = allStates.find("\"sensor.", pos)) != std::string::npos) {
            sensorCount++;
            pos++;
        }
        std::cout << "   Found approximately " << sensorCount << " sensors" << std::endl;
        std::cout << "   (Showing first 500 characters of response)" << std::endl;
        std::cout << "   " << allStates.substr(0, 500) << "..." << std::endl;
    } else {
        std::cout << "   Failed to get states" << std::endl;
    }
    std::cout << std::endl;
    
    // Example 5: Get historical data
    std::cout << "5. Getting historical data (last 24 hours)..." << std::endl;
    // ISO 8601 format: 2024-01-15T00:00:00+00:00
    std::string startTime = "2026-01-15T00:00:00+00:00";
    std::string history = client.getHistory("sensor.eva_meter_reader_summation_delivered", startTime);
    if (!history.empty() && history.find("[") != std::string::npos) {
        // Count data points (simplified)
        size_t pointCount = 0;
        size_t pos = 0;
        while ((pos = history.find("\"state\"", pos)) != std::string::npos) {
            pointCount++;
            pos++;
        }
        std::cout << "   Found approximately " << pointCount << " historical data points" << std::endl;
        std::cout << "   (Showing first 600 characters of response)" << std::endl;
        std::cout << "   " << history.substr(0, 600) << "..." << std::endl;
    } else {
        std::cout << "   Failed to get history" << std::endl;
    }
    std::cout << std::endl;
    
    // Example 6: Call a service (turn on a switch)
    std::cout << "6. Calling service to turn on heater switch..." << std::endl;
    std::string serviceResponse = client.callService("switch", "turn_on", "switch.heater");
    if (!serviceResponse.empty()) {
        std::cout << "   Service called successfully" << std::endl;
        std::cout << "   Response: " << serviceResponse << std::endl;
    } else {
        std::cout << "   Failed to call service" << std::endl;
    }
    std::cout << std::endl;
    
    // Example 7: Call service with extra data (set brightness)
    std::cout << "7. Calling service to set light brightness..." << std::endl;
    std::string lightResponse = client.callService("light", "turn_on", 
                                                   "light.living_room",
                                                   "\"brightness\": 128");
    if (!lightResponse.empty()) {
        std::cout << "   Service called successfully" << std::endl;
        std::cout << "   Response: " << lightResponse << std::endl;
    } else {
        std::cout << "   Failed to call service" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== Example Complete ===" << std::endl;
    std::cout << std::endl;
    std::cout << "TIP: For production use, consider:" << std::endl;
    std::cout << "  - Using a proper JSON library (nlohmann/json)" << std::endl;
    std::cout << "  - Implementing retry logic for failed requests" << std::endl;
    std::cout << "  - Adding connection pooling for multiple requests" << std::endl;
    std::cout << "  - Caching sensor states to reduce API calls" << std::endl;
    std::cout << "  - Using MQTT for real-time updates instead of polling" << std::endl;
    
    return 0;
}
