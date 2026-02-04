// MQTTClient.h - Interface remains unchanged
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <string>
#include <functional>
#include <map>

class MQTTClient {
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

    MQTTClient(const std::string& brokerAddress, int port = 1883);
    ~MQTTClient();

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
    
    // Mosquitto-specific members
    struct mosquitto* mosq_;
    static void on_connect_callback(struct mosquitto* mosq, void* obj, int result);
    static void on_message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message);
};

#endif // MQTT_CLIENT_H