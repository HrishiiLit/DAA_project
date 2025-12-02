#include "crow.h"
#include "jsondb.h"
#include <iostream>

// Initialize Database (Auto-seeds if file missing)
JsonDB db("flight_database.json");

int main() {
    crow::SimpleApp app;

    // ==========================================
    // PUBLIC ROUTES
    // ==========================================
    
    // 1. Get All Airports
    CROW_ROUTE(app, "/api/airports")
    ([](){
        return crow::response(db.get_all_airports().dump());
    });

    // 2. Get Flights (with limit)
    CROW_ROUTE(app, "/api/flights")
    ([](const crow::request& req){
        int limit = 10;
        if (req.url_params.get("limit")) limit = std::stoi(req.url_params.get("limit"));
        return crow::response(db.get_flights_limited(limit).dump());
    });

    // 3. Search Route
    CROW_ROUTE(app, "/api/search")
    ([](const crow::request& req){
        const char* src = req.url_params.get("from");
        const char* dst = req.url_params.get("to");
        if (!src || !dst) return crow::response(400, "Missing 'from' or 'to'");
        return crow::response(db.search_flights(src, dst).dump());
    });

    // 4. Search Date
    CROW_ROUTE(app, "/api/search_date")
    ([](const crow::request& req){
        const char* date = req.url_params.get("date");
        if (!date) return crow::response(400, "Missing 'date'");
        return crow::response(db.search_flights_by_date(date).dump());
    });

    // ==========================================
    // ADMIN ROUTES: AIRPORTS
    // ==========================================

    CROW_ROUTE(app, "/admin/airport/add").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        // Convert JSON to C++ Struct
        Airport apt;
        try {
            apt = body.get<Airport>(); // Automatic conversion
        } catch (...) {
            return crow::response(400, "Missing required airport fields");
        }

        if (db.add_airport(apt)) return crow::response(201, "Airport Added");
        return crow::response(409, "Airport code already exists");
    });

    CROW_ROUTE(app, "/admin/airport/delete").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");
        
        std::string code = body.value("code", "");
        if (db.delete_airport(code)) return crow::response(200, "Airport Deleted");
        return crow::response(404, "Airport not found");
    });

    CROW_ROUTE(app, "/admin/airport/update").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        const char* code = req.url_params.get("code");
        if (!code) return crow::response(400, "Missing 'code' param");

        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        if (db.update_airport(code, body)) return crow::response(200, "Airport Updated");
        return crow::response(404, "Airport not found");
    });

    // ==========================================
    // ADMIN ROUTES: FLIGHTS
    // ==========================================

    CROW_ROUTE(app, "/admin/flight/add").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        Flight fl;
        try {
            fl = body.get<Flight>();
        } catch (...) {
            return crow::response(400, "Missing required flight fields");
        }

        if (db.add_flight(fl)) return crow::response(201, "Flight Added");
        return crow::response(409, "Flight ID already exists");
    });

    CROW_ROUTE(app, "/admin/flight/delete").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");
        
        std::string id = body.value("id", "");
        if (db.delete_flight(id)) return crow::response(200, "Flight Deleted");
        return crow::response(404, "Flight not found");
    });

    CROW_ROUTE(app, "/admin/flight/update").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        const char* id = req.url_params.get("id");
        if (!id) return crow::response(400, "Missing 'id' param");

        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        if (db.update_flight(id, body)) return crow::response(200, "Flight Updated");
        return crow::response(404, "Flight not found");
    });

    std::cout << "Server running on port 18080..." << std::endl;
    app.port(18080).multithreaded().run();
}