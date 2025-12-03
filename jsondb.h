#ifndef JSONDB_H
#define JSONDB_H

#include <string>
#include <mutex>    // <--- REQUIRED for mutex
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "Models.h"

using json = nlohmann::json;

// Internal Edge Structure for Graph Algorithms
struct Edge {
    std::string destination; 
    int weight_minutes;      
    std::string flight_id;   
    std::string date;        
    std::string dep_time;    
    std::string arr_time;    
    int price;
    std::string airline;
};

class JsonDB {
private:
    std::string filename;
    json data;
    std::mutex db_mutex; // <--- REQUIRED: This is the variable causing your error

    // The Graph: Source Code -> List of Flights
    std::unordered_map<std::string, std::vector<Edge>> adj_list;

    void seed_data();
    void save();
    void build_graph(); 
    int parse_duration_string(const std::string& dur);

public:
    JsonDB(const std::string& fname);

    // Read APIs
    json get_all_airports();
    json get_flights_limited(int limit);
    
    // Smart Search
    json find_smart_routes(const std::string& src, const std::string& dst, const std::string& date, int k = 5);

    // Admin APIs
    bool add_airport(const Airport& airport);
    bool delete_airport(const std::string& code);
    bool update_airport(const std::string& code, const json& new_data);
    
    bool add_flight(const Flight& flight);
    bool delete_flight(const std::string& id);
    bool update_flight(const std::string& id, const json& new_data);
};

#endif