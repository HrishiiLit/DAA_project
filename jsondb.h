#ifndef JSONDB_H
#define JSONDB_H

#include <string>
#include <mutex>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonDB {
private:
    std::string filename;
    json data;
    std::mutex db_mutex; // Handles thread safety

    // Helper functions (internal use only)
    void seed_data(); // Generates dummy data if file is empty
    void save();      // Saves JSON to disk

public:
    // Constructor
    JsonDB(const std::string& fname);

    // API Functions
    json get_all_airports();
    json get_flights_limited(int limit);
    json search_flights(const std::string& src, const std::string& dst);
    json search_flights_by_date(const std::string& date);
    
    // Optional: Add a flight
    void add_flight(const json& flight_data);
};

#endif