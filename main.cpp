#include "crow.h"
#include "JsonDB.h" // Include your new class header
#include <iostream>

// Create a global instance of the Database
JsonDB db("flight_database.json");

int main() {
    crow::SimpleApp app;

    // ==========================================
    // ROUTE 1: Get All Airports
    // ==========================================
    CROW_ROUTE(app, "/api/airports")
    ([](){
        json airports = db.get_all_airports();
        return crow::response(airports.dump());
    });

    // ==========================================
    // ROUTE 2: Get Flights (Limit: 10)
    // ==========================================
    CROW_ROUTE(app, "/api/flights")
    ([](const crow::request& req){
        int limit = 10;
        if (req.url_params.get("limit") != nullptr) {
            limit = std::stoi(req.url_params.get("limit"));
        }
        json flights = db.get_flights_limited(limit);
        return crow::response(flights.dump());
    });

    // ==========================================
    // ROUTE 3: Search (Source & Destination)
    // URL: /api/search?from=JFK&to=LHR
    // ==========================================
    CROW_ROUTE(app, "/api/search")
    ([](const crow::request& req){
        const char* src = req.url_params.get("from");
        const char* dst = req.url_params.get("to");

        if (!src || !dst) return crow::response(400, "Missing 'from' or 'to'");

        json result = db.search_flights(src, dst);
        return crow::response(result.dump());
    });

    // ==========================================
    // ROUTE 4: Search (Date)
    // URL: /api/search_date?date=2023-12-01
    // ==========================================
    CROW_ROUTE(app, "/api/search_date")
    ([](const crow::request& req){
        const char* date = req.url_params.get("date");

        if (!date) return crow::response(400, "Missing 'date'");

        json result = db.search_flights_by_date(date);
        return crow::response(result.dump());
    });

    std::cout << "Server starting on port 18080..." << std::endl;
    app.port(18080).multithreaded().run();
}