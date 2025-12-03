#include "crow.h"
#include "JsonDB.h"
#include <iostream>
#include <string>

// ==========================================
// 1. DEFINE CORS MIDDLEWARE
// ==========================================
struct CORSHandler {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        // No action needed before handling request
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
};

// Initialize Database
JsonDB db("flight_database.json");

int main() {
    crow::App<CORSHandler> app;

    // ==========================================
    // GLOBAL OPTIONS ROUTE (The Fix is Here)
    // ==========================================
    // Because we use "/<path>", Crow passes the path as a string argument.
    // We added 'std::string' to the arguments list below.
    CROW_ROUTE(app, "/<path>")
    .methods(crow::HTTPMethod::OPTIONS)
    ([](const crow::request& req, std::string path){ 
        return crow::response(200);
    });

    // ==========================================
    // PUBLIC ROUTES
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

    // 1. Standard Direct Search (Legacy support)
    CROW_ROUTE(app, "/api/search")
    ([](const crow::request& req){
        const char* src = req.url_params.get("from");
        const char* dst = req.url_params.get("to");
        
        // Default date if missing
        std::string date = "2025-12-01";
        if (req.url_params.get("date")) date = req.url_params.get("date");

        if (!src || !dst) return crow::response(400, "Missing 'from' or 'to'");
        
        // Use the new smart function even for standard search
        return crow::response(db.find_smart_routes(src, dst, date, 5).dump());
    });

    // 2. Smart Search (Explicit Endpoint)
    // /api/search_smart?from=<src>&to=<dst>&date=<yyyy-mm-dd>
    // sample call: /api/search_smart?from=JFK&to=LAX&date=2025-12-01
    CROW_ROUTE(app, "/api/search_smart")
    ([](const crow::request& req){
        const char* src = req.url_params.get("from");
        const char* dst = req.url_params.get("to");
        const char* date = req.url_params.get("date"); 

        if (!src || !dst || !date) {
            return crow::response(400, "Missing 'from', 'to', or 'date' params");
        }

        // Call the algorithm inside JsonDB
        json routes = db.find_smart_routes(src, dst, date, 5); 

        return crow::response(routes.dump());
    });

    // ==========================================
    // ADMIN ROUTES
    // ==========================================

    CROW_ROUTE(app, "/admin/airport/add").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400);
        try {
            if (db.add_airport(body.get<Airport>())) return crow::response(201, "Added");
            return crow::response(409, "Exists");
        } catch(...) { return crow::response(400); }
    });

    CROW_ROUTE(app, "/admin/airport/delete").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400);
        std::string code = body.value("code", "");
        if (db.delete_airport(code)) return crow::response(200, "Deleted");
        return crow::response(404, "Not Found");
    });

    CROW_ROUTE(app, "/admin/airport/update").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        const char* code = req.url_params.get("code");
        if (!code) return crow::response(400, "Missing code");
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400);
        if (db.update_airport(code, body)) return crow::response(200, "Updated");
        return crow::response(404, "Not Found");
    });

    CROW_ROUTE(app, "/admin/flight/add").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400);
        try {
            if (db.add_flight(body.get<Flight>())) return crow::response(201, "Added");
            return crow::response(409, "Exists");
        } catch(...) { return crow::response(400); }
    });

    CROW_ROUTE(app, "/admin/flight/delete").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400);
        std::string id = body.value("id", "");
        if (db.delete_flight(id)) return crow::response(200, "Deleted");
        return crow::response(404, "Not Found");
    });

    CROW_ROUTE(app, "/admin/flight/update").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        const char* id = req.url_params.get("id");
        if (!id) return crow::response(400, "Missing id");
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) return crow::response(400);
        if (db.update_flight(id, body)) return crow::response(200, "Updated");
        return crow::response(404, "Not Found");
    });

    // 404 Handler
    app.catchall_route()
    ([](const crow::request& req, crow::response& res) {
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 200;
            res.end();
        } else {
            res.code = 404;
            res.body = "{\"error\": \"Route not found\"}";
            res.end();
        }
    });

    // Port Logic
    int port = 18080;
    if (const char* env_p = std::getenv("PORT")) {
        port = std::stoi(env_p);
    }
    
    std::cout << "Server starting on port " << port << "..." << std::endl;
    app.port(port).multithreaded().run();
}