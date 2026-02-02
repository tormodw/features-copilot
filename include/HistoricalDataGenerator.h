#ifndef HISTORICAL_DATA_GENERATOR_H
#define HISTORICAL_DATA_GENERATOR_H

#include "MLPredictor.h"
#include <vector>
#include <cmath>
#include <random>

class HistoricalDataGenerator {
public:
    // Generate synthetic historical data for training
    static std::vector<HistoricalDataPoint> generateSampleData(int numDays = 30) {
        std::vector<HistoricalDataPoint> data;
        
        // Use modern C++ random number generation
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> variationDist(-10.0, 10.0);
        std::uniform_real_distribution<> weatherDist(-30.0, 30.0);
        
        for (int day = 0; day < numDays; day++) {
            int dayOfWeek = day % 7;
            
            for (int hour = 0; hour < 24; hour++) {
                HistoricalDataPoint point;
                point.hour = hour;
                point.dayOfWeek = dayOfWeek;
                
                // Energy cost pattern: higher during day, lower at night
                // Weekdays have higher peak costs
                double baseCost = 0.10;
                if (hour >= 7 && hour <= 22) {
                    // Peak hours
                    bool isWeekday = (dayOfWeek >= 1 && dayOfWeek <= 5);
                    double peakMultiplier = isWeekday ? 1.5 : 1.2;
                    point.energyCost = baseCost + (0.08 * peakMultiplier);
                    
                    // Extra peak during evening rush (17-20)
                    if (hour >= 17 && hour <= 20) {
                        point.energyCost += 0.03;
                    }
                } else {
                    // Off-peak hours
                    point.energyCost = baseCost - 0.02;
                }
                
                // Add some random variation (+/- 10%)
                double variation = variationDist(gen) / 100.0;
                point.energyCost *= (1.0 + variation);
                
                // Solar production: follows sine curve during daylight
                if (hour >= 6 && hour <= 18) {
                    double angle = (hour - 6) * 3.14159 / 12.0;
                    point.solarProduction = 8.0 * std::sin(angle);
                    
                    // Add weather variation (+/- 30%)
                    double weatherVar = weatherDist(gen) / 100.0;
                    point.solarProduction *= (1.0 + weatherVar);
                    
                    if (point.solarProduction < 0) point.solarProduction = 0;
                } else {
                    point.solarProduction = 0.0;
                }
                
                // Outdoor temperature: varies by hour and season
                double baseTemp = 18.0; // Base temperature
                double dailyVariation = 8.0 * std::sin((hour - 6) * 3.14159 / 12.0);
                point.outdoorTemp = baseTemp + dailyVariation;
                
                // Add seasonal trend (simplified)
                double seasonalOffset = 5.0 * std::sin(day * 2 * 3.14159 / 365.0);
                point.outdoorTemp += seasonalOffset;
                
                data.push_back(point);
            }
        }
        
        return data;
    }
};

#endif // HISTORICAL_DATA_GENERATOR_H
