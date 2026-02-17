#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <map>

// Appliance configuration structure
struct ApplianceConfig {
    std::string name;
    bool isDeferrable;
    
    ApplianceConfig() : name(""), isDeferrable(false) {}
    ApplianceConfig(const std::string& n, bool defer = false) : name(n), isDeferrable(defer) {}
};

// Configuration class for the home automation system
class Config {
public:
    Config();
    
    // Appliances configuration (formerly deferrable loads)
    void setApplianceCount(int count);
    int getApplianceCount() const;
    
    void setAppliances(const std::vector<ApplianceConfig>& appliances);
    std::vector<ApplianceConfig> getAppliances() const;
    
    void addAppliance(const std::string& name, bool isDeferrable = false);
    void removeAppliance(const std::string& name);
    void setApplianceDeferrable(const std::string& name, bool isDeferrable);
    
    // Legacy methods for backward compatibility
    void setDeferrableLoadCount(int count);
    int getDeferrableLoadCount() const;
    void setDeferrableLoadNames(const std::vector<std::string>& names);
    std::vector<std::string> getDeferrableLoadNames() const;
    void addDeferrableLoad(const std::string& name);
    void removeDeferrableLoad(const std::string& name);
    
    // MQTT configuration
    void setMqttEnabled(bool enabled);
    bool isMqttEnabled() const;
    
    void setMqttBrokerAddress(const std::string& address);
    std::string getMqttBrokerAddress() const;
    
    void setMqttPort(int port);
    int getMqttPort() const;
    
    // Sensor configuration
    void setSensorValues(const std::vector<std::string>& sensors);
    std::vector<std::string> getSensorValues() const;
    
    void addSensorValue(const std::string& sensor);
    void removeSensorValue(const std::string& sensor);
    
    // Web interface configuration
    void setWebInterfaceEnabled(bool enabled);
    bool isWebInterfaceEnabled() const;
    
    void setWebInterfacePort(int port);
    int getWebInterfacePort() const;
    
    // Persistence
    bool loadFromFile(const std::string& filename = "config.json");
    bool saveToFile(const std::string& filename = "config.json") const;
    
    // JSON serialization
    std::string toJson() const;
    bool fromJson(const std::string& json);
    
    // Get default configuration
    static Config getDefaultConfig();

private:
    // Appliances (with deferrable status)
    std::vector<ApplianceConfig> appliances_;
    
    // MQTT settings
    bool mqttEnabled_;
    std::string mqttBrokerAddress_;
    int mqttPort_;
    
    // Sensor settings
    std::vector<std::string> sensorValues_;
    
    // Web interface settings
    bool webInterfaceEnabled_;
    int webInterfacePort_;
    
    // Helper methods for JSON parsing (simple implementation without external library)
    std::string escapeJsonString(const std::string& str) const;
    std::string unescapeJsonString(const std::string& str) const;
};

#endif // CONFIG_H
