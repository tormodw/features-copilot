#include "DeferrableLoadController.h"
#include <algorithm>
#include <numeric>

DeferrableLoadController::DeferrableLoadController(std::shared_ptr<MLPredictor> predictor)
    : predictor_(predictor),
      priceThreshold_(0.15),      // Default: $0.15/kWh
      busyHourThreshold_(0.13) {  // Default: $0.13/kWh for busy hour identification
}

void DeferrableLoadController::setPriceThreshold(double threshold) {
    priceThreshold_ = threshold;
}

void DeferrableLoadController::setBusyHourThreshold(double threshold) {
    busyHourThreshold_ = threshold;
}

void DeferrableLoadController::addDeferrableLoad(std::shared_ptr<Appliance> appliance) {
    if (appliance && appliance->isDeferrable()) {
        deferrableLoads_.push_back(appliance);
        std::cout << "Added deferrable load: " << appliance->getName() << std::endl;
    }
}

BusyHourAnalysis DeferrableLoadController::analyzeBusyHours(
    const std::vector<HistoricalDataPoint>& historicalData) {
    
    std::cout << "\n=== Analyzing Busy Hours from Historical Data ===" << std::endl;
    
    BusyHourAnalysis analysis;
    
    if (historicalData.empty()) {
        std::cout << "No historical data available" << std::endl;
        return analysis;
    }
    
    // Calculate average price per hour of day
    std::map<int, std::vector<double>> hourlyPrices;
    
    for (const auto& dataPoint : historicalData) {
        hourlyPrices[dataPoint.hour].push_back(dataPoint.energyCost);
    }
    
    // Calculate average and identify busy hours
    double totalPeakPrice = 0.0;
    int peakCount = 0;
    double totalOffPeakPrice = 0.0;
    int offPeakCount = 0;
    
    for (int hour = 0; hour < 24; hour++) {
        if (hourlyPrices[hour].empty()) continue;
        
        double avgPrice = std::accumulate(hourlyPrices[hour].begin(), 
                                         hourlyPrices[hour].end(), 0.0) 
                         / hourlyPrices[hour].size();
        
        if (avgPrice > busyHourThreshold_) {
            analysis.busyHours.push_back(hour);
            totalPeakPrice += avgPrice;
            peakCount++;
        } else {
            analysis.optimalHours.push_back(hour);
            totalOffPeakPrice += avgPrice;
            offPeakCount++;
        }
    }
    
    analysis.averagePeakPrice = peakCount > 0 ? totalPeakPrice / peakCount : 0.0;
    analysis.averageOffPeakPrice = offPeakCount > 0 ? totalOffPeakPrice / offPeakCount : 0.0;
    
    std::cout << "Busy hours identified: " << analysis.busyHours.size() << " hours" << std::endl;
    std::cout << "Average peak price: $" << analysis.averagePeakPrice << "/kWh" << std::endl;
    std::cout << "Average off-peak price: $" << analysis.averageOffPeakPrice << "/kWh" << std::endl;
    
    return analysis;
}

void DeferrableLoadController::controlLoadsByPrice(double currentPrice) {
    if (isHighPriceHour(currentPrice)) {
        std::cout << "\n⚠️  High price detected ($" << currentPrice 
                  << "/kWh) - Switching off deferrable loads" << std::endl;
        switchOffAllDeferrableLoads("High energy price");
    } else {
        std::cout << "\n✓ Price acceptable ($" << currentPrice 
                  << "/kWh) - Resuming deferrable loads" << std::endl;
        resumeDeferrableLoads();
    }
}

std::map<int, std::vector<std::string>> DeferrableLoadController::getDayAheadRecommendations(
    int currentHour, int currentDayOfWeek) {
    
    std::cout << "\n=== Generating Day-Ahead Recommendations for Deferrable Loads ===" << std::endl;
    
    std::map<int, std::vector<std::string>> recommendations;
    
    // Get ML predictions for next 24 hours
    auto forecasts = predictor_->predictNext24Hours(currentHour, currentDayOfWeek);
    
    // Identify busy hours
    auto busyHours = identifyBusyHours(forecasts);
    
    for (const auto& forecast : forecasts) {
        bool isBusy = std::find(busyHours.begin(), busyHours.end(), forecast.hour) 
                     != busyHours.end();
        
        std::vector<std::string> hourRecommendations;
        
        if (isBusy) {
            for (const auto& load : deferrableLoads_) {
                std::string rec = load->getName() + ": Switch OFF (busy hour, price: $" 
                                + std::to_string(forecast.predictedEnergyCost) + "/kWh)";
                hourRecommendations.push_back(rec);
            }
        } else {
            for (const auto& load : deferrableLoads_) {
                std::string rec = load->getName() + ": Can operate (optimal hour, price: $" 
                                + std::to_string(forecast.predictedEnergyCost) + "/kWh)";
                hourRecommendations.push_back(rec);
            }
        }
        
        if (!hourRecommendations.empty()) {
            recommendations[forecast.hour] = hourRecommendations;
        }
    }
    
    std::cout << "Generated recommendations for " << recommendations.size() << " hours" << std::endl;
    
    return recommendations;
}

void DeferrableLoadController::switchOffAllDeferrableLoads(const std::string& reason) {
    std::cout << "Switching off deferrable loads - Reason: " << reason << std::endl;
    
    for (auto& load : deferrableLoads_) {
        if (load->isOn()) {
            // Save previous state for potential resume
            previousStates_[load->getId()] = true;
            load->turnOff();
            std::cout << "  - " << load->getName() << " switched OFF" << std::endl;
        }
    }
}

void DeferrableLoadController::resumeDeferrableLoads() {
    std::cout << "Resuming deferrable loads" << std::endl;
    
    for (auto& load : deferrableLoads_) {
        // Only resume if it was on before
        if (previousStates_[load->getId()] && !load->isOn()) {
            load->turnOn();
            std::cout << "  - " << load->getName() << " resumed" << std::endl;
        }
    }
}

std::vector<std::shared_ptr<Appliance>> DeferrableLoadController::getDeferrableLoads() const {
    return deferrableLoads_;
}

bool DeferrableLoadController::isHighPriceHour(double price) const {
    return price > priceThreshold_;
}

std::vector<int> DeferrableLoadController::identifyBusyHours(
    const std::vector<HourlyForecast>& forecasts) {
    
    std::vector<int> busyHours;
    
    for (const auto& forecast : forecasts) {
        if (forecast.predictedEnergyCost > busyHourThreshold_) {
            busyHours.push_back(forecast.hour);
        }
    }
    
    return busyHours;
}
