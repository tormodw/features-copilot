#include "RestApiClient.h"
#include <iostream>
#include <sstream>
#include <curl/curl.h>

RestApiClient::RestApiClient(const std::string& baseUrl) 
    : baseUrl_(baseUrl)
    , useBasicAuth_(false) {
    // Initialize libcurl globally (should be done once)
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

RestApiClient::~RestApiClient() {
    // Cleanup libcurl
    curl_global_cleanup();
}

void RestApiClient::setAuthentication(const std::string& token) {
    authToken_ = token;
    useBasicAuth_ = false;
}

void RestApiClient::setBasicAuth(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
    useBasicAuth_ = true;
}

size_t RestApiClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string RestApiClient::makeRequest(const std::string& url, const std::string& method, 
                                       const std::string& data) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    if (!curl) {
        lastError_ = "Failed to initialize CURL";
        return "";
    }
    
    try {
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // Add authentication
        if (!authToken_.empty()) {
            std::string authHeader = "Authorization: Bearer " + authToken_;
            headers = curl_slist_append(headers, authHeader.c_str());
        }
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        // Set basic authentication if configured
        if (useBasicAuth_ && !username_.empty()) {
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
            curl_easy_setopt(curl, CURLOPT_USERNAME, username_.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password_.c_str());
        }
        
        // Set HTTP method
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        } else if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        } else if (method == "GET") {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        }
        
        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            lastError_ = curl_easy_strerror(res);
            response = "";
        } else {
            lastError_ = "";
        }
        
        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
    } catch (const std::exception& e) {
        lastError_ = e.what();
        curl_easy_cleanup(curl);
        return "";
    }
    
    return response;
}

std::string RestApiClient::get(const std::string& endpoint) {
    std::string url = baseUrl_ + endpoint;
    return makeRequest(url, "GET");
}

std::string RestApiClient::post(const std::string& endpoint, const std::string& data) {
    std::string url = baseUrl_ + endpoint;
    return makeRequest(url, "POST", data);
}

std::string RestApiClient::put(const std::string& endpoint, const std::string& data) {
    std::string url = baseUrl_ + endpoint;
    return makeRequest(url, "PUT", data);
}

std::string RestApiClient::getSensorState(const std::string& sensorId) {
    std::string endpoint = "/api/states/" + urlEncode(sensorId);
    std::string response = get(endpoint);
    
    // Parse JSON response to extract state value
    // For now, return raw response - can be enhanced with JSON parsing
    return response;
}

std::map<std::string, std::string> RestApiClient::getAllSensors() {
    std::map<std::string, std::string> sensors;
    std::string response = get("/api/states");
    
    // Parse JSON response to build sensor map
    // For now, return empty map - can be enhanced with JSON parsing
    return sensors;
}

bool RestApiClient::setApplianceState(const std::string& applianceId, bool turnOn) {
    std::string service = turnOn ? "turn_on" : "turn_off";
    std::string endpoint = "/api/services/switch/" + service;
    
    // Build JSON data
    std::ostringstream json;
    json << "{\"entity_id\": \"" << applianceId << "\"}";
    
    std::string response = post(endpoint, json.str());
    return !response.empty() && lastError_.empty();
}

std::string RestApiClient::getApplianceState(const std::string& applianceId) {
    return getSensorState(applianceId);
}

bool RestApiClient::isConnected() const {
    return lastError_.empty();
}

std::string RestApiClient::getLastError() const {
    return lastError_;
}

std::string RestApiClient::urlEncode(const std::string& str) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return str;
    }
    
    char* encoded = curl_easy_escape(curl, str.c_str(), str.length());
    std::string result(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);
    
    return result;
}
