#ifndef HISTORICAL_DATA_GENERATOR_H
#define HISTORICAL_DATA_GENERATOR_H

#include "MLPredictor.h"
#include <vector>
#include <cmath>
#include <random>

class HistoricalDataGenerator {
public:
    // Generate synthetic historical data for training
    static std::vector<HistoricalDataPoint> generateSampleData(int numDays = 30);
};

#endif // HISTORICAL_DATA_GENERATOR_H
