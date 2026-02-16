#include "HistoricalDataCollector.h"
#include <sstream>
#include <algorithm>

HistoricalDataCollector::HistoricalDataCollector(const DataCollectionConfig& config)
    : config_(config) {
    
    std::cout << "HistoricalDataCollector: Initialized" << std::endl;
    std::cout << "  Max retention: " << config_.maxDaysToRetain << " days" << std::endl;
    std::cout << "  Persistence: " << (config_.enablePersistence ? "enabled" : "disabled") << std::endl;
    
    // Try to load existing data if persistence is enabled
    if (config_.enablePersistence) {
        loadFromFile(config_.persistenceFile);
    }
}

void HistoricalDataCollector::addDataPoint(const HistoricalDataPoint& dataPoint) {
    dataPoints_.push_back(dataPoint);
    
    // Cleanup old data if we exceed retention limit
    if (dataPoints_.size() > static_cast<size_t>(config_.maxDaysToRetain * 24)) {
        cleanupOldData();
    }
    
    // Auto-save if persistence is enabled (save every 24 data points to avoid excessive I/O)
    if (config_.enablePersistence && dataPoints_.size() % 24 == 0) {
        saveToFile(config_.persistenceFile);
    }
}

void HistoricalDataCollector::recordCurrentState(double outdoorTemp, double solarProduction, double energyCost) {
    int hour, dayOfWeek;
    getCurrentTimeInfo(hour, dayOfWeek);
    
    HistoricalDataPoint point;
    point.hour = hour;
    point.dayOfWeek = dayOfWeek;
    point.outdoorTemp = outdoorTemp;
    point.solarProduction = solarProduction;
    point.energyCost = energyCost;
    
    addDataPoint(point);
    
    std::cout << "HistoricalDataCollector: Recorded data point - Hour: " << hour 
              << ", Cost: $" << energyCost << "/kWh, Solar: " << solarProduction 
              << " kW, Temp: " << outdoorTemp << "°C" << std::endl;
}

std::vector<HistoricalDataPoint> HistoricalDataCollector::getAllData() const {
    return std::vector<HistoricalDataPoint>(dataPoints_.begin(), dataPoints_.end());
}

std::vector<HistoricalDataPoint> HistoricalDataCollector::getRecentData(int numDays) const {
    size_t numPoints = std::min(static_cast<size_t>(numDays * 24), dataPoints_.size());
    
    if (numPoints == 0) {
        return std::vector<HistoricalDataPoint>();
    }
    
    // Get the last numPoints from the deque
    auto startIt = dataPoints_.end() - numPoints;
    return std::vector<HistoricalDataPoint>(startIt, dataPoints_.end());
}

size_t HistoricalDataCollector::getDataPointCount() const {
    return dataPoints_.size();
}

void HistoricalDataCollector::cleanupOldData() {
    size_t maxPoints = config_.maxDaysToRetain * 24;
    
    if (dataPoints_.size() > maxPoints) {
        size_t toRemove = dataPoints_.size() - maxPoints;
        std::cout << "HistoricalDataCollector: Removing " << toRemove 
                  << " old data points" << std::endl;
        dataPoints_.erase(dataPoints_.begin(), dataPoints_.begin() + toRemove);
    }
}

size_t HistoricalDataCollector::removeOldDataPoints() {
    size_t maxPoints = config_.maxDaysToRetain * 24;
    size_t originalSize = dataPoints_.size();
    
    if (dataPoints_.size() > maxPoints) {
        dataPoints_.erase(dataPoints_.begin(), dataPoints_.begin() + (dataPoints_.size() - maxPoints));
    }
    
    return originalSize - dataPoints_.size();
}

bool HistoricalDataCollector::saveToFile(const std::string& filename) const {
    std::string file = filename.empty() ? config_.persistenceFile : filename;
    
    std::ofstream outFile(file);
    if (!outFile.is_open()) {
        std::cerr << "HistoricalDataCollector: Failed to open file for writing: " << file << std::endl;
        return false;
    }
    
    // Write CSV header
    outFile << "hour,dayOfWeek,outdoorTemp,solarProduction,energyCost\n";
    
    // Write data points
    for (const auto& point : dataPoints_) {
        outFile << point.hour << ","
                << point.dayOfWeek << ","
                << point.outdoorTemp << ","
                << point.solarProduction << ","
                << point.energyCost << "\n";
    }
    
    outFile.close();
    std::cout << "HistoricalDataCollector: Saved " << dataPoints_.size() 
              << " data points to " << file << std::endl;
    return true;
}

bool HistoricalDataCollector::loadFromFile(const std::string& filename) {
    std::string file = filename.empty() ? config_.persistenceFile : filename;
    
    std::ifstream inFile(file);
    if (!inFile.is_open()) {
        std::cout << "HistoricalDataCollector: No existing data file found: " << file << std::endl;
        return false;
    }
    
    dataPoints_.clear();
    std::string line;
    
    // Skip header
    std::getline(inFile, line);
    
    // Read data points
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        HistoricalDataPoint point;
        char comma;
        
        if (iss >> point.hour >> comma 
            >> point.dayOfWeek >> comma 
            >> point.outdoorTemp >> comma 
            >> point.solarProduction >> comma 
            >> point.energyCost) {
            dataPoints_.push_back(point);
        }
    }
    
    inFile.close();
    std::cout << "HistoricalDataCollector: Loaded " << dataPoints_.size() 
              << " data points from " << file << std::endl;
    
    // Cleanup old data after loading
    cleanupOldData();
    
    return true;
}

void HistoricalDataCollector::subscribeToSensorEvents() {
    // This is a placeholder for subscribing to sensor events
    // In a full implementation, this would subscribe to EventManager events
    // and automatically collect data when sensor readings change
    std::cout << "HistoricalDataCollector: Sensor event subscription configured" << std::endl;
}

void HistoricalDataCollector::getCurrentTimeInfo(int& hour, int& dayOfWeek) const {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    
    hour = localTime->tm_hour;
    dayOfWeek = localTime->tm_wday;  // 0 = Sunday, 6 = Saturday
}
