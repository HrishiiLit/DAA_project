#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <map>
#include <algorithm>
#include <iomanip>

using namespace std;

// ==========================================
// 1. Data Structures
// ==========================================

// Represents a single scheduled flight (The Edge)
struct Flight {
    string id;          // Flight Number (e.g., "BA101")
    string from;        // Origin
    string to;          // Destination
    int cost;           // Price in USD
    int depTime;        // Departure Time (Minutes from T=0)
    int arrTime;        // Arrival Time (Minutes from T=0)
};

// Represents a complete Path found (for returning results)
struct PathResult {
    int totalCost;
    int arrivalTime;
    vector<string> routeDescription; // List of flight IDs and segments
};

// Represents the State in the Priority Queue
struct SearchState {
    int currentCost;            // Accumulator: Total Money Spent
    string currentAirport;      // Where are we?
    int arrivalTime;            // When did we arrive here?
    
    vector<string> pathHistory; // For tracking the route
    vector<string> visitedNodes; // To prevent cycles (A->B->A)

    // PRIORITY QUEUE SORTING LOGIC:
    // We want the LOWEST cost to be at the TOP.
    // In C++ std::priority_queue, 'true' means "put lower in hierarchy".
    // So for a Min-Heap, we need greater-than logic.
    bool operator>(const SearchState& other) const {
        return currentCost > other.currentCost;
    }
};

// ==========================================
// 2. The Flight Graph Class
// ==========================================

class FlightNetwork {
public:
    // Adjacency List: Map Airport Code -> List of Flights leaving it
    map<string, vector<Flight>> adjList;

    void addFlight(string id, string u, string v, int cost, int dep, int duration) {
        Flight f;
        f.id = id;
        f.from = u;
        f.to = v;
        f.cost = cost;
        f.depTime = dep;
        f.arrTime = dep + duration;
        adjList[u].push_back(f);
    }

    // ---------------------------------------------------------
    // THE ALGORITHM: Find Top K Shortest Paths (By Price)
    // ---------------------------------------------------------
    vector<PathResult> getTopKPaths(string startNode, string endNode, int k, int minLayoverMins) {
        
        vector<PathResult> results;
        
        // 1. Priority Queue (Min-Heap based on Cost)
        priority_queue<SearchState, vector<SearchState>, greater<SearchState>> pq;

        // 2. Initial State
        // Cost 0, At StartNode, Arrival Time -1 (indicates start of journey)
        pq.push({0, startNode, -1, {}, {startNode}});

        // 3. Counter for how many times we visited specific nodes
        // (Optimization: Stop processing a node if we've found K+ paths through it already)
        map<string, int> visitCounts;

        while (!pq.empty()) {
            // Pop the cheapest option
            SearchState current = pq.top();
            pq.pop();

            string u = current.currentAirport;

            // --- GOAL CHECK ---
            if (u == endNode) {
                // Construct Result
                PathResult res;
                res.totalCost = current.currentCost;
                res.arrivalTime = current.arrivalTime;
                res.routeDescription = current.pathHistory;
                results.push_back(res);

                // If we found K paths, we are done
                if (results.size() >= k) return results;
                
                // Otherwise, continue to find the next best one
                continue; 
            }

            // --- PRUNING ---
            // If we have extracted this airport from PQ more than K+2 times,
            // it's unlikely to yield a 'top K' result better than what we have.
            visitCounts[u]++;
            if (visitCounts[u] > k + 5) continue; 

            // --- EXPAND NEIGHBORS ---
            if (adjList.find(u) != adjList.end()) {
                for (const auto& flight : adjList[u]) {
                    
                    // 1. CYCLE CHECK: Don't go back to an airport we are already in
                    bool cycleDetected = false;
                    for (const string& visited : current.visitedNodes) {
                        if (visited == flight.to) { cycleDetected = true; break; }
                    }
                    if (cycleDetected) continue;

                    // 2. TIME CONSTRAINT:
                    // Valid if (First Flight) OR (Flight Departs >= Arrival + Layover)
                    bool timeValid = false;
                    if (current.arrivalTime == -1) {
                        timeValid = true; // First flight of the trip
                    } else {
                        if (flight.depTime >= current.arrivalTime + minLayoverMins) {
                            timeValid = true;
                        }
                    }

                    if (timeValid) {
                        SearchState nextState;
                        nextState.currentCost = current.currentCost + flight.cost;
                        nextState.currentAirport = flight.to;
                        nextState.arrivalTime = flight.arrTime;
                        
                        // Copy History
                        nextState.pathHistory = current.pathHistory;
                        nextState.pathHistory.push_back(flight.id + " (" + u + "->" + flight.to + ")");
                        
                        nextState.visitedNodes = current.visitedNodes;
                        nextState.visitedNodes.push_back(flight.to);

                        pq.push(nextState);
                    }
                }
            }
        }

        return results;
    }
};

// ==========================================
// 3. Helper Functions (Time Formatting)
// ==========================================

string formatTime(int totalMins) {
    int day = totalMins / 1440;
    int hour = (totalMins % 1440) / 60;
    int min = totalMins % 60;
    
    string hStr = (hour < 10 ? "0" : "") + to_string(hour);
    string mStr = (min < 10 ? "0" : "") + to_string(min);
    
    string dayStr = (day > 0) ? "Day " + to_string(day + 1) + ", " : "";
    return dayStr + hStr + ":" + mStr;
}

// ==========================================
// 4. Main Execution
// ==========================================

int main() {
    FlightNetwork network;

    // --- Building the Graph (11 Airports) ---
    // Nodes: JFK, LHR, CDG, FRA, DXB, DOH, SIN, HKG, HND, SYD, BOM
    
    // Time Reference: 600 = 10:00 AM, 1440 = 24 Hours

    // 1. JFK Outbound
    network.addFlight("BA001", "JFK", "LHR", 400, 600, 420);  // Cheap
    network.addFlight("AF022", "JFK", "CDG", 380, 660, 450);  // Cheapest
    network.addFlight("QR700", "JFK", "DOH", 900, 600, 780);  // Expensive Direct

    // 2. LHR Outbound (London)
    network.addFlight("BA100", "LHR", "DXB", 350, 1200, 420); // Connects from BA001
    network.addFlight("SQ300", "LHR", "SIN", 600, 1300, 780); 

    // 3. CDG Outbound (Paris)
    network.addFlight("AF300", "CDG", "DXB", 340, 1300, 400); // Connects from AF022
    network.addFlight("AF400", "CDG", "SIN", 550, 1400, 750); 

    // 4. DOH Outbound (Doha)
    network.addFlight("QR900", "DOH", "SYD", 700, 1600, 850); // Direct to target

    // 5. DXB Outbound (Dubai)
    network.addFlight("EK400", "DXB", "SYD", 650, 2000, 840); // Day 2 flight
    network.addFlight("EK500", "DXB", "SIN", 300, 1800, 420); 

    // 6. SIN Outbound (Singapore)
    network.addFlight("SQ200", "SIN", "SYD", 400, 2400, 450); // Day 2 night flight
    network.addFlight("SQ600", "SIN", "HND", 300, 2300, 360);

    // 7. Other Connections (Noise for the graph)
    network.addFlight("CX100", "HKG", "SYD", 500, 2000, 540);
    network.addFlight("JL050", "HND", "SYD", 550, 2900, 570);
    network.addFlight("LH700", "FRA", "SIN", 600, 1000, 720);
    network.addFlight("AI100", "BOM", "HKG", 300, 1500, 300);

    // --- SETUP ---
    string src = "JFK";
    string dst = "SYD";
    int topK = 3;
    int minLayover = 120; // 2 Hours

    cout << "=================================================\n";
    cout << "  FLIGHT SEARCH: " << src << " -> " << dst << "\n";
    cout << "  Criteria: Cheapest Price (Best First)\n";
    cout << "  Min Layover: " << minLayover << " mins\n";
    cout << "=================================================\n\n";

    // --- EXECUTE ALGORITHM ---
    vector<PathResult> paths = network.getTopKPaths(src, dst, topK, minLayover);

    // --- DISPLAY RESULTS ---
    if (paths.empty()) {
        cout << "No valid paths found.\n";
    } else {
        for (size_t i = 0; i < paths.size(); ++i) {
            cout << "OPTION " << i + 1 << "\n";
            cout << "   Price:       $" << paths[i].totalCost << "\n";
            cout << "   Arrival:     " << formatTime(paths[i].arrivalTime) << "\n";
            cout << "   Itinerary:   ";
            for (const string& seg : paths[i].routeDescription) {
                cout << "[" << seg << "] ";
            }
            cout << "\n-------------------------------------------------\n";
        }
    }

    return 0;
}