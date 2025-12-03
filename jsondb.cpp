#include "JsonDB.h"
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <cstdlib> 
#include <ctime>   
#include <mutex> // <--- Added explicit include to fix 'mutex not declared'

using namespace std;

// ==========================================
// CONSTRUCTOR & HELPERS
// ==========================================

JsonDB::JsonDB(const string& fname) : filename(fname) {
    ifstream file(filename);
    if (file.is_open()) {
        try { file >> data; } catch (...) { data = json::object(); }
    }
    
    // If file is empty or missing data, generate it
    if (data.empty() || !data.contains("airports")) {
        seed_data();
    }
    
    // Always build the graph for the algorithm on startup
    build_graph();
}

void JsonDB::save() {
    ofstream file(filename);
    file << data.dump(4);
    // Rebuild graph whenever data changes
    build_graph();
}

int JsonDB::parse_duration_string(const string& dur) {
    try {
        size_t h_pos = dur.find('h');
        size_t m_pos = dur.find('m');
        if (h_pos == string::npos) return 0;
        
        int hours = stoi(dur.substr(0, h_pos));
        int mins = 0;
        
        size_t space_pos = dur.find(' ');
        if (space_pos != string::npos && m_pos != string::npos) {
            mins = stoi(dur.substr(space_pos + 1, m_pos - space_pos - 1));
        }
        return (hours * 60) + mins;
    } catch (...) {
        return 60; 
    }
}

void JsonDB::build_graph() {
    // Note: We don't lock here because this is an internal helper called by locked functions
    adj_list.clear();
    
    if (!data.contains("flights")) return;

    for (const auto& f : data["flights"]) {
        Edge e;
        e.destination = f["to_code"];
        e.flight_id = f["id"];
        e.date = f["date"];
        e.dep_time = f["departure"];
        e.arr_time = f["arrival"];
        e.price = f["price"];
        e.airline = f["airline"];
        e.weight_minutes = parse_duration_string(f["duration"]);

        adj_list[f["from_code"]].push_back(e);
    }
}

// ==========================================
// THE K-SHORTEST PATH ALGORITHM
// ==========================================

struct PathState {
    int total_minutes;
    string current_node;
    vector<Edge> history;

    bool operator>(const PathState& other) const {
        return total_minutes > other.total_minutes;
    }
};

json JsonDB::find_smart_routes(const string& src, const string& dst, const string& req_date, int k) {
    lock_guard<mutex> lock(db_mutex); // Now this will work because headers are correct
    
    json results = json::array();
    
    priority_queue<PathState, vector<PathState>, greater<PathState>> pq;
    pq.push({0, src, {}});

    unordered_map<string, int> visits;

    while (!pq.empty() && results.size() < k) {
        PathState top = pq.top();
        pq.pop();

        string u = top.current_node;

        if (u == dst) {
            json route;
            route["total_time"] = top.total_minutes;
            
            int h = top.total_minutes / 60;
            int m = top.total_minutes % 60;
            route["duration_fmt"] = to_string(h) + "h " + to_string(m) + "m";
            
            route["stops"] = (int)top.history.size() - 1;
            
            json segments = json::array();
            string current_from = src; 

            for(const auto& h : top.history) {
                segments.push_back({
                    {"airline", h.airline},
                    {"flight_id", h.flight_id},
                    {"from", current_from}, 
                    {"to", h.destination},
                    {"dep", h.dep_time},
                    {"arr", h.arr_time},
                    {"price", h.price},
                    {"date", h.date}
                });
                current_from = h.destination;
            }
            
            route["segments"] = segments;
            
            int total_price = 0;
            for(const auto& s : segments) total_price += (int)s["price"];
            route["total_price"] = total_price;

            results.push_back(route);
            continue; 
        }

        if (visits[u] >= k) continue;
        visits[u]++;

        if (adj_list.find(u) != adj_list.end()) {
            for (const auto& edge : adj_list[u]) {
                
                if (edge.date != req_date) continue;

                bool cycle = false;
                for(const auto& prev : top.history) {
                     if (edge.destination == src || prev.destination == edge.destination) cycle = true;
                }
                if (cycle) continue;

                if (!top.history.empty()) {
                    string prev_arr = top.history.back().arr_time;
                    if (edge.dep_time < prev_arr) continue; 
                }

                vector<Edge> new_history = top.history;
                new_history.push_back(edge);
                
                int layover = top.history.empty() ? 0 : 60; 

                pq.push({
                    top.total_minutes + edge.weight_minutes + layover, 
                    edge.destination, 
                    new_history
                });
            }
        }
    }

    return results;
}

// ==========================================
// SEEDING LOGIC
// ==========================================
void JsonDB::seed_data() {
    cout << "[INFO] Seeding database with 50 Airports and Flights..." << endl;

    vector<Airport> airports = {
        {1, "DEL", "Indira Gandhi Intl", "New Delhi", 28.5562, 77.1000},
        {2, "BOM", "Chhatrapati Shivaji Maharaj Intl", "Mumbai", 19.0896, 72.8656},
        {3, "BLR", "Kempegowda Intl", "Bengaluru", 13.1986, 77.7066},
        {4, "MAA", "Chennai Intl", "Chennai", 12.9941, 80.1709},
        {5, "CCU", "Netaji Subhas Chandra Bose Intl", "Kolkata", 22.6547, 88.4467},
        {6, "HYD", "Rajiv Gandhi Intl", "Hyderabad", 17.2403, 78.4294},
        {7, "COK", "Cochin Intl", "Kochi", 10.1518, 76.3930},
        {8, "AMD", "Sardar Vallabhbhai Patel Intl", "Ahmedabad", 23.0732, 72.6347},
        {9, "PNQ", "Pune Intl", "Pune", 18.5821, 73.9197},
        {10, "GOI", "Dabolim", "Goa", 15.3800, 73.8314},
        {11, "TRV", "Thiruvananthapuram Intl", "Thiruvananthapuram", 8.4821, 76.9200},
        {12, "CCJ", "Calicut Intl", "Kozhikode", 11.1363, 75.9553},
        {13, "LKO", "Chaudhary Charan Singh Intl", "Lucknow", 26.7606, 80.8893},
        {14, "GAU", "Lokpriya Gopinath Bordoloi Intl", "Guwahati", 26.1061, 91.5859},
        {15, "JAI", "Jaipur Intl", "Jaipur", 26.8289, 75.8056},
        {16, "SXR", "Srinagar Intl", "Srinagar", 33.9876, 74.7741},
        {17, "BBI", "Biju Patnaik Intl", "Bhubaneswar", 20.2444, 85.8178},
        {18, "PAT", "Jay Prakash Narayan Intl", "Patna", 25.5913, 85.0880},
        {19, "IXC", "Chandigarh Intl", "Chandigarh", 30.6735, 76.7885},
        {20, "IXB", "Bagdogra Intl", "Bagdogra", 26.6812, 88.3286},
        {21, "IDR", "Devi Ahilya Bai Holkar", "Indore", 22.7217, 75.8011},
        {22, "NGP", "Dr. Babasaheb Ambedkar Intl", "Nagpur", 21.0922, 79.0472},
        {23, "VNS", "Lal Bahadur Shastri Intl", "Varanasi", 25.4497, 82.8537},
        {24, "ATQ", "Sri Guru Ram Dass Jee Intl", "Amritsar", 31.7096, 74.7973},
        {25, "VTZ", "Visakhapatnam Intl", "Visakhapatnam", 17.7211, 83.2245},
        {26, "RPR", "Swami Vivekananda", "Raipur", 21.1804, 81.7388},
        {27, "IXM", "Madurai", "Madurai", 9.8345, 78.0934},
        {28, "CJB", "Coimbatore Intl", "Coimbatore", 11.0295, 77.0434},
        {29, "IXR", "Birsa Munda", "Ranchi", 23.3143, 85.3217},
        {30, "UDR", "Maharana Pratap", "Udaipur", 24.6172, 73.8962},
        {31, "BDQ", "Vadodara", "Vadodara", 22.3360, 73.2263},
        {32, "JGA", "Jamnagar", "Jamnagar", 22.4665, 70.0125},
        {33, "IXL", "Kushok Bakula Rimpochee", "Leh", 34.1359, 77.5465},
        {34, "TRZ", "Tiruchirappalli Intl", "Tiruchirappalli", 10.7654, 78.7097},
        {35, "IXJ", "Jammu", "Jammu", 32.6891, 74.8375},
        {36, "BHO", "Raja Bhoj", "Bhopal", 23.2875, 77.3378},
        {37, "JDH", "Jodhpur", "Jodhpur", 26.2515, 73.0485},
        {38, "IXA", "Agartala", "Agartala", 23.8870, 91.2404},
        {39, "IMF", "Imphal", "Imphal", 24.7600, 93.8967},
        {40, "STV", "Surat", "Surat", 21.1137, 72.7418},
        {41, "IXE", "Mangaluru Intl", "Mangaluru", 12.9613, 74.8901},
        {42, "TIR", "Tirupati", "Tirupati", 13.6325, 79.5436},
        {43, "VGA", "Vijayawada", "Vijayawada", 16.5304, 80.7968},
        {44, "IXZ", "Veer Savarkar Intl", "Port Blair", 11.6410, 92.7297},
        {45, "DED", "Dehradun", "Dehradun", 30.1897, 78.1803},
        {46, "HBX", "Hubli", "Hubli", 15.3617, 75.0849},
        {47, "AJL", "Lengpui", "Aizawl", 23.8397, 92.6236},
        {48, "DMU", "Dimapur", "Dimapur", 25.8839, 93.7714},
        {49, "MYQ", "Mysuru", "Mysuru", 12.2300, 76.6500},
        {50, "GWL", "Gwalior", "Gwalior", 26.2936, 78.2274}
    };
    data["airports"] = airports;

    vector<Flight> flights;
    int flight_counter = 1000;
    string airlines[] = {"IndiGo", "Air India", "Vistara", "SpiceJet", "Akasa Air"};
    srand(time(0));

    for (size_t i = 0; i < airports.size(); ++i) {
        string src = airports[i].code;
        for (int k = 0; k < 5; ++k) {
            int dest_idx = (i + k + 1) % airports.size();
            string dst = airports[dest_idx].code;

            for (int day = 1; day <= 10; ++day) {
                string date = "2025-12-" + (day < 10 ? "0" + to_string(day) : to_string(day));
                
                int dep_h = 6 + (rand() % 14);
                int dep_m = (rand() % 4) * 15;
                int dur_h = 1 + (rand() % 3);
                int arr_h = (dep_h + dur_h) % 24;

                char time_buf[10];
                sprintf(time_buf, "%02d:%02d", dep_h, dep_m);
                string dep = time_buf;
                sprintf(time_buf, "%02d:%02d", arr_h, dep_m);
                string arr = time_buf;

                Flight f;
                f.id = "FL" + to_string(flight_counter++);
                f.airline = airlines[rand() % 5];
                f.from_code = src;
                f.to_code = dst;
                f.date = date;
                f.departure = dep;
                f.arrival = arr;
                f.duration = to_string(dur_h) + "h 00m";
                f.price = 3000 + (rand() % 5000);

                flights.push_back(f);
            }
        }
    }
    data["flights"] = flights;
    save(); 
}

// ==========================================
// API GETTERS & ADMIN OPS
// ==========================================

json JsonDB::get_all_airports() {
    lock_guard<mutex> lock(db_mutex);
    return data.value("airports", json::array());
}

json JsonDB::get_flights_limited(int limit) {
    lock_guard<mutex> lock(db_mutex);
    json res = json::array();
    if(data.contains("flights")) {
        int c=0;
        for(auto& f : data["flights"]) {
            if(c++ >= limit) break;
            res.push_back(f);
        }
    }
    return res;
}

bool JsonDB::add_airport(const Airport& apt) {
    lock_guard<mutex> lock(db_mutex);
    if (!data.contains("airports")) data["airports"] = json::array();
    for(auto& x : data["airports"]) if(x["code"] == apt.code) return false;
    json j = apt; data["airports"].push_back(j); save(); return true;
}

bool JsonDB::delete_airport(const string& code) {
    lock_guard<mutex> lock(db_mutex);
    if(!data.contains("airports")) return false;
    auto& arr = data["airports"];
    for(auto it = arr.begin(); it != arr.end(); ++it) {
        if((*it)["code"] == code) { arr.erase(it); save(); return true; }
    }
    return false;
}

bool JsonDB::update_airport(const string& code, const json& new_data) {
    lock_guard<mutex> lock(db_mutex);
    if (!data.contains("airports")) return false;
    for (auto& apt : data["airports"]) {
        if (apt["code"] == code) {
            for (auto& el : new_data.items()) apt[el.key()] = el.value();
            save(); return true;
        }
    }
    return false;
}

bool JsonDB::add_flight(const Flight& fl) {
    lock_guard<mutex> lock(db_mutex);
    if (!data.contains("flights")) data["flights"] = json::array();
    for (const auto& existing : data["flights"]) {
        if (existing.value("id", "") == fl.id) return false;
    }
    json j = fl; data["flights"].push_back(j); save(); return true;
}

bool JsonDB::delete_flight(const string& id) {
    lock_guard<mutex> lock(db_mutex);
    if(!data.contains("flights")) return false;
    auto& arr = data["flights"];
    for(auto it = arr.begin(); it != arr.end(); ++it) {
        if((*it)["id"] == id) { arr.erase(it); save(); return true; }
    }
    return false;
}

bool JsonDB::update_flight(const string& id, const json& new_data) {
    lock_guard<mutex> lock(db_mutex);
    if (!data.contains("flights")) return false;
    for (auto& fl : data["flights"]) {
        if (fl["id"] == id) {
            for (auto& el : new_data.items()) fl[el.key()] = el.value();
            save(); return true;
        }
    }
    return false;
}