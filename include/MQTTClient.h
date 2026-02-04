#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <string>
#include <functional>
#include <map>

// MQTT Client interface for communicating with sensors and appliances
// In production, this would use a library like Paho MQTT or mosquitto
// 
// For production integration with Eclipse Mosquitto, see MQTT_MOSQUITTO_GUIDE.md
// The guide includes:
// - Installation instructions
// - Complete implementation examples
// - Topic structure and message formats
// - Security configuration
// - Testing procedures
class MQTTClient {
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

    MQTTClient(const std::string& brokerAddress, int port = 1883);

    bool connect();
    void disconnect();
    bool isConnected() const;
    void subscribe(const std::string& topic, MessageCallback callback);
    void publish(const std::string& topic, const std::string& payload);
    void processMessages();
    
    // Simulate receiving a message (for testing without real broker)
    void simulateMessage(const std::string& topic, const std::string& payload);

private:
    std::string brokerAddress_;
    int port_;
    bool connected_;
    std::map<std::string, MessageCallback> subscriptions_;
    
    // Check if a topic matches a subscription pattern (supports + and # wildcards)
    bool topicMatches(const std::string& pattern, const std::string& topic) const;
};

#endif // MQTT_CLIENT_H
