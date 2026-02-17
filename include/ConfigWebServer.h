#ifndef CONFIG_WEB_SERVER_H
#define CONFIG_WEB_SERVER_H

#include "Config.h"
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <map>

// Simple HTTP server for configuration web interface
class ConfigWebServer {
public:
    ConfigWebServer(std::shared_ptr<Config> config, int port = 8080);
    ~ConfigWebServer();
    
    // Start the server (non-blocking)
    bool start();
    
    // Stop the server
    void stop();
    
    // Check if server is running
    bool isRunning() const;
    
    // Get server URL
    std::string getServerUrl() const;

private:
    std::shared_ptr<Config> config_;
    int port_;
    std::atomic<bool> running_;
    std::thread serverThread_;
    
    // Server loop (runs in separate thread)
    void serverLoop();
    
    // Handle incoming connection
    void handleConnection(int clientSocket);
    
    // Parse HTTP request
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string body;
        std::map<std::string, std::string> headers;
    };
    
    HttpRequest parseRequest(const std::string& requestData);
    
    // Generate HTTP responses
    std::string generateHttpResponse(int statusCode, const std::string& contentType, 
                                     const std::string& body);
    
    // Route handlers
    std::string handleGetConfig();
    std::string handlePostConfig(const std::string& body);
    std::string handleGetIndex();
    
    // HTML page generator
    std::string generateConfigPage();
};

#endif // CONFIG_WEB_SERVER_H
