#ifndef REST_API_CLIENT_H
#define REST_API_CLIENT_H

#include <string>
#include <map>
#include <functional>
#include <memory>

/**
 * REST API Client using libcurl
 * Replaces MQTT for sensor data retrieval and appliance control
 */
class RestApiClient {
public:
    RestApiClient(const std::string& baseUrl);
    ~RestApiClient();
    
    // Configure authentication (optional)
    void setAuthentication(const std::string& token);
    void setBasicAuth(const std::string& username, const std::string& password);
    
    // HTTP Methods
    std::string get(const std::string& endpoint);
    std::string post(const std::string& endpoint, const std::string& data);
    std::string put(const std::string& endpoint, const std::string& data);
    
    // Sensor-specific methods
    std::string getSensorState(const std::string& sensorId);
    std::map<std::string, std::string> getAllSensors();
    
    // Appliance-specific methods
    bool setApplianceState(const std::string& applianceId, bool turnOn);
    std::string getApplianceState(const std::string& applianceId);
    
    // Connection status
    bool isConnected() const;
    std::string getLastError() const;

private:
    std::string baseUrl_;
    std::string authToken_;
    std::string username_;
    std::string password_;
    bool useBasicAuth_;
    std::string lastError_;
    
    // Internal helper for making HTTP requests
    std::string makeRequest(const std::string& url, const std::string& method, 
                           const std::string& data = "");
    
    // URL encoding helper
    std::string urlEncode(const std::string& str);
    
    // libcurl callback
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif // REST_API_CLIENT_H
