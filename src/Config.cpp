#include "Config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

Config::Config() 
    : mqttEnabled_(true)
    , mqttBrokerAddress_("localhost")
    , mqttPort_(1883)
    , webInterfaceEnabled_(true)
    , webInterfacePort_(8080) {
}

// Deferrable loads configuration
void Config::setDeferrableLoadCount(int count) {
    // Resize the vector to match the count
    if (count < 0) count = 0;
    deferrableLoadNames_.resize(count);
}

int Config::getDeferrableLoadCount() const {
    return static_cast<int>(deferrableLoadNames_.size());
}

void Config::setDeferrableLoadNames(const std::vector<std::string>& names) {
    deferrableLoadNames_ = names;
}

std::vector<std::string> Config::getDeferrableLoadNames() const {
    return deferrableLoadNames_;
}

void Config::addDeferrableLoad(const std::string& name) {
    // Only add if not already present
    if (std::find(deferrableLoadNames_.begin(), deferrableLoadNames_.end(), name) 
        == deferrableLoadNames_.end()) {
        deferrableLoadNames_.push_back(name);
    }
}

void Config::removeDeferrableLoad(const std::string& name) {
    deferrableLoadNames_.erase(
        std::remove(deferrableLoadNames_.begin(), deferrableLoadNames_.end(), name),
        deferrableLoadNames_.end()
    );
}

// MQTT configuration
void Config::setMqttEnabled(bool enabled) {
    mqttEnabled_ = enabled;
}

bool Config::isMqttEnabled() const {
    return mqttEnabled_;
}

void Config::setMqttBrokerAddress(const std::string& address) {
    mqttBrokerAddress_ = address;
}

std::string Config::getMqttBrokerAddress() const {
    return mqttBrokerAddress_;
}

void Config::setMqttPort(int port) {
    mqttPort_ = port;
}

int Config::getMqttPort() const {
    return mqttPort_;
}

// Sensor configuration
void Config::setSensorValues(const std::vector<std::string>& sensors) {
    sensorValues_ = sensors;
}

std::vector<std::string> Config::getSensorValues() const {
    return sensorValues_;
}

void Config::addSensorValue(const std::string& sensor) {
    // Only add if not already present
    if (std::find(sensorValues_.begin(), sensorValues_.end(), sensor) 
        == sensorValues_.end()) {
        sensorValues_.push_back(sensor);
    }
}

void Config::removeSensorValue(const std::string& sensor) {
    sensorValues_.erase(
        std::remove(sensorValues_.begin(), sensorValues_.end(), sensor),
        sensorValues_.end()
    );
}

// Web interface configuration
void Config::setWebInterfaceEnabled(bool enabled) {
    webInterfaceEnabled_ = enabled;
}

bool Config::isWebInterfaceEnabled() const {
    return webInterfaceEnabled_;
}

void Config::setWebInterfacePort(int port) {
    webInterfacePort_ = port;
}

int Config::getWebInterfacePort() const {
    return webInterfacePort_;
}

// Helper method to escape JSON strings
std::string Config::escapeJsonString(const std::string& str) const {
    std::ostringstream escaped;
    for (char c : str) {
        switch (c) {
            case '\"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default: escaped << c; break;
        }
    }
    return escaped.str();
}

// Helper method to unescape JSON strings
std::string Config::unescapeJsonString(const std::string& str) const {
    std::ostringstream unescaped;
    bool escaping = false;
    for (char c : str) {
        if (escaping) {
            switch (c) {
                case '\"': unescaped << '\"'; break;
                case '\\': unescaped << '\\'; break;
                case 'b': unescaped << '\b'; break;
                case 'f': unescaped << '\f'; break;
                case 'n': unescaped << '\n'; break;
                case 'r': unescaped << '\r'; break;
                case 't': unescaped << '\t'; break;
                default: unescaped << c; break;
            }
            escaping = false;
        } else if (c == '\\') {
            escaping = true;
        } else {
            unescaped << c;
        }
    }
    return unescaped.str();
}

// JSON serialization
std::string Config::toJson() const {
    std::ostringstream json;
    json << "{\n";
    
    // MQTT configuration
    json << "  \"mqtt\": {\n";
    json << "    \"enabled\": " << (mqttEnabled_ ? "true" : "false") << ",\n";
    json << "    \"brokerAddress\": \"" << escapeJsonString(mqttBrokerAddress_) << "\",\n";
    json << "    \"port\": " << mqttPort_ << "\n";
    json << "  },\n";
    
    // Deferrable loads
    json << "  \"deferrableLoads\": [\n";
    for (size_t i = 0; i < deferrableLoadNames_.size(); ++i) {
        json << "    \"" << escapeJsonString(deferrableLoadNames_[i]) << "\"";
        if (i < deferrableLoadNames_.size() - 1) {
            json << ",";
        }
        json << "\n";
    }
    json << "  ],\n";
    
    // Sensors
    json << "  \"sensors\": [\n";
    for (size_t i = 0; i < sensorValues_.size(); ++i) {
        json << "    \"" << escapeJsonString(sensorValues_[i]) << "\"";
        if (i < sensorValues_.size() - 1) {
            json << ",";
        }
        json << "\n";
    }
    json << "  ],\n";
    
    // Web interface configuration
    json << "  \"webInterface\": {\n";
    json << "    \"enabled\": " << (webInterfaceEnabled_ ? "true" : "false") << ",\n";
    json << "    \"port\": " << webInterfacePort_ << "\n";
    json << "  }\n";
    
    json << "}";
    return json.str();
}

// Simple JSON parsing (without external library)
bool Config::fromJson(const std::string& json) {
    try {
        // Very simple JSON parser - looks for key patterns
        // In production, use a proper JSON library like nlohmann/json
        
        // Parse MQTT enabled
        size_t mqttEnabledPos = json.find("\"enabled\"");
        if (mqttEnabledPos != std::string::npos) {
            size_t truePos = json.find("true", mqttEnabledPos);
            size_t falsePos = json.find("false", mqttEnabledPos);
            size_t commaPos = json.find(",", mqttEnabledPos);
            
            if (truePos != std::string::npos && truePos < commaPos) {
                mqttEnabled_ = true;
            } else if (falsePos != std::string::npos && falsePos < commaPos) {
                mqttEnabled_ = false;
            }
        }
        
        // Parse broker address
        size_t brokerPos = json.find("\"brokerAddress\"");
        if (brokerPos != std::string::npos) {
            size_t colonPos = json.find(":", brokerPos);
            size_t firstQuote = json.find("\"", colonPos);
            size_t secondQuote = json.find("\"", firstQuote + 1);
            if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
                mqttBrokerAddress_ = json.substr(firstQuote + 1, secondQuote - firstQuote - 1);
            }
        }
        
        // Parse port
        size_t portPos = json.find("\"port\"");
        if (portPos != std::string::npos) {
            size_t colonPos = json.find(":", portPos);
            size_t commaPos = json.find_first_of(",\n}", colonPos);
            if (colonPos != std::string::npos && commaPos != std::string::npos) {
                std::string portStr = json.substr(colonPos + 1, commaPos - colonPos - 1);
                // Trim whitespace
                portStr.erase(0, portStr.find_first_not_of(" \t\n\r"));
                portStr.erase(portStr.find_last_not_of(" \t\n\r") + 1);
                mqttPort_ = std::stoi(portStr);
            }
        }
        
        // Parse deferrable loads array
        deferrableLoadNames_.clear();
        size_t deferrablePos = json.find("\"deferrableLoads\"");
        if (deferrablePos != std::string::npos) {
            size_t arrayStart = json.find("[", deferrablePos);
            size_t arrayEnd = json.find("]", arrayStart);
            if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                size_t pos = 0;
                while ((pos = arrayContent.find("\"", pos)) != std::string::npos) {
                    size_t endQuote = arrayContent.find("\"", pos + 1);
                    if (endQuote != std::string::npos) {
                        std::string name = arrayContent.substr(pos + 1, endQuote - pos - 1);
                        deferrableLoadNames_.push_back(unescapeJsonString(name));
                        pos = endQuote + 1;
                    } else {
                        break;
                    }
                }
            }
        }
        
        // Parse sensors array
        sensorValues_.clear();
        size_t sensorsPos = json.find("\"sensors\"");
        if (sensorsPos != std::string::npos) {
            size_t arrayStart = json.find("[", sensorsPos);
            size_t arrayEnd = json.find("]", arrayStart);
            if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                size_t pos = 0;
                while ((pos = arrayContent.find("\"", pos)) != std::string::npos) {
                    size_t endQuote = arrayContent.find("\"", pos + 1);
                    if (endQuote != std::string::npos) {
                        std::string sensor = arrayContent.substr(pos + 1, endQuote - pos - 1);
                        sensorValues_.push_back(unescapeJsonString(sensor));
                        pos = endQuote + 1;
                    } else {
                        break;
                    }
                }
            }
        }
        
        // Parse web interface enabled
        size_t webInterfacePos = json.find("\"webInterface\"");
        if (webInterfacePos != std::string::npos) {
            size_t enabledPos = json.find("\"enabled\"", webInterfacePos);
            if (enabledPos != std::string::npos) {
                size_t truePos = json.find("true", enabledPos);
                size_t falsePos = json.find("false", enabledPos);
                size_t nextCommaOrBrace = json.find_first_of(",}", enabledPos);
                
                if (truePos != std::string::npos && truePos < nextCommaOrBrace) {
                    webInterfaceEnabled_ = true;
                } else if (falsePos != std::string::npos && falsePos < nextCommaOrBrace) {
                    webInterfaceEnabled_ = false;
                }
            }
            
            // Parse web interface port
            size_t webPortPos = json.find("\"port\"", webInterfacePos);
            if (webPortPos != std::string::npos) {
                size_t colonPos = json.find(":", webPortPos);
                size_t nextCommaOrBrace = json.find_first_of(",\n}", colonPos);
                if (colonPos != std::string::npos && nextCommaOrBrace != std::string::npos) {
                    std::string portStr = json.substr(colonPos + 1, nextCommaOrBrace - colonPos - 1);
                    // Trim whitespace
                    portStr.erase(0, portStr.find_first_not_of(" \t\n\r"));
                    portStr.erase(portStr.find_last_not_of(" \t\n\r") + 1);
                    webInterfacePort_ = std::stoi(portStr);
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }
}

// Load configuration from file
bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open config file: " << filename << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonContent = buffer.str();
    file.close();
    
    return fromJson(jsonContent);
}

// Save configuration to file
bool Config::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open config file for writing: " << filename << std::endl;
        return false;
    }
    
    file << toJson();
    file.close();
    
    std::cout << "Configuration saved to " << filename << std::endl;
    return true;
}

// Get default configuration
Config Config::getDefaultConfig() {
    Config config;
    
    // Set default deferrable loads
    config.addDeferrableLoad("ev_charger");
    config.addDeferrableLoad("decorative_lights");
    config.addDeferrableLoad("pool_pump");
    
    // Set default sensors
    config.addSensorValue("temperature_indoor");
    config.addSensorValue("temperature_outdoor");
    config.addSensorValue("energy_meter");
    config.addSensorValue("solar_production");
    config.addSensorValue("ev_charger_power");
    
    // MQTT enabled by default
    config.setMqttEnabled(true);
    config.setMqttBrokerAddress("localhost");
    config.setMqttPort(1883);
    
    // Web interface enabled by default
    config.setWebInterfaceEnabled(true);
    config.setWebInterfacePort(8080);
    
    return config;
}
