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
    std::mutex db_mutex;

    void seed_data();
    void save();

public:
    JsonDB(const std::string& fname);

    // --- PUBLIC APIs --- //
    json get_all_airports();
    json get_flights_limited(int limit);
    json search_flights(const std::string& src, const std::string& dst);
    json search_flights_by_date(const std::string& date);

    // --- ADMIN APIs (Airports) --- //
    bool add_airport(const json& airport_data);
    bool delete_airport(const std::string& code);
    bool update_airport(const std::string& code, const json& new_data);

    // --- ADMIN APIs (Flights) --- //
    bool add_flight(const json& flight_data);
    bool delete_flight(const std::string& id);
    bool update_flight(const std::string& id, const json& new_data);
};

#endif