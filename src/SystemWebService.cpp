#include "SystemWebService.h"
#include "DayAheadOptimizer.h"
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
#include <iomanip>

SystemWebService::SystemWebService(
    std::shared_ptr<Config> config,
    std::shared_ptr<MLPredictor> mlPredictor,
    std::shared_ptr<DayAheadOptimizer> optimizer,
    std::shared_ptr<HistoricalDataCollector> dataCollector,
    int port
) : config_(config)
  , mlPredictor_(mlPredictor)
  , optimizer_(optimizer)
  , dataCollector_(dataCollector)
  , port_(port)
  , running_(false) {
    systemStatus_.running = false;
    systemStatus_.version = "1.0.0";
    systemStatus_.uptimeSeconds = 0;
    systemStatus_.dataPointsCollected = 0;
    systemStatus_.lastMLTraining = 0;
    systemStatus_.lastScheduleGeneration = 0;
    systemStatus_.mqttConnected = false;
    systemStatus_.webServerRunning = false;
}

SystemWebService::~SystemWebService() {
    stop();
}

bool SystemWebService::start() {
    if (running_) {
        return false;
    }
    
    running_ = true;
    serverThread_ = std::thread(&SystemWebService::serverLoop, this);
    
    return true;
}

void SystemWebService::stop() {
    if (running_) {
        running_ = false;
        if (serverThread_.joinable()) {
            serverThread_.join();
        }
    }
}

bool SystemWebService::isRunning() const {
    return running_;
}

std::string SystemWebService::getServiceUrl() const {
    return "http://localhost:" + std::to_string(port_);
}

void SystemWebService::registerSensor(std::shared_ptr<Sensor> sensor) {
    std::lock_guard<std::mutex> lock(componentsMutex_);
    sensors_.push_back(sensor);
}

void SystemWebService::registerAppliance(std::shared_ptr<Appliance> appliance) {
    std::lock_guard<std::mutex> lock(componentsMutex_);
    appliances_.push_back(appliance);
}

void SystemWebService::updateSystemStatus(const SystemStatus& status) {
    std::lock_guard<std::mutex> lock(componentsMutex_);
    systemStatus_ = status;
}

void SystemWebService::updateSchedule(const DayAheadSchedule& schedule) {
    std::lock_guard<std::mutex> lock(componentsMutex_);
    currentSchedule_ = schedule;
}

void SystemWebService::serverLoop() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        running_ = false;
        return;
    }
    
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSocket);
        running_ = false;
        return;
    }
    
    if (listen(serverSocket, 5) < 0) {
        close(serverSocket);
        running_ = false;
        return;
    }
    
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            continue;
        }
        
        handleConnection(clientSocket);
        close(clientSocket);
    }
    
    close(serverSocket);
}

void SystemWebService::handleConnection(int clientSocket) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        return;
    }
    
    std::string requestData(buffer, bytesRead);
    HttpRequest request = parseRequest(requestData);
    
    std::string response;
    
    if (request.path == "/" || request.path == "/dashboard") {
        response = generateHttpResponse(200, "text/html", handleGetDashboard());
    } else if (request.path == "/api/status") {
        response = generateHttpResponse(200, "application/json", handleGetStatus());
    } else if (request.path == "/api/sensors") {
        response = generateHttpResponse(200, "application/json", handleGetSensors());
    } else if (request.path == "/api/appliances") {
        response = generateHttpResponse(200, "application/json", handleGetAppliances());
    } else if (request.path == "/api/schedule") {
        response = generateHttpResponse(200, "application/json", handleGetSchedule());
    } else if (request.path == "/api/historical") {
        response = generateHttpResponse(200, "application/json", handleGetHistoricalData(request.queryParams));
    } else if (request.path == "/api/predictions") {
        response = generateHttpResponse(200, "application/json", handleGetPredictions(request.queryParams));
    } else if (request.path == "/api/control" && request.method == "POST") {
        response = generateHttpResponse(200, "application/json", handlePostApplianceControl(request.body));
    } else {
        response = generateHttpResponse(404, "text/plain", "Not Found");
    }
    
    send(clientSocket, response.c_str(), response.length(), 0);
}

SystemWebService::HttpRequest SystemWebService::parseRequest(const std::string& requestData) {
    HttpRequest request;
    
    std::istringstream stream(requestData);
    std::string line;
    
    if (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        std::string pathWithQuery;
        lineStream >> request.method >> pathWithQuery;
        
        // Parse query parameters
        size_t queryPos = pathWithQuery.find('?');
        if (queryPos != std::string::npos) {
            request.path = pathWithQuery.substr(0, queryPos);
            std::string query = pathWithQuery.substr(queryPos + 1);
            
            // Simple query parsing
            size_t pos = 0;
            while (pos < query.length()) {
                size_t eqPos = query.find('=', pos);
                size_t ampPos = query.find('&', pos);
                if (eqPos != std::string::npos) {
                    std::string key = query.substr(pos, eqPos - pos);
                    std::string value;
                    if (ampPos != std::string::npos) {
                        value = query.substr(eqPos + 1, ampPos - eqPos - 1);
                        pos = ampPos + 1;
                    } else {
                        value = query.substr(eqPos + 1);
                        pos = query.length();
                    }
                    request.queryParams[key] = value;
                } else {
                    break;
                }
            }
        } else {
            request.path = pathWithQuery;
        }
    }
    
    // Parse headers and body (simplified)
    bool headerSection = true;
    while (std::getline(stream, line) && headerSection) {
        if (line == "\r" || line.empty()) {
            headerSection = false;
            break;
        }
    }
    
    std::ostringstream bodyStream;
    while (std::getline(stream, line)) {
        bodyStream << line;
    }
    request.body = bodyStream.str();
    
    return request;
}

std::string SystemWebService::generateHttpResponse(int statusCode, const std::string& contentType, const std::string& body) {
    std::ostringstream response;
    
    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "Unknown"; break;
    }
    
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "\r\n";
    response << body;
    
    return response.str();
}

std::string SystemWebService::handleGetStatus() {
    std::lock_guard<std::mutex> lock(componentsMutex_);
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"running\": " << (systemStatus_.running ? "true" : "false") << ",\n";
    json << "  \"version\": \"" << escapeJson(systemStatus_.version) << "\",\n";
    json << "  \"uptime_seconds\": " << systemStatus_.uptimeSeconds << ",\n";
    json << "  \"data_points_collected\": " << systemStatus_.dataPointsCollected << ",\n";
    json << "  \"last_ml_training\": " << systemStatus_.lastMLTraining << ",\n";
    json << "  \"last_schedule_generation\": " << systemStatus_.lastScheduleGeneration << ",\n";
    json << "  \"mqtt_connected\": " << (systemStatus_.mqttConnected ? "true" : "false") << ",\n";
    json << "  \"web_server_running\": " << (systemStatus_.webServerRunning ? "true" : "false") << ",\n";
    json << "  \"sensors_count\": " << sensors_.size() << ",\n";
    json << "  \"appliances_count\": " << appliances_.size() << "\n";
    json << "}";
    
    return json.str();
}

std::string SystemWebService::handleGetSensors() {
    std::lock_guard<std::mutex> lock(componentsMutex_);
    
    std::ostringstream json;
    json << "{\n  \"sensors\": [\n";
    
    for (size_t i = 0; i < sensors_.size(); ++i) {
        auto& sensor = sensors_[i];
        json << "    {\n";
        json << "      \"id\": \"" << escapeJson(sensor->getId()) << "\",\n";
        json << "      \"name\": \"" << escapeJson(sensor->getName()) << "\",\n";
        json << "      \"enabled\": " << (sensor->isEnabled() ? "true" : "false") << "\n";
        json << "    }";
        if (i < sensors_.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ]\n}";
    return json.str();
}

std::string SystemWebService::handleGetAppliances() {
    std::lock_guard<std::mutex> lock(componentsMutex_);
    
    std::ostringstream json;
    json << "{\n  \"appliances\": [\n";
    
    for (size_t i = 0; i < appliances_.size(); ++i) {
        auto& appliance = appliances_[i];
        json << "    {\n";
        json << "      \"id\": \"" << escapeJson(appliance->getId()) << "\",\n";
        json << "      \"name\": \"" << escapeJson(appliance->getName()) << "\",\n";
        json << "      \"power\": " << appliance->getPowerConsumption() << ",\n";
        json << "      \"status\": \"" << (appliance->isOn() ? "on" : "off") << "\",\n";
        json << "      \"deferrable\": " << (appliance->isDeferrable() ? "true" : "false") << "\n";
        json << "    }";
        if (i < appliances_.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ]\n}";
    return json.str();
}

std::string SystemWebService::handleGetSchedule() {
    std::lock_guard<std::mutex> lock(componentsMutex_);
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"estimated_cost\": " << currentSchedule_.estimatedCost << ",\n";
    json << "  \"estimated_consumption\": " << currentSchedule_.estimatedConsumption << ",\n";
    json << "  \"actions\": [\n";
    
    for (size_t i = 0; i < currentSchedule_.actions.size(); ++i) {
        const auto& action = currentSchedule_.actions[i];
        json << "    {\n";
        json << "      \"hour\": " << action.hour << ",\n";
        json << "      \"appliance_id\": \"" << escapeJson(action.applianceId) << "\",\n";
        json << "      \"action\": \"" << escapeJson(action.action) << "\",\n";
        json << "      \"value\": " << action.value << ",\n";
        json << "      \"reason\": \"" << escapeJson(action.reason) << "\"\n";
        json << "    }";
        if (i < currentSchedule_.actions.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ]\n}";
    return json.str();
}

std::string SystemWebService::handleGetHistoricalData(const std::map<std::string, std::string>& params) {
    int days = 7;  // Default to last 7 days
    
    auto it = params.find("days");
    if (it != params.end()) {
        try {
            days = std::stoi(it->second);
        } catch (...) {
            days = 7;
        }
    }
    
    auto data = dataCollector_->getRecentData(days);
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"days\": " << days << ",\n";
    json << "  \"data_points\": " << data.size() << ",\n";
    json << "  \"data\": [\n";
    
    for (size_t i = 0; i < data.size() && i < 100; ++i) {  // Limit to 100 points for response size
        const auto& point = data[i];
        json << "    {\n";
        json << "      \"hour\": " << point.hour << ",\n";
        json << "      \"day_of_week\": " << point.dayOfWeek << ",\n";
        json << "      \"outdoor_temp\": " << point.outdoorTemp << ",\n";
        json << "      \"solar_production\": " << point.solarProduction << ",\n";
        json << "      \"energy_cost\": " << point.energyCost << "\n";
        json << "    }";
        if (i < data.size() - 1 && i < 99) json << ",";
        json << "\n";
    }
    
    json << "  ]\n}";
    return json.str();
}

std::string SystemWebService::handleGetPredictions(const std::map<std::string, std::string>& params) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"predictions\": [\n";
    
    // Generate predictions for next 24 hours
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    int currentHour = localTime->tm_hour;
    int currentDayOfWeek = localTime->tm_wday;
    
    auto predictions = mlPredictor_->predictNext24Hours(currentHour, currentDayOfWeek);
    
    for (size_t i = 0; i < predictions.size(); ++i) {
        const auto& pred = predictions[i];
        json << "    {\n";
        json << "      \"hour\": " << pred.hour << ",\n";
        json << "      \"predicted_cost\": " << pred.predictedEnergyCost << ",\n";
        json << "      \"predicted_solar\": " << pred.predictedSolarProduction << ",\n";
        json << "      \"predicted_temp\": " << pred.predictedOutdoorTemp << ",\n";
        json << "      \"confidence\": " << pred.confidenceScore << "\n";
        json << "    }";
        if (i < predictions.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ]\n}";
    return json.str();
}

std::string SystemWebService::handlePostApplianceControl(const std::string& body) {
    // Simple JSON parsing for appliance control
    // Expected format: {"appliance_id": "heater_main", "action": "on"}
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"message\": \"Appliance control feature available in full implementation\"\n";
    json << "}";
    
    return json.str();
}

std::string SystemWebService::handleGetDashboard() {
    return generateDashboardHTML();
}

std::string SystemWebService::generateDashboardHTML() {
    std::ostringstream html;
    
    html << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Home Automation Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1400px;
            margin: 0 auto;
        }
        .header {
            background: white;
            border-radius: 12px;
            padding: 30px;
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .header h1 {
            color: #667eea;
            margin-bottom: 10px;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 20px;
        }
        .card {
            background: white;
            border-radius: 12px;
            padding: 25px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .card h2 {
            color: #667eea;
            margin-bottom: 15px;
            font-size: 1.3em;
        }
        .status-indicator {
            display: inline-block;
            width: 10px;
            height: 10px;
            border-radius: 50%;
            margin-right: 8px;
        }
        .status-online { background: #10b981; }
        .status-offline { background: #ef4444; }
        .stat {
            padding: 10px 0;
            border-bottom: 1px solid #e5e7eb;
        }
        .stat:last-child { border-bottom: none; }
        .stat-label {
            color: #6b7280;
            font-size: 0.9em;
        }
        .stat-value {
            color: #1f2937;
            font-size: 1.2em;
            font-weight: 600;
            margin-top: 5px;
        }
        .sensor-item, .appliance-item {
            padding: 12px;
            margin: 8px 0;
            background: #f9fafb;
            border-radius: 8px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .btn {
            background: #667eea;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 0.9em;
        }
        .btn:hover { background: #5568d3; }
        .refresh-btn {
            float: right;
            background: #10b981;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🏠 Home Automation Dashboard</h1>
            <p>Real-time monitoring and control system</p>
            <button class="btn refresh-btn" onclick="refreshData()">🔄 Refresh</button>
        </div>
        
        <div class="grid">
            <div class="card">
                <h2>System Status</h2>
                <div id="system-status">Loading...</div>
            </div>
            
            <div class="card">
                <h2>Sensors</h2>
                <div id="sensors">Loading...</div>
            </div>
            
            <div class="card">
                <h2>Appliances</h2>
                <div id="appliances">Loading...</div>
            </div>
            
            <div class="card" style="grid-column: span 2;">
                <h2>Day-Ahead Schedule</h2>
                <div id="schedule">Loading...</div>
            </div>
        </div>
    </div>
    
    <script>
        async function loadSystemStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                
                const uptime = Math.floor(data.uptime_seconds / 3600) + 'h ' + 
                              Math.floor((data.uptime_seconds % 3600) / 60) + 'm';
                
                document.getElementById('system-status').innerHTML = `
                    <div class="stat">
                        <div class="stat-label">Status</div>
                        <div class="stat-value">
                            <span class="status-indicator ${data.running ? 'status-online' : 'status-offline'}"></span>
                            ${data.running ? 'Online' : 'Offline'}
                        </div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">Uptime</div>
                        <div class="stat-value">${uptime}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">Data Points Collected</div>
                        <div class="stat-value">${data.data_points_collected.toLocaleString()}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">MQTT Connection</div>
                        <div class="stat-value">
                            <span class="status-indicator ${data.mqtt_connected ? 'status-online' : 'status-offline'}"></span>
                            ${data.mqtt_connected ? 'Connected' : 'Disconnected'}
                        </div>
                    </div>
                `;
            } catch (error) {
                document.getElementById('system-status').innerHTML = '<p>Error loading status</p>';
            }
        }
        
        async function loadSensors() {
            try {
                const response = await fetch('/api/sensors');
                const data = await response.json();
                
                let html = '';
                data.sensors.forEach(sensor => {
                    html += `
                        <div class="sensor-item">
                            <div>
                                <strong>${sensor.name}</strong><br>
                                <span style="color: #6b7280; font-size: 0.9em;">${sensor.id}</span>
                            </div>
                            <div style="text-align: right;">
                                <strong>${sensor.value.toFixed(2)}</strong> ${sensor.unit}
                            </div>
                        </div>
                    `;
                });
                
                document.getElementById('sensors').innerHTML = html || '<p>No sensors registered</p>';
            } catch (error) {
                document.getElementById('sensors').innerHTML = '<p>Error loading sensors</p>';
            }
        }
        
        async function loadAppliances() {
            try {
                const response = await fetch('/api/appliances');
                const data = await response.json();
                
                let html = '';
                data.appliances.forEach(appliance => {
                    html += `
                        <div class="appliance-item">
                            <div>
                                <strong>${appliance.name}</strong><br>
                                <span style="color: #6b7280; font-size: 0.9em;">
                                    ${appliance.power} kW
                                    ${appliance.deferrable ? '• Deferrable' : ''}
                                </span>
                            </div>
                            <div>
                                <span class="status-indicator ${appliance.status === 'on' ? 'status-online' : 'status-offline'}"></span>
                                ${appliance.status.toUpperCase()}
                            </div>
                        </div>
                    `;
                });
                
                document.getElementById('appliances').innerHTML = html || '<p>No appliances registered</p>';
            } catch (error) {
                document.getElementById('appliances').innerHTML = '<p>Error loading appliances</p>';
            }
        }
        
        async function loadSchedule() {
            try {
                const response = await fetch('/api/schedule');
                const data = await response.json();
                
                let html = `
                    <div style="margin-bottom: 20px;">
                        <strong>Estimated Cost:</strong> $${data.estimated_cost.toFixed(2)} | 
                        <strong>Consumption:</strong> ${data.estimated_consumption.toFixed(2)} kWh
                    </div>
                    <div style="max-height: 400px; overflow-y: auto;">
                `;
                
                // Group actions by hour
                const actionsByHour = {};
                data.actions.forEach(action => {
                    if (!actionsByHour[action.hour]) {
                        actionsByHour[action.hour] = [];
                    }
                    actionsByHour[action.hour].push(action);
                });
                
                // Display first 10 hours
                Object.keys(actionsByHour).slice(0, 10).forEach(hour => {
                    html += `<div style="margin-bottom: 15px; padding: 10px; background: #f9fafb; border-radius: 8px;">`;
                    html += `<strong>Hour ${hour}:00</strong><br>`;
                    actionsByHour[hour].forEach(action => {
                        html += `<div style="margin-left: 15px; margin-top: 5px; color: #6b7280; font-size: 0.9em;">`;
                        html += `• ${action.appliance_id}: ${action.action} - ${action.reason}`;
                        html += `</div>`;
                    });
                    html += `</div>`;
                });
                
                html += '</div>';
                
                document.getElementById('schedule').innerHTML = html;
            } catch (error) {
                document.getElementById('schedule').innerHTML = '<p>Error loading schedule</p>';
            }
        }
        
        async function refreshData() {
            loadSystemStatus();
            loadSensors();
            loadAppliances();
            loadSchedule();
        }
        
        // Initial load
        refreshData();
        
        // Auto-refresh every 10 seconds
        setInterval(refreshData, 10000);
    </script>
</body>
</html>)HTML";
    
    return html.str();
}

std::string SystemWebService::escapeJson(const std::string& str) {
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
