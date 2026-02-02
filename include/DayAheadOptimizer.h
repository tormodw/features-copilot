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
                   double value, const std::string& reason) {
        ScheduledAction sa;
        sa.hour = hour;
        sa.applianceId = applianceId;
        sa.action = action;
        sa.value = value;
        sa.reason = reason;
        actions.push_back(sa);
    }
    
    std::vector<ScheduledAction> getActionsForHour(int hour) const {
        std::vector<ScheduledAction> hourActions;
        for (const auto& action : actions) {
            if (action.hour == hour) {
                hourActions.push_back(action);
            }
        }
        return hourActions;
    }
};

// Day-ahead optimizer using ML predictions
class DayAheadOptimizer {
public:
    DayAheadOptimizer(std::shared_ptr<MLPredictor> predictor)
        : predictor_(predictor), 
          targetIndoorTemp_(22.0),
          highCostThreshold_(0.15),
          lowCostThreshold_(0.10),
          evChargingHoursNeeded_(4) {}

    void addAppliance(std::shared_ptr<Appliance> appliance) {
        appliances_.push_back(appliance);
    }

    void setTargetTemperature(double temp) {
        targetIndoorTemp_ = temp;
    }

    void setEVChargingHoursNeeded(int hours) {
        evChargingHoursNeeded_ = hours;
    }

    // Generate optimal schedule for next 24 hours
    DayAheadSchedule generateSchedule(int currentHour, int currentDayOfWeek) {
        std::cout << "\n=== Generating Day-Ahead Schedule with ML ===" << std::endl;
        
        // Get ML predictions
        auto forecasts = predictor_->predictNext24Hours(currentHour, currentDayOfWeek);
        
        DayAheadSchedule schedule;
        schedule.estimatedCost = 0.0;
        schedule.estimatedConsumption = 0.0;

        // Find best hours for EV charging (lowest cost, highest solar)
        std::vector<int> bestEVHours = findBestEVChargingHours(forecasts);

        // Generate schedule for each hour
        for (const auto& forecast : forecasts) {
            optimizeHour(forecast, bestEVHours, schedule);
        }

        std::cout << "Schedule generated: " << schedule.actions.size() << " actions" << std::endl;
        std::cout << "Estimated daily cost: $" << schedule.estimatedCost << std::endl;
        std::cout << "Estimated consumption: " << schedule.estimatedConsumption << " kWh" << std::endl;

        return schedule;
    }

    void printSchedule(const DayAheadSchedule& schedule) {
        std::cout << "\n=== Day-Ahead Schedule ===" << std::endl;
        std::cout << "Total estimated cost: $" << schedule.estimatedCost << std::endl;
        std::cout << "Total estimated consumption: " << schedule.estimatedConsumption << " kWh\n" << std::endl;

        for (int hour = 0; hour < 24; hour++) {
            auto actions = schedule.getActionsForHour(hour);
            if (!actions.empty()) {
                std::cout << "Hour " << hour << ":00" << std::endl;
                for (const auto& action : actions) {
                    std::cout << "  - " << action.applianceId << ": " << action.action;
                    if (action.value != 0.0) {
                        std::cout << " (" << action.value << ")";
                    }
                    std::cout << " - " << action.reason << std::endl;
                }
            }
        }
        std::cout << "=========================\n" << std::endl;
    }

private:
    std::vector<int> findBestEVChargingHours(const std::vector<HourlyForecast>& forecasts) {
        // Score each hour for EV charging
        struct HourScore {
            int hour;
            double score;
        };
        
        std::vector<HourScore> scores;
        for (const auto& forecast : forecasts) {
            HourScore hs;
            hs.hour = forecast.hour;
            
            // Lower cost is better, higher solar is better
            hs.score = -forecast.predictedEnergyCost + (forecast.predictedSolarProduction * 0.1);
            
            scores.push_back(hs);
        }
        
        // Sort by score (best first)
        std::sort(scores.begin(), scores.end(), 
                  [](const HourScore& a, const HourScore& b) { return a.score > b.score; });
        
        // Select top N hours
        std::vector<int> bestHours;
        for (int i = 0; i < std::min(evChargingHoursNeeded_, (int)scores.size()); i++) {
            bestHours.push_back(scores[i].hour);
        }
        
        return bestHours;
    }

    void optimizeHour(const HourlyForecast& forecast, 
                     const std::vector<int>& bestEVHours,
                     DayAheadSchedule& schedule) {
        int hour = forecast.hour;
        double cost = forecast.predictedEnergyCost;
        double solar = forecast.predictedSolarProduction;
        
        // EV Charging optimization
        for (auto& appliance : appliances_) {
            auto evCharger = std::dynamic_pointer_cast<EVCharger>(appliance);
            if (evCharger) {
                bool shouldCharge = std::find(bestEVHours.begin(), bestEVHours.end(), hour) 
                                   != bestEVHours.end();
                
                if (shouldCharge) {
                    std::string reason = "Low cost ($" + std::to_string(cost) + "/kWh)";
                    if (solar > 5.0) {
                        reason += ", high solar (" + std::to_string(solar) + " kW)";
                    }
                    schedule.addAction(hour, evCharger->getId(), "charge", 
                                     evCharger->getMaxChargePower(), reason);
                    schedule.estimatedConsumption += evCharger->getMaxChargePower();
                    schedule.estimatedCost += evCharger->getMaxChargePower() * cost;
                } else {
                    schedule.addAction(hour, evCharger->getId(), "defer", 0, 
                                     "Not optimal hour for charging");
                }
            }
        }

        // Temperature control optimization
        // Preheat/precool during low-cost hours
        if (cost < lowCostThreshold_) {
            for (auto& appliance : appliances_) {
                auto heater = std::dynamic_pointer_cast<Heater>(appliance);
                if (heater) {
                    schedule.addAction(hour, heater->getId(), "on", targetIndoorTemp_ + 1.0,
                                     "Preheat during low cost");
                    schedule.estimatedConsumption += heater->getPowerConsumption();
                    schedule.estimatedCost += heater->getPowerConsumption() * cost;
                }
            }
        } else if (cost > highCostThreshold_) {
            // Minimize HVAC during high cost
            for (auto& appliance : appliances_) {
                auto heater = std::dynamic_pointer_cast<Heater>(appliance);
                auto ac = std::dynamic_pointer_cast<AirConditioner>(appliance);
                if (heater) {
                    schedule.addAction(hour, heater->getId(), "minimize", 0,
                                     "Reduce heating during high cost");
                } else if (ac) {
                    schedule.addAction(hour, ac->getId(), "minimize", 0,
                                     "Reduce cooling during high cost");
                }
            }
        }
    }

    std::shared_ptr<MLPredictor> predictor_;
    std::vector<std::shared_ptr<Appliance>> appliances_;
    double targetIndoorTemp_;
    double highCostThreshold_;
    double lowCostThreshold_;
    int evChargingHoursNeeded_;
};

#endif // DAY_AHEAD_OPTIMIZER_H
