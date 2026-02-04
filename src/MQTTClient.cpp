#include "MQTTClient.h"
#include <sstream>
#include <vector>

MQTTClient::MQTTClient(const std::string& brokerAddress, int port)
    : brokerAddress_(brokerAddress), port_(port), connected_(false) {}

bool MQTTClient::connect() {
    // Simulate connection to MQTT broker
    connected_ = true;
    return connected_;
}

void MQTTClient::disconnect() {
    connected_ = false;
    subscriptions_.clear();
}

bool MQTTClient::isConnected() const {
    return connected_;
}

void MQTTClient::subscribe(const std::string& topic, MessageCallback callback) {
    if (connected_) {
        subscriptions_[topic] = callback;
    }
}

void MQTTClient::publish(const std::string& topic, const std::string& payload) {
    if (connected_) {
        // In production: send MQTT message
        // For simulation: topic and payload would be sent to broker
    }
}

void MQTTClient::processMessages() {
    // In production: this would be called by MQTT library callback
    // For simulation: this would process received messages
    if (connected_) {
        // Process any pending messages
    }
}

void MQTTClient::simulateMessage(const std::string& topic, const std::string& payload) {
    if (!connected_) {
        return;
    }
    
    // Find matching subscriptions and call their callbacks
    for (const auto& sub : subscriptions_) {
        if (topicMatches(sub.first, topic)) {
            sub.second(topic, payload);
        }
    }
}

bool MQTTClient::topicMatches(const std::string& pattern, const std::string& topic) const {
    // Simple MQTT topic matching with + and # wildcards
    // + matches a single level
    // # matches multiple levels (must be at end)
    
    if (pattern == topic) {
        return true;
    }
    
    // Check for # wildcard (matches everything after this point)
    size_t hashPos = pattern.find('#');
    if (hashPos != std::string::npos) {
        // # must be at the end
        if (hashPos == pattern.length() - 1) {
            std::string prefix = pattern.substr(0, hashPos);
            return topic.find(prefix) == 0;
        }
        // Invalid pattern if # is not at the end
        return false;
    }
    
    // Check for + wildcard (single level match)
    size_t plusPos = pattern.find('+');
    if (plusPos != std::string::npos) {
        // Split both pattern and topic by '/'
        std::vector<std::string> patternParts;
        std::vector<std::string> topicParts;
        
        std::stringstream patternStream(pattern);
        std::stringstream topicStream(topic);
        std::string part;
        
        while (std::getline(patternStream, part, '/')) {
            patternParts.push_back(part);
        }
        while (std::getline(topicStream, part, '/')) {
            topicParts.push_back(part);
        }
        
        if (patternParts.size() != topicParts.size()) {
            return false;
        }
        
        for (size_t i = 0; i < patternParts.size(); ++i) {
            if (patternParts[i] != "+" && patternParts[i] != topicParts[i]) {
                return false;
            }
        }
        
        return true;
    }
    
    return false;
}
