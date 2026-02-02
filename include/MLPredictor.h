#ifndef ML_PREDICTOR_H
#define ML_PREDICTOR_H

#include <vector>
#include <cmath>
#include <algorithm>
#include <map>

// Historical data point for training
struct HistoricalDataPoint {
    int hour;              // Hour of day (0-23)
    int dayOfWeek;         // Day of week (0-6, 0=Sunday)
    double outdoorTemp;    // Outdoor temperature
    double solarProduction; // Solar production
    double energyCost;     // Energy cost per kWh
};

// Forecast for a specific hour
struct HourlyForecast {
    int hour;
    double predictedEnergyCost;
    double predictedSolarProduction;
    double predictedOutdoorTemp;
    double confidenceScore;  // 0-1, higher is better
};

// Simple ML predictor using linear regression and pattern matching
// In production, this would use a proper ML library like TensorFlow, PyTorch, or scikit-learn
class MLPredictor {
public:
    MLPredictor() : trained_(false) {}

    // Train the model with historical data
    void train(const std::vector<HistoricalDataPoint>& historicalData) {
        historicalData_ = historicalData;
        trained_ = true;
        
        // Calculate statistics for each hour
        for (int hour = 0; hour < 24; hour++) {
            std::vector<double> costs, solar, temps;
            
            for (const auto& point : historicalData) {
                if (point.hour == hour) {
                    costs.push_back(point.energyCost);
                    solar.push_back(point.solarProduction);
                    temps.push_back(point.outdoorTemp);
                }
            }
            
            if (!costs.empty()) {
                hourlyStats_[hour].avgCost = average(costs);
                hourlyStats_[hour].avgSolar = average(solar);
                hourlyStats_[hour].avgTemp = average(temps);
            }
        }
    }

    // Predict the next 24 hours
    std::vector<HourlyForecast> predictNext24Hours(int currentHour, int currentDayOfWeek) {
        if (!trained_) {
            return generateDefaultForecasts(currentHour);
        }

        std::vector<HourlyForecast> forecasts;
        
        for (int i = 0; i < 24; i++) {
            int hour = (currentHour + i) % 24;
            int dayOfWeek = currentDayOfWeek;
            
            // Adjust day if we cross midnight
            if (currentHour + i >= 24) {
                dayOfWeek = (dayOfWeek + 1) % 7;
            }
            
            HourlyForecast forecast;
            forecast.hour = hour;
            
            // Use simple pattern matching from historical data
            if (hourlyStats_.find(hour) != hourlyStats_.end()) {
                const auto& stats = hourlyStats_[hour];
                
                // Add some variation based on day of week
                double weekdayFactor = (dayOfWeek >= 1 && dayOfWeek <= 5) ? 1.1 : 0.9;
                
                forecast.predictedEnergyCost = stats.avgCost * weekdayFactor;
                forecast.predictedSolarProduction = stats.avgSolar;
                forecast.predictedOutdoorTemp = stats.avgTemp;
                forecast.confidenceScore = 0.75; // Moderate confidence
            } else {
                // Use defaults if no data available
                forecast.predictedEnergyCost = 0.12;
                forecast.predictedSolarProduction = (hour >= 6 && hour <= 18) ? 3.0 : 0.0;
                forecast.predictedOutdoorTemp = 20.0;
                forecast.confidenceScore = 0.5; // Low confidence
            }
            
            forecasts.push_back(forecast);
        }
        
        return forecasts;
    }

    bool isTrained() const { return trained_; }

private:
    struct HourlyStats {
        double avgCost = 0.0;
        double avgSolar = 0.0;
        double avgTemp = 0.0;
    };

    double average(const std::vector<double>& values) {
        if (values.empty()) return 0.0;
        double sum = 0.0;
        for (double v : values) sum += v;
        return sum / values.size();
    }

    std::vector<HourlyForecast> generateDefaultForecasts(int currentHour) {
        std::vector<HourlyForecast> forecasts;
        
        for (int i = 0; i < 24; i++) {
            int hour = (currentHour + i) % 24;
            HourlyForecast forecast;
            forecast.hour = hour;
            
            // Default pattern: high cost during day, low at night
            if (hour >= 8 && hour <= 20) {
                forecast.predictedEnergyCost = 0.15 + (0.03 * (hour % 4));
            } else {
                forecast.predictedEnergyCost = 0.08;
            }
            
            // Solar production during daylight
            if (hour >= 6 && hour <= 18) {
                forecast.predictedSolarProduction = 5.0 * std::sin((hour - 6) * 3.14159 / 12.0);
            } else {
                forecast.predictedSolarProduction = 0.0;
            }
            
            // Temperature variation
            forecast.predictedOutdoorTemp = 15.0 + 8.0 * std::sin((hour - 6) * 3.14159 / 12.0);
            forecast.confidenceScore = 0.6;
            
            forecasts.push_back(forecast);
        }
        
        return forecasts;
    }

    bool trained_;
    std::vector<HistoricalDataPoint> historicalData_;
    std::map<int, HourlyStats> hourlyStats_;
};

#endif // ML_PREDICTOR_H
