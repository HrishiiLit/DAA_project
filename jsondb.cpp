#include "JsonDB.h"
#include <fstream>
#include <iostream>

// Constructor: Loads data from file
JsonDB::JsonDB(const std::string& fname) : filename(fname) {
    std::ifstream file(filename);
    if (file.is_open()) {
        try {
            file >> data;
        } catch (...) {
            data = json::object(); // Reset if corrupted
        }
    }
    
    // If database is empty, create dummy data
    if (data.empty() || !data.contains("airports")) {
        seed_data();
    }
}

// Save to disk
void JsonDB::save() {
    std::ofstream file(filename);
    file << data.dump(4);
}

// Generate Dummy Data
void JsonDB::seed_data() {
    data["airports"] = {
        {{"code", "JFK"}, {"name", "John F. Kennedy"}, {"city", "New York"}},
        {{"code", "LHR"}, {"name", "Heathrow"}, {"city", "London"}},
        {{"code", "DXB"}, {"name", "Dubai Intl"}, {"city", "Dubai"}},
        {{"code", "DEL"}, {"name", "Indira Gandhi Intl"}, {"city", "New Delhi"}},
        {{"code", "HND"}, {"name", "Haneda"}, {"city", "Tokyo"}}
    };

    data["flights"] = {
        {{"id", "FL101"}, {"from", "JFK"}, {"to", "LHR"}, {"date", "2023-12-01"}, {"price", 500}},
        {{"id", "FL102"}, {"from", "LHR"}, {"to", "DXB"}, {"date", "2023-12-01"}, {"price", 450}},
        {{"id", "FL103"}, {"from", "DEL"}, {"to", "DXB"}, {"date", "2023-12-02"}, {"price", 200}},
        {{"id", "FL104"}, {"from", "HND"}, {"to", "JFK"}, {"date", "2023-12-05"}, {"price", 800}},
        {{"id", "FL105"}, {"from", "JFK"}, {"to", "LHR"}, {"date", "2023-12-05"}, {"price", 550}}
    };

    // Generate extra flights for testing pagination
    for(int i=6; i<=20; i++) {
        data["flights"].push_back({
            {"id", "FL1" + std::to_string(i)}, 
            {"from", "JFK"}, 
            {"to", "DXB"}, 
            {"date", "2023-12-10"}, 
            {"price", 400 + i}
        });
    }
    save();
}

// 1. Fetch All Airports
json JsonDB::get_all_airports() {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (data.contains("airports")) {
        return data["airports"];
    }
    return json::array();
}

// 2. Fetch First N Flights
json JsonDB::get_flights_limited(int limit) {
    std::lock_guard<std::mutex> lock(db_mutex);
    json result = json::array();
    
    if (!data.contains("flights")) return result;

    const auto& flights = data["flights"];
    int count = 0;
    
    for (const auto& flight : flights) {
        if (count >= limit) break;
        result.push_back(flight);
        count++;
    }
    return result;
}

// 3. Search Flights (Source + Destination)
json JsonDB::search_flights(const std::string& src, const std::string& dst) {
    std::lock_guard<std::mutex> lock(db_mutex);
    json result = json::array();

    if (!data.contains("flights")) return result;

    for (const auto& flight : data["flights"]) {
        if (flight.value("from", "") == src && flight.value("to", "") == dst) {
            result.push_back(flight);
        }
    }
    return result;
}

// 4. Search Flights (Date)
json JsonDB::search_flights_by_date(const std::string& date) {
    std::lock_guard<std::mutex> lock(db_mutex);
    json result = json::array();

    if (!data.contains("flights")) return result;

    for (const auto& flight : data["flights"]) {
        if (flight.value("date", "") == date) {
            result.push_back(flight);
        }
    }
    return result;
}

void JsonDB::add_flight(const json& flight_data) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!data.contains("flights")) data["flights"] = json::array();
    data["flights"].push_back(flight_data);
    save();
}