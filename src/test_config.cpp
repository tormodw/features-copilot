#include "Config.h"
#include "ConfigWebServer.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

int main() {
    std::cout << "\n=== Home Automation Configuration System Test ===" << std::endl;
    std::cout << "This demonstrates the configuration class and web interface\n" << std::endl;
    
    // Create a config object
    auto config = std::make_shared<Config>(Config::getDefaultConfig());
    
    std::cout << "=== Step 1: Default Configuration ===" << std::endl;
    std::cout << "REST API URL: " << config->getRestApiUrl() << std::endl;
    std::cout << "REST API Token: " << (config->getRestApiToken().empty() ? "(none)" : "***") << std::endl;
    std::cout << "Deferrable Loads: " << config->getDeferrableLoadCount() << std::endl;
    for (const auto& load : config->getDeferrableLoadNames()) {
        std::cout << "  - " << load << std::endl;
    }
    std::cout << "Sensors: " << config->getSensorValues().size() << std::endl;
    for (const auto& sensor : config->getSensorValues()) {
        std::cout << "  - " << sensor << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== Step 2: Saving Configuration to File ===" << std::endl;
    if (config->saveToFile("config.json")) {
        std::cout << "✓ Configuration saved successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to save configuration" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== Step 3: JSON Representation ===" << std::endl;
    std::cout << config->toJson() << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Step 4: Modifying Configuration ===" << std::endl;
    config->addDeferrableLoad("dishwasher");
    config->addSensorValue("humidity_sensor");
    config->setRestApiUrl("http://homeassistant.local:8123");
    config->setRestApiToken("my-secret-token");
    std::cout << "Added 'dishwasher' to deferrable loads" << std::endl;
    std::cout << "Added 'humidity_sensor' to sensors" << std::endl;
    std::cout << "Changed REST API URL to " << config->getRestApiUrl() << std::endl;
    std::cout << "Total deferrable loads: " << config->getDeferrableLoadCount() << std::endl;
    std::cout << "Total sensors: " << config->getSensorValues().size() << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Step 5: Starting Web Interface ===" << std::endl;
    auto webServer = std::make_shared<ConfigWebServer>(config, 8080);
    
    if (webServer->start()) {
        std::cout << "✓ Web server started successfully" << std::endl;
        std::cout << "✓ Access the configuration interface at: " << webServer->getServerUrl() << std::endl;
        std::cout << std::endl;
        
        std::cout << "=== Web Interface Running ===" << std::endl;
        std::cout << "The web interface is now running and accessible via your browser." << std::endl;
        std::cout << "Open " << webServer->getServerUrl() << " to configure the system." << std::endl;
        std::cout << std::endl;
        std::cout << "Features available in the web interface:" << std::endl;
        std::cout << "  • Configure REST API settings (URL, authentication token)" << std::endl;
        std::cout << "  • Manage deferrable loads (add/remove)" << std::endl;
        std::cout << "  • Manage sensor values (add/remove)" << std::endl;
        std::cout << "  • Configure web interface settings" << std::endl;
        std::cout << "  • Save configuration to file" << std::endl;
        std::cout << "  • Reload configuration from file" << std::endl;
        std::cout << std::endl;
        
        std::cout << "Press Ctrl+C to stop the server..." << std::endl;
        
        // Keep the server running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } else {
        std::cout << "✗ Failed to start web server" << std::endl;
        std::cout << "Port 8080 might be in use. Try a different port." << std::endl;
    }
    
    return 0;
}
