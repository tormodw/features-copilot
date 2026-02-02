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
    MLPredictor();

    // Train the model with historical data
    void train(const std::vector<HistoricalDataPoint>& historicalData);

    // Predict the next 24 hours
    std::vector<HourlyForecast> predictNext24Hours(int currentHour, int currentDayOfWeek);

    bool isTrained() const;

private:
    struct HourlyStats {
        double avgCost = 0.0;
        double avgSolar = 0.0;
        double avgTemp = 0.0;
    };

    double average(const std::vector<double>& values);
    std::vector<HourlyForecast> generateDefaultForecasts(int currentHour);

    bool trained_;
    std::vector<HistoricalDataPoint> historicalData_;
    std::map<int, HourlyStats> hourlyStats_;
};

#endif // ML_PREDICTOR_H
