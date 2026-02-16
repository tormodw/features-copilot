#include "MLTrainingScheduler.h"

MLTrainingScheduler::MLTrainingScheduler(std::shared_ptr<MLPredictor> predictor,
                                       std::shared_ptr<HistoricalDataCollector> collector,
                                       const TrainingScheduleConfig& config)
    : predictor_(predictor),
      collector_(collector),
      config_(config),
      autoTrainingActive_(false),
      lastTrainingTime_(std::chrono::system_clock::now()) {
    
    std::cout << "MLTrainingScheduler: Initialized" << std::endl;
    std::cout << "  Retraining interval: " << config_.retrainingIntervalHours << " hours" << std::endl;
    std::cout << "  Minimum data points: " << config_.minDataPointsForTraining << std::endl;
    std::cout << "  Auto retrain: " << (config_.autoRetrain ? "enabled" : "disabled") << std::endl;
}

MLTrainingScheduler::~MLTrainingScheduler() {
    stopAutoTraining();
}

void MLTrainingScheduler::startAutoTraining() {
    if (autoTrainingActive_) {
        std::cout << "MLTrainingScheduler: Auto training already active" << std::endl;
        return;
    }
    
    if (!config_.autoRetrain) {
        std::cout << "MLTrainingScheduler: Auto training is disabled in config" << std::endl;
        return;
    }
    
    autoTrainingActive_ = true;
    trainingThread_ = std::thread(&MLTrainingScheduler::trainingLoop, this);
    
    std::cout << "MLTrainingScheduler: Auto training started" << std::endl;
}

void MLTrainingScheduler::stopAutoTraining() {
    if (!autoTrainingActive_) {
        return;
    }
    
    autoTrainingActive_ = false;
    
    if (trainingThread_.joinable()) {
        trainingThread_.join();
    }
    
    std::cout << "MLTrainingScheduler: Auto training stopped" << std::endl;
}

bool MLTrainingScheduler::triggerRetraining() {
    std::cout << "\n=== Manual Retraining Triggered ===" << std::endl;
    return performTraining();
}

bool MLTrainingScheduler::hasSufficientData() const {
    return collector_->getDataPointCount() >= static_cast<size_t>(config_.minDataPointsForTraining);
}

long MLTrainingScheduler::getTimeUntilNextTraining() const {
    auto now = std::chrono::system_clock::now();
    auto nextTraining = lastTrainingTime_ + std::chrono::hours(config_.retrainingIntervalHours);
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(nextTraining - now);
    return duration.count();
}

std::chrono::system_clock::time_point MLTrainingScheduler::getLastTrainingTime() const {
    return lastTrainingTime_;
}

void MLTrainingScheduler::setTrainingCallback(std::function<void(bool, size_t)> callback) {
    trainingCallback_ = callback;
}

void MLTrainingScheduler::trainingLoop() {
    std::cout << "MLTrainingScheduler: Training loop started" << std::endl;
    
    while (autoTrainingActive_) {
        // Check if it's time to retrain
        auto now = std::chrono::system_clock::now();
        auto timeSinceLastTraining = std::chrono::duration_cast<std::chrono::hours>(now - lastTrainingTime_);
        
        if (timeSinceLastTraining.count() >= config_.retrainingIntervalHours) {
            if (config_.verboseLogging) {
                std::cout << "\n=== Scheduled Retraining ===" << std::endl;
                std::cout << "Time since last training: " << timeSinceLastTraining.count() << " hours" << std::endl;
            }
            
            performTraining();
        }
        
        // Sleep for a short period before checking again (check every hour)
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
    
    std::cout << "MLTrainingScheduler: Training loop stopped" << std::endl;
}

bool MLTrainingScheduler::performTraining() {
    // Check if we have sufficient data
    if (!hasSufficientData()) {
        std::cout << "MLTrainingScheduler: Insufficient data for training" << std::endl;
        std::cout << "  Current: " << collector_->getDataPointCount() << " points" << std::endl;
        std::cout << "  Required: " << config_.minDataPointsForTraining << " points" << std::endl;
        
        if (trainingCallback_) {
            trainingCallback_(false, collector_->getDataPointCount());
        }
        return false;
    }
    
    // Get all historical data
    auto historicalData = collector_->getAllData();
    
    std::cout << "MLTrainingScheduler: Starting training with " << historicalData.size() 
              << " data points" << std::endl;
    
    try {
        // Train the predictor
        predictor_->train(historicalData);
        
        // Update last training time
        lastTrainingTime_ = std::chrono::system_clock::now();
        
        std::cout << "MLTrainingScheduler: Training completed successfully" << std::endl;
        
        if (trainingCallback_) {
            trainingCallback_(true, historicalData.size());
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "MLTrainingScheduler: Training failed - " << e.what() << std::endl;
        
        if (trainingCallback_) {
            trainingCallback_(false, historicalData.size());
        }
        
        return false;
    }
}
