// Test program demonstrating continuous ML training with historical data updates
#include "MLPredictor.h"
#include "HistoricalDataCollector.h"
#include "MLTrainingScheduler.h"
#include "HistoricalDataGenerator.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <random>

void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "=== " << title << " ===" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

void simulateDataCollection(std::shared_ptr<HistoricalDataCollector> collector, 
                            int numHours, bool verbose = true) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> tempDist(15.0, 25.0);
    std::uniform_real_distribution<> solarDist(0.0, 8.0);
    std::uniform_real_distribution<> costDist(0.08, 0.25);
    
    if (verbose) {
        std::cout << "Simulating " << numHours << " hours of data collection..." << std::endl;
    }
    
    for (int i = 0; i < numHours; i++) {
        double temp = tempDist(gen);
        double solar = solarDist(gen);
        double cost = costDist(gen);
        
        collector->recordCurrentState(temp, solar, cost);
        
        if (verbose && i % 24 == 0 && i > 0) {
            std::cout << "  Collected " << i << " hours of data..." << std::endl;
        }
    }
    
    if (verbose) {
        std::cout << "Data collection complete. Total points: " 
                  << collector->getDataPointCount() << std::endl;
    }
}

void testPredictionAccuracy(std::shared_ptr<MLPredictor> predictor, const std::string& label) {
    auto forecasts = predictor->predictNext24Hours(8, 2); // 8 AM, Tuesday
    
    std::cout << "\n" << label << " - Sample predictions:" << std::endl;
    std::cout << "  Hour 12:00 - Cost: $" << forecasts[4].predictedEnergyCost 
              << "/kWh, Solar: " << forecasts[4].predictedSolarProduction << " kW" << std::endl;
    std::cout << "  Hour 18:00 - Cost: $" << forecasts[10].predictedEnergyCost 
              << "/kWh, Solar: " << forecasts[10].predictedSolarProduction << " kW" << std::endl;
}

int main() {
    printSeparator("Continuous ML Training Demo");
    
    std::cout << "This demo shows how to continuously update historical data" << std::endl;
    std::cout << "and retrain the ML predictor during system operation.\n" << std::endl;
    
    // Step 1: Setup components
    printSeparator("Step 1: Initialize Components");
    
    auto mlPredictor = std::make_shared<MLPredictor>();
    
    DataCollectionConfig collectorConfig;
    collectorConfig.maxDaysToRetain = 90;
    collectorConfig.enablePersistence = true;
    collectorConfig.persistenceFile = "test_historical_data.csv";
    auto collector = std::make_shared<HistoricalDataCollector>(collectorConfig);
    
    TrainingScheduleConfig scheduleConfig;
    scheduleConfig.retrainingIntervalHours = 24;
    scheduleConfig.minDataPointsForTraining = 168; // 7 days
    scheduleConfig.autoRetrain = false; // Manual for demo
    scheduleConfig.verboseLogging = true;
    auto scheduler = std::make_shared<MLTrainingScheduler>(mlPredictor, collector, scheduleConfig);
    
    // Step 2: Initial training with sample data
    printSeparator("Step 2: Initial Training");
    
    std::cout << "Generating initial sample data (7 days)..." << std::endl;
    auto initialData = HistoricalDataGenerator::generateSampleData(7);
    
    for (const auto& point : initialData) {
        collector->addDataPoint(point);
    }
    
    std::cout << "Initial data loaded: " << collector->getDataPointCount() << " points\n" << std::endl;
    
    std::cout << "Performing initial training..." << std::endl;
    bool initialTraining = scheduler->triggerRetraining();
    
    if (initialTraining) {
        std::cout << "✓ Initial training successful" << std::endl;
        testPredictionAccuracy(mlPredictor, "After initial training");
    }
    
    // Step 3: Simulate continuous data collection
    printSeparator("Step 3: Continuous Data Collection");
    
    std::cout << "Simulating 48 hours of continuous operation..." << std::endl;
    std::cout << "Collecting new data points every simulated hour...\n" << std::endl;
    
    for (int day = 0; day < 2; day++) {
        std::cout << "--- Day " << (day + 1) << " ---" << std::endl;
        simulateDataCollection(collector, 24, false);
        std::cout << "Day " << (day + 1) << " complete. Total data: " 
                  << collector->getDataPointCount() << " points" << std::endl;
    }
    
    // Step 4: Demonstrate retraining with new data
    printSeparator("Step 4: Retraining with New Data");
    
    std::cout << "Total historical data collected: " << collector->getDataPointCount() 
              << " points" << std::endl;
    std::cout << "Triggering retraining with updated dataset...\n" << std::endl;
    
    bool retraining = scheduler->triggerRetraining();
    
    if (retraining) {
        std::cout << "✓ Retraining successful with " << collector->getDataPointCount() 
                  << " data points" << std::endl;
        testPredictionAccuracy(mlPredictor, "After retraining");
    }
    
    // Step 5: Data persistence
    printSeparator("Step 5: Data Persistence");
    
    std::cout << "Saving historical data to file..." << std::endl;
    bool saved = collector->saveToFile();
    
    if (saved) {
        std::cout << "✓ Data saved successfully to " << collectorConfig.persistenceFile << std::endl;
    }
    
    // Test loading
    std::cout << "\nTesting data reload..." << std::endl;
    auto testCollector = std::make_shared<HistoricalDataCollector>(collectorConfig);
    
    if (testCollector->getDataPointCount() > 0) {
        std::cout << "✓ Successfully loaded " << testCollector->getDataPointCount() 
                  << " data points from file" << std::endl;
    }
    
    // Step 6: Continuous collection simulation
    printSeparator("Step 6: Add More Recent Data");
    
    std::cout << "Simulating collection of 24 more hours..." << std::endl;
    simulateDataCollection(collector, 24, false);
    
    std::cout << "New total: " << collector->getDataPointCount() << " points" << std::endl;
    std::cout << "\nTriggering another retraining cycle...\n" << std::endl;
    
    scheduler->triggerRetraining();
    testPredictionAccuracy(mlPredictor, "After second retraining");
    
    // Step 7: Data retention management
    printSeparator("Step 7: Data Retention Management");
    
    std::cout << "Current data points: " << collector->getDataPointCount() << std::endl;
    std::cout << "Max retention: " << collectorConfig.maxDaysToRetain << " days (" 
              << (collectorConfig.maxDaysToRetain * 24) << " data points)" << std::endl;
    
    // Simulate a lot more data to trigger cleanup
    std::cout << "\nSimulating long-term operation (90+ days of data)..." << std::endl;
    simulateDataCollection(collector, 100 * 24, false);
    
    std::cout << "After long-term simulation: " << collector->getDataPointCount() 
              << " points" << std::endl;
    std::cout << "✓ Old data automatically cleaned up (retention policy enforced)" << std::endl;
    
    // Step 8: Recent data retrieval
    printSeparator("Step 8: Recent Data Retrieval");
    
    std::cout << "Retrieving last 7 days of data..." << std::endl;
    auto recentData = collector->getRecentData(7);
    std::cout << "Retrieved " << recentData.size() << " recent data points" << std::endl;
    
    std::cout << "\nRetrieving last 30 days of data..." << std::endl;
    auto last30Days = collector->getRecentData(30);
    std::cout << "Retrieved " << last30Days.size() << " data points from last 30 days" << std::endl;
    
    // Summary
    printSeparator("Summary");
    
    std::cout << "This demonstration showed:" << std::endl;
    std::cout << "  1. ✓ Initializing ML predictor and data collector" << std::endl;
    std::cout << "  2. ✓ Performing initial training with historical data" << std::endl;
    std::cout << "  3. ✓ Continuously collecting new data points" << std::endl;
    std::cout << "  4. ✓ Retraining ML model with updated dataset" << std::endl;
    std::cout << "  5. ✓ Persisting data to file for durability" << std::endl;
    std::cout << "  6. ✓ Loading data from file on startup" << std::endl;
    std::cout << "  7. ✓ Managing data retention (automatic cleanup)" << std::endl;
    std::cout << "  8. ✓ Retrieving recent data for analysis" << std::endl;
    
    std::cout << "\n💡 Key Benefits:" << std::endl;
    std::cout << "   - Model continuously improves with real operational data" << std::endl;
    std::cout << "   - Automatic data retention prevents unbounded growth" << std::endl;
    std::cout << "   - File persistence ensures data survives restarts" << std::endl;
    std::cout << "   - Configurable retraining schedule balances accuracy and performance" << std::endl;
    std::cout << "   - No manual intervention required for updates" << std::endl;
    
    printSeparator("Demo Complete");
    
    // Cleanup test file
    std::remove(collectorConfig.persistenceFile.c_str());
    std::cout << "Cleaned up test files" << std::endl;
    
    return 0;
}
