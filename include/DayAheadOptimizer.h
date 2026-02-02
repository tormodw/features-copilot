#ifndef DAY_AHEAD_OPTIMIZER_H
#define DAY_AHEAD_OPTIMIZER_H

#include "MLPredictor.h"
#include "Appliance.h"
#include "EVCharger.h"
#include "Heater.h"
#include "AirConditioner.h"
#include <memory>
#include <vector>
#include <map>
#include <iostream>

// Scheduled action for a specific hour
struct ScheduledAction {
    int hour;
    std::string applianceId;
    std::string action;  // "on", "off", "charge", "defer"
    double value;        // For power levels, temperatures, etc.
    std::string reason;  // Explanation of the decision
};

// Day-ahead schedule for all appliances
struct DayAheadSchedule {
    std::vector<ScheduledAction> actions;
    double estimatedCost;      // Estimated total cost for the day
    double estimatedConsumption; // Estimated total energy consumption (kWh)
    
    void addAction(int hour, const std::string& applianceId, const std::string& action, 
                   double value, const std::string& reason);
    std::vector<ScheduledAction> getActionsForHour(int hour) const;
};

// Day-ahead optimizer using ML predictions
class DayAheadOptimizer {
public:
    DayAheadOptimizer(std::shared_ptr<MLPredictor> predictor);

    void addAppliance(std::shared_ptr<Appliance> appliance);
    void setTargetTemperature(double temp);
    void setEVChargingHoursNeeded(int hours);

    // Generate optimal schedule for next 24 hours
    DayAheadSchedule generateSchedule(int currentHour, int currentDayOfWeek);
    void printSchedule(const DayAheadSchedule& schedule);

private:
    std::vector<int> findBestEVChargingHours(const std::vector<HourlyForecast>& forecasts);
    void optimizeHour(const HourlyForecast& forecast, 
                     const std::vector<int>& bestEVHours,
                     DayAheadSchedule& schedule);

    std::shared_ptr<MLPredictor> predictor_;
    std::vector<std::shared_ptr<Appliance>> appliances_;
    double targetIndoorTemp_;
    double highCostThreshold_;
    double lowCostThreshold_;
    int evChargingHoursNeeded_;
};

#endif // DAY_AHEAD_OPTIMIZER_H
