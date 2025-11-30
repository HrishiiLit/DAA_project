#include "crow.h"
#include <nlohmann/json.hpp>
#include <bits/stdc++.h>

using namespace std;
using json = nlohmann::json;

json parth(const string& source, const string& destination) {
    // Dummy implementation for demonstration
    json paths = json::array();
    paths.push_back({{"path_id", 1}, {"nodes", {"Hrishi", "Clite", "Gym"}}});
    paths.push_back({{"path_id", 2}, {"nodes", {"Hrishi", "Main building", "Gym"}}});
    paths.push_back({{"path_id", 3}, {"nodes", {"Hrishi", "Road", "GYm"}}});
    return paths;
}

int main()
{
    crow::SimpleApp app;
    // mysql connector
    // ==========================================
    // ROUTE 1: GET /health
    // Check if server is alive
    // ==========================================
    CROW_ROUTE(app, "/health")
    ([](){
        json response;
        response["status"] = "running";
        response["port"] = 18080;
        return crow::response(response.dump());
    });

    // ==========================================
    // ROUTE 2: POST /api/user
    // Receives JSON data from Frontend
    // ==========================================
    CROW_ROUTE(app, "/api/user").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        
        auto req_body = json::parse(req.body, nullptr, false); // parse JSON body

        // Validate JSON
        if (req_body.is_discarded()) {
            return crow::response(400, "Invalid JSON");
        }

        // Extract data from JSON
        string name = req_body["username"];
        int age = req_body["age"];

        // json object for response
        json response_data; 
        response_data["status"] = "success";
        response_data["message"] = "User " + name + " created!";
        response_data["id"] = 101; 

        return crow::response(201, response_data.dump());
    });

    // get method to search with two params source and destination 
    CROW_ROUTE(app,"/api/search").methods(crow::HTTPMethod::GET)
    ([](const crow::request& req){
        string source = req.url_params.get("source");
        string destination = req.url_params.get("destination");

        if (source.empty() || destination.empty()) {
            json error;
            error["status"] = "error";
            error["message"] = "Missing 'source' or 'destination' parameter";
            return crow::response(400, error.dump());
        }

        // Call parth function and get 3 paths
        json result_paths = parth(source, destination);

        // Prepare response
        json response;
        response["status"] = "success";
        response["source"] = source;
        response["destination"] = destination;
        response["paths_found"] = result_paths.size();
        response["paths"] = result_paths;

        return crow::response(200, response.dump());
    });

    // ==========================================
    // START SERVER
    // ==========================================
    app.port(18080).multithreaded().run();
}

