#include "HAIntegration.h"
#include <iostream>
#include <sstream>

HAIntegration::HAIntegration(std::shared_ptr<MQTTClient> mqttClient, const std::string& haDiscoveryPrefix)
    : mqttClient_(mqttClient), haDiscoveryPrefix_(haDiscoveryPrefix) {
}

void HAIntegration::subscribeToEntity(const std::string& entityId, StateCallback callback) {
    if (!mqttClient_ || !mqttClient_->isConnected()) {
        std::cerr << "HAIntegration: MQTT client not connected" << std::endl;
        return;
    }
    
    entityCallbacks_[entityId] = callback;
    std::string topic = getStateTopic(entityId);
    
    mqttClient_->subscribe(topic, [this, entityId](const std::string& topic, const std::string& payload) {
        handleStateMessage(topic, payload);
    });
    
    std::cout << "HAIntegration: Subscribed to entity " << entityId << " on topic: " << topic << std::endl;
}

void HAIntegration::subscribeToDomain(const std::string& domain, StateCallback callback) {
    if (!mqttClient_ || !mqttClient_->isConnected()) {
        std::cerr << "HAIntegration: MQTT client not connected" << std::endl;
        return;
    }
    
    domainCallbacks_[domain] = callback;
    // Topic pattern to match all entities in a domain
    // Format: homeassistant/state/domain.*
    std::string topic = haDiscoveryPrefix_ + "/state/" + domain + ".+";
 //twi   std::string topic = haDiscoveryPrefix_ + "/" + domain;
    
    mqttClient_->subscribe(topic, [this, domain](const std::string& topic, const std::string& payload) {
        handleStateMessage(topic, payload);
    });
    
    std::cout << "HAIntegration: Subscribed to domain " << domain << " on topic: " << topic << std::endl;
}

void HAIntegration::publishCommand(const std::string& entityId, const std::string& command) {
    if (!mqttClient_ || !mqttClient_->isConnected()) {
        std::cerr << "HAIntegration: MQTT client not connected" << std::endl;
        return;
    }
    
    std::string topic = getCommandTopic(entityId);
    mqttClient_->publish(topic, command);
    
    std::cout << "HAIntegration: Published command '" << command << "' to " << entityId << std::endl;
}

void HAIntegration::publishCommandWithData(const std::string& entityId, const std::string& command, const std::string& data) {
    if (!mqttClient_ || !mqttClient_->isConnected()) {
        std::cerr << "HAIntegration: MQTT client not connected" << std::endl;
        return;
    }
    
    std::string topic = getCommandTopic(entityId);
    std::string payload = createCommandPayload(command, data);
    mqttClient_->publish(topic, payload);
    
    std::cout << "HAIntegration: Published command '" << command << "' with data to " << entityId << std::endl;
}

void HAIntegration::requestState(const std::string& entityId) {
    if (!mqttClient_ || !mqttClient_->isConnected()) {
        std::cerr << "HAIntegration: MQTT client not connected" << std::endl;
        return;
    }
    
    // Request state by publishing to the state request topic
    std::string topic = getStateTopic(entityId) + "/get";
    mqttClient_->publish(topic, "");
    
    std::cout << "HAIntegration: Requested state for " << entityId << std::endl;
}

void HAIntegration::subscribeToDiscovery(DiscoveryCallback callback) {
    if (!mqttClient_ || !mqttClient_->isConnected()) {
        std::cerr << "HAIntegration: MQTT client not connected" << std::endl;
        return;
    }
    
    discoveryCallback_ = callback;
    std::string topic = haDiscoveryPrefix_ + "/#";
    
    mqttClient_->subscribe(topic, [this](const std::string& topic, const std::string& payload) {
        handleDiscoveryMessage(topic, payload);
    });
    
    std::cout << "HAIntegration: Subscribed to HA discovery on: " << topic << std::endl;
}

void HAIntegration::publishDiscovery(const std::string& component, const std::string& nodeId, 
                                     const std::string& objectId, const std::string& config) {
    if (!mqttClient_ || !mqttClient_->isConnected()) {
        std::cerr << "HAIntegration: MQTT client not connected" << std::endl;
        return;
    }
    
    std::string topic = getDiscoveryTopic(component, nodeId, objectId);
    mqttClient_->publish(topic, config);
    
    std::cout << "HAIntegration: Published discovery for " << component << "." << objectId << std::endl;
}

void HAIntegration::publishState(const std::string& entityId, const std::string& state, const std::string& attributes) {
    if (!mqttClient_ || !mqttClient_->isConnected()) {
        std::cerr << "HAIntegration: MQTT client not connected" << std::endl;
        return;
    }
    
    std::string topic = getStateTopic(entityId);
    std::string payload;
    
    // If attributes provided, create JSON payload, otherwise just send state value
    if (!attributes.empty()) {
        std::ostringstream oss;
        oss << "{\"state\": \"" << state << "\", \"attributes\": " << attributes << "}";
        payload = oss.str();
    } else {
        payload = state;
    }
    
    mqttClient_->publish(topic, payload);
    
    std::cout << "HAIntegration: Published state for " << entityId << ": " << state << std::endl;
}

bool HAIntegration::parseStateMessage(const std::string& payload, std::string& state, std::string& attributes) {
    // Simple JSON-like parsing
    // Expected format: {"state": "value", "attributes": {...}}
    // For mock implementation, we'll do simple string parsing
    // In production, use a proper JSON library like nlohmann/json
    
    if (payload.empty()) {
        return false;
    }
    
    // Check if payload is JSON
    if (payload[0] == '{') {
        // Simple extraction of state field
        size_t statePos = payload.find("\"state\"");
        if (statePos != std::string::npos) {
            size_t colonPos = payload.find(":", statePos);
            size_t quoteStart = payload.find("\"", colonPos);
            if (quoteStart == std::string::npos) {
                return false;
            }
            size_t quoteEnd = payload.find("\"", quoteStart + 1);
            
            if (quoteEnd != std::string::npos) {
                state = payload.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            }
        }
        
        // Extract attributes if present
        // Note: This is a simplified extraction. For complex JSON, use a proper parser.
        size_t attrPos = payload.find("\"attributes\"");
        if (attrPos != std::string::npos) {
            size_t braceStart = payload.find("{", attrPos);
            if (braceStart != std::string::npos) {
                // Count braces to find matching closing brace
                int braceCount = 1;
                size_t pos = braceStart + 1;
                while (pos < payload.length() && braceCount > 0) {
                    if (payload[pos] == '{') {
                        braceCount++;
                    } else if (payload[pos] == '}') {
                        braceCount--;
                    }
                    pos++;
                }
                if (braceCount == 0) {
                    attributes = payload.substr(braceStart, pos - braceStart);
                }
            }
        }
        
        return !state.empty();
    } else {
        // Plain text state
        state = payload;
        attributes = "";
        return true;
    }
}

std::string HAIntegration::createCommandPayload(const std::string& command, const std::string& data) {
    if (data.empty()) {
        return command;
    }
    
    // Create simple JSON payload
    std::ostringstream oss;
    oss << "{\"command\": \"" << command << "\", \"data\": " << data << "}";
    return oss.str();
}

std::string HAIntegration::getStateTopic(const std::string& entityId) const {
    // HA state topic format: homeassistant/<domain>/<node_id>/<object_id>/state
    // or simpler: homeassistant/state/<entity_id>
    return haDiscoveryPrefix_ + "/state/" + entityId;
//twi    return haDiscoveryPrefix_ + "/" + entityId;
}

std::string HAIntegration::getCommandTopic(const std::string& entityId) const {
    // HA command topic format: homeassistant/<domain>/<node_id>/<object_id>/set
    // or simpler: homeassistant/command/<entity_id>
    return haDiscoveryPrefix_ + "/command/" + entityId;
}

std::string HAIntegration::getDiscoveryTopic(const std::string& component, const std::string& nodeId, 
                                             const std::string& objectId) const {
    // HA discovery topic format: <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
    return haDiscoveryPrefix_ + "/" + component + "/" + nodeId + "/" + objectId + "/config";
}

std::string HAIntegration::extractDomain(const std::string& entityId) {
    size_t dotPos = entityId.find('.');
    if (dotPos != std::string::npos) {
        return entityId.substr(0, dotPos);
    }
    return "";
}

void HAIntegration::handleStateMessage(const std::string& topic, const std::string& payload) {
    // Validate topic starts with expected prefix
    std::string expectedPrefix = haDiscoveryPrefix_ + "/state/";
    if (topic.find(expectedPrefix) != 0) {
        return;
    }
    
    // Extract entity ID from topic
    // Topic format: homeassistant/state/<entity_id>
    std::string entityId = topic.substr(expectedPrefix.length());
    
    // Parse the message
    std::string state, attributes;
    if (parseStateMessage(payload, state, attributes)) {
        // Call entity-specific callback if registered
        auto entityIt = entityCallbacks_.find(entityId);
        if (entityIt != entityCallbacks_.end()) {
            entityIt->second(entityId, state, attributes);
        }
        
        // Call domain callback if registered
        std::string domain = extractDomain(entityId);
        auto domainIt = domainCallbacks_.find(domain);
        if (domainIt != domainCallbacks_.end()) {
            domainIt->second(entityId, state, attributes);
        }
        
        std::cout << "HAIntegration: Received state update for " << entityId 
                  << ": state=" << state << std::endl;
    }
}

void HAIntegration::handleDiscoveryMessage(const std::string& topic, const std::string& payload) {
    // Extract component and object_id from discovery topic
    // Topic format: homeassistant/<component>/<node_id>/<object_id>/config
    
    std::string topicCopy = topic;
    size_t prefixLen = haDiscoveryPrefix_.length() + 1;
    
    if (topicCopy.length() <= prefixLen) {
        return;
    }
    
    std::string remainder = topicCopy.substr(prefixLen);
    size_t firstSlash = remainder.find('/');
    
    if (firstSlash != std::string::npos) {
        std::string component = remainder.substr(0, firstSlash);
        
        if (discoveryCallback_) {
            discoveryCallback_(component, payload);
        }
        
        std::cout << "HAIntegration: Received discovery for component: " << component << std::endl;
    }
}
