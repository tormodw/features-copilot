#ifndef DEFERRABLE_LOAD_CONTROLLER_H
#define DEFERRABLE_LOAD_CONTROLLER_H

#include "Appliance.h"
#include "HistoricalDataGenerator.h"
#include "MLPredictor.h"
#include <memory>
#include <vector>
#include <map>
#include <iostream>

// Default configuration constants
namespace DeferrableLoadDefaults {
    constexpr double DEFAULT_PRICE_THRESHOLD = 0.15;        // $0.15/kWh
    constexpr double DEFAULT_BUSY_HOUR_THRESHOLD = 0.13;    // $0.13/kWh
    constexpr int DEFAULT_TRAINING_DATA_DAYS = 30;          // 30 days
}

// Busy hour analysis result
struct BusyHourAnalysis {
    std::vector<int> busyHours;         // Hours identified as busy (high price/demand)
    std::vector<int> optimalHours;      // Hours optimal for deferrable loads
    double averagePeakPrice;             // Average price during busy hours
    double averageOffPeakPrice;          // Average price during optimal hours
};

// Controller for managing deferrable loads based on price and historical data
class DeferrableLoadController {
public:
    DeferrableLoadController(std::shared_ptr<MLPredictor> predictor);
    
    // Configure thresholds
    void setPriceThreshold(double threshold);
    void setBusyHourThreshold(double threshold);
    
    // Add appliances to manage
    void addDeferrableLoad(std::shared_ptr<Appliance> appliance);
    
    // Analyze historical data to identify busy hours
    BusyHourAnalysis analyzeBusyHours(const std::vector<HistoricalDataPoint>& historicalData);
    
    // Control deferrable loads based on current price
    void controlLoadsByPrice(double currentPrice);
    
    // Get recommendations for next 24 hours
    std::map<int, std::vector<std::string>> getDayAheadRecommendations(
        int currentHour, int currentDayOfWeek);
    
    // Switch off all deferrable loads (emergency/high price)
    void switchOffAllDeferrableLoads(const std::string& reason);
    
    // Resume deferrable loads when conditions improve
    void resumeDeferrableLoads();
    
    std::vector<std::shared_ptr<Appliance>> getDeferrableLoads() const;

private:
    std::shared_ptr<MLPredictor> predictor_;
    std::vector<std::shared_ptr<Appliance>> deferrableLoads_;
    std::map<std::string, bool> previousStates_;  // Track previous states for resume
    
    double priceThreshold_;        // Price threshold for switching off loads ($/kWh)
    double busyHourThreshold_;     // Threshold for identifying busy hours
    
    // Helper functions
    bool isHighPriceHour(double price) const;
    std::vector<int> identifyBusyHours(const std::vector<HourlyForecast>& forecasts);
};

#endif // DEFERRABLE_LOAD_CONTROLLER_H
