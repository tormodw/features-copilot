#ifndef ML_TRAINING_SCHEDULER_H
#define ML_TRAINING_SCHEDULER_H

#include "MLPredictor.h"
#include "HistoricalDataCollector.h"
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include <iostream>

// Configuration for ML training schedule
struct TrainingScheduleConfig {
    int retrainingIntervalHours = 24;    // Retrain every 24 hours
    int minDataPointsForTraining = 168; // Minimum 7 days of hourly data (7*24)
    bool autoRetrain = true;             // Enable automatic retraining
    bool verboseLogging = true;          // Enable detailed logging
};

// Scheduler for periodic ML model retraining
class MLTrainingScheduler {
public:
    MLTrainingScheduler(std::shared_ptr<MLPredictor> predictor,
                       std::shared_ptr<HistoricalDataCollector> collector,
                       const TrainingScheduleConfig& config = TrainingScheduleConfig());
    
    ~MLTrainingScheduler();
    
    // Start automatic retraining on schedule
    void startAutoTraining();
    
    // Stop automatic retraining
    void stopAutoTraining();
    
    // Manually trigger retraining
    bool triggerRetraining();
    
    // Check if sufficient data is available for training
    bool hasSufficientData() const;
    
    // Get time until next scheduled retraining (in seconds)
    long getTimeUntilNextTraining() const;
    
    // Get last training time
    std::chrono::system_clock::time_point getLastTrainingTime() const;
    
    // Set callback for training completion
    void setTrainingCallback(std::function<void(bool success, size_t dataPoints)> callback);

private:
    std::shared_ptr<MLPredictor> predictor_;
    std::shared_ptr<HistoricalDataCollector> collector_;
    TrainingScheduleConfig config_;
    
    std::atomic<bool> autoTrainingActive_;
    std::thread trainingThread_;
    
    std::chrono::system_clock::time_point lastTrainingTime_;
    std::function<void(bool, size_t)> trainingCallback_;
    
    // Background thread function
    void trainingLoop();
    
    // Perform actual training
    bool performTraining();
};

#endif // ML_TRAINING_SCHEDULER_H
