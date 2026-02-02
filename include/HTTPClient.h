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
    HTTPClient(const std::string& apiUrl);

    std::vector<EnergyCostData> fetchHourlyEnergyCosts();
    double getCurrentEnergyCost();

private:
    std::string apiUrl_;
};

#endif // HTTP_CLIENT_H
