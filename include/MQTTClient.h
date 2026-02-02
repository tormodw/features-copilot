#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <string>
#include <functional>
#include <map>

// MQTT Client interface for communicating with sensors and appliances
// In production, this would use a library like Paho MQTT or mosquitto
class MQTTClient {
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

    MQTTClient(const std::string& brokerAddress, int port = 1883)
        : brokerAddress_(brokerAddress), port_(port), connected_(false) {}

    bool connect() {
        // Simulate connection to MQTT broker
        connected_ = true;
        return connected_;
    }

    void disconnect() {
        connected_ = false;
        subscriptions_.clear();
    }

    bool isConnected() const {
        return connected_;
    }

    void subscribe(const std::string& topic, MessageCallback callback) {
        if (connected_) {
            subscriptions_[topic] = callback;
        }
    }

    void publish(const std::string& topic, const std::string& payload) {
        if (connected_) {
            // In production: send MQTT message
            // For simulation: topic and payload would be sent to broker
        }
    }

    void processMessages() {
        // In production: this would be called by MQTT library callback
        // For simulation: this would process received messages
        if (connected_) {
            // Process any pending messages
        }
    }

private:
    std::string brokerAddress_;
    int port_;
    bool connected_;
    std::map<std::string, MessageCallback> subscriptions_;
};

#endif // MQTT_CLIENT_H
