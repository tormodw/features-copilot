#include "ConfigWebServer.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

ConfigWebServer::ConfigWebServer(std::shared_ptr<Config> config, int port)
    : config_(config)
    , port_(port)
    , running_(false) {
}

ConfigWebServer::~ConfigWebServer() {
    stop();
}

bool ConfigWebServer::start() {
    if (running_) {
        std::cerr << "Server already running" << std::endl;
        return false;
    }
    
    running_ = true;
    serverThread_ = std::thread(&ConfigWebServer::serverLoop, this);
    
    std::cout << "Web server started on port " << port_ << std::endl;
    std::cout << "Access configuration at: " << getServerUrl() << std::endl;
    
    return true;
}

void ConfigWebServer::stop() {
    if (running_) {
        running_ = false;
        if (serverThread_.joinable()) {
            serverThread_.join();
        }
        std::cout << "Web server stopped" << std::endl;
    }
}

bool ConfigWebServer::isRunning() const {
    return running_;
}

std::string ConfigWebServer::getServerUrl() const {
    return "http://localhost:" + std::to_string(port_);
}

void ConfigWebServer::serverLoop() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        running_ = false;
        return;
    }
    
    // Allow reuse of address
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        close(serverSocket);
        running_ = false;
        return;
    }
    
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket);
        running_ = false;
        return;
    }
    
    // Set socket to non-blocking mode for graceful shutdown
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            // Check if it's just a timeout (expected)
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        // Handle connection in the same thread (simple implementation)
        handleConnection(clientSocket);
        close(clientSocket);
    }
    
    close(serverSocket);
}

void ConfigWebServer::handleConnection(int clientSocket) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        return;
    }
    
    std::string requestData(buffer, bytesRead);
    HttpRequest request = parseRequest(requestData);
    
    std::string response;
    
    if (request.path == "/" || request.path == "/index.html") {
        response = generateHttpResponse(200, "text/html", handleGetIndex());
    } else if (request.path == "/api/config" && request.method == "GET") {
        response = generateHttpResponse(200, "application/json", handleGetConfig());
    } else if (request.path == "/api/config" && request.method == "POST") {
        response = generateHttpResponse(200, "application/json", handlePostConfig(request.body));
    } else {
        response = generateHttpResponse(404, "text/plain", "Not Found");
    }
    
    send(clientSocket, response.c_str(), response.length(), 0);
}

ConfigWebServer::HttpRequest ConfigWebServer::parseRequest(const std::string& requestData) {
    HttpRequest request;
    
    std::istringstream stream(requestData);
    std::string line;
    
    // Parse request line
    if (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        lineStream >> request.method >> request.path;
    }
    
    // Parse headers
    bool headerSection = true;
    while (std::getline(stream, line) && headerSection) {
        if (line == "\r" || line.empty()) {
            headerSection = false;
            break;
        }
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            request.headers[key] = value;
        }
    }
    
    // Parse body
    std::ostringstream bodyStream;
    while (std::getline(stream, line)) {
        bodyStream << line;
    }
    request.body = bodyStream.str();
    
    return request;
}

std::string ConfigWebServer::generateHttpResponse(int statusCode, 
                                                   const std::string& contentType,
                                                   const std::string& body) {
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

std::string ConfigWebServer::handleGetConfig() {
    return config_->toJson();
}

std::string ConfigWebServer::handlePostConfig(const std::string& body) {
    bool success = config_->fromJson(body);
    
    if (success) {
        // Save to file
        config_->saveToFile();
        return "{\"success\": true, \"message\": \"Configuration updated successfully\"}";
    } else {
        return "{\"success\": false, \"message\": \"Failed to parse configuration\"}";
    }
}

std::string ConfigWebServer::handleGetIndex() {
    return generateConfigPage();
}

std::string ConfigWebServer::generateConfigPage() {
    // Using raw string literal with delimiter to avoid script tag issues
    std::string html = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Home Automation Configuration</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 900px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        
        .header h1 {
            font-size: 2em;
            margin-bottom: 10px;
        }
        
        .header p {
            opacity: 0.9;
            font-size: 1.1em;
        }
        
        .content {
            padding: 30px;
        }
        
        .section {
            margin-bottom: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 8px;
        }
        
        .section h2 {
            color: #667eea;
            margin-bottom: 15px;
            font-size: 1.5em;
            display: flex;
            align-items: center;
        }
        
        .section h2::before {
            content: '⚙️';
            margin-right: 10px;
        }
        
        .form-group {
            margin-bottom: 20px;
        }
        
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 600;
        }
        
        input[type="text"],
        input[type="number"],
        textarea {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            font-size: 1em;
            transition: border-color 0.3s;
        }
        
        input[type="text"]:focus,
        input[type="number"]:focus,
        textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        
        textarea {
            min-height: 100px;
            font-family: inherit;
            resize: vertical;
        }
        
        .checkbox-group {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        input[type="checkbox"] {
            width: 20px;
            height: 20px;
            cursor: pointer;
        }
        
        .list-container {
            background: white;
            padding: 15px;
            border-radius: 6px;
            border: 2px solid #e0e0e0;
        }
        
        .list-item {
            display: flex;
            align-items: center;
            padding: 10px;
            margin-bottom: 8px;
            background: #f8f9fa;
            border-radius: 4px;
        }
        
        .list-item span {
            flex: 1;
        }
        
        .list-item button {
            background: #dc3545;
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 0.9em;
        }
        
        .list-item button:hover {
            background: #c82333;
        }
        
        .add-item-group {
            display: flex;
            gap: 10px;
            margin-top: 10px;
        }
        
        .add-item-group input {
            flex: 1;
        }
        
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 6px;
            font-size: 1em;
            cursor: pointer;
            transition: all 0.3s;
            font-weight: 600;
        }
        
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        
        .btn-secondary {
            background: #28a745;
            color: white;
        }
        
        .btn-secondary:hover {
            background: #218838;
        }
        
        .button-group {
            display: flex;
            gap: 15px;
            justify-content: center;
            margin-top: 30px;
        }
        
        .message {
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
            display: none;
        }
        
        .message.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        
        .message.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        
        .info-box {
            background: #e7f3ff;
            border-left: 4px solid #2196F3;
            padding: 15px;
            margin-bottom: 20px;
            border-radius: 4px;
        }
        
        .info-box p {
            margin: 5px 0;
            color: #0066cc;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🏠 Home Automation Configuration</h1>
            <p>Configure your home automation system settings</p>
        </div>
        
        <div class="content">
            <div id="message" class="message"></div>
            
            <div class="info-box">
                <p><strong>💡 Configuration is automatically saved when you click "Save Configuration"</strong></p>
                <p>Changes will be applied to the system immediately.</p>
            </div>
            
            <!-- MQTT Configuration -->
            <div class="section">
                <h2>MQTT Settings</h2>
                <div class="form-group">
                    <div class="checkbox-group">
                        <input type="checkbox" id="mqttEnabled" checked>
                        <label for="mqttEnabled">Enable MQTT</label>
                    </div>
                </div>
                <div class="form-group">
                    <label for="mqttBroker">MQTT Broker Address</label>
                    <input type="text" id="mqttBroker" placeholder="localhost" value="localhost">
                </div>
                <div class="form-group">
                    <label for="mqttPort">MQTT Port</label>
                    <input type="number" id="mqttPort" placeholder="1883" value="1883">
                </div>
            </div>
            
            <!-- Appliances Configuration -->
            <div class="section">
                <h2>⚙️ Appliances</h2>
                <div class="form-group">
                    <label>Configured Appliances</label>
                    <div class="list-container">
                        <div id="appliancesList"></div>
                        <div class="add-item-group">
                            <input type="text" id="newAppliance" placeholder="Enter appliance name (e.g., EV Charger)">
                            <button class="btn btn-secondary" onclick="addAppliance()">Add</button>
                        </div>
                    </div>
                </div>
            </div>
            
            <!-- Sensors Configuration -->
            <div class="section">
                <h2>Sensor Values</h2>
                <div class="form-group">
                    <label>Included Sensors</label>
                    <div class="list-container">
                        <div id="sensorsList"></div>
                        <div class="add-item-group">
                            <input type="text" id="newSensor" placeholder="Enter sensor name (e.g., temperature_indoor)">
                            <button class="btn btn-secondary" onclick="addSensor()">Add</button>
                        </div>
                    </div>
                </div>
            </div>
            
            <!-- Web Interface Configuration -->
            <div class="section">
                <h2>⚙️ Web Interface Settings</h2>
                <div class="form-group">
                    <div class="checkbox-group">
                        <input type="checkbox" id="webEnabled" checked>
                        <label for="webEnabled">Enable Web Interface</label>
                    </div>
                </div>
                <div class="form-group">
                    <label for="webPort">Web Interface Port</label>
                    <input type="number" id="webPort" placeholder="8080" value="8080">
                </div>
            </div>
            
            <!-- REST API Configuration -->
            <div class="section">
                <h2>🔌 REST API Settings</h2>
                <div class="form-group">
                    <div class="checkbox-group">
                        <input type="checkbox" id="restApiEnabled" checked>
                        <label for="restApiEnabled">Enable REST API</label>
                    </div>
                </div>
                <div class="form-group">
                    <label for="restApiPort">REST API Port</label>
                    <input type="number" id="restApiPort" placeholder="8081" value="8081">
                </div>
            </div>
            
            <div class="button-group">
                <button class="btn btn-primary" onclick="saveConfig()">💾 Save Configuration</button>
                <button class="btn btn-secondary" onclick="loadConfig()">🔄 Reload Configuration</button>
            </div>
        </div>
    </div>
)HTML";

    // Add JavaScript separately to avoid parsing issues
    html += R"HTML(
    <script>
        let config = null;
        
        window.onload = function() {
            loadConfig();
        };
        
        function showMessage(text, isError = false) {
            const messageEl = document.getElementById('message');
            messageEl.textContent = text;
            messageEl.className = 'message ' + (isError ? 'error' : 'success');
            messageEl.style.display = 'block';
            
            setTimeout(() => {
                messageEl.style.display = 'none';
            }, 5000);
        }
        
        async function loadConfig() {
            try {
                const response = await fetch('/api/config');
                config = await response.json();
                
                document.getElementById('mqttEnabled').checked = config.mqtt.enabled;
                document.getElementById('mqttBroker').value = config.mqtt.brokerAddress;
                document.getElementById('mqttPort').value = config.mqtt.port;
                document.getElementById('webEnabled').checked = config.webInterface.enabled;
                document.getElementById('webPort').value = config.webInterface.port;
                
                // Load REST API settings if present, otherwise use defaults
                if (config.restApi) {
                    document.getElementById('restApiEnabled').checked = config.restApi.enabled;
                    document.getElementById('restApiPort').value = config.restApi.port;
                } else {
                    // Default values if not in config
                    document.getElementById('restApiEnabled').checked = true;
                    document.getElementById('restApiPort').value = 8081;
                }
                
                // Convert old format to new format if needed
                if (config.deferrableLoads && !config.appliances) {
                    config.appliances = config.deferrableLoads.map(name => ({
                        name: name,
                        isDeferrable: true
                    }));
                }
                
                updateAppliancesList();
                updateSensorsList();
                
                showMessage('Configuration loaded successfully');
            } catch (error) {
                showMessage('Failed to load configuration: ' + error.message, true);
            }
        }
        
        function updateAppliancesList() {
            const list = document.getElementById('appliancesList');
            list.innerHTML = '';
            
            if (config.appliances && config.appliances.length > 0) {
                config.appliances.forEach((appliance, index) => {
                    const item = document.createElement('div');
                    item.className = 'list-item';
                    item.style.display = 'flex';
                    item.style.alignItems = 'center';
                    item.style.gap = '10px';
                    
                    // Checkbox for deferrable status
                    const checkbox = document.createElement('input');
                    checkbox.type = 'checkbox';
                    checkbox.checked = appliance.isDeferrable;
                    checkbox.onchange = () => toggleApplianceDeferrable(index);
                    checkbox.style.cursor = 'pointer';
                    item.appendChild(checkbox);
                    
                    // Appliance name
                    const span = document.createElement('span');
                    span.textContent = appliance.name;
                    span.style.flex = '1';
                    item.appendChild(span);
                    
                    // Deferrable label
                    const label = document.createElement('span');
                    label.textContent = appliance.isDeferrable ? '(Deferrable)' : '(Not Deferrable)';
                    label.style.fontSize = '0.9em';
                    label.style.color = appliance.isDeferrable ? '#10b981' : '#6b7280';
                    item.appendChild(label);
                    
                    // Remove button
                    const button = document.createElement('button');
                    button.textContent = 'Remove';
                    button.onclick = () => removeAppliance(index);
                    item.appendChild(button);
                    
                    list.appendChild(item);
                });
            } else {
                const p = document.createElement('p');
                p.style.color = '#999';
                p.style.padding = '10px';
                p.textContent = 'No appliances configured';
                list.appendChild(p);
            }
        }
        
        function updateSensorsList() {
            const list = document.getElementById('sensorsList');
            list.innerHTML = '';
            
            if (config.sensors && config.sensors.length > 0) {
                config.sensors.forEach((sensor, index) => {
                    const item = document.createElement('div');
                    item.className = 'list-item';
                    
                    const span = document.createElement('span');
                    span.textContent = sensor;  // Use textContent to prevent XSS
                    item.appendChild(span);
                    
                    const button = document.createElement('button');
                    button.textContent = 'Remove';
                    button.onclick = () => removeSensor(index);
                    item.appendChild(button);
                    
                    list.appendChild(item);
                });
            } else {
                const p = document.createElement('p');
                p.style.color = '#999';
                p.style.padding = '10px';
                p.textContent = 'No sensors configured';
                list.appendChild(p);
            }
        }
        
        function addAppliance() {
            const input = document.getElementById('newAppliance');
            const value = input.value.trim();
            
            if (value) {
                if (!config.appliances) {
                    config.appliances = [];
                }
                
                // Check if appliance already exists
                const exists = config.appliances.some(a => a.name === value);
                if (!exists) {
                    config.appliances.push({
                        name: value,
                        isDeferrable: false  // Default to not deferrable
                    });
                    updateAppliancesList();
                    input.value = '';
                    showMessage('Appliance added: ' + value);
                } else {
                    showMessage('This appliance is already in the list', true);
                }
            }
        }
        
        function removeAppliance(index) {
            const removed = config.appliances[index];
            config.appliances.splice(index, 1);
            updateAppliancesList();
            showMessage('Removed appliance: ' + removed.name);
        }
        
        function toggleApplianceDeferrable(index) {
            config.appliances[index].isDeferrable = !config.appliances[index].isDeferrable;
            updateAppliancesList();
            const status = config.appliances[index].isDeferrable ? 'deferrable' : 'not deferrable';
            showMessage(config.appliances[index].name + ' is now ' + status);
        }
        
        function addSensor() {
            const input = document.getElementById('newSensor');
            const value = input.value.trim();
            
            if (value) {
                if (!config.sensors) {
                    config.sensors = [];
                }
                
                if (!config.sensors.includes(value)) {
                    config.sensors.push(value);
                    updateSensorsList();
                    input.value = '';
                    showMessage('Sensor added: ' + value);
                } else {
                    showMessage('This sensor is already in the list', true);
                }
            }
        }
        
        function removeSensor(index) {
            const removed = config.sensors[index];
            config.sensors.splice(index, 1);
            updateSensorsList();
            showMessage('Removed sensor: ' + removed);
        }
        
        async function saveConfig() {
            try {
                config.mqtt.enabled = document.getElementById('mqttEnabled').checked;
                config.mqtt.brokerAddress = document.getElementById('mqttBroker').value;
                config.mqtt.port = parseInt(document.getElementById('mqttPort').value);
                config.webInterface.enabled = document.getElementById('webEnabled').checked;
                config.webInterface.port = parseInt(document.getElementById('webPort').value);
                
                // Save REST API settings
                if (!config.restApi) {
                    config.restApi = {};
                }
                config.restApi.enabled = document.getElementById('restApiEnabled').checked;
                config.restApi.port = parseInt(document.getElementById('restApiPort').value);
                
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(config)
                });
                
                const result = await response.json();
                
                if (result.success) {
                    showMessage('✅ Configuration saved successfully!');
                } else {
                    showMessage('Failed to save configuration: ' + result.message, true);
                }
            } catch (error) {
                showMessage('Error saving configuration: ' + error.message, true);
            }
        }
        
        document.getElementById('newDeferrableLoad').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                addDeferrableLoad();
            }
        });
        
        document.getElementById('newSensor').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                addSensor();
            }
        });
    <)HTML" + std::string("/script>\n</body>\n</html>");
    
    return html;
}
