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

// Appliances configuration
void Config::setApplianceCount(int count) {
    if (count < 0) count = 0;
    appliances_.resize(count);
}

int Config::getApplianceCount() const {
    return static_cast<int>(appliances_.size());
}

void Config::setAppliances(const std::vector<ApplianceConfig>& appliances) {
    appliances_ = appliances;
}

std::vector<ApplianceConfig> Config::getAppliances() const {
    return appliances_;
}

void Config::addAppliance(const std::string& name, bool isDeferrable) {
    // Only add if not already present
    for (const auto& appliance : appliances_) {
        if (appliance.name == name) {
            return;  // Already exists
        }
    }
    appliances_.push_back(ApplianceConfig(name, isDeferrable));
}

void Config::removeAppliance(const std::string& name) {
    appliances_.erase(
        std::remove_if(appliances_.begin(), appliances_.end(),
            [&name](const ApplianceConfig& a) { return a.name == name; }),
        appliances_.end()
    );
}

void Config::setApplianceDeferrable(const std::string& name, bool isDeferrable) {
    for (auto& appliance : appliances_) {
        if (appliance.name == name) {
            appliance.isDeferrable = isDeferrable;
            return;
        }
    }
}

// Legacy methods for backward compatibility
void Config::setDeferrableLoadCount(int count) {
    setApplianceCount(count);
}

int Config::getDeferrableLoadCount() const {
    // Count only deferrable appliances
    int count = 0;
    for (const auto& appliance : appliances_) {
        if (appliance.isDeferrable) {
            count++;
        }
    }
    return count;
}

void Config::setDeferrableLoadNames(const std::vector<std::string>& names) {
    appliances_.clear();
    for (const auto& name : names) {
        appliances_.push_back(ApplianceConfig(name, true));
    }
}

std::vector<std::string> Config::getDeferrableLoadNames() const {
    std::vector<std::string> names;
    for (const auto& appliance : appliances_) {
        if (appliance.isDeferrable) {
            names.push_back(appliance.name);
        }
    }
    return names;
}

void Config::addDeferrableLoad(const std::string& name) {
    addAppliance(name, true);
}

void Config::removeDeferrableLoad(const std::string& name) {
    removeAppliance(name);
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
    
    // Appliances
    json << "  \"appliances\": [\n";
    for (size_t i = 0; i < appliances_.size(); ++i) {
        json << "    {\n";
        json << "      \"name\": \"" << escapeJsonString(appliances_[i].name) << "\",\n";
        json << "      \"isDeferrable\": " << (appliances_[i].isDeferrable ? "true" : "false") << "\n";
        json << "    }";
        if (i < appliances_.size() - 1) {
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
                try {
                    mqttPort_ = std::stoi(portStr);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid MQTT port value in JSON: " << portStr << std::endl;
                } catch (const std::out_of_range& e) {
                    std::cerr << "MQTT port value out of range in JSON: " << portStr << std::endl;
                }
            }
        }
        
        // Parse appliances array (new format)
        appliances_.clear();
        size_t appliancesPos = json.find("\"appliances\"");
        if (appliancesPos != std::string::npos) {
            size_t arrayStart = json.find("[", appliancesPos);
            size_t arrayEnd = json.find("]", arrayStart);
            if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                size_t pos = 0;
                
                // Find each object in the array
                while ((pos = arrayContent.find("{", pos)) != std::string::npos) {
                    size_t objEnd = arrayContent.find("}", pos);
                    if (objEnd == std::string::npos) break;
                    
                    std::string objContent = arrayContent.substr(pos + 1, objEnd - pos - 1);
                    
                    // Parse name
                    size_t namePos = objContent.find("\"name\"");
                    std::string name;
                    if (namePos != std::string::npos) {
                        size_t colonPos = objContent.find(":", namePos);
                        size_t firstQuote = objContent.find("\"", colonPos);
                        size_t secondQuote = objContent.find("\"", firstQuote + 1);
                        if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
                            name = objContent.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                        }
                    }
                    
                    // Parse isDeferrable
                    bool isDeferrable = false;
                    size_t deferrablePos = objContent.find("\"isDeferrable\"");
                    if (deferrablePos != std::string::npos) {
                        size_t truePos = objContent.find("true", deferrablePos);
                        size_t falsePos = objContent.find("false", deferrablePos);
                        if (truePos != std::string::npos && truePos < objContent.length()) {
                            isDeferrable = true;
                        }
                    }
                    
                    if (!name.empty()) {
                        appliances_.push_back(ApplianceConfig(unescapeJsonString(name), isDeferrable));
                    }
                    
                    pos = objEnd + 1;
                }
            }
        }
        
        // Backward compatibility: Parse old deferrableLoads array
        size_t deferrablePos = json.find("\"deferrableLoads\"");
        if (deferrablePos != std::string::npos && appliancesPos == std::string::npos) {
            // Only use old format if new format doesn't exist
            size_t arrayStart = json.find("[", deferrablePos);
            size_t arrayEnd = json.find("]", arrayStart);
            if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                size_t pos = 0;
                while ((pos = arrayContent.find("\"", pos)) != std::string::npos) {
                    size_t endQuote = arrayContent.find("\"", pos + 1);
                    if (endQuote != std::string::npos) {
                        std::string name = arrayContent.substr(pos + 1, endQuote - pos - 1);
                        appliances_.push_back(ApplianceConfig(unescapeJsonString(name), true));
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
                    try {
                        webInterfacePort_ = std::stoi(portStr);
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid web interface port value in JSON: " << portStr << std::endl;
                    } catch (const std::out_of_range& e) {
                        std::cerr << "Web interface port value out of range in JSON: " << portStr << std::endl;
                    }
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
