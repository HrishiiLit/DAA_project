#include "crow.h"
#include "JsonDB.h"
#include <iostream>

JsonDB db("flight_database.json");

int main() {
    crow::SimpleApp app;

    // ==========================================
    // PUBLIC ROUTES (Read Only)
    // ==========================================
    
    CROW_ROUTE(app, "/api/airports")
    ([](){
        return crow::response(db.get_all_airports().dump());
    });

    CROW_ROUTE(app, "/api/flights")
    ([](const crow::request& req){
        int limit = 10;
        if (req.url_params.get("limit")) limit = std::stoi(req.url_params.get("limit"));
        return crow::response(db.get_flights_limited(limit).dump());
    });

    CROW_ROUTE(app, "/api/search")
    ([](const crow::request& req){
        const char* src = req.url_params.get("from");
        const char* dst = req.url_params.get("to");
        if (!src || !dst) return crow::response(400, "Missing 'from' or 'to'");
        return crow::response(db.search_flights(src, dst).dump());
    });

    CROW_ROUTE(app, "/api/search_date")
    ([](const crow::request& req){
        const char* date = req.url_params.get("date");
        if (!date) return crow::response(400, "Missing 'date'");
        return crow::response(db.search_flights_by_date(date).dump());
    });

    // ==========================================
    // ADMIN ROUTES: AIRPORTS
    // ==========================================

    // 1. Add Airport (POST)
    CROW_ROUTE(app, "/admin/airport/add").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        if (db.add_airport(body)) return crow::response(201, "Airport Added");
        else return crow::response(409, "Airport code already exists");
    });

    // 2. Delete Airport (Using POST)
    // URL: /admin/airport/delete
    // Body: { "code": "JFK" }
    CROW_ROUTE(app, "/admin/airport/delete").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        std::string code = body.value("code", "");
        if (code.empty()) return crow::response(400, "Missing 'code'");

        if (db.delete_airport(code)) return crow::response(200, "Airport Deleted");
        else return crow::response(404, "Airport not found");
    });

    // 3. Update Airport (Using POST to simulate PUT)
    // URL: /admin/airport/update?code=JFK
    CROW_ROUTE(app, "/admin/airport/update").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        const char* code = req.url_params.get("code");
        if (!code) return crow::response(400, "Missing 'code' param");

        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        if (db.update_airport(code, body)) return crow::response(200, "Airport Updated");
        else return crow::response(404, "Airport not found");
    });


    // ==========================================
    // ADMIN ROUTES: FLIGHTS
    // ==========================================

    // 1. Add Flight (POST)
    CROW_ROUTE(app, "/admin/flight/add").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        if (db.add_flight(body)) return crow::response(201, "Flight Added");
        else return crow::response(409, "Flight ID already exists");
    });

    // 2. Delete Flight (Using POST)
    // URL: /admin/flight/delete
    // Body: { "id": "FL101" }
    CROW_ROUTE(app, "/admin/flight/delete").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        std::string id = body.value("id", "");
        if (id.empty()) return crow::response(400, "Missing 'id'");

        if (db.delete_flight(id)) return crow::response(200, "Flight Deleted");
        else return crow::response(404, "Flight not found");
    });

    // 3. Update Flight (Using POST to simulate PUT)
    // URL: /admin/flight/update?id=FL101
    CROW_ROUTE(app, "/admin/flight/update").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        const char* id = req.url_params.get("id");
        if (!id) return crow::response(400, "Missing 'id' param");

        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400, "Invalid JSON");

        if (db.update_flight(id, body)) return crow::response(200, "Flight Updated");
        else return crow::response(404, "Flight not found");
    });

    // ==========================================
    // OPTIONAL: Delete via GET (Easiest for links)
    // URL: /admin/flight/delete_link?id=FL101
    // ==========================================
    CROW_ROUTE(app, "/admin/flight/delete_link").methods(crow::HTTPMethod::GET)
    ([](const crow::request& req){
        const char* id = req.url_params.get("id");
        if (!id) return crow::response(400, "Missing 'id'");

        if (db.delete_flight(id)) return crow::response(200, "Flight Deleted via Link");
        else return crow::response(404, "Flight not found");
    });

    std::cout << "Server starting on port 18080..." << std::endl;
    app.port(18080).multithreaded().run();
}