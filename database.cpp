#include "../include/database.h"
#include "../include/csv_parser.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <unordered_set>
#include <unordered_map>

using namespace std;

Database::Database() {
}

Database::~Database() {
}

bool Database::loadAirlines(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        Airline airline = CSVParser::parseAirline(line);
        if (airline.id > 0 && !airline.iata.empty()) {
            airlines_by_iata[airline.iata] = airline;
            airlines_by_id[airline.id] = airline;
            airlines_sorted_by_iata[airline.iata] = airline;
        }
    }
    
    file.close();
    return true;
}

bool Database::loadAirports(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        Airport airport = CSVParser::parseAirport(line);
        if (airport.id > 0 && !airport.iata.empty()) {
            airports_by_iata[airport.iata] = airport;
            airports_by_id[airport.id] = airport;
            airports_sorted_by_iata[airport.iata] = airport;
        }
    }
    
    file.close();
    return true;
}

bool Database::loadRoutes(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        Route route = CSVParser::parseRoute(line);
        if (!route.airline_iata.empty() && 
            !route.source_iata.empty() && 
            !route.dest_iata.empty()) {
            routes.push_back(route);
        }
    }
    
    file.close();
    buildIndexes();
    return true;
}

void Database::buildIndexes() {
    // Clear existing indexes
    routes_by_source.clear();
    routes_by_dest.clear();
    routes_by_airline.clear();
    
    // Build indexes
    for (auto& route : routes) {
        routes_by_source[route.source_iata].push_back(&route);
        routes_by_dest[route.dest_iata].push_back(&route);
        routes_by_airline[route.airline_iata].push_back(&route);
    }
}

Airline Database::getAirlineByIATA(const std::string& iata) const {
    auto it = airlines_by_iata.find(iata);
    if (it != airlines_by_iata.end()) {
        return it->second;
    }
    return Airline(); // Returns airline with id=-1 if not found
}

Airport Database::getAirportByIATA(const std::string& iata) const {
    auto it = airports_by_iata.find(iata);
    if (it != airports_by_iata.end()) {
        return it->second;
    }
    return Airport(); // Returns airport with id=-1 if not found
}

std::vector<AirportRouteCount> Database::getAirportsByAirline(const std::string& airline_iata) const {
    std::unordered_map<std::string, int> airport_counts;
    
    // Count routes for each airport
    auto it = routes_by_airline.find(airline_iata);
    if (it != routes_by_airline.end()) {
        for (const Route* route : it->second) {
            airport_counts[route->source_iata]++;
            airport_counts[route->dest_iata]++;
        }
    }
    
    // Build result vector
    std::vector<AirportRouteCount> result;
    for (const auto& pair : airport_counts) {
        Airport airport = getAirportByIATA(pair.first);
        if (airport.id > 0) {
            result.push_back(AirportRouteCount(airport, pair.second));
        }
    }
    
    // Sort by route count (descending)
    std::sort(result.begin(), result.end(), 
        [](const AirportRouteCount& a, const AirportRouteCount& b) {
            return a.route_count > b.route_count;
        });
    
    return result;
}

std::vector<AirlineRouteCount> Database::getAirlinesByAirport(const std::string& airport_iata) const {
    std::unordered_map<std::string, int> airline_counts;
    
    // Count routes for each airline
    auto source_it = routes_by_source.find(airport_iata);
    if (source_it != routes_by_source.end()) {
        for (const Route* route : source_it->second) {
            airline_counts[route->airline_iata]++;
        }
    }
    
    auto dest_it = routes_by_dest.find(airport_iata);
    if (dest_it != routes_by_dest.end()) {
        for (const Route* route : dest_it->second) {
            airline_counts[route->airline_iata]++;
        }
    }
    
    // Build result vector
    std::vector<AirlineRouteCount> result;
    for (const auto& pair : airline_counts) {
        Airline airline = getAirlineByIATA(pair.first);
        if (airline.id > 0) {
            result.push_back(AirlineRouteCount(airline, pair.second));
        }
    }
    
    // Sort by route count (descending)
    std::sort(result.begin(), result.end(), 
        [](const AirlineRouteCount& a, const AirlineRouteCount& b) {
            return a.route_count > b.route_count;
        });
    
    return result;
}

std::vector<Airline> Database::getAllAirlinesSorted() const {
    std::vector<Airline> result;
    for (const auto& pair : airlines_sorted_by_iata) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<Airport> Database::getAllAirportsSorted() const {
    std::vector<Airport> result;
    for (const auto& pair : airports_sorted_by_iata) {
        result.push_back(pair.second);
    }
    return result;
}

std::string Database::getStudentInfo() const {
    // TODO: Replace with your actual student ID and name
    return "Student ID: 20526487, Name: Shreesh Prakash";
}

double Database::calculateDistance(const Airport& a1, const Airport& a2) const {
    // Haversine formula to calculate distance between two GPS coordinates
    const double R = 3959.0; // Earth radius in miles
    double lat1 = a1.latitude * M_PI / 180.0;
    double lat2 = a2.latitude * M_PI / 180.0;
    double dlat = (a2.latitude - a1.latitude) * M_PI / 180.0;
    double dlon = (a2.longitude - a1.longitude) * M_PI / 180.0;
    
    double a = sin(dlat/2) * sin(dlat/2) +
               cos(lat1) * cos(lat2) *
               sin(dlon/2) * sin(dlon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return R * c;
}

std::vector<Database::OneHopRoute> Database::getOneHopRoutes(const std::string& source_iata, const std::string& dest_iata) const {
    std::vector<OneHopRoute> result;
    
    // Get source and destination airports
    Airport source = getAirportByIATA(source_iata);
    Airport dest = getAirportByIATA(dest_iata);
    
    if (source.id <= 0 || dest.id <= 0) {
        return result; // Empty if airports not found
    }
    
    // Find all routes from source airport
    auto source_routes = routes_by_source.find(source_iata);
    if (source_routes == routes_by_source.end()) {
        return result;
    }
    
    // Find all routes to destination airport
    auto dest_routes = routes_by_dest.find(dest_iata);
    if (dest_routes == routes_by_dest.end()) {
        return result;
    }
    
    // Create a set of intermediate airports that have routes TO destination
    std::unordered_set<std::string> intermediate_airports;
    for (const Route* route : dest_routes->second) {
        intermediate_airports.insert(route->source_iata);
    }
    
    // Find routes from source that connect to intermediate airports
    // that also have routes to destination
    std::unordered_map<std::string, std::string> intermediate_to_airline;
    for (const Route* route : source_routes->second) {
        if (intermediate_airports.find(route->dest_iata) != intermediate_airports.end()) {
            // Found a connection: source -> intermediate -> dest
            intermediate_to_airline[route->dest_iata] = route->airline_iata;
        }
    }
    
    // Build result with distances
    for (const auto& pair : intermediate_to_airline) {
        std::string intermediate_iata = pair.first;
        Airport intermediate = getAirportByIATA(intermediate_iata);
        
        if (intermediate.id > 0) {
            OneHopRoute hop;
            hop.intermediate = intermediate_iata;
            hop.airline = pair.second;
            // Calculate total distance: source -> intermediate -> dest
            hop.distance = calculateDistance(source, intermediate) + 
                          calculateDistance(intermediate, dest);
            result.push_back(hop);
        }
    }
    
    // Sort by distance (shortest first)
    std::sort(result.begin(), result.end(), 
        [](const OneHopRoute& a, const OneHopRoute& b) {
            return a.distance < b.distance;
        });
    
    return result;
}

std::vector<Database::DirectRoute> Database::getDirectRoutes(const std::string& source_iata, const std::string& dest_iata) const {
    std::vector<DirectRoute> result;
    
    // Get source and destination airports
    Airport source = getAirportByIATA(source_iata);
    Airport dest = getAirportByIATA(dest_iata);
    
    if (source.id <= 0 || dest.id <= 0) {
        return result;
    }
    
    // Find all routes from source to destination
    auto source_routes = routes_by_source.find(source_iata);
    if (source_routes == routes_by_source.end()) {
        return result;
    }
    
    // Look for direct routes (source -> dest)
    for (const Route* route : source_routes->second) {
        if (route->dest_iata == dest_iata) {
            DirectRoute direct;
            direct.airline_iata = route->airline_iata;
            direct.stops = route->stops;
            
            // Get airline name
            Airline airline = getAirlineByIATA(route->airline_iata);
            direct.airline_name = airline.name.empty() ? route->airline_iata : airline.name;
            
            // Calculate distance
            direct.distance = calculateDistance(source, dest);
            
            result.push_back(direct);
        }
    }
    
    // Sort by distance (shortest first), then by stops
    std::sort(result.begin(), result.end(), 
        [](const DirectRoute& a, const DirectRoute& b) {
            if (a.distance != b.distance) {
                return a.distance < b.distance;
            }
            return a.stops < b.stops;
        });
    
    return result;
}

// Data update operations
int Database::getNextAirlineId() const {
    int max_id = 0;
    for (const auto& pair : airlines_by_id) {
        if (pair.first > max_id) {
            max_id = pair.first;
        }
    }
    return max_id + 1;
}

int Database::getNextAirportId() const {
    int max_id = 0;
    for (const auto& pair : airports_by_id) {
        if (pair.first > max_id) {
            max_id = pair.first;
        }
    }
    return max_id + 1;
}

int Database::getNextRouteId() const {
    return static_cast<int>(routes.size());
}

Database::UpdateResult Database::insertAirline(const Airline& airline) {
    UpdateResult result;
    
    if (!airline.iata.empty() && airlines_by_iata.find(airline.iata) != airlines_by_iata.end()) {
        result.success = false;
        result.message = "Airline with IATA code " + airline.iata + " already exists";
        return result;
    }
    
    if (airline.id > 0 && airlines_by_id.find(airline.id) != airlines_by_id.end()) {
        result.success = false;
        result.message = "Airline with ID " + std::to_string(airline.id) + " already exists";
        return result;
    }
    
    Airline new_airline = airline;
    if (new_airline.id <= 0) {
        new_airline.id = getNextAirlineId();
    }
    
    airlines_by_iata[new_airline.iata] = new_airline;
    airlines_by_id[new_airline.id] = new_airline;
    airlines_sorted_by_iata[new_airline.iata] = new_airline;
    
    result.success = true;
    result.message = "Airline inserted successfully with ID " + std::to_string(new_airline.id);
    return result;
}

Database::UpdateResult Database::updateAirline(const std::string& iata, const Airline& updates) {
    UpdateResult result;
    
    auto it = airlines_by_iata.find(iata);
    if (it == airlines_by_iata.end()) {
        result.success = false;
        result.message = "Airline with IATA code " + iata + " not found";
        return result;
    }
    
    Airline& existing = it->second;
    
    if (updates.id > 0 && updates.id != existing.id) {
        result.success = false;
        result.message = "Cannot change airline ID";
        return result;
    }
    
    if (!updates.iata.empty() && updates.iata != iata) {
        result.success = false;
        result.message = "Cannot change airline IATA code";
        return result;
    }
    
    if (!updates.name.empty()) existing.name = updates.name;
    if (!updates.alias.empty()) existing.alias = updates.alias;
    if (!updates.icao.empty()) existing.icao = updates.icao;
    if (!updates.callsign.empty()) existing.callsign = updates.callsign;
    if (!updates.country.empty()) existing.country = updates.country;
    if (!updates.active.empty()) existing.active = updates.active;
    
    airlines_by_id[existing.id] = existing;
    airlines_sorted_by_iata[iata] = existing;
    
    result.success = true;
    result.message = "Airline updated successfully";
    return result;
}

Database::UpdateResult Database::deleteAirline(const std::string& iata) {
    UpdateResult result;
    
    auto it = airlines_by_iata.find(iata);
    if (it == airlines_by_iata.end()) {
        result.success = false;
        result.message = "Airline with IATA code " + iata + " not found";
        return result;
    }
    
    int airline_id = it->second.id;
    
    auto route_it = routes.begin();
    while (route_it != routes.end()) {
        if (route_it->airline_iata == iata) {
            route_it = routes.erase(route_it);
        } else {
            ++route_it;
        }
    }
    
    rebuildIndexes();
    
    airlines_by_iata.erase(it);
    airlines_by_id.erase(airline_id);
    airlines_sorted_by_iata.erase(iata);
    
    result.success = true;
    result.message = "Airline and all its routes deleted successfully";
    return result;
}

Database::UpdateResult Database::insertAirport(const Airport& airport) {
    UpdateResult result;
    
    if (!airport.iata.empty() && airports_by_iata.find(airport.iata) != airports_by_iata.end()) {
        result.success = false;
        result.message = "Airport with IATA code " + airport.iata + " already exists";
        return result;
    }
    
    if (airport.id > 0 && airports_by_id.find(airport.id) != airports_by_id.end()) {
        result.success = false;
        result.message = "Airport with ID " + std::to_string(airport.id) + " already exists";
        return result;
    }
    
    Airport new_airport = airport;
    if (new_airport.id <= 0) {
        new_airport.id = getNextAirportId();
    }
    
    airports_by_iata[new_airport.iata] = new_airport;
    airports_by_id[new_airport.id] = new_airport;
    airports_sorted_by_iata[new_airport.iata] = new_airport;
    
    result.success = true;
    result.message = "Airport inserted successfully with ID " + std::to_string(new_airport.id);
    return result;
}

Database::UpdateResult Database::updateAirport(const std::string& iata, const Airport& updates) {
    UpdateResult result;
    
    auto it = airports_by_iata.find(iata);
    if (it == airports_by_iata.end()) {
        result.success = false;
        result.message = "Airport with IATA code " + iata + " not found";
        return result;
    }
    
    Airport& existing = it->second;
    
    if (updates.id > 0 && updates.id != existing.id) {
        result.success = false;
        result.message = "Cannot change airport ID";
        return result;
    }
    
    if (!updates.iata.empty() && updates.iata != iata) {
        result.success = false;
        result.message = "Cannot change airport IATA code";
        return result;
    }
    
    if (!updates.name.empty()) existing.name = updates.name;
    if (!updates.city.empty()) existing.city = updates.city;
    if (!updates.country.empty()) existing.country = updates.country;
    if (!updates.icao.empty()) existing.icao = updates.icao;
    if (updates.latitude != 0.0) existing.latitude = updates.latitude;
    if (updates.longitude != 0.0) existing.longitude = updates.longitude;
    if (updates.altitude != 0) existing.altitude = updates.altitude;
    if (updates.timezone != 0.0) existing.timezone = updates.timezone;
    if (!updates.dst.empty()) existing.dst = updates.dst;
    if (!updates.tz.empty()) existing.tz = updates.tz;
    if (!updates.type.empty()) existing.type = updates.type;
    if (!updates.source.empty()) existing.source = updates.source;
    
    airports_by_id[existing.id] = existing;
    airports_sorted_by_iata[iata] = existing;
    
    result.success = true;
    result.message = "Airport updated successfully";
    return result;
}

Database::UpdateResult Database::deleteAirport(const std::string& iata) {
    UpdateResult result;
    
    auto it = airports_by_iata.find(iata);
    if (it == airports_by_iata.end()) {
        result.success = false;
        result.message = "Airport with IATA code " + iata + " not found";
        return result;
    }
    
    int airport_id = it->second.id;
    
    auto route_it = routes.begin();
    while (route_it != routes.end()) {
        if (route_it->source_iata == iata || route_it->dest_iata == iata) {
            route_it = routes.erase(route_it);
        } else {
            ++route_it;
        }
    }
    
    rebuildIndexes();
    
    airports_by_iata.erase(it);
    airports_by_id.erase(airport_id);
    airports_sorted_by_iata.erase(iata);
    
    result.success = true;
    result.message = "Airport and all routes to/from it deleted successfully";
    return result;
}

Database::UpdateResult Database::insertRoute(const Route& route) {
    UpdateResult result;
    
    if (airlines_by_iata.find(route.airline_iata) == airlines_by_iata.end()) {
        result.success = false;
        result.message = "Airline with IATA code " + route.airline_iata + " does not exist";
        return result;
    }
    
    if (airports_by_iata.find(route.source_iata) == airports_by_iata.end()) {
        result.success = false;
        result.message = "Source airport with IATA code " + route.source_iata + " does not exist";
        return result;
    }
    
    if (airports_by_iata.find(route.dest_iata) == airports_by_iata.end()) {
        result.success = false;
        result.message = "Destination airport with IATA code " + route.dest_iata + " does not exist";
        return result;
    }
    
    for (const auto& existing_route : routes) {
        if (existing_route.airline_iata == route.airline_iata &&
            existing_route.source_iata == route.source_iata &&
            existing_route.dest_iata == route.dest_iata) {
            result.success = false;
            result.message = "Route already exists";
            return result;
        }
    }
    
    routes.push_back(route);
    rebuildIndexes();
    
    result.success = true;
    result.message = "Route inserted successfully";
    return result;
}

Database::UpdateResult Database::updateRoute(int route_id, const Route& updates) {
    UpdateResult result;
    
    if (route_id < 0 || route_id >= static_cast<int>(routes.size())) {
        result.success = false;
        result.message = "Route ID " + std::to_string(route_id) + " not found";
        return result;
    }
    
    Route& existing = routes[route_id];
    
    if (!updates.airline_iata.empty() && updates.airline_iata != existing.airline_iata) {
        if (airlines_by_iata.find(updates.airline_iata) == airlines_by_iata.end()) {
            result.success = false;
            result.message = "Airline with IATA code " + updates.airline_iata + " does not exist";
            return result;
        }
        existing.airline_iata = updates.airline_iata;
        existing.airline_id = airlines_by_iata[updates.airline_iata].id;
    }
    
    if (!updates.source_iata.empty() && updates.source_iata != existing.source_iata) {
        if (airports_by_iata.find(updates.source_iata) == airports_by_iata.end()) {
            result.success = false;
            result.message = "Source airport with IATA code " + updates.source_iata + " does not exist";
            return result;
        }
        existing.source_iata = updates.source_iata;
        existing.source_id = airports_by_iata[updates.source_iata].id;
    }
    
    if (!updates.dest_iata.empty() && updates.dest_iata != existing.dest_iata) {
        if (airports_by_iata.find(updates.dest_iata) == airports_by_iata.end()) {
            result.success = false;
            result.message = "Destination airport with IATA code " + updates.dest_iata + " does not exist";
            return result;
        }
        existing.dest_iata = updates.dest_iata;
        existing.dest_id = airports_by_iata[updates.dest_iata].id;
    }
    
    if (!updates.codeshare.empty()) existing.codeshare = updates.codeshare;
    if (updates.stops >= 0) existing.stops = updates.stops;
    if (!updates.equipment.empty()) existing.equipment = updates.equipment;
    
    rebuildIndexes();
    
    result.success = true;
    result.message = "Route updated successfully";
    return result;
}

Database::UpdateResult Database::deleteRoute(int route_id) {
    UpdateResult result;
    
    if (route_id < 0 || route_id >= static_cast<int>(routes.size())) {
        result.success = false;
        result.message = "Route ID " + std::to_string(route_id) + " not found";
        return result;
    }
    
    routes.erase(routes.begin() + route_id);
    rebuildIndexes();
    
    result.success = true;
    result.message = "Route deleted successfully";
    return result;
}

void Database::rebuildIndexes() {
    buildIndexes();
}

