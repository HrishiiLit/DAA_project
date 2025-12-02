#include "crow.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>  // Required for file handling
#include <mutex>    // Required for thread safety
#include <vector>

using json = nlohmann::json;

// ==========================================
// CONFIGURATION
// ==========================================
const std::string DB_FILE = "database.json";
std::mutex db_mutex; // Prevents crashing if two users post at the same time

// ==========================================
// HELPER FUNCTIONS (The "Database" Logic)
// ==========================================

// Load data from disk
json load_db() {
    std::ifstream file(DB_FILE);
    if (!file.is_open() || file.peek() == std::ifstream::traits_type::eof()) {
        return json::array(); // Return empty list if file doesn't exist
    }
    try {
        return json::parse(file);
    } catch (...) {
        return json::array(); // Return empty list if file is corrupted
    }
}

// Save data to disk
void save_db(const json& data) {
    std::ofstream file(DB_FILE);
    file << data.dump(4); // 4 = Indent with 4 spaces (pretty print)
}

int main()
{
    crow::SimpleApp app;

    // ==========================================
    // ROUTE 1: GET /health
    // Check if server is alive
    // ==========================================
    CROW_ROUTE(app, "/health")
    ([](){
        json response;
        response["status"] = "running";
        response["port"] = 18080;
        response["database_type"] = "json_file";
        return crow::response(response.dump());
    });

    // ==========================================
    // ROUTE 2: POST /api/user
    // Save User to JSON File
    // ==========================================
    CROW_ROUTE(app, "/api/user").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        
        auto req_body = json::parse(req.body, nullptr, false);

        if (req_body.is_discarded()) {
            return crow::response(400, "Invalid JSON");
        }

        std::string name = req_body["username"];
        int age = req_body["age"];

        // LOCK THE DB (Thread Safety)
        std::lock_guard<std::mutex> lock(db_mutex);

        // 1. Load existing
        json db = load_db();

        // 2. Add new user
        json new_user;
        new_user["id"] = db.size() + 1; // Simple Auto-Increment ID
        new_user["username"] = name;
        new_user["age"] = age;
        
        db.push_back(new_user);

        // 3. Save back to file
        save_db(db);

        // Response
        json response_data;
        response_data["status"] = "success";
        response_data["message"] = "User " + name + " saved to file!";
        response_data["user"] = new_user;

        return crow::response(201, response_data.dump());
    });

    // ==========================================
    // ROUTE 3: GET /users
    // Retrieve all users
    // ==========================================
    CROW_ROUTE(app, "/users")
    ([](){
        std::lock_guard<std::mutex> lock(db_mutex);
        json db = load_db();
        return crow::response(db.dump());
    });

    // ==========================================
    // START SERVER
    // ==========================================
    app.port(18080).multithreaded().run();
}