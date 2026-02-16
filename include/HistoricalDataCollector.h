#ifndef HISTORICAL_DATA_COLLECTOR_H
#define HISTORICAL_DATA_COLLECTOR_H

#include "MLPredictor.h"
#include "EventManager.h"
#include <vector>
#include <deque>
#include <memory>
#include <ctime>
#include <fstream>
#include <iostream>

// Configuration for data collection
struct DataCollectionConfig {
    int maxDaysToRetain = 90;           // Keep last 90 days
    bool enablePersistence = true;      // Save to file
    std::string persistenceFile = "historical_data.csv";
    int collectionIntervalMinutes = 60; // Collect data every hour
};

// Collector for accumulating historical data during runtime
class HistoricalDataCollector {
public:
    HistoricalDataCollector(const DataCollectionConfig& config = DataCollectionConfig());
    
    // Add a new data point
    void addDataPoint(const HistoricalDataPoint& dataPoint);
    
    // Record current system state as a data point
    void recordCurrentState(double outdoorTemp, double solarProduction, double energyCost);
    
    // Get all historical data
    std::vector<HistoricalDataPoint> getAllData() const;
    
    // Get recent data (last N days)
    std::vector<HistoricalDataPoint> getRecentData(int numDays) const;
    
    // Get data count
    size_t getDataPointCount() const;
    
    // Clear old data beyond retention period
    void cleanupOldData();
    
    // Save data to file
    bool saveToFile(const std::string& filename = "") const;
    
    // Load data from file
    bool loadFromFile(const std::string& filename = "");
    
    // Subscribe to sensor events for automatic data collection
    void subscribeToSensorEvents();

private:
    DataCollectionConfig config_;
    std::deque<HistoricalDataPoint> dataPoints_;  // Use deque for efficient removal from front
    
    // Helper to get current hour and day of week
    void getCurrentTimeInfo(int& hour, int& dayOfWeek) const;
    
    // Helper to remove data older than retention period
    size_t removeOldDataPoints();
};

#endif // HISTORICAL_DATA_COLLECTOR_H
