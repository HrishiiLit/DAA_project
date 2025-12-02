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
// ==========================================
// ADMIN IMPLEMENTATION
// ==========================================

// --- AIRPORTS ---
bool JsonDB::add_airport(const json& airport_data) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!data.contains("airports")) data["airports"] = json::array();
    
    // Check duplicate
    std::string new_code = airport_data.value("code", "");
    for(const auto& apt : data["airports"]) {
        if(apt.value("code", "") == new_code) return false; // Exists
    }

    data["airports"].push_back(airport_data);
    save();
    return true;
}

bool JsonDB::delete_airport(const std::string& code) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!data.contains("airports")) return false;

    auto& arr = data["airports"];
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        if ((*it).value("code", "") == code) {
            arr.erase(it);
            save();
            return true;
        }
    }
    return false;
}

bool JsonDB::update_airport(const std::string& code, const json& new_data) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!data.contains("airports")) return false;

    for (auto& apt : data["airports"]) {
        if (apt.value("code", "") == code) {
            // Update fields provided in new_data
            for (auto& el : new_data.items()) {
                apt[el.key()] = el.value();
            }
            save();
            return true;
        }
    }
    return false;
}

// --- FLIGHTS ---
bool JsonDB::add_flight(const json& flight_data) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!data.contains("flights")) data["flights"] = json::array();
    
    // Check duplicate ID
    std::string new_id = flight_data.value("id", "");
    for(const auto& fl : data["flights"]) {
        if(fl.value("id", "") == new_id) return false;
    }

    data["flights"].push_back(flight_data);
    save();
    return true;
}

bool JsonDB::delete_flight(const std::string& id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!data.contains("flights")) return false;

    auto& arr = data["flights"];
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        if ((*it).value("id", "") == id) {
            arr.erase(it);
            save();
            return true;
        }
    }
    return false;
}

bool JsonDB::update_flight(const std::string& id, const json& new_data) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (!data.contains("flights")) return false;

    for (auto& fl : data["flights"]) {
        if (fl.value("id", "") == id) {
            for (auto& el : new_data.items()) {
                fl[el.key()] = el.value();
            }
            save();
            return true;
        }
    }
    return false;
}