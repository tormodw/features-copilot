#ifndef SYSTEM_WEB_SERVICE_H
#define SYSTEM_WEB_SERVICE_H

#include "Config.h"
#include "MLPredictor.h"
#include "DayAheadOptimizer.h"
#include "HistoricalDataCollector.h"
#include "Appliance.h"
#include "Sensor.h"
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>

// System status information
struct SystemStatus {
    bool running;
    std::string version;
    long uptimeSeconds;
    size_t dataPointsCollected;
    std::time_t lastMLTraining;
    std::time_t lastScheduleGeneration;
    bool mqttConnected;
    bool webServerRunning;
};

// REST API web service for system monitoring and control
class SystemWebService {
public:
    SystemWebService(
        std::shared_ptr<Config> config,
        std::shared_ptr<MLPredictor> mlPredictor,
        std::shared_ptr<DayAheadOptimizer> optimizer,
        std::shared_ptr<HistoricalDataCollector> dataCollector,
        int port = 8081
    );
    ~SystemWebService();
    
    // Start the web service (non-blocking)
    bool start();
    
    // Stop the web service
    void stop();
    
    // Check if service is running
    bool isRunning() const;
    
    // Get service URL
    std::string getServiceUrl() const;
    
    // Register sensors and appliances for monitoring
    void registerSensor(std::shared_ptr<Sensor> sensor);
    void registerAppliance(std::shared_ptr<Appliance> appliance);
    
    // Update system status
    void updateSystemStatus(const SystemStatus& status);
    
    // Update current schedule
    void updateSchedule(const DayAheadSchedule& schedule);

private:
    std::shared_ptr<Config> config_;
    std::shared_ptr<MLPredictor> mlPredictor_;
    std::shared_ptr<DayAheadOptimizer> optimizer_;
    std::shared_ptr<HistoricalDataCollector> dataCollector_;
    int port_;
    std::atomic<bool> running_;
    std::thread serverThread_;
    
    // Registered components (protected by mutex)
    std::mutex componentsMutex_;
    std::vector<std::shared_ptr<Sensor>> sensors_;
    std::vector<std::shared_ptr<Appliance>> appliances_;
    SystemStatus systemStatus_;
    DayAheadSchedule currentSchedule_;
    
    // Server loop
    void serverLoop();
    
    // Handle incoming connection
    void handleConnection(int clientSocket);
    
    // HTTP request structure
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string body;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> queryParams;
    };
    
    HttpRequest parseRequest(const std::string& requestData);
    
    // Generate HTTP response
    std::string generateHttpResponse(int statusCode, const std::string& contentType, 
                                     const std::string& body);
    
    // API Route handlers
    std::string handleGetStatus();
    std::string handleGetSensors();
    std::string handleGetAppliances();
    std::string handleGetSchedule();
    std::string handleGetHistoricalData(const std::map<std::string, std::string>& params);
    std::string handleGetPredictions(const std::map<std::string, std::string>& params);
    std::string handlePostApplianceControl(const std::string& body);
    std::string handleGetDashboard();
    
    // Helper functions
    std::string generateDashboardHTML();
    std::string escapeJson(const std::string& str);
};

#endif // SYSTEM_WEB_SERVICE_H
