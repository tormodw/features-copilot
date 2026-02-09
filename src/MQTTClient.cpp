// MQTTClient.cpp - Mock Implementation for Simulation
#include "MQTTClient.h"
#include <sstream>
#include <vector>
#include <iostream>
#include <cstring>

MQTTClient::MQTTClient(const std::string& brokerAddress, int port)
    : brokerAddress_(brokerAddress), port_(port), connected_(false), mosq_(nullptr) {
    std::cout << "MQTTClient: Initialized (mock mode - no real broker connection)" << std::endl;
    std::cout << "  Broker: " << brokerAddress << ":" << port << std::endl;
}

MQTTClient::~MQTTClient() {
    if (connected_) {
        disconnect();
    }
}

bool MQTTClient::connect() {
    std::cout << "MQTTClient: Connected to mock MQTT broker at " 
              << brokerAddress_ << ":" << port_ << std::endl;
    connected_ = true;
    return true;
}

void MQTTClient::disconnect() {
    if (connected_) {
        std::cout << "MQTTClient: Disconnected from mock MQTT broker" << std::endl;
        connected_ = false;
        subscriptions_.clear();
    }
}

bool MQTTClient::isConnected() const {
    return connected_;
}

void MQTTClient::subscribe(const std::string& topic, MessageCallback callback) {
    if (connected_) {
        subscriptions_[topic] = callback;
        std::cout << "MQTTClient: Subscribed to topic: " << topic << std::endl;
    }
}

void MQTTClient::publish(const std::string& topic, const std::string& payload) {
    if (connected_) {
        std::cout << "MQTTClient: Published to topic '" << topic << "': " << payload << std::endl;
    } else {
        std::cerr << "MQTTClient: Cannot publish - not connected" << std::endl;
    }
}

void MQTTClient::processMessages() {
    // In mock mode, messages are handled by simulateMessage()
    // This method is kept for interface compatibility
}

void MQTTClient::simulateMessage(const std::string& topic, const std::string& payload) {
    if (!connected_) {
        return;
    }
    
    std::cout << "MQTTClient: Simulating message on topic '" << topic << "'" << std::endl;
    
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
