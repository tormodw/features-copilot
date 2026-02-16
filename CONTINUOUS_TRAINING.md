# Continuous ML Training and Historical Data Collection Guide

## Overview

This guide explains how to continuously update historical data and retrain the ML predictor during system operation. This feature enables the home automation system to continuously improve its predictions by learning from real operational data.

## Problem Statement

"How can I continuously update historical data and train the ML predictor during use?"

## Solution Overview

The system provides three main components for continuous learning:

1. **HistoricalDataCollector** - Collects and stores operational data in real-time
2. **MLTrainingScheduler** - Manages periodic retraining of the ML model
3. **File Persistence** - Ensures data survives system restarts

## Architecture

### Data Flow

```
Sensors/APIs → HistoricalDataCollector → File Storage
                        ↓
            MLTrainingScheduler (periodic)
                        ↓
                  MLPredictor (retrained)
                        ↓
            Improved Predictions
```

### Components

#### HistoricalDataCollector

Continuously collects operational data including:
- Outdoor temperature
- Solar production
- Energy costs
- Hour of day and day of week

**Key Features:**
- In-memory storage with deque for efficient old data removal
- Automatic data retention management (default: 90 days)
- CSV file persistence for durability
- Automatic save every 24 data points
- Recent data retrieval (last N days)

#### MLTrainingScheduler

Manages the retraining process:
- Configurable retraining interval (default: 24 hours)
- Minimum data requirements (default: 168 points / 7 days)
- Automatic and manual retraining modes
- Training completion callbacks
- Background thread for scheduled training

## Configuration

### HistoricalDataCollector Configuration

```cpp
DataCollectionConfig config;
config.maxDaysToRetain = 90;           // Keep last 90 days
config.enablePersistence = true;       // Save to file
config.persistenceFile = "historical_data.csv";
config.collectionIntervalMinutes = 60; // Collect every hour
```

**Parameters:**
- `maxDaysToRetain`: Maximum days of data to keep (prevents unbounded growth)
- `enablePersistence`: Enable/disable file storage
- `persistenceFile`: Path to CSV file for data storage
- `collectionIntervalMinutes`: Suggested collection interval

### MLTrainingScheduler Configuration

```cpp
TrainingScheduleConfig config;
config.retrainingIntervalHours = 24;    // Retrain every 24 hours
config.minDataPointsForTraining = 168; // Minimum 7 days
config.autoRetrain = true;              // Enable automatic retraining
config.verboseLogging = true;           // Enable detailed logs
```

**Parameters:**
- `retrainingIntervalHours`: Hours between automatic retraining
- `minDataPointsForTraining`: Minimum data points before training
- `autoRetrain`: Enable/disable automatic scheduled retraining
- `verboseLogging`: Enable/disable detailed logging

## Usage Examples

### Basic Setup

```cpp
#include "MLPredictor.h"
#include "HistoricalDataCollector.h"
#include "MLTrainingScheduler.h"

// Create ML predictor
auto mlPredictor = std::make_shared<MLPredictor>();

// Create data collector
DataCollectionConfig collectorConfig;
collectorConfig.maxDaysToRetain = 90;
collectorConfig.enablePersistence = true;
auto collector = std::make_shared<HistoricalDataCollector>(collectorConfig);

// Create training scheduler
TrainingScheduleConfig scheduleConfig;
scheduleConfig.retrainingIntervalHours = 24;
scheduleConfig.autoRetrain = true;
auto scheduler = std::make_shared<MLTrainingScheduler>(
    mlPredictor, collector, scheduleConfig);
```

### Initial Training with Sample Data

```cpp
// Generate initial sample data
auto initialData = HistoricalDataGenerator::generateSampleData(7);

// Add to collector
for (const auto& point : initialData) {
    collector->addDataPoint(point);
}

// Perform initial training
scheduler->triggerRetraining();
```

### Continuous Data Collection

During operation, collect data from sensors and APIs:

```cpp
// In your main loop or sensor callback
void updateData(double outdoorTemp, double solarProduction, double energyCost) {
    collector->recordCurrentState(outdoorTemp, solarProduction, energyCost);
}

// Example: Every hour
void hourlyUpdate() {
    double temp = temperatureSensor->getTemperature();
    double solar = solarSensor->getProduction();
    double cost = httpClient->getCurrentEnergyCost();
    
    collector->recordCurrentState(temp, solar, cost);
}
```

### Automatic Retraining

Enable automatic retraining on a schedule:

```cpp
// Start automatic retraining (runs in background thread)
scheduler->startAutoTraining();

// System continues operating...
// Retraining happens automatically every 24 hours

// When shutting down
scheduler->stopAutoTraining();
```

### Manual Retraining

Trigger retraining manually when needed:

```cpp
// Check if sufficient data is available
if (scheduler->hasSufficientData()) {
    // Trigger manual retraining
    bool success = scheduler->triggerRetraining();
    
    if (success) {
        std::cout << "Model retrained successfully" << std::endl;
    }
}
```

### Training Callbacks

Get notified when training completes:

```cpp
scheduler->setTrainingCallback([](bool success, size_t dataPoints) {
    if (success) {
        std::cout << "Training successful with " << dataPoints << " points" << std::endl;
        // Update UI, log metrics, etc.
    } else {
        std::cerr << "Training failed" << std::endl;
    }
});
```

### Data Persistence

Data is automatically saved to file. On startup:

```cpp
// Data is automatically loaded from file if it exists
auto collector = std::make_shared<HistoricalDataCollector>(config);
// Existing data is loaded in constructor

std::cout << "Loaded " << collector->getDataPointCount() << " data points" << std::endl;
```

Manually save if needed:

```cpp
// Save to configured file
collector->saveToFile();

// Or save to different file
collector->saveToFile("backup_data.csv");
```

### Retrieving Historical Data

Get recent data for analysis:

```cpp
// Get last 7 days
auto last7Days = collector->getRecentData(7);

// Get last 30 days
auto last30Days = collector->getRecentData(30);

// Get all data
auto allData = collector->getAllData();
```

## Integration Examples

### With Real Sensors

```cpp
class DataCollectionManager {
public:
    DataCollectionManager(std::shared_ptr<HistoricalDataCollector> collector)
        : collector_(collector) {
        
        // Subscribe to sensor events
        auto& eventMgr = EventManager::getInstance();
        
        eventMgr.subscribe(EventType::TEMPERATURE_CHANGE, 
            [this](const Event& e) {
                lastOutdoorTemp_ = e.getData("temperature");
                checkAndRecord();
            });
            
        eventMgr.subscribe(EventType::SOLAR_PRODUCTION_UPDATE,
            [this](const Event& e) {
                lastSolarProduction_ = e.getData("production_kw");
                checkAndRecord();
            });
            
        eventMgr.subscribe(EventType::ENERGY_COST_UPDATE,
            [this](const Event& e) {
                lastEnergyCost_ = e.getData("cost_per_kwh");
                checkAndRecord();
            });
    }
    
private:
    void checkAndRecord() {
        // Record if we have all data and enough time has passed
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
            now - lastRecordTime_);
        
        if (elapsed.count() >= 60) {  // Record every hour
            collector_->recordCurrentState(
                lastOutdoorTemp_, 
                lastSolarProduction_, 
                lastEnergyCost_
            );
            lastRecordTime_ = now;
        }
    }
    
    std::shared_ptr<HistoricalDataCollector> collector_;
    double lastOutdoorTemp_ = 0.0;
    double lastSolarProduction_ = 0.0;
    double lastEnergyCost_ = 0.0;
    std::chrono::system_clock::time_point lastRecordTime_;
};
```

### With Day-Ahead Optimizer

```cpp
// Setup continuous training
auto collector = std::make_shared<HistoricalDataCollector>();
auto mlPredictor = std::make_shared<MLPredictor>();
auto scheduler = std::make_shared<MLTrainingScheduler>(mlPredictor, collector);

// Start automatic retraining
scheduler->startAutoTraining();

// Use predictor with day-ahead optimizer
auto dayAheadOptimizer = std::make_shared<DayAheadOptimizer>(mlPredictor);

// Predictions automatically improve as model is retrained
auto schedule = dayAheadOptimizer->generateSchedule(currentHour, dayOfWeek);
```

## Best Practices

### Data Collection Frequency

- **Recommended**: Collect data every hour (60 minutes)
- **Minimum**: Every 30 minutes for faster learning
- **Maximum**: Every 4 hours (reduces data volume but slower learning)

### Retraining Schedule

- **Daily retraining (24 hours)**: Good balance for most systems
- **Twice daily (12 hours)**: For rapidly changing conditions
- **Weekly (168 hours)**: For stable, predictable patterns

### Data Retention

- **30 days**: Minimum for seasonal patterns
- **90 days**: Recommended for good seasonal coverage
- **365 days**: Full year of data for complete seasonal learning

### Minimum Data Requirements

- **7 days (168 hours)**: Absolute minimum for training
- **14 days (336 hours)**: Better for stable predictions
- **30 days (720 hours)**: Recommended for production use

## Performance Considerations

### Memory Usage

Memory usage depends on data retention:
- Each data point: ~40 bytes
- 90 days: ~86,400 points = ~3.5 MB
- 365 days: ~350,000 points = ~14 MB

This is minimal for modern systems.

### File I/O

- Auto-save every 24 points (once per day)
- Loading on startup is fast (< 1 second for 90 days)
- CSV format is human-readable for debugging

### Training Time

- Training is fast (< 100ms for typical datasets)
- Runs in background thread when using auto-training
- Does not block main application

## Troubleshooting

### Training Fails: Insufficient Data

```
MLTrainingScheduler: Insufficient data for training
  Current: 100 points
  Required: 168 points
```

**Solution**: Wait for more data to be collected or reduce `minDataPointsForTraining`.

### File Loading Errors

```
HistoricalDataCollector: Failed to open file for writing
```

**Solution**: Check file permissions and disk space.

### High Memory Usage

**Solution**: Reduce `maxDaysToRetain` in configuration.

### Training Too Frequent

**Solution**: Increase `retrainingIntervalHours` to reduce frequency.

## Testing

Run the test program to see continuous training in action:

```bash
cd build
cmake ..
make test_continuous_training
./test_continuous_training
```

The test demonstrates:
1. Initial training with sample data
2. Continuous data collection (2400+ points)
3. Multiple retraining cycles
4. Data persistence and loading
5. Automatic retention management
6. Recent data retrieval

## API Reference

### HistoricalDataCollector

#### Constructor
```cpp
HistoricalDataCollector(const DataCollectionConfig& config = DataCollectionConfig());
```

#### Methods
```cpp
void addDataPoint(const HistoricalDataPoint& dataPoint);
void recordCurrentState(double outdoorTemp, double solarProduction, double energyCost);
std::vector<HistoricalDataPoint> getAllData() const;
std::vector<HistoricalDataPoint> getRecentData(int numDays) const;
size_t getDataPointCount() const;
void cleanupOldData();
bool saveToFile(const std::string& filename = "") const;
bool loadFromFile(const std::string& filename = "");
```

### MLTrainingScheduler

#### Constructor
```cpp
MLTrainingScheduler(std::shared_ptr<MLPredictor> predictor,
                   std::shared_ptr<HistoricalDataCollector> collector,
                   const TrainingScheduleConfig& config = TrainingScheduleConfig());
```

#### Methods
```cpp
void startAutoTraining();
void stopAutoTraining();
bool triggerRetraining();
bool hasSufficientData() const;
long getTimeUntilNextTraining() const;
std::chrono::system_clock::time_point getLastTrainingTime() const;
void setTrainingCallback(std::function<void(bool success, size_t dataPoints)> callback);
```

## Benefits

### Continuous Improvement

- Model learns from actual operational data
- Predictions improve over time
- Adapts to changing conditions and seasons

### Automatic Operation

- No manual intervention required
- Background retraining doesn't interrupt operation
- Automatic data retention prevents growth

### Durability

- Data persists across restarts
- System recovers from crashes
- Historical data preserved

### Flexibility

- Configurable schedules and thresholds
- Manual override available
- Works with existing ML predictor

## Future Enhancements

Potential improvements:

1. **Advanced Persistence**: Database storage instead of CSV
2. **Data Validation**: Outlier detection and filtering
3. **Model Comparison**: Compare old vs new model performance
4. **Incremental Training**: Update model without full retraining
5. **Remote Backup**: Cloud storage for data redundancy
6. **Analytics Dashboard**: Visualize training progress
7. **A/B Testing**: Test multiple models simultaneously

## Conclusion

The continuous ML training system enables your home automation to continuously learn and improve from real operational data. With minimal configuration, the system automatically collects data, manages retention, persists to disk, and retrains the ML predictor on a schedule.

This results in:
- Better predictions over time
- Adaptation to seasonal changes
- Personalized learning from your specific usage patterns
- Automatic operation requiring no manual intervention

For questions or support, refer to the main README and other documentation files in this repository.
