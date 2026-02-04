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

    mosquitto_username_pw_set(mosq_, "tormod", "swucriUedi!56");
    
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
    (void) mosq;
    (void) obj;
    if (result == 0) {
        std::cout << "Successfully connected to MQTT broker" << std::endl;
    } else {
        std::cerr << "Connection failed with code: " << result << std::endl;
    }
}

void MQTTClient::on_message_callback(struct mosquitto* mosq, void* obj, 
                                     const struct mosquitto_message* message) {
    (void) mosq;
    MQTTClient* client = static_cast<MQTTClient*>(obj);
    
    std::string topic(message->topic);
    std::string payload(static_cast<char*>(message->payload), message->payloadlen);
    
    // Find and call the appropriate callback
    auto it = client->subscriptions_.find(topic);
    if (it != client->subscriptions_.end()) {
        it->second(topic, payload);
    }
}