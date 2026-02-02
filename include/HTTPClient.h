#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <vector>

struct EnergyCostData {
    long timestamp;
    double costPerKwh;
};

// HTTP Client for fetching energy cost data from API
// In production, this would use a library like libcurl or cpp-httplib
class HTTPClient {
public:
    HTTPClient(const std::string& apiUrl)
        : apiUrl_(apiUrl) {}

    std::vector<EnergyCostData> fetchHourlyEnergyCosts() {
        // In production: make HTTP GET request to API
        // For simulation: return mock data
        std::vector<EnergyCostData> costs;
        
        // Simulate 24 hours of energy costs
        long currentTime = 0; // In production: use actual timestamp
        for (int hour = 0; hour < 24; hour++) {
            EnergyCostData cost;
            cost.timestamp = currentTime + (hour * 3600);
            
            // Simulate varying energy costs (peak hours more expensive)
            if (hour >= 8 && hour <= 20) {
                cost.costPerKwh = 0.15 + (0.05 * (hour % 4)); // Higher during day
            } else {
                cost.costPerKwh = 0.08; // Lower at night
            }
            
            costs.push_back(cost);
        }
        
        return costs;
    }

    double getCurrentEnergyCost() {
        // In production: make HTTP GET request for current cost
        // For simulation: return mock value
        return 0.12; // Default cost per kWh
    }

private:
    std::string apiUrl_;
};

#endif // HTTP_CLIENT_H
