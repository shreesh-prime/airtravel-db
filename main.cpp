#include "../include/database.h"
#include <httplib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <map>
#include <cstdlib>

using namespace std;

// Helper function to convert Airline to JSON string
std::string airlineToJSON(const Airline& airline) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << airline.id << ","
        << "\"name\":\"" << airline.name << "\","
        << "\"alias\":\"" << airline.alias << "\","
        << "\"iata\":\"" << airline.iata << "\","
        << "\"icao\":\"" << airline.icao << "\","
        << "\"callsign\":\"" << airline.callsign << "\","
        << "\"country\":\"" << airline.country << "\","
        << "\"active\":\"" << airline.active << "\""
        << "}";
    return oss.str();
}

// Helper function to convert Airport to JSON string
std::string airportToJSON(const Airport& airport) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{"
        << "\"id\":" << airport.id << ","
        << "\"name\":\"" << airport.name << "\","
        << "\"city\":\"" << airport.city << "\","
        << "\"country\":\"" << airport.country << "\","
        << "\"iata\":\"" << airport.iata << "\","
        << "\"icao\":\"" << airport.icao << "\","
        << "\"latitude\":" << airport.latitude << ","
        << "\"longitude\":" << airport.longitude << ","
        << "\"altitude\":" << airport.altitude << ","
        << "\"timezone\":" << airport.timezone << ","
        << "\"dst\":\"" << airport.dst << "\","
        << "\"tz\":\"" << airport.tz << "\","
        << "\"type\":\"" << airport.type << "\","
        << "\"source\":\"" << airport.source << "\""
        << "}";
    return oss.str();
}

// Helper function to escape JSON strings
std::string escapeJSON(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        if (c == '"') oss << "\\\"";
        else if (c == '\\') oss << "\\\\";
        else if (c == '\n') oss << "\\n";
        else if (c == '\r') oss << "\\r";
        else if (c == '\t') oss << "\\t";
        else oss << c;
    }
    return oss.str();
}

// Simple JSON value extractor
std::string getJSONValue(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos += search.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.length()) return "";
    
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    } else {
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != ' ') end++;
        std::string value = json.substr(pos, end - pos);
        // Remove trailing whitespace
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();
        return value;
    }
}

int getJSONInt(const std::string& json, const std::string& key, int default_val = 0) {
    std::string val = getJSONValue(json, key);
    if (val.empty()) return default_val;
    try {
        return std::stoi(val);
    } catch (...) {
        return default_val;
    }
}

double getJSONDouble(const std::string& json, const std::string& key, double default_val = 0.0) {
    std::string val = getJSONValue(json, key);
    if (val.empty()) return default_val;
    try {
        return std::stod(val);
    } catch (...) {
        return default_val;
    }
}

int main() {
    Database db;
    
    // Load data files
    std::cout << "Loading airlines..." << std::endl;
    if (!db.loadAirlines("airlines.dat")) {
        std::cerr << "Error: Could not load airlines.dat" << std::endl;
        return 1;
    }
    
    std::cout << "Loading airports..." << std::endl;
    if (!db.loadAirports("airports.dat")) {
        std::cerr << "Error: Could not load airports.dat" << std::endl;
        return 1;
    }
    
    std::cout << "Loading routes..." << std::endl;
    if (!db.loadRoutes("routes.dat")) {
        std::cerr << "Error: Could not load routes.dat" << std::endl;
        return 1;
    }
    
    std::cout << "Data loaded successfully!" << std::endl;
    
    // Create HTTP server
    httplib::Server svr;
    
    // Enable CORS
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });
    
    // Handle OPTIONS requests for CORS
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        return;
    });
    
    // Root endpoint - Modern interactive web interface
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>OpenFlights Air Travel Database</title>
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Roboto', 'Oxygen', 'Ubuntu', 'Cantarell', sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 50%, #7e8ba3 100%);
            min-height: 100vh;
            padding: 20px;
            color: #333;
        }
        
        .container {
            max-width: 1400px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.2);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #0d47a1 0%, #1565c0 50%, #1976d2 100%);
            color: white;
            padding: 25px 40px;
            text-align: center;
            border-bottom: 5px solid #ffc107;
            box-shadow: 0 4px 12px rgba(0,0,0,0.15);
            position: relative;
        }
        
        .header::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 3px;
            background: linear-gradient(90deg, #ffc107, #ff9800, #ffc107);
        }
        
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 8px;
            text-shadow: 2px 2px 6px rgba(0,0,0,0.4);
            font-weight: 700;
            letter-spacing: 1px;
            text-transform: uppercase;
        }
        
        .header p {
            font-size: 1.1em;
            opacity: 0.9;
        }
        
        .content {
            padding: 30px 40px;
            background: #f5f7fa;
        }
        
        .search-section {
            background: white;
            padding: 30px;
            border-radius: 10px;
            margin-bottom: 25px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.08);
            border: 1px solid #e0e0e0;
        }
        
        .search-group {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin-bottom: 20px;
        }
        
        .input-group {
            display: flex;
            flex-direction: column;
        }
        
        .input-group label {
            font-weight: 600;
            margin-bottom: 8px;
            color: #555;
        }
        
        .input-group input {
            padding: 12px 15px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 16px;
            transition: all 0.3s;
        }
        
        .input-group input:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        
        .btn {
            padding: 12px 30px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        
        .btn-secondary {
            background: #6c757d;
            color: white;
        }
        
        .btn-secondary:hover {
            background: #5a6268;
            transform: translateY(-2px);
        }
        
        .btn-success {
            background: #28a745;
            color: white;
        }
        
        .btn-success:hover {
            background: #218838;
            transform: translateY(-2px);
        }
        
        .btn-info {
            background: #17a2b8;
            color: white;
        }
        
        .btn-info:hover {
            background: #138496;
            transform: translateY(-2px);
        }
        
        .button-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 15px;
            margin-top: 30px;
        }
        
        .button-grid .btn {
            width: 100%;
            padding: 15px;
        }
        
        .results {
            margin-top: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 10px;
            display: none;
        }
        
        .results.show {
            display: block;
        }
        
        .results h3 {
            margin-bottom: 15px;
            color: #667eea;
        }
        
        .result-item {
            background: white;
            padding: 15px;
            margin-bottom: 10px;
            border-radius: 8px;
            border-left: 4px solid #667eea;
        }
        
        .result-item h4 {
            color: #333;
            margin-bottom: 8px;
        }
        
        .result-item p {
            color: #666;
            margin: 5px 0;
        }
        
        .loading {
            text-align: center;
            padding: 20px;
            display: none;
        }
        
        .loading.show {
            display: block;
        }
        
        .spinner {
            border: 4px solid #f3f3f3;
            border-top: 4px solid #667eea;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
            margin: 0 auto;
        }
        
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        
        .error {
            background: #f8d7da;
            color: #721c24;
            padding: 15px;
            border-radius: 8px;
            margin-top: 20px;
            border-left: 4px solid #dc3545;
        }
        
        .info-section {
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
            color: white;
            padding: 20px;
            border-radius: 10px;
            margin-top: 30px;
        }
        
        .info-section h3 {
            margin-bottom: 10px;
        }
        
        pre {
            background: #2d2d2d;
            color: #f8f8f2;
            padding: 15px;
            border-radius: 8px;
            overflow-x: auto;
            font-size: 14px;
        }
        
        .tabs {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
            border-bottom: 2px solid #e0e0e0;
        }
        
        .tab {
            padding: 12px 24px;
            background: none;
            border: none;
            cursor: pointer;
            font-size: 16px;
            font-weight: 600;
            color: #666;
            border-bottom: 3px solid transparent;
            transition: all 0.3s;
        }
        
        .tab.active {
            color: #667eea;
            border-bottom-color: #667eea;
        }
        
        .tab-content {
            display: none;
        }
        
        .tab-content.active {
            display: block;
        }
        
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin: 20px 0;
        }
        
        .stat-card {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
        }
        
        .stat-card h3 {
            font-size: 2em;
            margin-bottom: 5px;
        }
        
        .stat-card p {
            opacity: 0.9;
        }
        
        .btn-warning {
            background: #ffc107;
            color: #333;
        }
        
        .btn-warning:hover {
            background: #e0a800;
            transform: translateY(-2px);
        }
        
        .dropdown-menu {
            position: absolute;
            top: 100%;
            left: 0;
            right: 0;
            background: white;
            border: 1px solid #ddd;
            border-radius: 8px;
            max-height: 300px;
            overflow-y: auto;
            z-index: 1000;
            box-shadow: 0 4px 12px rgba(0,0,0,0.15);
            margin-top: 5px;
        }
        
        .dropdown-item {
            padding: 12px 15px;
            cursor: pointer;
            border-bottom: 1px solid #f0f0f0;
            transition: background 0.2s;
        }
        
        .dropdown-item:hover {
            background: #f5f7fa;
        }
        
        .dropdown-item:last-child {
            border-bottom: none;
        }
        
        .dropdown-item strong {
            color: #1565c0;
            display: block;
            margin-bottom: 3px;
        }
        
        .dropdown-item small {
            color: #666;
            font-size: 0.85em;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>OpenFlights Air Travel Database</h1>
            <p>Explore airlines, airports, and routes worldwide</p>
        </div>
        
        <div class="content">
            <div class="tabs">
                <button class="tab active" onclick="showTab('search', this)">Search</button>
                <button class="tab" onclick="showTab('reports', this)">Reports</button>
                <button class="tab" onclick="showTab('advanced', this)">Route Planning</button>
                <button class="tab" onclick="showTab('analytics', this)">Analytics</button>
                <button class="tab" onclick="showTab('compare', this)">Compare</button>
                <button class="tab" onclick="showTab('help', this)">Help</button>
                <button class="tab" onclick="showTab('contact', this)">Contact</button>
                <button class="tab" onclick="showTab('about', this)">About</button>
            </div>
            
            <div id="search-tab" class="tab-content active">
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px;">
                    <div class="search-section">
                        <h2 style="margin-bottom: 20px; color: #1565c0; border-bottom: 2px solid #1565c0; padding-bottom: 10px;">Search Airlines</h2>
                        <div class="input-group">
                            <label for="airline-iata">Airline IATA Code</label>
                            <div style="position: relative;">
                                <input type="text" id="airline-iata" placeholder="Type or select airline (e.g., AA, UA, DL)" 
                                       maxlength="2" style="text-transform: uppercase; width: 100%;" 
                                       oninput="filterAirlines(this.value)" 
                                       onfocus="showAirlineDropdown()"
                                       onblur="setTimeout(() => hideAirlineDropdown(), 200)">
                                <div id="airline-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                        <button class="btn btn-primary" onclick="searchAirline()" style="width: 100%; margin-top: 10px;">Search Airline</button>
                    </div>
                    
                    <div class="search-section">
                        <h2 style="margin-bottom: 20px; color: #1565c0; border-bottom: 2px solid #1565c0; padding-bottom: 10px;">Search Airports</h2>
                        <div class="input-group">
                            <label for="airport-iata">Airport IATA Code</label>
                            <div style="position: relative;">
                                <input type="text" id="airport-iata" placeholder="Type airport code, city, or name (e.g., SFO, Los Angeles, JFK)" 
                                       style="width: 100%;" 
                                       oninput="filterAirports(this.value)" 
                                       onfocus="showAirportDropdown()"
                                       onblur="setTimeout(() => hideAirportDropdown(), 200)">
                                <div id="airport-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                        <button class="btn btn-primary" onclick="searchAirport()" style="width: 100%; margin-top: 10px;">Search Airport</button>
                    </div>
                </div>
                <div style="margin-top: 20px; text-align: center;">
                    <button class="btn btn-info" onclick="getStudentInfo()">Student Info</button>
                </div>
            </div>
            
            <div id="reports-tab" class="tab-content">
                <h2 style="margin-bottom: 20px; color: #1565c0; border-bottom: 2px solid #1565c0; padding-bottom: 10px;">Reports &amp; Analytics</h2>
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 20px;">
                    <div class="search-section">
                        <h3 style="margin-bottom: 15px; color: #1565c0;">Airline Routes Report</h3>
                        <div class="input-group">
                            <label for="report-airline-iata">Select Airline</label>
                            <div style="position: relative;">
                                <input type="text" id="report-airline-iata" placeholder="Type or select airline (e.g., AA, UA)" 
                                       maxlength="2" style="text-transform: uppercase; width: 100%;" 
                                       oninput="filterAirlinesForReport(this.value)" 
                                       onfocus="showAirlineReportDropdown()"
                                       onblur="setTimeout(() => hideAirlineReportDropdown(), 200)">
                                <div id="report-airline-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                        <button class="btn btn-info" onclick="getAirlineRoutesFromInput()" style="width: 100%; margin-top: 10px;">Get Airline Routes</button>
                    </div>
                    
                    <div class="search-section">
                        <h3 style="margin-bottom: 15px; color: #1565c0;">Airport Airlines Report</h3>
                        <div class="input-group">
                            <label for="report-airport-iata">Select Airport</label>
                            <div style="position: relative;">
                                <input type="text" id="report-airport-iata" placeholder="Type airport code, city, or name (e.g., SFO, Los Angeles, JFK)" 
                                       style="width: 100%;" 
                                       oninput="filterAirportsForReport(this.value)" 
                                       onfocus="showAirportReportDropdown()"
                                       onblur="setTimeout(() => hideAirportReportDropdown(), 200)">
                                <div id="report-airport-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                        <button class="btn btn-info" onclick="getAirportAirlinesFromInput()" style="width: 100%; margin-top: 10px;">Get Airport Airlines</button>
                    </div>
                </div>
                <div class="button-grid">
                    <button class="btn btn-success" onclick="getAllAirlines()">View All Airlines</button>
                    <button class="btn btn-success" onclick="getAllAirports()">View All Airports</button>
                </div>
            </div>
            
            <div id="advanced-tab" class="tab-content">
                <h2 style="margin-bottom: 20px; color: #667eea;">Route Planning & Maps</h2>
                <div class="search-section" style="margin-bottom: 20px;">
                    <div class="search-group">
                        <div class="input-group">
                            <label for="route-source">From Airport</label>
                            <div style="position: relative;">
                                <input type="text" id="route-source" placeholder="e.g., SFO or Los Angeles" 
                                       style="width: 100%;" 
                                       oninput="filterAirportsForRoute('source', this.value)" 
                                       onfocus="showRouteSourceDropdown()"
                                       onblur="setTimeout(() => hideRouteSourceDropdown(), 200)">
                                <div id="route-source-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                        <div class="input-group">
                            <label for="route-dest">To Airport</label>
                            <div style="position: relative;">
                                <input type="text" id="route-dest" placeholder="e.g., JFK or New York" 
                                       style="width: 100%;" 
                                       oninput="filterAirportsForRoute('dest', this.value)" 
                                       onfocus="showRouteDestDropdown()"
                                       onblur="setTimeout(() => hideRouteDestDropdown(), 200)">
                                <div id="route-dest-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                    </div>
                    <div style="display: flex; gap: 10px; flex-wrap: wrap;">
                        <button class="btn btn-primary" onclick="findDirectRoutes()">Find Direct Routes</button>
                        <button class="btn btn-info" onclick="promptOneHop()">Find One-Hop Routes</button>
                    </div>
                </div>
                <div id="route-map-container" style="display: none; margin-top: 20px;">
                    <div id="route-map" style="height: 500px; border-radius: 10px; border: 2px solid #e0e0e0;"></div>
                </div>
                <div class="button-grid" style="margin-top: 20px;">
                    <button class="btn btn-warning" onclick="getSourceCode()">View Source Code</button>
                    <button class="btn btn-secondary" onclick="showStats()">Database Statistics</button>
                </div>
            </div>
            
            <div id="analytics-tab" class="tab-content">
                <h2 style="margin-bottom: 20px; color: #1565c0; border-bottom: 2px solid #1565c0; padding-bottom: 10px;">Analytics Dashboard</h2>
                <div class="button-grid">
                    <button class="btn btn-primary" onclick="showRouteStatistics()">Route Statistics</button>
                    <button class="btn btn-info" onclick="showTopAirlines()">Top Airlines by Routes</button>
                    <button class="btn btn-info" onclick="showTopAirports()">Top Airports by Traffic</button>
                    <button class="btn btn-success" onclick="showGeographicStats()">Geographic Statistics</button>
                    <button class="btn btn-warning" onclick="exportData('csv')">Export Data (CSV)</button>
                    <button class="btn btn-warning" onclick="exportData('json')">Export Data (JSON)</button>
                </div>
            </div>
            
            <div id="compare-tab" class="tab-content">
                <h2 style="margin-bottom: 20px; color: #1565c0; border-bottom: 2px solid #1565c0; padding-bottom: 10px;">Compare Airlines & Airports</h2>
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 20px;">
                    <div class="search-section">
                        <h3 style="margin-bottom: 15px; color: #1565c0;">Compare Airlines</h3>
                        <div class="input-group">
                            <label>Airline 1</label>
                            <div style="position: relative;">
                                <input type="text" id="compare-airline-1" placeholder="Type airline code (e.g., AA)" 
                                       maxlength="2" style="text-transform: uppercase; width: 100%;" 
                                       oninput="filterAirlinesForCompare('1', this.value)">
                                <div id="compare-airline-1-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                        <div class="input-group" style="margin-top: 10px;">
                            <label>Airline 2</label>
                            <div style="position: relative;">
                                <input type="text" id="compare-airline-2" placeholder="Type airline code (e.g., UA)" 
                                       maxlength="2" style="text-transform: uppercase; width: 100%;" 
                                       oninput="filterAirlinesForCompare('2', this.value)">
                                <div id="compare-airline-2-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                        <button class="btn btn-primary" onclick="compareAirlines()" style="width: 100%; margin-top: 10px;">Compare Airlines</button>
                    </div>
                    
                    <div class="search-section">
                        <h3 style="margin-bottom: 15px; color: #1565c0;">Compare Airports</h3>
                        <div class="input-group">
                            <label>Airport 1</label>
                            <div style="position: relative;">
                                <input type="text" id="compare-airport-1" placeholder="Type airport code, city, or name" 
                                       style="width: 100%;" 
                                       oninput="filterAirportsForCompare('1', this.value)"
                                       onfocus="showCompareAirportDropdown('1')"
                                       onblur="setTimeout(() => hideCompareAirportDropdown('1'), 200)">
                                <div id="compare-airport-1-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                        <div class="input-group" style="margin-top: 10px;">
                            <label>Airport 2</label>
                            <div style="position: relative;">
                                <input type="text" id="compare-airport-2" placeholder="Type airport code, city, or name" 
                                       style="width: 100%;" 
                                       oninput="filterAirportsForCompare('2', this.value)"
                                       onfocus="showCompareAirportDropdown('2')"
                                       onblur="setTimeout(() => hideCompareAirportDropdown('2'), 200)">
                                <div id="compare-airport-2-dropdown" class="dropdown-menu" style="display: none;"></div>
                            </div>
                        </div>
                        <button class="btn btn-primary" onclick="compareAirports()" style="width: 100%; margin-top: 10px;">Compare Airports</button>
                    </div>
                </div>
            </div>
            
            <div id="help-tab" class="tab-content">
                <h2 style="margin-bottom: 20px; color: #1565c0; border-bottom: 2px solid #1565c0; padding-bottom: 10px;">Help & Documentation</h2>
                <div class="info-section">
                    <h3>Getting Started</h3>
                    <p>Welcome to the OpenFlights Air Travel Database! This application provides comprehensive access to airline, airport, and route data.</p>
                    
                    <h3 style="margin-top: 25px;">Search Tab</h3>
                    <ul style="margin-left: 20px; margin-top: 10px;">
                        <li><strong>Search Airlines:</strong> Enter an airline IATA code (e.g., AA for American Airlines) or type the airline name</li>
                        <li><strong>Search Airports:</strong> Enter an airport IATA code (e.g., SFO), city name (e.g., Los Angeles), or airport name</li>
                        <li>Use the autocomplete dropdowns to quickly find what you're looking for</li>
                    </ul>
                    
                    <h3 style="margin-top: 25px;">Reports Tab</h3>
                    <ul style="margin-left: 20px; margin-top: 10px;">
                        <li><strong>Airline Routes Report:</strong> See all airports served by a specific airline, ordered by route count</li>
                        <li><strong>Airport Airlines Report:</strong> See all airlines serving a specific airport, ordered by route count</li>
                        <li><strong>View All:</strong> Browse all airlines or airports sorted by IATA code</li>
                    </ul>
                    
                    <h3 style="margin-top: 25px;">Route Planning Tab</h3>
                    <ul style="margin-left: 20px; margin-top: 10px;">
                        <li><strong>Find Direct Routes:</strong> Search for direct flights between two airports with interactive map visualization</li>
                        <li><strong>Find One-Hop Routes:</strong> Find connecting flights with one intermediate stop, sorted by total distance</li>
                        <li>Each route is displayed on an interactive map with color-coded lines for different airlines</li>
                    </ul>
                    
                    <h3 style="margin-top: 25px;">Analytics Tab</h3>
                    <ul style="margin-left: 20px; margin-top: 10px;">
                        <li>View comprehensive statistics about routes, airlines, and airports</li>
                        <li>Export data in CSV or JSON format</li>
                        <li>Explore geographic distribution of airports and routes</li>
                    </ul>
                    
                    <h3 style="margin-top: 25px;">Compare Tab</h3>
                    <ul style="margin-left: 20px; margin-top: 10px;">
                        <li>Compare two airlines side-by-side to see route counts, countries, and other details</li>
                        <li>Compare two airports to see traffic, airlines, and geographic information</li>
                    </ul>
                    
                    <h3 style="margin-top: 25px;">Tips</h3>
                    <ul style="margin-left: 20px; margin-top: 10px;">
                        <li>You can search airports by city name (e.g., "Los Angeles" will show LAX)</li>
                        <li>Press Enter in any search field to quickly submit</li>
                        <li>Click on map markers and route lines for detailed information</li>
                        <li>All data is from the OpenFlights database (openflights.org)</li>
                    </ul>
                </div>
            </div>
            
            <div id="contact-tab" class="tab-content">
                <h2 style="margin-bottom: 20px; color: #1565c0; border-bottom: 2px solid #1565c0; padding-bottom: 10px;">Contact Us</h2>
                <div class="search-section" style="max-width: 600px; margin: 0 auto;">
                    <form id="contact-form" onsubmit="submitContactForm(event)">
                        <div class="input-group">
                            <label for="contact-name">Your Name *</label>
                            <input type="text" id="contact-name" required style="width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px;">
                        </div>
                        <div class="input-group" style="margin-top: 15px;">
                            <label for="contact-email">Your Email *</label>
                            <input type="email" id="contact-email" required style="width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px;">
                        </div>
                        <div class="input-group" style="margin-top: 15px;">
                            <label for="contact-subject">Subject *</label>
                            <input type="text" id="contact-subject" required style="width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px;">
                        </div>
                        <div class="input-group" style="margin-top: 15px;">
                            <label for="contact-message">Message *</label>
                            <textarea id="contact-message" required rows="6" style="width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; font-family: inherit;"></textarea>
                        </div>
                        <button type="submit" class="btn btn-primary" style="width: 100%; margin-top: 15px;">Send Message</button>
                    </form>
                    <div id="contact-response" style="margin-top: 20px; display: none;"></div>
                </div>
            </div>
            
            <div id="about-tab" class="tab-content">
                <h2 style="margin-bottom: 20px; color: #1565c0; border-bottom: 2px solid #1565c0; padding-bottom: 10px;">About This Application</h2>
                <div class="info-section">
                    <h3>OpenFlights Air Travel Database</h3>
                    <p>This comprehensive web application provides full access to airline, airport, and route data from the OpenFlights database. Built using C++ with a modern web interface, it demonstrates advanced data structures and algorithms.</p>
                    
                    <h3 style="margin-top: 25px;">Project Details</h3>
                    <p><strong>Course:</strong> CIS 22C Honors Cohort - Capstone Project</p>
                    <p><strong>Technology Stack:</strong> C++ (Backend), HTML/CSS/JavaScript (Frontend), cpp-httplib (HTTP Server)</p>
                    <p><strong>Data Source:</strong> OpenFlights.org - Copyright-free aviation data</p>
                    
                    <h3 style="margin-top: 25px;">Key Features</h3>
                    <ul style="margin-left: 20px; margin-top: 10px;">
                        <li>✅ Search airlines and airports by IATA code, name, or city</li>
                        <li>✅ Comprehensive route reports and analytics</li>
                        <li>✅ Interactive map visualization for route planning</li>
                        <li>✅ Direct and one-hop route finder with distance calculations</li>
                        <li>✅ Real-time data access via RESTful API</li>
                        <li>✅ Export functionality (CSV/JSON)</li>
                        <li>✅ Comparison tools for airlines and airports</li>
                        <li>✅ Professional, responsive user interface</li>
                    </ul>
                    
                    <h3 style="margin-top: 25px;">Data Structures Used</h3>
                    <ul style="margin-left: 20px; margin-top: 10px;">
                        <li><strong>Hash Tables (unordered_map):</strong> O(1) lookup by IATA code for airlines and airports</li>
                        <li><strong>Ordered Maps (map):</strong> Sorted collections for reports ordered by IATA code</li>
                        <li><strong>Vectors:</strong> Storage for routes and collections</li>
                        <li><strong>Indexes:</strong> Multiple hash-based indexes for efficient route queries</li>
                    </ul>
                    
                    <h3 style="margin-top: 25px;">Student Information</h3>
                    <div id="student-info-display" style="background: #f5f7fa; padding: 15px; border-radius: 8px; margin-top: 10px; color: #333; border: 1px solid #ddd;"></div>
                    
                    <h3 style="margin-top: 25px;">Acknowledgments</h3>
                    <p>This project was developed using Vibe Coding methodology, collaborating with AI tools to create a fully functional web application. Special thanks to OpenFlights.org for providing the comprehensive aviation database.</p>
                </div>
            </div>
                </div>
            </div>
            
            <div class="loading" id="loading">
                <div class="spinner"></div>
                <p style="margin-top: 10px;">Loading data...</p>
            </div>
            
            <div class="results" id="results"></div>
        </div>
    </div>
    
    <script>
        function showTab(tabName, element) {
            // Hide all tabs
            document.querySelectorAll('.tab-content').forEach(tab => {
                tab.classList.remove('active');
            });
            document.querySelectorAll('.tab').forEach(tab => {
                tab.classList.remove('active');
            });
            
            // Show selected tab
            const tabContent = document.getElementById(tabName + '-tab');
            if (tabContent) {
                tabContent.classList.add('active');
            }
            
            // Activate the clicked tab button
            if (element) {
                element.classList.add('active');
            } else if (typeof event !== 'undefined' && event.target) {
                event.target.classList.add('active');
            } else {
                // Fallback: find tab button by text
                document.querySelectorAll('.tab').forEach(btn => {
                    if (btn.textContent.trim().toLowerCase() === tabName.toLowerCase()) {
                        btn.classList.add('active');
                    }
                });
            }
        }
        
        function showLoading() {
            document.getElementById('loading').classList.add('show');
            document.getElementById('results').classList.remove('show');
        }
        
        function hideLoading() {
            document.getElementById('loading').classList.remove('show');
        }
        
        function showResults(html) {
            hideLoading();
            const resultsDiv = document.getElementById('results');
            resultsDiv.innerHTML = html;
            resultsDiv.classList.add('show');
            resultsDiv.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        }
        
        function showError(message) {
            hideLoading();
            showResults('<div class="error"><strong>Error:</strong> ' + message + '</div>');
        }
        
        function formatJSON(data) {
            return JSON.stringify(data, null, 2);
        }
        
        function formatAirline(airline) {
            return '<div class="result-item"><h4>' + airline.name + ' (' + airline.iata + ')</h4>' +
                   '<p><strong>ICAO:</strong> ' + (airline.icao || 'N/A') + '</p>' +
                   '<p><strong>Country:</strong> ' + (airline.country || 'N/A') + '</p>' +
                   '<p><strong>Callsign:</strong> ' + (airline.callsign || 'N/A') + '</p>' +
                   '<p><strong>Status:</strong> ' + (airline.active === 'Y' ? 'Active' : 'Inactive') + '</p></div>';
        }
        
        function formatAirport(airport) {
            return '<div class="result-item"><h4>' + airport.name + ' (' + airport.iata + ')</h4>' +
                   '<p><strong>City:</strong> ' + (airport.city || 'N/A') + '</p>' +
                   '<p><strong>Country:</strong> ' + (airport.country || 'N/A') + '</p>' +
                   '<p><strong>ICAO:</strong> ' + (airport.icao || 'N/A') + '</p>' +
                   '<p><strong>Coordinates:</strong> ' + airport.latitude + ', ' + airport.longitude + '</p>' +
                   '<p><strong>Altitude:</strong> ' + airport.altitude + ' ft</p></div>';
        }
        
        async function searchAirline() {
            const iata = document.getElementById('airline-iata').value.trim().toUpperCase();
            if (!iata) {
                showError('Please enter an airline IATA code');
                return;
            }
            
            showLoading();
            try {
                const response = await fetch('/airline/' + iata);
                const data = await response.json();
                
                if (data.error) {
                    showError(data.error);
                } else {
                    showResults('<h3>Airline Information</h3>' + formatAirline(data));
                }
            } catch (error) {
                showError('Failed to fetch airline data: ' + error.message);
            }
        }
        
        async function searchAirport() {
            let iata = document.getElementById('airport-iata').value.trim();
            // Extract IATA code if it's a 3-letter code, otherwise use first 3 characters
            if (iata.length >= 3 && /^[A-Z]{3}$/i.test(iata.substring(0, 3))) {
                iata = iata.substring(0, 3).toUpperCase();
            } else if (iata.length > 0) {
                // If it's not a clear IATA code, try to extract it
                // The dropdown should have set it to IATA, but if user typed city name, extract first 3 chars
                iata = iata.substring(0, 3).toUpperCase();
            }
            
            if (!iata) {
                showError('Please enter an airport code, city, or name');
                return;
            }
            
            showLoading();
            try {
                const response = await fetch('/airport/' + iata);
                const data = await response.json();
                
                if (data.error) {
                    showError(data.error);
                } else {
                    showResults('<h3>Airport Information</h3>' + formatAirport(data));
                }
            } catch (error) {
                showError('Failed to fetch airport data: ' + error.message);
            }
        }
        
        async function getAllAirlines(page = 1) {
            showLoading();
            try {
                const response = await fetch('/airlines/list?page=' + page + '&size=100');
                if (!response.ok) {
                    throw new Error('Failed to fetch airlines: ' + response.status);
                }
                
                const data = await response.json();
                
                if (!data.airlines || !Array.isArray(data.airlines)) {
                    showError('Invalid data format received');
                    return;
                }
                
                let html = '<h3>All Airlines (Sorted by IATA Code)</h3>';
                html += '<p style="margin-bottom: 15px;"><strong>Total Airlines:</strong> ' + data.total + '</p>';
                html += '<p style="margin-bottom: 15px; color: #666;">Page ' + data.page + ' of ' + Math.ceil(data.total / data.pageSize) + '</p>';
                
                for (let i = 0; i < data.airlines.length; i++) {
                    html += formatAirline(data.airlines[i]);
                }
                
                // Pagination controls
                if (data.total > data.pageSize) {
                    html += '<div style="margin-top: 20px; display: flex; gap: 10px; align-items: center;">';
                    if (data.page > 1) {
                        html += '<button class="btn btn-primary" onclick="getAllAirlines(' + (data.page - 1) + ')">Previous</button>';
                    }
                    html += '<span>Page ' + data.page + ' of ' + Math.ceil(data.total / data.pageSize) + '</span>';
                    if (data.page * data.pageSize < data.total) {
                        html += '<button class="btn btn-primary" onclick="getAllAirlines(' + (data.page + 1) + ')">Next</button>';
                    }
                    html += '</div>';
                }
                
                showResults(html);
            } catch (error) {
                showError('Failed to fetch airlines: ' + error.message);
            }
        }
        
        async function getAllAirports(page = 1) {
            showLoading();
            try {
                const response = await fetch('/airports/list?page=' + page + '&size=100');
                if (!response.ok) {
                    throw new Error('Failed to fetch airports: ' + response.status);
                }
                
                const data = await response.json();
                
                if (!data.airports || !Array.isArray(data.airports)) {
                    showError('Invalid data format received');
                    return;
                }
                
                let html = '<h3>All Airports (Sorted by IATA Code)</h3>';
                html += '<p style="margin-bottom: 15px;"><strong>Total Airports:</strong> ' + data.total + '</p>';
                html += '<p style="margin-bottom: 15px; color: #666;">Page ' + data.page + ' of ' + Math.ceil(data.total / data.pageSize) + '</p>';
                
                for (let i = 0; i < data.airports.length; i++) {
                    html += formatAirport(data.airports[i]);
                }
                
                // Pagination controls
                if (data.total > data.pageSize) {
                    html += '<div style="margin-top: 20px; display: flex; gap: 10px; align-items: center;">';
                    if (data.page > 1) {
                        html += '<button class="btn btn-primary" onclick="getAllAirports(' + (data.page - 1) + ')">Previous</button>';
                    }
                    html += '<span>Page ' + data.page + ' of ' + Math.ceil(data.total / data.pageSize) + '</span>';
                    if (data.page * data.pageSize < data.total) {
                        html += '<button class="btn btn-primary" onclick="getAllAirports(' + (data.page + 1) + ')">Next</button>';
                    }
                    html += '</div>';
                }
                
                showResults(html);
            } catch (error) {
                showError('Failed to fetch airports: ' + error.message);
            }
        }
        
        function promptAirlineRoutes() {
            const iata = prompt('Enter Airline IATA Code (e.g., AA, UA):');
            if (iata) {
                getAirlineRoutes(iata.toUpperCase());
            }
        }
        
        function getAirlineRoutesFromInput() {
            const iata = document.getElementById('report-airline-iata').value.trim().toUpperCase();
            if (!iata) {
                showError('Please enter or select an airline code');
                return;
            }
            getAirlineRoutes(iata);
        }
        
        async function getAirlineRoutes(iata) {
            showLoading();
            try {
                const response = await fetch('/airline/' + iata + '/routes');
                const data = await response.json();
                
                let html = '<h3>Airports Served by ' + iata + ' (Ordered by Route Count)</h3>';
                html += '<p style="margin-bottom: 15px;"><strong>Total Airports:</strong> ' + data.length + '</p>';
                
                const displayCount = Math.min(30, data.length);
                for (let i = 0; i < displayCount; i++) {
                    html += '<div class="result-item"><h4>' + data[i].airport.name + ' (' + data[i].airport.iata + ')</h4>' +
                            '<p><strong>City:</strong> ' + (data[i].airport.city || 'N/A') + '</p>' +
                            '<p><strong>Country:</strong> ' + (data[i].airport.country || 'N/A') + '</p>' +
                            '<p><strong style="color: #667eea;">Route Count: ' + data[i].route_count + '</strong></p></div>';
                }
                
                if (data.length > 30) {
                    html += '<p style="margin-top: 15px; color: #666;">Showing top 30 of ' + data.length + ' airports.</p>';
                }
                
                showResults(html);
            } catch (error) {
                showError('Failed to fetch airline routes: ' + error.message);
            }
        }
        
        function promptAirportAirlines() {
            const iata = prompt('Enter Airport IATA Code (e.g., SFO, JFK):');
            if (iata) {
                getAirportAirlines(iata.toUpperCase());
            }
        }
        
        function getAirportAirlinesFromInput() {
            const iata = document.getElementById('report-airport-iata').value.trim().toUpperCase();
            if (!iata) {
                showError('Please enter or select an airport code');
                return;
            }
            getAirportAirlines(iata);
        }
        
        async function getAirportAirlines(iata) {
            showLoading();
            try {
                const response = await fetch('/airport/' + iata + '/airlines');
                const data = await response.json();
                
                let html = '<h3>Airlines Serving ' + iata + ' (Ordered by Route Count)</h3>';
                html += '<p style="margin-bottom: 15px;"><strong>Total Airlines:</strong> ' + data.length + '</p>';
                
                const displayCount = Math.min(30, data.length);
                for (let i = 0; i < displayCount; i++) {
                    html += '<div class="result-item"><h4>' + data[i].airline.name + ' (' + data[i].airline.iata + ')</h4>' +
                            '<p><strong>Country:</strong> ' + (data[i].airline.country || 'N/A') + '</p>' +
                            '<p><strong style="color: #667eea;">Route Count: ' + data[i].route_count + '</strong></p></div>';
                }
                
                if (data.length > 30) {
                    html += '<p style="margin-top: 15px; color: #666;">Showing top 30 of ' + data.length + ' airlines.</p>';
                }
                
                showResults(html);
            } catch (error) {
                showError('Failed to fetch airport airlines: ' + error.message);
            }
        }
        
        async function getStudentInfo() {
            showLoading();
            try {
                const response = await fetch('/student');
                const data = await response.json();
                showResults('<div class="info-section"><h3>Student Information</h3><p>' + data.info + '</p></div>');
            } catch (error) {
                showError('Failed to fetch student info: ' + error.message);
            }
        }
        
        async function getSourceCode() {
            showLoading();
            try {
                const response = await fetch('/code');
                const data = await response.json();
                showResults('<h3>Source Code</h3><pre>' + data.code + '</pre>');
            } catch (error) {
                showError('Failed to fetch source code: ' + error.message);
            }
        }
        
        let routeMap = null;
        let routeMarkers = [];
        let routeLines = [];
        
        function initMap() {
            if (routeMap) {
                routeMap.remove();
            }
            routeMap = L.map('route-map').setView([37.7749, -122.4194], 3);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '© OpenStreetMap contributors'
            }).addTo(routeMap);
        }
        
        function clearMap() {
            if (routeMap) {
                routeMarkers.forEach(m => routeMap.removeLayer(m));
                routeLines.forEach(l => routeMap.removeLayer(l));
                routeMarkers = [];
                routeLines = [];
            }
        }
        
        async function findDirectRoutes() {
            // Extract IATA code from input (dropdown should set it to IATA, but handle both cases)
            let source = document.getElementById('route-source').value.trim();
            let dest = document.getElementById('route-dest').value.trim();
            
            // Extract IATA code (first 3 uppercase letters)
            if (source.length >= 3) {
                source = source.substring(0, 3).toUpperCase();
            } else {
                showError('Please enter a valid source airport');
                return;
            }
            
            if (dest.length >= 3) {
                dest = dest.substring(0, 3).toUpperCase();
            } else {
                showError('Please enter a valid destination airport');
                return;
            }
            
            if (!source || !dest) {
                showError('Please enter both source and destination airports');
                return;
            }
            
            showLoading();
            try {
                const response = await fetch('/direct/' + source + '/' + dest);
                const data = await response.json();
                
                if (data.error) {
                    showError(data.error);
                    return;
                }
                
                if (!data.source || !data.dest) {
                    showError('Airport not found');
                    return;
                }
                
                // Show map container
                document.getElementById('route-map-container').style.display = 'block';
                
                // Initialize map if needed
                if (!routeMap) {
                    initMap();
                } else {
                    clearMap();
                }
                
                // Center map on route
                const centerLat = (data.source.latitude + data.dest.latitude) / 2;
                const centerLon = (data.source.longitude + data.dest.longitude) / 2;
                routeMap.setView([centerLat, centerLon], 4);
                
                // Add source airport marker
                const sourceMarker = L.marker([data.source.latitude, data.source.longitude])
                    .addTo(routeMap)
                    .bindPopup('<b>' + data.source.name + '</b><br>' + data.source.iata + '<br>' + data.source.city);
                routeMarkers.push(sourceMarker);
                
                // Add destination airport marker
                const destMarker = L.marker([data.dest.latitude, data.dest.longitude])
                    .addTo(routeMap)
                    .bindPopup('<b>' + data.dest.name + '</b><br>' + data.dest.iata + '<br>' + data.dest.city);
                routeMarkers.push(destMarker);
                
                // Draw route lines for each airline
                const colors = ['#FF0000', '#0000FF', '#00FF00', '#FF00FF', '#FFFF00', '#00FFFF', '#FFA500', '#800080'];
                let html = '<h3>Direct Routes from ' + source + ' to ' + dest + '</h3>';
                html += '<p style="margin-bottom: 15px;"><strong>Total Direct Routes Found:</strong> ' + data.routes.length + '</p>';
                
                if (data.routes.length === 0) {
                    html += '<p>No direct routes found. Try one-hop routes instead.</p>';
                } else {
                    // Draw lines for each route
                    for (let i = 0; i < data.routes.length; i++) {
                        const route = data.routes[i];
                        const color = colors[i % colors.length];
                        
                        // Draw route line
                        const line = L.polyline(
                            [[data.source.latitude, data.source.longitude], 
                             [data.dest.latitude, data.dest.longitude]],
                            {color: color, weight: 3, opacity: 0.7}
                        ).addTo(routeMap);
                        routeLines.push(line);
                        
                        // Add popup to line
                        line.bindPopup('<b>' + route.airline_name + '</b><br>' + 
                                     'Distance: ' + route.distance.toFixed(2) + ' miles<br>' +
                                     'Stops: ' + route.stops);
                        
                        html += '<div class="result-item" style="border-left-color: ' + color + ';"><h4>' + 
                                route.airline_name + ' (' + route.airline_iata + ')</h4>' +
                                '<p><strong>Distance:</strong> ' + route.distance.toFixed(2) + ' miles</p>' +
                                '<p><strong>Stops:</strong> ' + route.stops + '</p>' +
                                '<p><strong>Route:</strong> ' + source + ' → ' + dest + '</p></div>';
                    }
                }
                
                showResults(html);
                
                // Fit map to show both airports
                const group = new L.featureGroup(routeMarkers);
                routeMap.fitBounds(group.getBounds().pad(0.1));
            } catch (error) {
                showError('Failed to fetch direct routes: ' + error.message);
            }
        }
        
        function promptOneHop() {
            let source = document.getElementById('route-source').value.trim();
            let dest = document.getElementById('route-dest').value.trim();
            
            // Extract IATA codes
            if (source.length >= 3) {
                source = source.substring(0, 3).toUpperCase();
            } else {
                source = prompt('Enter Source Airport (e.g., SFO or Los Angeles):');
                if (source) source = source.substring(0, 3).toUpperCase();
            }
            
            if (dest.length >= 3) {
                dest = dest.substring(0, 3).toUpperCase();
            } else {
                dest = prompt('Enter Destination Airport (e.g., JFK or New York):');
                if (dest) dest = dest.substring(0, 3).toUpperCase();
            }
            
            if (source && dest) {
                getOneHopRoutes(source, dest);
            }
        }
        
        async function getOneHopRoutes(source, dest) {
            showLoading();
            try {
                const response = await fetch('/onehop/' + source + '/' + dest);
                const data = await response.json();
                
                if (data.error) {
                    showError(data.error);
                    return;
                }
                
                // Use airport data from response
                const sourceAirport = data.source;
                const destAirport = data.dest;
                
                if (!sourceAirport || !destAirport) {
                    showError('Could not load airport data for mapping');
                    return;
                }
                
                // Show map container
                document.getElementById('route-map-container').style.display = 'block';
                
                // Initialize map if needed
                if (!routeMap) {
                    initMap();
                } else {
                    clearMap();
                }
                
                // Center map on route
                const centerLat = (sourceAirport.latitude + destAirport.latitude) / 2;
                const centerLon = (sourceAirport.longitude + destAirport.longitude) / 2;
                routeMap.setView([centerLat, centerLon], 4);
                
                // Add source airport marker
                const sourceMarker = L.marker([sourceAirport.latitude, sourceAirport.longitude])
                    .addTo(routeMap)
                    .bindPopup('<b>' + sourceAirport.name + '</b><br>' + source + '<br>' + sourceAirport.city);
                routeMarkers.push(sourceMarker);
                
                // Add destination airport marker
                const destMarker = L.marker([destAirport.latitude, destAirport.longitude])
                    .addTo(routeMap)
                    .bindPopup('<b>' + destAirport.name + '</b><br>' + dest + '<br>' + destAirport.city);
                routeMarkers.push(destMarker);
                
                const colors = ['#FF0000', '#0000FF', '#00FF00', '#FF00FF', '#FFFF00', '#00FFFF', '#FFA500', '#800080'];
                let html = '<h3>One-Hop Routes from ' + source + ' to ' + dest + '</h3>';
                html += '<p style="margin-bottom: 15px;"><strong>Total Routes Found:</strong> ' + data.routes.length + '</p>';
                
                if (data.routes.length === 0) {
                    html += '<p>No one-hop routes found.</p>';
                } else {
                    // Fetch intermediate airport data and draw routes
                    for (let i = 0; i < data.routes.length; i++) {
                        const route = data.routes[i];
                        const color = colors[i % colors.length];
                        
                        // Fetch intermediate airport
                        try {
                            const intermediateAirport = await fetch('/airport/' + route.intermediate).then(r => r.json());
                            
                            if (!intermediateAirport.error) {
                                // Add intermediate marker
                                const intMarker = L.marker([intermediateAirport.latitude, intermediateAirport.longitude], {
                                    icon: L.icon({
                                        iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-' + 
                                                (i % 8 + 1) + '.png',
                                        iconSize: [25, 41],
                                        iconAnchor: [12, 41]
                                    })
                                })
                                .addTo(routeMap)
                                .bindPopup('<b>' + intermediateAirport.name + '</b><br>' + route.intermediate + '<br>Intermediate Stop');
                                routeMarkers.push(intMarker);
                                
                                // Draw two-segment route
                                const line1 = L.polyline(
                                    [[sourceAirport.latitude, sourceAirport.longitude], 
                                     [intermediateAirport.latitude, intermediateAirport.longitude]],
                                    {color: color, weight: 3, opacity: 0.7, dashArray: '5, 5'}
                                ).addTo(routeMap);
                                routeLines.push(line1);
                                
                                const line2 = L.polyline(
                                    [[intermediateAirport.latitude, intermediateAirport.longitude], 
                                     [destAirport.latitude, destAirport.longitude]],
                                    {color: color, weight: 3, opacity: 0.7, dashArray: '5, 5'}
                                ).addTo(routeMap);
                                routeLines.push(line2);
                                
                                line1.bindPopup('<b>' + route.airline + '</b><br>' + source + ' → ' + route.intermediate);
                                line2.bindPopup('<b>' + route.airline + '</b><br>' + route.intermediate + ' → ' + dest);
                            }
                        } catch (e) {
                            console.log('Could not load intermediate airport:', route.intermediate);
                        }
                        
                        html += '<div class="result-item" style="border-left-color: ' + color + ';"><h4>Route ' + (i + 1) + '</h4>' +
                                '<p><strong>Path:</strong> ' + source + ' → ' + route.intermediate + ' → ' + dest + '</p>' +
                                '<p><strong>Airline:</strong> ' + route.airline + '</p>' +
                                '<p><strong>Distance:</strong> ' + route.distance.toFixed(2) + ' miles</p></div>';
                    }
                }
                
                showResults(html);
                
                // Fit map to show all airports
                if (routeMarkers.length > 0) {
                    const group = new L.featureGroup(routeMarkers);
                    routeMap.fitBounds(group.getBounds().pad(0.15));
                }
            } catch (error) {
                showError('Failed to fetch one-hop routes: ' + error.message);
            }
        }
        
        async function showStats() {
            showLoading();
            try {
                const airlinesRes = await fetch('/airlines');
                const airlines = await airlinesRes.json();
                
                // For airports, handle potential large response
                let airportCount = 0;
                try {
                    const airportsRes = await fetch('/airports');
                    const airports = await airportsRes.json();
                    airportCount = airports.length;
                } catch (e) {
                    // If too large, estimate or show message
                    airportCount = '7000+';
                }
                
                let html = '<h3>Database Statistics</h3>';
                html += '<div class="stats-grid">';
                html += '<div class="stat-card"><h3>' + airlines.length + '</h3><p>Airlines</p></div>';
                html += '<div class="stat-card"><h3>' + airportCount + '</h3><p>Airports</p></div>';
                html += '</div>';
                
                showResults(html);
            } catch (error) {
                showError('Failed to fetch statistics: ' + error.message);
            }
        }
        
        // Allow Enter key to trigger search
        document.getElementById('airline-iata').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') searchAirline();
        });
        
        document.getElementById('airport-iata').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') searchAirport();
        });
        
        document.getElementById('route-source').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                document.getElementById('route-dest').focus();
            }
        });
        
        document.getElementById('route-dest').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') findDirectRoutes();
        });
        
        document.getElementById('report-airline-iata').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') getAirlineRoutesFromInput();
        });
        
        document.getElementById('report-airport-iata').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') getAirportAirlinesFromInput();
        });
        
        // Autocomplete functionality
        let allAirlines = [];
        let allAirports = [];
        let autocompleteLoaded = false;
        
        async function loadAutocompleteData() {
            if (autocompleteLoaded) return;
            
            try {
                // Load airlines first (smaller dataset)
                const airlinesRes = await fetch('/airlines');
                if (airlinesRes.ok) {
                    allAirlines = await airlinesRes.json();
                }
                
                // Try to load airports, but handle large dataset gracefully
                try {
                    const airportsRes = await fetch('/airports');
                    if (airportsRes.ok) {
                        const text = await airportsRes.text();
                        try {
                            allAirports = JSON.parse(text);
                            autocompleteLoaded = true;
                        } catch (e) {
                            console.log('Airports data too large, will load on demand');
                            // Keep allAirports empty, will load on demand
                        }
                    }
                } catch (e) {
                    console.log('Will load airports on demand');
                }
            } catch (error) {
                console.error('Failed to load autocomplete data:', error);
            }
        }
        
        function filterAirlines(query) {
            const dropdown = document.getElementById('airline-dropdown');
            if (!dropdown) return;
            
            const queryLower = (query || '').toLowerCase().trim();
            if (queryLower.length === 0) {
                dropdown.innerHTML = '';
                dropdown.style.display = 'none';
                return;
            }
            
            if (allAirlines.length === 0) {
                // Load if not loaded
                loadAutocompleteData();
                // Show loading message
                dropdown.innerHTML = '<div class="dropdown-item">Loading...</div>';
                dropdown.style.display = 'block';
                return;
            }
            
            const filtered = allAirlines.filter(airline => {
                if (!airline.iata) return false;
                const iata = (airline.iata || '').toLowerCase();
                const name = (airline.name || '').toLowerCase();
                return iata.includes(queryLower) || name.includes(queryLower);
            });
            
            if (filtered.length === 0) {
                dropdown.style.display = 'none';
                return;
            }
            
            // Sort: exact match first, then starts with query, then contains query
            filtered.sort((a, b) => {
                const aIata = (a.iata || '').toLowerCase();
                const bIata = (b.iata || '').toLowerCase();
                const aName = (a.name || '').toLowerCase();
                const bName = (b.name || '').toLowerCase();
                
                // Exact IATA match first
                if (aIata === queryLower && bIata !== queryLower) return -1;
                if (bIata === queryLower && aIata !== queryLower) return 1;
                
                // IATA starts with query
                if (aIata.startsWith(queryLower) && !bIata.startsWith(queryLower)) return -1;
                if (bIata.startsWith(queryLower) && !aIata.startsWith(queryLower)) return 1;
                
                // Name starts with query
                if (aName.startsWith(queryLower) && !bName.startsWith(queryLower)) return -1;
                if (bName.startsWith(queryLower) && !aName.startsWith(queryLower)) return 1;
                
                // Alphabetical by IATA
                return aIata.localeCompare(bIata);
            });
            
            dropdown.innerHTML = filtered.slice(0, 15).map(airline => {
                const name = (airline.name || '').replace(/'/g, "\\'");
                return '<div class="dropdown-item" onclick="selectAirline(\'' + airline.iata + '\', \'' + name + '\')">' +
                       '<strong>' + airline.iata + '</strong>' +
                       '<small>' + name + ' - ' + (airline.country || 'N/A') + '</small>' +
                       '</div>';
            }).join('');
            dropdown.style.display = 'block';
        }
        
        async function filterAirports(query) {
            const dropdown = document.getElementById('airport-dropdown');
            if (!dropdown) return;
            
            const queryLower = (query || '').toLowerCase().trim();
            if (queryLower.length === 0) {
                dropdown.innerHTML = '';
                dropdown.style.display = 'none';
                return;
            }
            
            // Always use server-side search endpoint (airports are too large to cache)
            try {
                dropdown.innerHTML = '<div class="dropdown-item">Loading...</div>';
                dropdown.style.display = 'block';
                
                const response = await fetch('/airports/search?q=' + encodeURIComponent(query));
                if (response.ok) {
                    const filtered = await response.json();
                    
                    if (filtered.length === 0) {
                        dropdown.style.display = 'none';
                        return;
                    }
                    
                    // Sort: exact match first, then starts with query, then contains query (same as airlines)
                    filtered.sort((a, b) => {
                        const aIata = (a.iata || '').toLowerCase();
                        const bIata = (b.iata || '').toLowerCase();
                        const aName = (a.name || '').toLowerCase();
                        const bName = (b.name || '').toLowerCase();
                        
                        // Exact IATA match first
                        if (aIata === queryLower && bIata !== queryLower) return -1;
                        if (bIata === queryLower && aIata !== queryLower) return 1;
                        
                        // IATA starts with query
                        if (aIata.startsWith(queryLower) && !bIata.startsWith(queryLower)) return -1;
                        if (bIata.startsWith(queryLower) && !aIata.startsWith(queryLower)) return 1;
                        
                        // Name starts with query
                        if (aName.startsWith(queryLower) && !bName.startsWith(queryLower)) return -1;
                        if (bName.startsWith(queryLower) && !aName.startsWith(queryLower)) return 1;
                        
                        // Alphabetical by IATA
                        return aIata.localeCompare(bIata);
                    });
                    
                    dropdown.innerHTML = filtered.slice(0, 15).map(airport => {
                        const name = (airport.name || '').replace(/'/g, "\\'");
                        const city = (airport.city || '').replace(/'/g, "\\'");
                        return '<div class="dropdown-item" onclick="selectAirport(\'' + airport.iata + '\', \'' + name + '\')">' +
                               '<strong>' + airport.iata + '</strong>' +
                               '<small>' + name + ' - ' + city + ', ' + (airport.country || 'N/A') + '</small>' +
                               '</div>';
                    }).join('');
                    dropdown.style.display = 'block';
                } else {
                    dropdown.style.display = 'none';
                    return;
                }
            } catch (e) {
                console.error('Error searching airports:', e);
                dropdown.style.display = 'none';
                return;
            }
        }
        
        function selectAirline(iata, name) {
            document.getElementById('airline-iata').value = iata;
            hideAirlineDropdown();
        }
        
        function selectAirport(iata, name) {
            document.getElementById('airport-iata').value = iata;
            hideAirportDropdown();
        }
        
        function showAirlineDropdown() {
            const input = document.getElementById('airline-iata');
            if (input.value.length > 0) {
                filterAirlines(input.value);
            }
        }
        
        function hideAirlineDropdown() {
            document.getElementById('airline-dropdown').style.display = 'none';
        }
        
        function showAirportDropdown() {
            const input = document.getElementById('airport-iata');
            if (input && input.value.length > 0) {
                filterAirports(input.value);
            }
        }
        
        function hideAirportDropdown() {
            const dropdown = document.getElementById('airport-dropdown');
            if (dropdown) {
                dropdown.style.display = 'none';
            }
        }
        
        // Report dropdown functions
        function filterAirlinesForReport(query) {
            const dropdown = document.getElementById('report-airline-dropdown');
            if (!dropdown) return;
            
            const queryLower = (query || '').toLowerCase().trim();
            if (queryLower.length === 0) {
                dropdown.innerHTML = '';
                dropdown.style.display = 'none';
                return;
            }
            
            if (allAirlines.length === 0) {
                loadAutocompleteData();
                return;
            }
            
            const filtered = allAirlines.filter(airline => {
                if (!airline.iata) return false;
                const iata = (airline.iata || '').toLowerCase();
                const name = (airline.name || '').toLowerCase();
                return iata.includes(queryLower) || name.includes(queryLower);
            });
            
            if (filtered.length === 0) {
                dropdown.style.display = 'none';
                return;
            }
            
            // Sort: exact match first, then starts with query, then contains query
            filtered.sort((a, b) => {
                const aIata = (a.iata || '').toLowerCase();
                const bIata = (b.iata || '').toLowerCase();
                const aName = (a.name || '').toLowerCase();
                const bName = (b.name || '').toLowerCase();
                
                if (aIata === queryLower && bIata !== queryLower) return -1;
                if (bIata === queryLower && aIata !== queryLower) return 1;
                if (aIata.startsWith(queryLower) && !bIata.startsWith(queryLower)) return -1;
                if (bIata.startsWith(queryLower) && !aIata.startsWith(queryLower)) return 1;
                if (aName.startsWith(queryLower) && !bName.startsWith(queryLower)) return -1;
                if (bName.startsWith(queryLower) && !aName.startsWith(queryLower)) return 1;
                return aIata.localeCompare(bIata);
            });
            
            dropdown.innerHTML = filtered.slice(0, 15).map(airline => {
                const name = (airline.name || '').replace(/'/g, "\\'");
                return '<div class="dropdown-item" onclick="selectAirlineForReport(\'' + airline.iata + '\', \'' + name + '\')">' +
                       '<strong>' + airline.iata + '</strong>' +
                       '<small>' + name + ' - ' + (airline.country || 'N/A') + '</small>' +
                       '</div>';
            }).join('');
            dropdown.style.display = 'block';
        }
        
        async function filterAirportsForReport(query) {
            const dropdown = document.getElementById('report-airport-dropdown');
            if (!query || query.length === 0) {
                dropdown.innerHTML = '';
                dropdown.style.display = 'none';
                return;
            }
            
            const queryLower = query.toLowerCase().trim();
            let filtered = [];
            
            if (allAirports.length > 0) {
                filtered = allAirports.filter(airport => {
                    if (!airport.iata) return false;
                    
                    const iata = (airport.iata || '').toLowerCase();
                    const name = (airport.name || '').toLowerCase();
                    const city = (airport.city || '').toLowerCase();
                    const country = (airport.country || '').toLowerCase();
                    
                    return iata.includes(queryLower) ||
                           name.includes(queryLower) ||
                           city.includes(queryLower) ||
                           country.includes(queryLower) ||
                           (queryLower.split(' ').every(word => city.includes(word)));
                }).slice(0, 15);
            } else {
                // If airports not loaded, fetch from server
                try {
                    const response = await fetch('/airports');
                    if (response.ok) {
                        const text = await response.text();
                        try {
                            const airports = JSON.parse(text);
                            filtered = airports.filter(airport => {
                                if (!airport.iata) return false;
                                
                                const iata = (airport.iata || '').toLowerCase();
                                const name = (airport.name || '').toLowerCase();
                                const city = (airport.city || '').toLowerCase();
                                const country = (airport.country || '').toLowerCase();
                                
                                return iata.includes(queryLower) ||
                                       name.includes(queryLower) ||
                                       city.includes(queryLower) ||
                                       country.includes(queryLower) ||
                                       (queryLower.split(' ').every(word => city.includes(word)));
                            }).slice(0, 15);
                        } catch (e) {
                            dropdown.innerHTML = '<div class="dropdown-item">Type airport code, city, or name (e.g., SFO, Los Angeles, JFK)</div>';
                            dropdown.style.display = 'block';
                            return;
                        }
                    }
                } catch (e) {
                    console.error('Error fetching airports:', e);
                }
            }
            
            if (filtered.length === 0) {
                dropdown.style.display = 'none';
                return;
            }
            
            // Sort results: exact IATA matches first, then city matches, then name matches
            filtered.sort((a, b) => {
                const aIata = (a.iata || '').toLowerCase();
                const bIata = (b.iata || '').toLowerCase();
                const aCity = (a.city || '').toLowerCase();
                const bCity = (b.city || '').toLowerCase();
                const aName = (a.name || '').toLowerCase();
                const bName = (b.name || '').toLowerCase();
                
                // Exact IATA match first
                if (aIata === queryLower && bIata !== queryLower) return -1;
                if (bIata === queryLower && aIata !== queryLower) return 1;
                
                // City starts with query
                if (aCity.startsWith(queryLower) && !bCity.startsWith(queryLower)) return -1;
                if (bCity.startsWith(queryLower) && !aCity.startsWith(queryLower)) return 1;
                
                // City contains query
                if (aCity.includes(queryLower) && !bCity.includes(queryLower)) return -1;
                if (bCity.includes(queryLower) && !aCity.includes(queryLower)) return 1;
                
                return 0;
            });
            
            dropdown.innerHTML = filtered.map(airport => {
                const name = (airport.name || '').replace(/'/g, "\\'");
                const city = (airport.city || '').replace(/'/g, "\\'");
                return '<div class="dropdown-item" onclick="selectAirportForReport(\'' + airport.iata + '\', \'' + name + '\')">' +
                       '<strong>' + airport.iata + '</strong>' +
                       '<small>' + name + ' - ' + city + ', ' + (airport.country || 'N/A') + '</small>' +
                       '</div>';
            }).join('');
            dropdown.style.display = 'block';
        }
        
        function selectAirlineForReport(iata, name) {
            document.getElementById('report-airline-iata').value = iata;
            hideAirlineReportDropdown();
        }
        
        function selectAirportForReport(iata, name) {
            document.getElementById('report-airport-iata').value = iata;
            hideAirportReportDropdown();
        }
        
        function showAirlineReportDropdown() {
            const input = document.getElementById('report-airline-iata');
            if (input.value.length > 0) {
                filterAirlinesForReport(input.value);
            }
        }
        
        function hideAirlineReportDropdown() {
            document.getElementById('report-airline-dropdown').style.display = 'none';
        }
        
        function showAirportReportDropdown() {
            const input = document.getElementById('report-airport-iata');
            if (input.value.length > 0) {
                filterAirportsForReport(input.value);
            }
        }
        
        function hideAirportReportDropdown() {
            document.getElementById('report-airport-dropdown').style.display = 'none';
        }
        
        // Route planning dropdown functions
        async function filterAirportsForRoute(type, query) {
            const dropdownId = type === 'source' ? 'route-source-dropdown' : 'route-dest-dropdown';
            const dropdown = document.getElementById(dropdownId);
            if (!dropdown) return;
            
            const queryLower = (query || '').toLowerCase().trim();
            if (queryLower.length === 0) {
                dropdown.innerHTML = '';
                dropdown.style.display = 'none';
                return;
            }
            
            // Always use server-side search endpoint (airports are too large to cache)
            try {
                dropdown.innerHTML = '<div class="dropdown-item">Loading...</div>';
                dropdown.style.display = 'block';
                
                const response = await fetch('/airports/search?q=' + encodeURIComponent(query));
                if (response.ok) {
                    const filtered = await response.json();
                    
                    if (filtered.length === 0) {
                        dropdown.style.display = 'none';
                        return;
                    }
                    
                    // Sort: exact match first, then starts with query, then contains query (same as airlines)
                    filtered.sort((a, b) => {
                        const aIata = (a.iata || '').toLowerCase();
                        const bIata = (b.iata || '').toLowerCase();
                        const aName = (a.name || '').toLowerCase();
                        const bName = (b.name || '').toLowerCase();
                        
                        // Exact IATA match first
                        if (aIata === queryLower && bIata !== queryLower) return -1;
                        if (bIata === queryLower && aIata !== queryLower) return 1;
                        
                        // IATA starts with query
                        if (aIata.startsWith(queryLower) && !bIata.startsWith(queryLower)) return -1;
                        if (bIata.startsWith(queryLower) && !aIata.startsWith(queryLower)) return 1;
                        
                        // Name starts with query
                        if (aName.startsWith(queryLower) && !bName.startsWith(queryLower)) return -1;
                        if (bName.startsWith(queryLower) && !aName.startsWith(queryLower)) return 1;
                        
                        // Alphabetical by IATA
                        return aIata.localeCompare(bIata);
                    });
                    
                    const selectFunc = type === 'source' ? 'selectRouteSource' : 'selectRouteDest';
                    dropdown.innerHTML = filtered.slice(0, 15).map(airport => {
                        const name = (airport.name || '').replace(/'/g, "\\'");
                        const city = (airport.city || '').replace(/'/g, "\\'");
                        return '<div class="dropdown-item" onclick="' + selectFunc + '(\'' + airport.iata + '\', \'' + name + '\')">' +
                               '<strong>' + airport.iata + '</strong>' +
                               '<small>' + name + ' - ' + city + ', ' + (airport.country || 'N/A') + '</small>' +
                               '</div>';
                    }).join('');
                    dropdown.style.display = 'block';
                } else {
                    dropdown.style.display = 'none';
                    return;
                }
            } catch (e) {
                console.error('Error searching airports:', e);
                dropdown.style.display = 'none';
                return;
            }
        }
        
        function selectRouteSource(iata, name) {
            document.getElementById('route-source').value = iata;
            hideRouteSourceDropdown();
        }
        
        function selectRouteDest(iata, name) {
            document.getElementById('route-dest').value = iata;
            hideRouteDestDropdown();
        }
        
        function showRouteSourceDropdown() {
            const input = document.getElementById('route-source');
            if (input.value.length > 0) {
                filterAirportsForRoute('source', input.value);
            }
        }
        
        function hideRouteSourceDropdown() {
            document.getElementById('route-source-dropdown').style.display = 'none';
        }
        
        function showRouteDestDropdown() {
            const input = document.getElementById('route-dest');
            if (input.value.length > 0) {
                filterAirportsForRoute('dest', input.value);
            }
        }
        
        function hideRouteDestDropdown() {
            document.getElementById('route-dest-dropdown').style.display = 'none';
        }
        
        // Analytics functions
        async function showRouteStatistics() {
            showLoading();
            try {
                const airlinesRes = await fetch('/airlines');
                const airlines = await airlinesRes.json();
                
                let airportCount = 0;
                try {
                    const airportsRes = await fetch('/airports');
                    const airports = await airportsRes.json();
                    airportCount = airports.length;
                } catch (e) {
                    airportCount = 7000; // Approximate if too large
                }
                
                let html = '<h3>Route Statistics</h3>';
                html += '<div class="stats-grid">';
                html += '<div class="stat-card"><h3>' + airlines.length + '</h3><p>Total Airlines</p></div>';
                html += '<div class="stat-card"><h3>' + airportCount + '</h3><p>Total Airports</p></div>';
                html += '</div>';
                
                // Filter to US airlines only
                html += '<div style="margin-top: 20px; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);">';
                html += '<h4>Key Metrics (US Airlines Only)</h4>';
                
                const usAirlines = airlines.filter(a => a.country && a.country.toLowerCase() === 'united states');
                
                // Calculate from US airlines
                let totalRoutes = 0;
                const batchSize = 20;
                for (let i = 0; i < usAirlines.length; i += batchSize) {
                    const batch = usAirlines.slice(i, i + batchSize);
                    const promises = batch.map(async (airline) => {
                        try {
                            const routes = await fetch('/airline/' + airline.iata + '/routes').then(r => r.json());
                            return routes.length;
                        } catch (e) {
                            return 0;
                        }
                    });
                    const results = await Promise.all(promises);
                    totalRoutes += results.reduce((sum, count) => sum + count, 0);
                }
                
                const avgRoutes = usAirlines.length > 0 ? (totalRoutes / usAirlines.length).toFixed(1) : 0;
                
                html += '<p><strong>US Airlines Analyzed:</strong> ' + usAirlines.length + '</p>';
                html += '<p><strong>Average Routes per US Airline:</strong> ' + avgRoutes + '</p>';
                html += '<p><strong>Total Route Connections (US Airlines):</strong> ' + totalRoutes.toLocaleString() + '</p>';
                html += '</div>';
                
                showResults(html);
            } catch (error) {
                showError('Failed to load route statistics: ' + error.message);
            }
        }
        
        async function showTopAirlines() {
            showLoading();
            try {
                const airlines = await fetch('/airlines').then(r => r.json());
                
                // Filter to US airlines only
                const usAirlines = airlines.filter(a => a.country && a.country.toLowerCase() === 'united states');
                
                const airlineStats = [];
                
                // Process in batches to avoid overwhelming the server
                const batchSize = 20;
                
                for (let i = 0; i < usAirlines.length; i += batchSize) {
                    const batch = usAirlines.slice(i, i + batchSize);
                    const promises = batch.map(async (airline) => {
                        try {
                            const routes = await fetch('/airline/' + airline.iata + '/routes').then(r => r.json());
                            return {airline: airline, routeCount: routes.length};
                        } catch (e) {
                            return {airline: airline, routeCount: 0};
                        }
                    });
                    
                    const results = await Promise.all(promises);
                    airlineStats.push(...results);
                }
                
                airlineStats.sort((a, b) => b.routeCount - a.routeCount);
                
                let html = '<h3>Top US Airlines by Route Count</h3>';
                html += '<p style="margin-bottom: 15px; color: #666;">Showing top airlines from ' + usAirlines.length + ' US airlines</p>';
                html += '<div style="margin-top: 15px;">';
                for (let i = 0; i < Math.min(20, airlineStats.length); i++) {
                    const stat = airlineStats[i];
                    html += '<div class="result-item"><h4>' + (i + 1) + '. ' + stat.airline.name + ' (' + stat.airline.iata + ')</h4>';
                    html += '<p><strong>Routes:</strong> ' + stat.routeCount + ' | <strong>Country:</strong> ' + (stat.airline.country || 'N/A') + '</p></div>';
                }
                html += '</div>';
                
                showResults(html);
            } catch (error) {
                showError('Failed to load top airlines: ' + error.message);
            }
        }
        
        async function showTopAirports() {
            showLoading();
            try {
                const response = await fetch('/airports/top?limit=20');
                if (!response.ok) {
                    throw new Error('Failed to fetch top airports: ' + response.status);
                }
                
                const data = await response.json();
                
                let html = '<h3>Top Airports by Traffic</h3>';
                html += '<p style="margin-bottom: 15px; color: #666;">Showing top 20 airports by route count</p>';
                html += '<div style="margin-top: 15px;">';
                for (let i = 0; i < data.length; i++) {
                    const item = data[i];
                    html += '<div class="result-item"><h4>' + (i + 1) + '. ' + item.airport.name + ' (' + item.airport.iata + ')</h4>';
                    html += '<p><strong>Total Routes:</strong> ' + item.routeCount + ' | <strong>Airlines:</strong> ' + item.airlineCount + ' | <strong>City:</strong> ' + item.airport.city + '</p></div>';
                }
                html += '</div>';
                
                showResults(html);
            } catch (error) {
                showError('Failed to load top airports: ' + error.message);
            }
        }
        
        async function showGeographicStats() {
            showLoading();
            try {
                const response = await fetch('/airports/geographic');
                if (!response.ok) {
                    throw new Error('Failed to fetch geographic statistics: ' + response.status);
                }
                
                const data = await response.json();
                
                let html = '<h3>Geographic Distribution</h3>';
                html += '<p style="margin-bottom: 15px;"><strong>Total Countries:</strong> ' + data.totalCountries + '</p>';
                html += '<h4>Top 20 Countries by Airport Count</h4>';
                html += '<div style="margin-top: 15px;">';
                for (const country of data.countries) {
                    html += '<div class="result-item"><strong>' + country.country + ':</strong> ' + country.count + ' airports</div>';
                }
                html += '</div>';
                
                showResults(html);
            } catch (error) {
                showError('Failed to load geographic statistics: ' + error.message);
            }
        }
        
        function exportData(format) {
            showLoading();
            if (format === 'csv') {
                fetch('/airlines').then(r => r.json()).then(airlines => {
                    let csv = 'IATA,Name,Country,Active\\n';
                    for (const airline of airlines) {
                        csv += airline.iata + ',' + airline.name.replace(/,/g, ';') + ',' + (airline.country || '') + ',' + airline.active + '\\n';
                    }
                    const blob = new Blob([csv], {type: 'text/csv'});
                    const url = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = 'airlines_export.csv';
                    a.click();
                    hideLoading();
                });
            } else {
                fetch('/airlines').then(r => r.json()).then(airlines => {
                    const blob = new Blob([JSON.stringify(airlines, null, 2)], {type: 'application/json'});
                    const url = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = 'airlines_export.json';
                    a.click();
                    hideLoading();
                });
            }
        }
        
        // Comparison functions
        function filterAirlinesForCompare(num, query) {
            const dropdown = document.getElementById('compare-airline-' + num + '-dropdown');
            if (!dropdown) return;
            
            const queryLower = (query || '').toLowerCase().trim();
            if (queryLower.length === 0) {
                dropdown.style.display = 'none';
                return;
            }
            
            if (allAirlines.length === 0) {
                loadAutocompleteData();
                return;
            }
            
            const filtered = allAirlines.filter(airline => {
                if (!airline.iata) return false;
                const iata = (airline.iata || '').toLowerCase();
                const name = (airline.name || '').toLowerCase();
                return iata.includes(queryLower) || name.includes(queryLower);
            });
            
            if (filtered.length === 0) {
                dropdown.style.display = 'none';
                return;
            }
            
            // Sort: exact match first, then starts with query, then contains query
            filtered.sort((a, b) => {
                const aIata = (a.iata || '').toLowerCase();
                const bIata = (b.iata || '').toLowerCase();
                const aName = (a.name || '').toLowerCase();
                const bName = (b.name || '').toLowerCase();
                
                if (aIata === queryLower && bIata !== queryLower) return -1;
                if (bIata === queryLower && aIata !== queryLower) return 1;
                if (aIata.startsWith(queryLower) && !bIata.startsWith(queryLower)) return -1;
                if (bIata.startsWith(queryLower) && !aIata.startsWith(queryLower)) return 1;
                if (aName.startsWith(queryLower) && !bName.startsWith(queryLower)) return -1;
                if (bName.startsWith(queryLower) && !aName.startsWith(queryLower)) return 1;
                return aIata.localeCompare(bIata);
            });
            
            dropdown.innerHTML = filtered.slice(0, 15).map(airline => {
                const name = (airline.name || '').replace(/'/g, "\\'");
                return '<div class="dropdown-item" onclick="selectAirlineForCompare(' + num + ', \'' + airline.iata + '\', \'' + name + '\')">' +
                       '<strong>' + airline.iata + '</strong> - ' + name + '</div>';
            }).join('');
            dropdown.style.display = 'block';
        }
        
        async function filterAirportsForCompare(num, query) {
            const dropdown = document.getElementById('compare-airport-' + num + '-dropdown');
            if (!dropdown) return;
            
            const queryLower = (query || '').toLowerCase().trim();
            if (queryLower.length === 0) {
                dropdown.style.display = 'none';
                return;
            }
            
            let filtered = [];
            
            if (allAirports.length > 0) {
                filtered = allAirports.filter(airport => {
                    if (!airport.iata) return false;
                    const iata = (airport.iata || '').toLowerCase();
                    const name = (airport.name || '').toLowerCase();
                    const city = (airport.city || '').toLowerCase();
                    const country = (airport.country || '').toLowerCase();
                    return iata.includes(queryLower) || name.includes(queryLower) || 
                           city.includes(queryLower) || country.includes(queryLower) ||
                           (queryLower.split(' ').every(word => city.includes(word)));
                });
            } else {
                // Use server-side search endpoint
                try {
                    dropdown.innerHTML = '<div class="dropdown-item">Searching...</div>';
                    dropdown.style.display = 'block';
                    const response = await fetch('/airports/search?q=' + encodeURIComponent(query));
                    if (response.ok) {
                        filtered = await response.json();
                        // Cache results
                        if (allAirports.length === 0) {
                            allAirports = filtered;
                        } else {
                            const existingIatas = new Set(allAirports.map(a => a.iata));
                            for (const airport of filtered) {
                                if (!existingIatas.has(airport.iata)) {
                                    allAirports.push(airport);
                                }
                            }
                        }
                    } else {
                        dropdown.style.display = 'none';
                        return;
                    }
                } catch (e) {
                    console.error('Error:', e);
                    dropdown.style.display = 'none';
                    return;
                }
            }
            
            if (filtered.length === 0) {
                dropdown.innerHTML = '<div class="dropdown-item" style="color: #666;">No airports found</div>';
                dropdown.style.display = 'block';
                return;
            }
            
            filtered.sort((a, b) => {
                const aIata = (a.iata || '').toLowerCase();
                const bIata = (b.iata || '').toLowerCase();
                const aCity = (a.city || '').toLowerCase();
                const bCity = (b.city || '').toLowerCase();
                if (aIata === queryLower && bIata !== queryLower) return -1;
                if (bIata === queryLower && aIata !== queryLower) return 1;
                if (aCity.startsWith(queryLower) && !bCity.startsWith(queryLower)) return -1;
                if (bCity.startsWith(queryLower) && !aCity.startsWith(queryLower)) return 1;
                return 0;
            });
            
            dropdown.innerHTML = filtered.slice(0, 15).map(airport => {
                const name = (airport.name || '').replace(/'/g, "\\'");
                const city = (airport.city || '').replace(/'/g, "\\'");
                return '<div class="dropdown-item" onclick="selectAirportForCompare(' + num + ', \'' + airport.iata + '\', \'' + name + '\')">' +
                       '<strong>' + airport.iata + '</strong>' +
                       '<small>' + name + ' - ' + city + ', ' + (airport.country || 'N/A') + '</small>' +
                       '</div>';
            }).join('');
            dropdown.style.display = 'block';
        }
        
        function selectAirlineForCompare(num, iata, name) {
            document.getElementById('compare-airline-' + num).value = iata;
            document.getElementById('compare-airline-' + num + '-dropdown').style.display = 'none';
        }
        
        function selectAirportForCompare(num, iata, name) {
            document.getElementById('compare-airport-' + num).value = iata;
            document.getElementById('compare-airport-' + num + '-dropdown').style.display = 'none';
        }
        
        function showCompareAirportDropdown(num) {
            const input = document.getElementById('compare-airport-' + num);
            if (input.value.length > 0) {
                filterAirportsForCompare(num, input.value);
            }
        }
        
        function hideCompareAirportDropdown(num) {
            document.getElementById('compare-airport-' + num + '-dropdown').style.display = 'none';
        }
        
        async function compareAirlines() {
            const iata1 = document.getElementById('compare-airline-1').value.trim().toUpperCase();
            const iata2 = document.getElementById('compare-airline-2').value.trim().toUpperCase();
            
            if (!iata1 || !iata2) {
                showError('Please select both airlines to compare');
                return;
            }
            
            showLoading();
            try {
                const [airline1, airline2, routes1, routes2] = await Promise.all([
                    fetch('/airline/' + iata1).then(r => r.json()),
                    fetch('/airline/' + iata2).then(r => r.json()),
                    fetch('/airline/' + iata1 + '/routes').then(r => r.json()),
                    fetch('/airline/' + iata2 + '/routes').then(r => r.json())
                ]);
                
                let html = '<h3>Airline Comparison</h3>';
                html += '<div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-top: 20px;">';
                
                html += '<div class="result-item"><h4>' + airline1.name + ' (' + airline1.iata + ')</h4>';
                html += '<p><strong>Country:</strong> ' + (airline1.country || 'N/A') + '</p>';
                html += '<p><strong>ICAO:</strong> ' + (airline1.icao || 'N/A') + '</p>';
                html += '<p><strong>Active:</strong> ' + airline1.active + '</p>';
                html += '<p><strong>Total Routes:</strong> ' + routes1.length + '</p>';
                html += '<p><strong>Airports Served:</strong> ' + routes1.length + '</p></div>';
                
                html += '<div class="result-item"><h4>' + airline2.name + ' (' + airline2.iata + ')</h4>';
                html += '<p><strong>Country:</strong> ' + (airline2.country || 'N/A') + '</p>';
                html += '<p><strong>ICAO:</strong> ' + (airline2.icao || 'N/A') + '</p>';
                html += '<p><strong>Active:</strong> ' + airline2.active + '</p>';
                html += '<p><strong>Total Routes:</strong> ' + routes2.length + '</p>';
                html += '<p><strong>Airports Served:</strong> ' + routes2.length + '</p></div>';
                
                html += '</div>';
                
                showResults(html);
            } catch (error) {
                showError('Failed to compare airlines: ' + error.message);
            }
        }
        
        async function compareAirports() {
            const iata1 = document.getElementById('compare-airport-1').value.trim().toUpperCase().substring(0, 3);
            const iata2 = document.getElementById('compare-airport-2').value.trim().toUpperCase().substring(0, 3);
            
            if (!iata1 || !iata2) {
                showError('Please select both airports to compare');
                return;
            }
            
            showLoading();
            try {
                const [airport1, airport2, airlines1, airlines2] = await Promise.all([
                    fetch('/airport/' + iata1).then(r => r.json()),
                    fetch('/airport/' + iata2).then(r => r.json()),
                    fetch('/airport/' + iata1 + '/airlines').then(r => r.json()),
                    fetch('/airport/' + iata2 + '/airlines').then(r => r.json())
                ]);
                
                let totalRoutes1 = 0, totalRoutes2 = 0;
                for (const a of airlines1) totalRoutes1 += a.route_count;
                for (const a of airlines2) totalRoutes2 += a.route_count;
                
                let html = '<h3>Airport Comparison</h3>';
                html += '<div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-top: 20px;">';
                
                html += '<div class="result-item"><h4>' + airport1.name + ' (' + airport1.iata + ')</h4>';
                html += '<p><strong>City:</strong> ' + airport1.city + '</p>';
                html += '<p><strong>Country:</strong> ' + airport1.country + '</p>';
                html += '<p><strong>Coordinates:</strong> ' + airport1.latitude.toFixed(4) + ', ' + airport1.longitude.toFixed(4) + '</p>';
                html += '<p><strong>Total Routes:</strong> ' + totalRoutes1 + '</p>';
                html += '<p><strong>Airlines:</strong> ' + airlines1.length + '</p></div>';
                
                html += '<div class="result-item"><h4>' + airport2.name + ' (' + airport2.iata + ')</h4>';
                html += '<p><strong>City:</strong> ' + airport2.city + '</p>';
                html += '<p><strong>Country:</strong> ' + airport2.country + '</p>';
                html += '<p><strong>Coordinates:</strong> ' + airport2.latitude.toFixed(4) + ', ' + airport2.longitude.toFixed(4) + '</p>';
                html += '<p><strong>Total Routes:</strong> ' + totalRoutes2 + '</p>';
                html += '<p><strong>Airlines:</strong> ' + airlines2.length + '</p></div>';
                
                html += '</div>';
                
                showResults(html);
            } catch (error) {
                showError('Failed to compare airports: ' + error.message);
            }
        }
        
        // Contact form
        function submitContactForm(event) {
            event.preventDefault();
            const name = document.getElementById('contact-name').value;
            const email = document.getElementById('contact-email').value;
            const subject = document.getElementById('contact-subject').value;
            const message = document.getElementById('contact-message').value;
            
            const responseDiv = document.getElementById('contact-response');
            responseDiv.style.display = 'block';
            responseDiv.innerHTML = '<div class="result-item" style="background: #d4edda; border-left-color: #28a745;"><strong>Thank you!</strong><p>Your message has been received. We will get back to you soon.</p><p><strong>Name:</strong> ' + name + '</p><p><strong>Subject:</strong> ' + subject + '</p></div>';
            
            // Reset form
            document.getElementById('contact-form').reset();
            
            // In a real application, you would send this to a server endpoint
            console.log('Contact form submitted:', {name, email, subject, message});
        }
        
        // Load student info on About page
        async function loadStudentInfo() {
            try {
                const response = await fetch('/student');
                const data = await response.json();
                document.getElementById('student-info-display').innerHTML = '<p style="color: #333; margin: 0;">' + data.info + '</p>';
            } catch (e) {
                console.error('Failed to load student info:', e);
                document.getElementById('student-info-display').innerHTML = '<p style="color: #333; margin: 0;">Student information not available</p>';
            }
        }
        
        // Initialize on page load
        document.addEventListener('DOMContentLoaded', function() {
            // Ensure map container is hidden initially
            const mapContainer = document.getElementById('route-map-container');
            if (mapContainer) {
                mapContainer.style.display = 'none';
            }
            
            // Load student info for About page
            loadStudentInfo();
            
            // Load autocomplete data in background (non-blocking)
            setTimeout(function() {
                loadAutocompleteData();
            }, 500);
        });
        
        // Also load when window loads
        window.addEventListener('load', function() {
            if (!autocompleteLoaded) {
                setTimeout(function() {
                    loadAutocompleteData();
                }, 1000);
            }
        });
    </script>
</body>
</html>
        )raw", "text/html");
    });
    
    // Get airline by IATA code
    svr.Get("/airline/:iata", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string iata = req.path_params.at("iata");
        Airline airline = db.getAirlineByIATA(iata);
        
        if (airline.id > 0) {
            res.set_content(airlineToJSON(airline), "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"error\":\"Airline not found\"}", "application/json");
        }
    });
    
    // Get airport by IATA code
    svr.Get("/airport/:iata", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string iata = req.path_params.at("iata");
        Airport airport = db.getAirportByIATA(iata);
        
        if (airport.id > 0) {
            res.set_content(airportToJSON(airport), "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"error\":\"Airport not found\"}", "application/json");
        }
    });
    
    // Get airports served by airline (ordered by route count)
    svr.Get("/airline/:iata/routes", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string iata = req.path_params.at("iata");
        auto airports = db.getAirportsByAirline(iata);
        
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < airports.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{"
                << "\"airport\":" << airportToJSON(airports[i].airport) << ","
                << "\"route_count\":" << airports[i].route_count
                << "}";
        }
        oss << "]";
        
        res.set_content(oss.str(), "application/json");
    });
    
    // Get airlines serving airport (ordered by route count)
    svr.Get("/airport/:iata/airlines", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string iata = req.path_params.at("iata");
        auto airlines = db.getAirlinesByAirport(iata);
        
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < airlines.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{"
                << "\"airline\":" << airlineToJSON(airlines[i].airline) << ","
                << "\"route_count\":" << airlines[i].route_count
                << "}";
        }
        oss << "]";
        
        res.set_content(oss.str(), "application/json");
    });
    
    // Get all airlines (sorted by IATA)
    svr.Get("/airlines", [&db](const httplib::Request&, httplib::Response& res) {
        auto airlines = db.getAllAirlinesSorted();
        
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < airlines.size(); ++i) {
            if (i > 0) oss << ",";
            oss << airlineToJSON(airlines[i]);
        }
        oss << "]";
        
        res.set_content(oss.str(), "application/json");
    });
    
    // Get airlines with pagination
    svr.Get("/airlines/list", [&db](const httplib::Request& req, httplib::Response& res) {
        int page = 1;
        int pageSize = 100;
        
        std::string pageStr = req.get_param_value("page");
        std::string sizeStr = req.get_param_value("size");
        
        if (!pageStr.empty()) {
            try {
                page = std::stoi(pageStr);
                if (page < 1) page = 1;
            } catch (...) {}
        }
        
        if (!sizeStr.empty()) {
            try {
                pageSize = std::stoi(sizeStr);
                if (pageSize < 10) pageSize = 10;
                if (pageSize > 500) pageSize = 500;
            } catch (...) {}
        }
        
        auto allAirlines = db.getAllAirlinesSorted();
        int total = static_cast<int>(allAirlines.size());
        int start = (page - 1) * pageSize;
        int end = std::min(start + pageSize, total);
        
        std::ostringstream oss;
        oss << "{\"total\":" << total << ",\"page\":" << page << ",\"pageSize\":" << pageSize << ",\"airlines\":[";
        for (int i = start; i < end; ++i) {
            if (i > start) oss << ",";
            oss << airlineToJSON(allAirlines[i]);
        }
        oss << "]}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Get all airports (sorted by IATA)
    svr.Get("/airports", [&db](const httplib::Request&, httplib::Response& res) {
        auto airports = db.getAllAirportsSorted();
        
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < airports.size(); ++i) {
            if (i > 0) oss << ",";
            oss << airportToJSON(airports[i]);
        }
        oss << "]";
        
        res.set_content(oss.str(), "application/json");
    });
    
    // Search airports endpoint (for autocomplete - returns limited results)
    svr.Get("/airports/search", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string query = req.get_param_value("q");
        if (query.empty()) {
            res.set_content("[]", "application/json");
            return;
        }
        
        // Convert to lowercase for case-insensitive search
        std::string queryLower = query;
        std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
        
        auto allAirports = db.getAllAirportsSorted();
        std::vector<Airport> results;
        
        // Collect all matching airports first
        for (const auto& airport : allAirports) {
            std::string iata = airport.iata;
            std::string name = airport.name;
            std::string city = airport.city;
            std::string country = airport.country;
            
            std::transform(iata.begin(), iata.end(), iata.begin(), ::tolower);
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            std::transform(city.begin(), city.end(), city.begin(), ::tolower);
            std::transform(country.begin(), country.end(), country.begin(), ::tolower);
            
            // Check if query matches IATA code or name only (same as airline search)
            if (iata.find(queryLower) != std::string::npos ||
                name.find(queryLower) != std::string::npos) {
                results.push_back(airport);
            }
        }
        
        // Sort results: exact IATA match first, then IATA starts with, then name starts with (same as airline search)
        std::sort(results.begin(), results.end(), [&queryLower](const Airport& a, const Airport& b) {
            std::string aIata = a.iata;
            std::string bIata = b.iata;
            std::string aName = a.name;
            std::string bName = b.name;
            
            std::transform(aIata.begin(), aIata.end(), aIata.begin(), ::tolower);
            std::transform(bIata.begin(), bIata.end(), bIata.begin(), ::tolower);
            std::transform(aName.begin(), aName.end(), aName.begin(), ::tolower);
            std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
            
            // Exact IATA match first
            if (aIata == queryLower && bIata != queryLower) return true;
            if (bIata == queryLower && aIata != queryLower) return false;
            // IATA starts with query
            if (aIata.find(queryLower) == 0 && bIata.find(queryLower) != 0) return true;
            if (bIata.find(queryLower) == 0 && aIata.find(queryLower) != 0) return false;
            // Name starts with query
            if (aName.find(queryLower) == 0 && bName.find(queryLower) != 0) return true;
            if (bName.find(queryLower) == 0 && aName.find(queryLower) != 0) return false;
            // Alphabetical by IATA
            return aIata < bIata;
        });
        
        // Limit to top 20 after sorting
        if (results.size() > 20) {
            results.resize(20);
        }
        
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < results.size(); ++i) {
            if (i > 0) oss << ",";
            oss << airportToJSON(results[i]);
        }
        oss << "]";
        res.set_content(oss.str(), "application/json");
    });
    
    // Get top airports by traffic (server-side calculation)
    svr.Get("/airports/top", [&db](const httplib::Request& req, httplib::Response& res) {
        int limit = 20;
        std::string limitStr = req.get_param_value("limit");
        if (!limitStr.empty()) {
            try {
                limit = std::stoi(limitStr);
                if (limit > 100) limit = 100; // Cap at 100
            } catch (...) {}
        }
        
        auto allAirports = db.getAllAirportsSorted();
        std::vector<std::pair<Airport, int>> airportStats; // airport, route count
        
        // Calculate route counts for each airport
        for (const auto& airport : allAirports) {
            if (airport.iata.empty()) continue;
            
            auto airlines = db.getAirlinesByAirport(airport.iata);
            int totalRoutes = 0;
            for (const auto& ar : airlines) {
                totalRoutes += ar.route_count;
            }
            
            airportStats.push_back({airport, totalRoutes});
        }
        
        // Sort by route count descending
        std::sort(airportStats.begin(), airportStats.end(), 
                  [](const std::pair<Airport, int>& a, const std::pair<Airport, int>& b) {
                      return a.second > b.second;
                  });
        
        // Limit results
        if (airportStats.size() > static_cast<size_t>(limit)) {
            airportStats.resize(limit);
        }
        
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < airportStats.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{"
                << "\"airport\":" << airportToJSON(airportStats[i].first) << ","
                << "\"routeCount\":" << airportStats[i].second << ","
                << "\"airlineCount\":" << db.getAirlinesByAirport(airportStats[i].first.iata).size()
                << "}";
        }
        oss << "]";
        res.set_content(oss.str(), "application/json");
    });
    
    // Get geographic statistics (server-side calculation)
    svr.Get("/airports/geographic", [&db](const httplib::Request&, httplib::Response& res) {
        auto allAirports = db.getAllAirportsSorted();
        std::map<std::string, int> countryCount;
        
        for (const auto& airport : allAirports) {
            std::string country = airport.country.empty() ? "Unknown" : airport.country;
            countryCount[country]++;
        }
        
        // Convert to vector and sort
        std::vector<std::pair<std::string, int>> sorted;
        for (const auto& pair : countryCount) {
            sorted.push_back(pair);
        }
        std::sort(sorted.begin(), sorted.end(), 
                  [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                      return a.second > b.second;
                  });
        
        // Limit to top 20
        if (sorted.size() > 20) {
            sorted.resize(20);
        }
        
        std::ostringstream oss;
        oss << "{\"totalCountries\":" << countryCount.size() << ",\"countries\":[";
        for (size_t i = 0; i < sorted.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{\"country\":\"" << sorted[i].first << "\",\"count\":" << sorted[i].second << "}";
        }
        oss << "]}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Get airports with pagination
    svr.Get("/airports/list", [&db](const httplib::Request& req, httplib::Response& res) {
        int page = 1;
        int pageSize = 100;
        
        std::string pageStr = req.get_param_value("page");
        std::string sizeStr = req.get_param_value("size");
        
        if (!pageStr.empty()) {
            try {
                page = std::stoi(pageStr);
                if (page < 1) page = 1;
            } catch (...) {}
        }
        
        if (!sizeStr.empty()) {
            try {
                pageSize = std::stoi(sizeStr);
                if (pageSize < 10) pageSize = 10;
                if (pageSize > 500) pageSize = 500;
            } catch (...) {}
        }
        
        auto allAirports = db.getAllAirportsSorted();
        int total = static_cast<int>(allAirports.size());
        int start = (page - 1) * pageSize;
        int end = std::min(start + pageSize, total);
        
        std::ostringstream oss;
        oss << "{\"total\":" << total << ",\"page\":" << page << ",\"pageSize\":" << pageSize << ",\"airports\":[";
        for (int i = start; i < end; ++i) {
            if (i > start) oss << ",";
            oss << airportToJSON(allAirports[i]);
        }
        oss << "]}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Get student info
    svr.Get("/student", [&db](const httplib::Request&, httplib::Response& res) {
        std::string info = db.getStudentInfo();
        res.set_content("{\"info\":\"" + info + "\"}", "application/json");
    });
    
    // Get source code (extra credit)
    svr.Get("/code", [](const httplib::Request&, httplib::Response& res) {
        // Try multiple possible paths
        std::ifstream file("../src/main.cpp");
        if (!file.is_open()) {
            file.open("src/main.cpp");
        }
        if (!file.is_open()) {
            file.open("../../src/main.cpp");
        }
        if (!file.is_open()) {
            res.status = 404;
            res.set_content("{\"error\":\"Source file not found\"}", "application/json");
            return;
        }
        std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // Escape JSON
        std::ostringstream oss;
        oss << "{\"code\":\"";
        for (char c : code) {
            if (c == '"') oss << "\\\"";
            else if (c == '\\') oss << "\\\\";
            else if (c == '\n') oss << "\\n";
            else if (c == '\r') oss << "\\r";
            else if (c == '\t') oss << "\\t";
            else oss << c;
        }
        oss << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Direct routes finder
    svr.Get("/direct/:source/:dest", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string source = req.path_params.at("source");
        std::string dest = req.path_params.at("dest");
        
        Airport source_airport = db.getAirportByIATA(source);
        Airport dest_airport = db.getAirportByIATA(dest);
        
        if (source_airport.id <= 0 || dest_airport.id <= 0) {
            res.status = 404;
            res.set_content("{\"error\":\"Airport not found\",\"routes\":[],\"source\":null,\"dest\":null}", "application/json");
            return;
        }
        
        auto routes = db.getDirectRoutes(source, dest);
        
        std::ostringstream oss;
        oss << "{"
            << "\"source\":" << airportToJSON(source_airport) << ","
            << "\"dest\":" << airportToJSON(dest_airport) << ","
            << "\"routes\":[";
        for (size_t i = 0; i < routes.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{"
                << "\"airline_iata\":\"" << routes[i].airline_iata << "\","
                << "\"airline_name\":\"" << routes[i].airline_name << "\","
                << "\"distance\":" << std::fixed << std::setprecision(2) << routes[i].distance << ","
                << "\"stops\":" << routes[i].stops
                << "}";
        }
        oss << "]}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Direct routes finder with airport details
    svr.Get("/direct/:source/:dest", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string source = req.path_params.at("source");
        std::string dest = req.path_params.at("dest");
        
        Airport source_airport = db.getAirportByIATA(source);
        Airport dest_airport = db.getAirportByIATA(dest);
        
        if (source_airport.id <= 0 || dest_airport.id <= 0) {
            res.status = 404;
            res.set_content("{\"error\":\"Airport not found\",\"routes\":[],\"source\":null,\"dest\":null}", "application/json");
            return;
        }
        
        auto routes = db.getDirectRoutes(source, dest);
        
        std::ostringstream oss;
        oss << "{"
            << "\"source\":" << airportToJSON(source_airport) << ","
            << "\"dest\":" << airportToJSON(dest_airport) << ","
            << "\"routes\":[";
        for (size_t i = 0; i < routes.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{"
                << "\"airline_iata\":\"" << routes[i].airline_iata << "\","
                << "\"airline_name\":\"" << routes[i].airline_name << "\","
                << "\"distance\":" << std::fixed << std::setprecision(2) << routes[i].distance << ","
                << "\"stops\":" << routes[i].stops
                << "}";
        }
        oss << "]}";
        res.set_content(oss.str(), "application/json");
    });
    
    // One-hop route finder (extra credit)
    svr.Get("/onehop/:source/:dest", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string source = req.path_params.at("source");
        std::string dest = req.path_params.at("dest");
        
        // Get source and destination airports
        Airport source_airport = db.getAirportByIATA(source);
        Airport dest_airport = db.getAirportByIATA(dest);
        
        if (source_airport.id <= 0 || dest_airport.id <= 0) {
            res.status = 404;
            res.set_content("{\"error\":\"Airport not found\",\"routes\":[],\"source\":null,\"dest\":null}", "application/json");
            return;
        }
        
        // Find one-hop routes
        auto routes = db.getOneHopRoutes(source, dest);
        
        std::ostringstream oss;
        oss << "{"
            << "\"source\":" << airportToJSON(source_airport) << ","
            << "\"dest\":" << airportToJSON(dest_airport) << ","
            << "\"routes\":[";
        for (size_t i = 0; i < routes.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{"
                << "\"intermediate\":\"" << routes[i].intermediate << "\","
                << "\"airline\":\"" << routes[i].airline << "\","
                << "\"distance\":" << std::fixed << std::setprecision(2) << routes[i].distance
                << "}";
        }
        oss << "]}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Data update endpoints
    // Insert Airline
    svr.Post("/airline/insert", [&db](const httplib::Request& req, httplib::Response& res) {
        Airline airline;
        airline.id = getJSONInt(req.body, "id", 0);
        airline.name = getJSONValue(req.body, "name");
        airline.alias = getJSONValue(req.body, "alias");
        airline.iata = getJSONValue(req.body, "iata");
        airline.icao = getJSONValue(req.body, "icao");
        airline.callsign = getJSONValue(req.body, "callsign");
        airline.country = getJSONValue(req.body, "country");
        airline.active = getJSONValue(req.body, "active");
        if (airline.active.empty()) airline.active = "Y";
        
        auto result = db.insertAirline(airline);
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false") 
            << ",\"message\":\"" << escapeJSON(result.message) << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Update Airline
    svr.Post("/airline/:iata/update", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string iata = req.path_params.at("iata");
        Airline updates;
        updates.id = getJSONInt(req.body, "id", 0);
        updates.name = getJSONValue(req.body, "name");
        updates.alias = getJSONValue(req.body, "alias");
        updates.icao = getJSONValue(req.body, "icao");
        updates.callsign = getJSONValue(req.body, "callsign");
        updates.country = getJSONValue(req.body, "country");
        updates.active = getJSONValue(req.body, "active");
        
        auto result = db.updateAirline(iata, updates);
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false") 
            << ",\"message\":\"" << escapeJSON(result.message) << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Delete Airline
    svr.Delete("/airline/:iata", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string iata = req.path_params.at("iata");
        auto result = db.deleteAirline(iata);
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false") 
            << ",\"message\":\"" << escapeJSON(result.message) << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Insert Airport
    svr.Post("/airport/insert", [&db](const httplib::Request& req, httplib::Response& res) {
        Airport airport;
        airport.id = getJSONInt(req.body, "id", 0);
        airport.name = getJSONValue(req.body, "name");
        airport.city = getJSONValue(req.body, "city");
        airport.country = getJSONValue(req.body, "country");
        airport.iata = getJSONValue(req.body, "iata");
        airport.icao = getJSONValue(req.body, "icao");
        airport.latitude = getJSONDouble(req.body, "latitude", 0.0);
        airport.longitude = getJSONDouble(req.body, "longitude", 0.0);
        airport.altitude = getJSONInt(req.body, "altitude", 0);
        airport.timezone = getJSONDouble(req.body, "timezone", 0.0);
        airport.dst = getJSONValue(req.body, "dst");
        airport.tz = getJSONValue(req.body, "tz");
        airport.type = getJSONValue(req.body, "type");
        airport.source = getJSONValue(req.body, "source");
        
        auto result = db.insertAirport(airport);
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false") 
            << ",\"message\":\"" << escapeJSON(result.message) << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Update Airport
    svr.Post("/airport/:iata/update", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string iata = req.path_params.at("iata");
        Airport updates;
        updates.id = getJSONInt(req.body, "id", 0);
        updates.name = getJSONValue(req.body, "name");
        updates.city = getJSONValue(req.body, "city");
        updates.country = getJSONValue(req.body, "country");
        updates.icao = getJSONValue(req.body, "icao");
        updates.latitude = getJSONDouble(req.body, "latitude", 0.0);
        updates.longitude = getJSONDouble(req.body, "longitude", 0.0);
        updates.altitude = getJSONInt(req.body, "altitude", 0);
        updates.timezone = getJSONDouble(req.body, "timezone", 0.0);
        updates.dst = getJSONValue(req.body, "dst");
        updates.tz = getJSONValue(req.body, "tz");
        updates.type = getJSONValue(req.body, "type");
        updates.source = getJSONValue(req.body, "source");
        
        auto result = db.updateAirport(iata, updates);
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false") 
            << ",\"message\":\"" << escapeJSON(result.message) << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Delete Airport
    svr.Delete("/airport/:iata", [&db](const httplib::Request& req, httplib::Response& res) {
        std::string iata = req.path_params.at("iata");
        auto result = db.deleteAirport(iata);
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false") 
            << ",\"message\":\"" << escapeJSON(result.message) << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Insert Route
    svr.Post("/route/insert", [&db](const httplib::Request& req, httplib::Response& res) {
        Route route;
        route.airline_iata = getJSONValue(req.body, "airline_iata");
        route.airline_id = getJSONInt(req.body, "airline_id", -1);
        route.source_iata = getJSONValue(req.body, "source_iata");
        route.source_id = getJSONInt(req.body, "source_id", -1);
        route.dest_iata = getJSONValue(req.body, "dest_iata");
        route.dest_id = getJSONInt(req.body, "dest_id", -1);
        route.codeshare = getJSONValue(req.body, "codeshare");
        route.stops = getJSONInt(req.body, "stops", 0);
        route.equipment = getJSONValue(req.body, "equipment");
        
        // Look up IDs if not provided
        if (route.airline_id <= 0 && !route.airline_iata.empty()) {
            Airline airline = db.getAirlineByIATA(route.airline_iata);
            if (airline.id > 0) route.airline_id = airline.id;
        }
        if (route.source_id <= 0 && !route.source_iata.empty()) {
            Airport airport = db.getAirportByIATA(route.source_iata);
            if (airport.id > 0) route.source_id = airport.id;
        }
        if (route.dest_id <= 0 && !route.dest_iata.empty()) {
            Airport airport = db.getAirportByIATA(route.dest_iata);
            if (airport.id > 0) route.dest_id = airport.id;
        }
        
        auto result = db.insertRoute(route);
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false") 
            << ",\"message\":\"" << escapeJSON(result.message) << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Update Route
    svr.Post("/route/:id/update", [&db](const httplib::Request& req, httplib::Response& res) {
        int route_id = std::stoi(req.path_params.at("id"));
        Route updates;
        updates.airline_iata = getJSONValue(req.body, "airline_iata");
        updates.source_iata = getJSONValue(req.body, "source_iata");
        updates.dest_iata = getJSONValue(req.body, "dest_iata");
        updates.codeshare = getJSONValue(req.body, "codeshare");
        updates.stops = getJSONInt(req.body, "stops", -1);
        updates.equipment = getJSONValue(req.body, "equipment");
        
        auto result = db.updateRoute(route_id, updates);
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false") 
            << ",\"message\":\"" << escapeJSON(result.message) << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Delete Route
    svr.Delete("/route/:id", [&db](const httplib::Request& req, httplib::Response& res) {
        int route_id = std::stoi(req.path_params.at("id"));
        auto result = db.deleteRoute(route_id);
        std::ostringstream oss;
        oss << "{\"success\":" << (result.success ? "true" : "false") 
            << ",\"message\":\"" << escapeJSON(result.message) << "\"}";
        res.set_content(oss.str(), "application/json");
    });
    
    // Start server - use PORT environment variable if available, otherwise default to 8080
    const char* port_env = std::getenv("PORT");
    int port = port_env ? std::atoi(port_env) : 8080;
    if (port == 0) port = 8080; // Fallback if PORT is invalid
    
    std::cout << "Starting server on http://0.0.0.0:" << port << std::endl;
    svr.listen("0.0.0.0", port);
    
    return 0;
}


