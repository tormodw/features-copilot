# Production System Deployment Guide

## Overview

The Home Automation System production executable provides a complete, ready-to-deploy solution with:
- Hot-reloadable configuration
- Continuous ML training with historical data
- Day-ahead optimization
- Dual web interfaces (configuration + monitoring)
- RESTful API
- Graceful shutdown and signal handling

## Quick Start

### Building

```bash
cd /path/to/features-copilot
mkdir build && cd build
cmake ..
make home_automation_production
```

### Running

```bash
./home_automation_production
```

The system will:
1. Load or create `config.json`
2. Start configuration web interface on port 8080
3. Initialize MQTT and Home Assistant integration
4. Create sensors and appliances
5. Start historical data collection
6. Initialize continuous ML training
7. Generate initial day-ahead schedule
8. Start monitoring web service on port 8081
9. Enter main event loop

## Web Interfaces

### Configuration Interface (Port 8080)

**URL:** http://localhost:8080

**Features:**
- Manage deferrable loads (add/remove)
- Configure MQTT settings (broker, port, enable/disable)
- Manage sensors (add/remove)
- Configure web interface settings
- Hot reload capability
- All changes saved automatically to `config.json`

**Screenshot:**
![Configuration Interface](https://github.com/user-attachments/assets/bceebf87-02c1-4f71-a8b5-ae699fae1222)

### System Monitoring Dashboard (Port 8081)

**URL:** http://localhost:8081/dashboard

**Features:**
- Real-time system status (uptime, data points, MQTT connection)
- Live sensor readings
- Appliance status with power ratings
- Day-ahead schedule with cost estimates
- Auto-refresh every 10 seconds

**Screenshot:**
![System Dashboard](https://github.com/user-attachments/assets/20e3d293-86e5-43b9-906e-fb06774ebabb)

## REST API

Base URL: `http://localhost:8081/api`

### Endpoints

#### GET /api/status
Returns current system status.

**Response:**
```json
{
  "running": true,
  "version": "1.0.0",
  "uptime_seconds": 3600,
  "data_points_collected": 48,
  "last_ml_training": 1708174800,
  "last_schedule_generation": 1708178400,
  "mqtt_connected": true,
  "web_server_running": true,
  "sensors_count": 5,
  "appliances_count": 4
}
```

#### GET /api/sensors
Returns all registered sensors.

**Response:**
```json
{
  "sensors": [
    {
      "id": "temp_indoor",
      "name": "Indoor Temperature",
      "enabled": true
    }
  ]
}
```

#### GET /api/appliances
Returns all appliances with status.

**Response:**
```json
{
  "appliances": [
    {
      "id": "heater_main",
      "name": "Main Heater",
      "power": 2.5,
      "status": "off",
      "deferrable": false
    }
  ]
}
```

#### GET /api/schedule
Returns current day-ahead schedule.

**Response:**
```json
{
  "estimated_cost": 10.78,
  "estimated_consumption": 71.5,
  "actions": [
    {
      "hour": 0,
      "appliance_id": "ev_charger",
      "action": "on",
      "value": 0.0,
      "reason": "Optimal charging hour"
    }
  ]
}
```

#### GET /api/historical?days=7
Returns historical data for the specified number of days.

**Parameters:**
- `days` - Number of days to retrieve (default: 7)

**Response:**
```json
{
  "days": 7,
  "data_points": 168,
  "data": [
    {
      "hour": 0,
      "day_of_week": 1,
      "outdoor_temp": 15.5,
      "solar_production": 0.0,
      "energy_cost": 0.08
    }
  ]
}
```

#### GET /api/predictions
Returns ML predictions for next 24 hours.

**Response:**
```json
{
  "predictions": [
    {
      "hour": 0,
      "predicted_cost": 0.08,
      "predicted_solar": 0.0,
      "predicted_temp": 15.5,
      "confidence": 0.85
    }
  ]
}
```

## Configuration

### config.json

The system uses `config.json` for all configuration. This file is automatically created with defaults on first run.

**Default Configuration:**
```json
{
  "mqtt": {
    "enabled": true,
    "brokerAddress": "localhost",
    "port": 1883
  },
  "deferrableLoads": [
    "ev_charger",
    "decorative_lights",
    "pool_pump"
  ],
  "sensors": [
    "temperature_indoor",
    "temperature_outdoor",
    "energy_meter",
    "solar_production",
    "ev_charger_power"
  ],
  "webInterface": {
    "enabled": true,
    "port": 8080
  }
}
```

### Hot Reload

Configuration changes are detected and applied automatically:

**Method 1: File Watching** (every 30 seconds)
- Modify `config.json`
- System detects changes automatically
- Configuration reloaded without restart

**Method 2: Web Interface**
- Change settings via http://localhost:8080
- Click "Save Configuration"
- Changes applied immediately

**Method 3: Signal**
```bash
kill -USR1 <pid>
```

## Signal Handling

The production system responds to the following signals:

- **SIGINT** (Ctrl+C) - Graceful shutdown
- **SIGTERM** - Graceful shutdown
- **SIGUSR1** - Reload configuration

**Graceful Shutdown Process:**
1. Stop continuous training scheduler
2. Save all historical data to file
3. Stop system web service
4. Stop configuration web interface
5. Disconnect MQTT
6. Exit cleanly

## Data Persistence

### Historical Data

**File:** `historical_data.csv`

**Collection:** Every 60 minutes (configurable)

**Retention:** 90 days (configurable)

**Format:**
```
hour,dayOfWeek,outdoorTemp,solarProduction,energyCost
0,1,15.5,0.0,0.08
```

**Configuration:**
```cpp
DataCollectionConfig dataConfig;
dataConfig.maxDaysToRetain = 90;
dataConfig.enablePersistence = true;
dataConfig.persistenceFile = "historical_data.csv";
dataConfig.collectionIntervalMinutes = 60;
```

## Machine Learning

### Continuous Training

**Schedule:** Every 24 hours (configurable)

**Minimum Data:** 168 points (7 days of hourly data)

**Process:**
1. Collect historical data every hour
2. When sufficient data available, initial training
3. Automatic retraining every 24 hours
4. Model improves continuously with new data

**Training Callback:**
```cpp
trainingScheduler->setTrainingCallback([](bool success, size_t dataPoints) {
    if (success) {
        std::cout << "Model retrained with " << dataPoints << " points" << std::endl;
    }
});
```

## Day-Ahead Optimization

### Schedule Generation

**Frequency:** Every hour

**Process:**
1. Predict next 24 hours using ML model
2. Identify optimal hours for:
   - EV charging (4 best hours)
   - Pre-heating/cooling
   - Deferrable load operation
3. Generate schedule with cost estimates
4. Execute actions for current hour

**Configuration:**
```cpp
dayAheadOptimizer->setTargetTemperature(21.0);
dayAheadOptimizer->setEVChargingHoursNeeded(4);
deferrableController->setPriceThreshold(0.15);
deferrableController->setBusyHourThreshold(0.13);
```

## Event Loop

The main event loop handles periodic tasks:

### Tasks

1. **Status Update** (every 10 seconds)
   - Update web service with current status
   - Refresh uptime, data points, connection status

2. **Configuration Check** (every 30 seconds)
   - Monitor config.json modification time
   - Reload if changed

3. **Data Collection** (every 60 minutes)
   - Record current sensor readings
   - Save to historical data file
   - Trigger ML training if scheduled

4. **Schedule Generation** (every 60 minutes)
   - Regenerate day-ahead schedule
   - Update web service with new schedule
   - Execute actions for current hour

## Production Deployment

### Prerequisites

- C++17 compatible compiler
- CMake 3.10+
- pthread support

### Optional Dependencies

- **mosquitto** - For real MQTT broker connection
- **curl** - For REST API integration with external services

### System Requirements

- **Memory:** Minimum 256MB RAM
- **Disk:** 100MB for binaries + storage for historical data
- **CPU:** Any modern processor
- **Network:** For MQTT and web interfaces

### Deployment Steps

1. **Build the production executable**
   ```bash
   mkdir build && cd build
   cmake ..
   make home_automation_production
   ```

2. **Create configuration**
   ```bash
   # System creates default config.json on first run
   ./home_automation_production
   # Stop with Ctrl+C after config is created
   ```

3. **Customize configuration**
   ```bash
   # Edit config.json with your settings
   vim config.json
   ```

4. **Run in production**
   ```bash
   # Start with nohup for background operation
   nohup ./home_automation_production > /var/log/home_automation.log 2>&1 &
   
   # Or use systemd (recommended)
   sudo systemctl start home_automation
   ```

### Systemd Service

Create `/etc/systemd/system/home-automation.service`:

```ini
[Unit]
Description=Home Automation System
After=network.target

[Service]
Type=simple
User=homeautomation
WorkingDirectory=/opt/home-automation
ExecStart=/opt/home-automation/home_automation_production
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable home-automation
sudo systemctl start home-automation
```

### Monitoring

Check system status:
```bash
# Via systemd
sudo systemctl status home-automation

# Via web interface
curl http://localhost:8081/api/status

# View logs
tail -f /var/log/home_automation.log
```

### Backup

Important files to backup:
- `config.json` - Configuration
- `historical_data.csv` - Training data

```bash
# Backup script
tar -czf backup-$(date +%Y%m%d).tar.gz config.json historical_data.csv
```

## Troubleshooting

### Port Already in Use

**Symptoms:** Web servers fail to start

**Solution:**
1. Check what's using the ports:
   ```bash
   sudo lsof -i :8080
   sudo lsof -i :8081
   ```
2. Kill conflicting process or change ports in config.json

### Insufficient Data for Training

**Symptoms:** Warning about insufficient data points

**Solution:** Wait for data collection. System needs 168 hourly data points (7 days) before first training.

### Configuration Not Reloading

**Symptoms:** Changes to config.json not applied

**Solution:**
1. Verify file permissions
2. Send SIGUSR1 signal manually: `kill -USR1 <pid>`
3. Restart system

### High Memory Usage

**Symptoms:** System using excessive memory

**Solution:**
1. Reduce historical data retention in config
2. Increase data collection interval
3. Clear old historical data

## Security Considerations

### Web Interface Security

**Current:** Both web interfaces are open without authentication

**Production Recommendations:**
1. Add authentication middleware
2. Use HTTPS with SSL/TLS certificates
3. Implement rate limiting
4. Restrict access by IP address
5. Use reverse proxy (nginx) with authentication

### MQTT Security

**Production Recommendations:**
1. Use TLS encryption
2. Enable username/password authentication
3. Use client certificates
4. Configure broker ACLs

### File Permissions

Secure configuration and data files:
```bash
chmod 600 config.json
chmod 600 historical_data.csv
chown homeautomation:homeautomation *.json *.csv
```

## Performance Tuning

### Data Collection Interval

Adjust based on needs:
- **Real-time monitoring:** 5-15 minutes
- **Standard operation:** 60 minutes (default)
- **Low overhead:** 120 minutes

### ML Training Frequency

Adjust based on data stability:
- **Rapidly changing:** 12 hours
- **Standard:** 24 hours (default)
- **Stable patterns:** 48-72 hours

### Web Interface Refresh

Dashboard auto-refresh: 10 seconds (default)
- Adjust in JavaScript for different update frequency

## Integration Examples

### Curl Commands

```bash
# Get system status
curl http://localhost:8081/api/status

# Get all sensors
curl http://localhost:8081/api/sensors

# Get schedule
curl http://localhost:8081/api/schedule

# Get predictions
curl http://localhost:8081/api/predictions

# Get historical data (last 30 days)
curl "http://localhost:8081/api/historical?days=30"
```

### Python Integration

```python
import requests

# Get system status
response = requests.get('http://localhost:8081/api/status')
status = response.json()
print(f"System uptime: {status['uptime_seconds']} seconds")

# Get current schedule
response = requests.get('http://localhost:8081/api/schedule')
schedule = response.json()
print(f"Estimated cost: ${schedule['estimated_cost']}")
```

### Home Assistant Integration

Add to Home Assistant configuration:

```yaml
sensor:
  - platform: rest
    name: "Home Automation Status"
    resource: "http://localhost:8081/api/status"
    json_attributes:
      - uptime_seconds
      - data_points_collected
      - mqtt_connected
    value_template: "{{ value_json.running }}"
```

## Appendix

### Directory Structure

```
home-automation/
├── build/
│   └── home_automation_production
├── config.json
├── historical_data.csv
└── logs/
    └── home_automation.log
```

### Log Levels

The production system uses minimal logging by default. For verbose logging:

```cpp
DataCollectionConfig dataConfig;
dataConfig.verboseLogging = true;  // Enable detailed logging

TrainingScheduleConfig trainingConfig;
trainingConfig.verboseLogging = true;  // Enable training logs
```

### Support

For issues or questions:
1. Check this documentation
2. Review application logs
3. Check web interface status
4. Verify configuration file
5. Open GitHub issue with logs and config
