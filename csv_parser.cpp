#include "../include/csv_parser.h"
#include <sstream>
#include <algorithm>

using namespace std;

std::vector<std::string> CSVParser::parseLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                // Escaped quote
                field += '"';
                ++i;
            } else {
                // Toggle quote state
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field); // Last field
    
    return fields;
}

int CSVParser::parseInt(const std::string& s, int defaultVal) {
    if (s.empty() || s == "\\N" || s == "N/A") {
        return defaultVal;
    }
    try {
        return std::stoi(s);
    } catch (...) {
        return defaultVal;
    }
}

double CSVParser::parseDouble(const std::string& s, double defaultVal) {
    if (s.empty() || s == "\\N" || s == "N/A") {
        return defaultVal;
    }
    try {
        return std::stod(s);
    } catch (...) {
        return defaultVal;
    }
}

std::string CSVParser::cleanString(const std::string& s) {
    if (s == "\\N" || s == "N/A") {
        return "";
    }
    return s;
}

Airline CSVParser::parseAirline(const std::string& line) {
    Airline airline;
    auto fields = parseLine(line);
    
    if (fields.size() >= 8) {
        airline.id = parseInt(fields[0]);
        airline.name = cleanString(fields[1]);
        airline.alias = cleanString(fields[2]);
        airline.iata = cleanString(fields[3]);
        airline.icao = cleanString(fields[4]);
        airline.callsign = cleanString(fields[5]);
        airline.country = cleanString(fields[6]);
        airline.active = cleanString(fields[7]);
    }
    
    return airline;
}

Airport CSVParser::parseAirport(const std::string& line) {
    Airport airport;
    auto fields = parseLine(line);
    
    if (fields.size() >= 14) {
        airport.id = parseInt(fields[0]);
        airport.name = cleanString(fields[1]);
        airport.city = cleanString(fields[2]);
        airport.country = cleanString(fields[3]);
        airport.iata = cleanString(fields[4]);
        airport.icao = cleanString(fields[5]);
        airport.latitude = parseDouble(fields[6]);
        airport.longitude = parseDouble(fields[7]);
        airport.altitude = parseInt(fields[8]);
        airport.timezone = parseDouble(fields[9]);
        airport.dst = cleanString(fields[10]);
        airport.tz = cleanString(fields[11]);
        airport.type = cleanString(fields[12]);
        airport.source = cleanString(fields[13]);
    }
    
    return airport;
}

Route CSVParser::parseRoute(const std::string& line) {
    Route route;
    auto fields = parseLine(line);
    
    if (fields.size() >= 9) {
        route.airline_iata = cleanString(fields[0]);
        route.airline_id = parseInt(fields[1]);
        route.source_iata = cleanString(fields[2]);
        route.source_id = parseInt(fields[3]);
        route.dest_iata = cleanString(fields[4]);
        route.dest_id = parseInt(fields[5]);
        route.codeshare = cleanString(fields[6]);
        route.stops = parseInt(fields[7]);
        route.equipment = cleanString(fields[8]);
    }
    
    return route;
}






