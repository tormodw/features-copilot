#ifndef HA_INTEGRATION_H
#define HA_INTEGRATION_H

#include "MQTTClient.h"
#include <string>
#include <functional>
#include <memory>
#include <map>

// Home Assistant MQTT Integration
// Handles communication with Home Assistant via MQTT for:
// - Fetching sensor data from HA
// - Executing commands to control HA devices
// - Supporting HA's MQTT discovery protocol
class HAIntegration {
public:
    // Callback types for handling HA data
    using StateCallback = std::function<void(const std::string& entityId, const std::string& state, const std::string& attributes)>;
    using DiscoveryCallback = std::function<void(const std::string& entityId, const std::string& config)>;

    HAIntegration(std::shared_ptr<MQTTClient> mqttClient, const std::string& haDiscoveryPrefix = "homeassistant");
    
    // Subscribe to HA entity state updates
    // entityId: HA entity ID (e.g., "sensor.temperature_living_room")
    // callback: Called when state changes
    void subscribeToEntity(const std::string& entityId, StateCallback callback);
    
    // Subscribe to all entities of a specific domain
    // domain: HA domain (e.g., "sensor", "switch", "light")
    // callback: Called when any entity in domain changes
    void subscribeToDomain(const std::string& domain, StateCallback callback);
    
    // Publish command to control HA device
    // entityId: HA entity ID (e.g., "switch.heater")
    // command: Command to send (e.g., "ON", "OFF")
    void publishCommand(const std::string& entityId, const std::string& command);
    
    // Publish command with additional data (for lights, climate, etc.)
    // entityId: HA entity ID
    // command: Command to send
    // data: Additional data in JSON format (e.g., brightness, temperature)
    void publishCommandWithData(const std::string& entityId, const std::string& command, const std::string& data);
    
    // Request current state of an entity
    // entityId: HA entity ID
    void requestState(const std::string& entityId);
    
    // Subscribe to HA discovery messages
    void subscribeToDiscovery(DiscoveryCallback callback);
    
    // Publish a discovery message for this system's entities
    // Used to make this system's sensors/controls visible in HA
    void publishDiscovery(const std::string& component, const std::string& nodeId, 
                         const std::string& objectId, const std::string& config);
    
    // Publish sensor state to MQTT (for publishing LOCAL sensor states TO HA)
    // entityId: HA entity ID for this sensor (e.g., "sensor.local_temperature")
    // state: State value to publish (e.g., "22.5")
    // attributes: Optional JSON attributes (e.g., {"unit": "°C", "friendly_name": "Living Room"})
    void publishState(const std::string& entityId, const std::string& state, const std::string& attributes = "");
    
    // Helper to parse HA state message (JSON format)
    // Returns: state value and attributes as separate strings
    static bool parseStateMessage(const std::string& payload, std::string& state, std::string& attributes);
    
    // Helper to create HA-compatible JSON command
    static std::string createCommandPayload(const std::string& command, const std::string& data = "");
    
    // Helper to escape JSON strings (prevents injection attacks)
    static std::string escapeJsonString(const std::string& input);

private:
    std::shared_ptr<MQTTClient> mqttClient_;
    std::string haDiscoveryPrefix_;
    std::map<std::string, StateCallback> entityCallbacks_;
    std::map<std::string, StateCallback> domainCallbacks_;
    DiscoveryCallback discoveryCallback_;
    
    // Generate HA MQTT topic for entity state
    std::string getStateTopic(const std::string& entityId) const;
    
    // Generate HA MQTT topic for entity command
    std::string getCommandTopic(const std::string& entityId) const;
    
    // Generate HA MQTT topic for discovery
    std::string getDiscoveryTopic(const std::string& component, const std::string& nodeId, 
                                  const std::string& objectId) const;
    
    // Extract domain from entity ID (e.g., "sensor" from "sensor.temperature")
    static std::string extractDomain(const std::string& entityId);
    
    // Handle incoming state message from HA
    void handleStateMessage(const std::string& topic, const std::string& payload);
    
    // Handle incoming discovery message from HA
    void handleDiscoveryMessage(const std::string& topic, const std::string& payload);
};

#endif // HA_INTEGRATION_H
