// Simple demonstration of deferrable load control features
#include "DeferrableLoadController.h"
#include "DayAheadOptimizer.h"
#include "MLPredictor.h"
#include "HistoricalDataGenerator.h"
#include "Heater.h"
#include "AirConditioner.h"
#include "Light.h"
#include "EVCharger.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "=== Deferrable Load Control Demo ===" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nDemonstrating control of deferrable loads during busy hours" << std::endl;
    std::cout << "and when energy prices exceed threshold\n" << std::endl;
    
    // Setup ML predictor and historical data
    std::cout << "=== Step 1: Training ML Model ===" << std::endl;
    auto mlPredictor = std::make_shared<MLPredictor>();
    auto historicalData = HistoricalDataGenerator::generateSampleData(30);
    mlPredictor->train(historicalData);
    std::cout << "ML model trained with " << historicalData.size() << " data points\n" << std::endl;
    
    // Create deferrable load controller
    std::cout << "=== Step 2: Setting Up Deferrable Load Controller ===" << std::endl;
    auto deferrableController = std::make_shared<DeferrableLoadController>(mlPredictor);
    
    // Configure thresholds
    deferrableController->setPriceThreshold(0.15);     // $0.15/kWh - switch off above this
    deferrableController->setBusyHourThreshold(0.13);  // $0.13/kWh - identify busy hours
    std::cout << "Price threshold: $0.15/kWh (switch off deferrable loads above)" << std::endl;
    std::cout << "Busy hour threshold: $0.13/kWh\n" << std::endl;
    
    // Create appliances and mark some as deferrable
    std::cout << "=== Step 3: Creating Appliances ===" << std::endl;
    auto heater = std::make_shared<Heater>("heater_1", "Living Room Heater", 2.5);
    auto ac = std::make_shared<AirConditioner>("ac_1", "Living Room AC", 3.0);
    auto evCharger = std::make_shared<EVCharger>("ev_1", "EV Charger", 11.0);
    auto light1 = std::make_shared<Light>("light_1", "Decorative Lights", 0.3);
    auto light2 = std::make_shared<Light>("light_2", "Essential Lights", 0.2);
    
    // Mark deferrable loads
    heater->setDeferrable(false);     // Critical for comfort
    ac->setDeferrable(false);         // Critical for comfort
    evCharger->setDeferrable(true);   // Can be deferred
    light1->setDeferrable(true);      // Decorative, can be deferred
    light2->setDeferrable(false);     // Essential lighting
    
    std::cout << "Created 5 appliances:" << std::endl;
    std::cout << "  - " << heater->getName() << " (NOT deferrable - critical)" << std::endl;
    std::cout << "  - " << ac->getName() << " (NOT deferrable - critical)" << std::endl;
    std::cout << "  - " << evCharger->getName() << " (DEFERRABLE)" << std::endl;
    std::cout << "  - " << light1->getName() << " (DEFERRABLE)" << std::endl;
    std::cout << "  - " << light2->getName() << " (NOT deferrable - essential)\n" << std::endl;
    
    // Add deferrable loads to controller
    deferrableController->addDeferrableLoad(evCharger);
    deferrableController->addDeferrableLoad(light1);
    
    // Turn on all appliances initially
    heater->turnOn();
    ac->turnOn();
    evCharger->turnOn();
    light1->turnOn();
    light2->turnOn();
    
    std::cout << "\n=== Step 4: Analyzing Busy Hours from Historical Data ===" << std::endl;
    auto busyHourAnalysis = deferrableController->analyzeBusyHours(historicalData);
    
    std::cout << "\nBusy Hours: ";
    for (int hour : busyHourAnalysis.busyHours) {
        std::cout << hour << ":00 ";
    }
    std::cout << std::endl;
    
    std::cout << "Optimal Hours: ";
    for (int hour : busyHourAnalysis.optimalHours) {
        std::cout << hour << ":00 ";
    }
    std::cout << "\n" << std::endl;
    
    // Test price-based control
    std::cout << "=== Step 5: Testing Price-Based Control ===" << std::endl;
    
    std::cout << "\nScenario 1: Low price period ($0.10/kWh)" << std::endl;
    deferrableController->controlLoadsByPrice(0.10);
    std::cout << "  EV Charger status: " << (evCharger->isOn() ? "ON" : "OFF") << std::endl;
    std::cout << "  Decorative Lights status: " << (light1->isOn() ? "ON" : "OFF") << std::endl;
    
    std::cout << "\nScenario 2: High price period ($0.18/kWh)" << std::endl;
    deferrableController->controlLoadsByPrice(0.18);
    std::cout << "  EV Charger status: " << (evCharger->isOn() ? "ON" : "OFF") << std::endl;
    std::cout << "  Decorative Lights status: " << (light1->isOn() ? "ON" : "OFF") << std::endl;
    std::cout << "  Essential Lights status: " << (light2->isOn() ? "ON" : "OFF") << " (not affected - not deferrable)" << std::endl;
    std::cout << "  Heater status: " << (heater->isOn() ? "ON" : "OFF") << " (not affected - not deferrable)" << std::endl;
    
    // Day-ahead recommendations
    std::cout << "\n=== Step 6: Day-Ahead Recommendations ===" << std::endl;
    int currentHour = 8;  // 8 AM
    int currentDayOfWeek = 2;  // Tuesday
    
    auto recommendations = deferrableController->getDayAheadRecommendations(currentHour, currentDayOfWeek);
    
    std::cout << "\nSample recommendations for key hours:" << std::endl;
    for (int hour : {8, 12, 18, 22}) {
        if (recommendations.find(hour) != recommendations.end()) {
            std::cout << "\nHour " << hour << ":00" << std::endl;
            for (const auto& rec : recommendations[hour]) {
                std::cout << "  - " << rec << std::endl;
            }
        }
    }
    
    // Integration with DayAheadOptimizer
    std::cout << "\n=== Step 7: Integration with Day-Ahead Optimizer ===" << std::endl;
    auto dayAheadOptimizer = std::make_shared<DayAheadOptimizer>(mlPredictor);
    dayAheadOptimizer->setDeferrableLoadController(deferrableController);
    dayAheadOptimizer->addAppliance(heater);
    dayAheadOptimizer->addAppliance(ac);
    dayAheadOptimizer->addAppliance(evCharger);
    
    auto schedule = dayAheadOptimizer->generateSchedule(currentHour, currentDayOfWeek);
    
    std::cout << "\n=== Generated Day-Ahead Schedule (with Deferrable Load Control) ===" << std::endl;
    std::cout << "Total estimated cost: $" << schedule.estimatedCost << std::endl;
    std::cout << "Total estimated consumption: " << schedule.estimatedConsumption << " kWh\n" << std::endl;
    
    // Show sample hours from schedule
    std::cout << "Sample schedule for key hours:" << std::endl;
    for (int hour : {8, 12, 18, 22}) {
        auto actions = schedule.getActionsForHour(hour);
        if (!actions.empty()) {
            std::cout << "\nHour " << hour << ":00" << std::endl;
            for (const auto& action : actions) {
                std::cout << "  - " << action.applianceId << ": " << action.action;
                if (action.value != 0.0) {
                    std::cout << " (" << action.value << ")";
                }
                std::cout << " - " << action.reason << std::endl;
            }
        }
    }
    
    std::cout << "\n=== Deferrable Load Control Demo Summary ===" << std::endl;
    std::cout << "\nThis demonstration showed how to:" << std::endl;
    std::cout << "  1. ✓ Mark appliances as deferrable or non-deferrable" << std::endl;
    std::cout << "  2. ✓ Analyze historical data to identify busy hours" << std::endl;
    std::cout << "  3. ✓ Switch off deferrable loads when price exceeds threshold" << std::endl;
    std::cout << "  4. ✓ Resume deferrable loads when price drops" << std::endl;
    std::cout << "  5. ✓ Generate day-ahead recommendations for deferrable loads" << std::endl;
    std::cout << "  6. ✓ Integrate with day-ahead optimizer for complete scheduling" << std::endl;
    
    std::cout << "\n💡 Key Benefits:" << std::endl;
    std::cout << "   - Automatic load shedding during high-price periods" << std::endl;
    std::cout << "   - Protection of critical loads (heating, cooling, essential lighting)" << std::endl;
    std::cout << "   - Historical data analysis for pattern recognition" << std::endl;
    std::cout << "   - Day-ahead planning for optimal energy usage" << std::endl;
    std::cout << "   - Significant cost savings without compromising comfort" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "=== Demo Complete ===" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return 0;
}
