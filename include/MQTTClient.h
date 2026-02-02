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

    MQTTClient(const std::string& brokerAddress, int port = 1883);

    bool connect();
    void disconnect();
    bool isConnected() const;
    void subscribe(const std::string& topic, MessageCallback callback);
    void publish(const std::string& topic, const std::string& payload);
    void processMessages();

private:
    std::string brokerAddress_;
    int port_;
    bool connected_;
    std::map<std::string, MessageCallback> subscriptions_;
};

#endif // MQTT_CLIENT_H
