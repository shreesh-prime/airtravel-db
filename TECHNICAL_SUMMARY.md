# Complete Technical Summary: Air Travel Database Project

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Data Structures](#data-structures)
4. [Data Loading Process](#data-loading-process)
5. [How Searches Work](#how-searches-work)
6. [How Routes Are Found](#how-routes-are-found)
7. [API Endpoints](#api-endpoints)
8. [Frontend Implementation](#frontend-implementation)
9. [Deployment](#deployment)

---

## Project Overview

This is a C++ web application that provides a front-end interface for querying airline, airport, and route data from the OpenFlights database. The application uses efficient data structures for fast lookups and serves data via RESTful API endpoints through an HTTP server.

**Technology Stack:**
- **Backend:** C++17 with cpp-httplib HTTP server
- **Frontend:** HTML, CSS, JavaScript (embedded in C++)
- **Build System:** CMake
- **Deployment:** Docker on Render.com

---

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Web Browser                          │
│  (HTML/CSS/JavaScript - Leaflet.js for maps)           │
└────────────────────┬────────────────────────────────────┘
                     │ HTTP Requests/Responses
                     │ (JSON data)
┌────────────────────▼────────────────────────────────────┐
│              HTTP Server (cpp-httplib)                  │
│              Port: 8080 (or PORT env var)               │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│              Database Class (C++)                       │
│  - Hash Maps (O(1) lookups)                            │
│  - Indexes for route queries                            │
│  - In-memory data storage                               │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│           CSV Parser (C++)                              │
│  - Parses airlines.dat                                  │
│  - Parses airports.dat                                  │
│  - Parses routes.dat                                    │
└─────────────────────────────────────────────────────────┘
```

### File Structure

```
air-travel-db/
├── src/
│   ├── main.cpp          # HTTP server + frontend HTML/JS
│   ├── database.cpp      # Database logic & route finding
│   └── csv_parser.cpp    # CSV file parsing
├── include/
│   ├── database.h        # Database class interface
│   ├── csv_parser.h      # CSV parser interface
│   └── models.h          # Data structures (Airline, Airport, Route)
├── *.dat                 # Data files (airlines, airports, routes)
├── CMakeLists.txt        # Build configuration
├── Dockerfile            # Container definition
└── build.sh              # Build script
```

---

## Data Structures

### Core Data Models

#### 1. Airline Structure
```cpp
struct Airline {
    int id;                    // OpenFlights Airline ID
    std::string name;          // Airline name
    std::string alias;         // Airline alias
    std::string iata;          // IATA code (2-letter, e.g., "AA")
    std::string icao;          // ICAO code (3-letter)
    std::string callsign;      // Airline callsign
    std::string country;       // Country
    std::string active;        // Active status (Y/N)
};
```

#### 2. Airport Structure
```cpp
struct Airport {
    int id;                    // OpenFlights Airport ID
    std::string name;          // Airport name
    std::string city;          // City
    std::string country;       // Country
    std::string iata;          // IATA code (3-letter, e.g., "SFO")
    std::string icao;          // ICAO code (4-letter)
    double latitude;           // GPS latitude
    double longitude;          // GPS longitude
    int altitude;              // Altitude in feet
    double timezone;           // Timezone offset
    std::string dst;           // DST code
    std::string tz;            // Timezone name
    std::string type;          // Airport type
    std::string source;        // Data source
};
```

#### 3. Route Structure
```cpp
struct Route {
    std::string airline_iata;  // Airline IATA code
    int airline_id;            // Airline OpenFlights ID
    std::string source_iata;   // Source airport IATA
    int source_id;             // Source airport ID
    std::string dest_iata;     // Destination airport IATA
    int dest_id;               // Destination airport ID
    std::string codeshare;     // Codeshare status
    int stops;                 // Number of stops (0 = direct)
    std::string equipment;     // Aircraft type
};
```

### Database Storage Structures

The `Database` class uses multiple data structures for efficient access:

#### Hash Maps for O(1) Lookups
```cpp
// Primary lookup by IATA code (fastest access)
std::unordered_map<std::string, Airline> airlines_by_iata;
std::unordered_map<std::string, Airport> airports_by_iata;

// Lookup by ID (for route resolution)
std::unordered_map<int, Airline> airlines_by_id;
std::unordered_map<int, Airport> airports_by_id;
```

**Why Hash Maps?**
- O(1) average case lookup time
- Perfect for frequent searches by IATA code
- Example: Finding "AA" (American Airlines) is instant

#### Indexes for Route Queries
```cpp
// Routes indexed by source airport
std::unordered_map<std::string, std::vector<Route*>> routes_by_source;
// Example: routes_by_source["SFO"] = [all routes FROM SFO]

// Routes indexed by destination airport
std::unordered_map<std::string, std::vector<Route*>> routes_by_dest;
// Example: routes_by_dest["JFK"] = [all routes TO JFK]

// Routes indexed by airline
std::unordered_map<std::string, std::vector<Route*>> routes_by_airline;
// Example: routes_by_airline["AA"] = [all routes by American Airlines]
```

**Why These Indexes?**
- Enables fast route queries without scanning all routes
- Finding "all routes from SFO" is O(1) lookup + O(k) where k = routes from SFO
- Without indexes, would need O(n) scan of all routes

#### Sorted Collections for Reports
```cpp
// Sorted by IATA code for ordered reports
std::map<std::string, Airline> airlines_sorted_by_iata;
std::map<std::string, Airport> airports_sorted_by_iata;
```

**Why Ordered Maps?**
- Automatically sorted by IATA code
- Efficient for generating sorted reports
- O(log n) insertion, O(n) iteration in sorted order

#### Vector for All Routes
```cpp
std::vector<Route> routes;  // All routes stored sequentially
```

**Why Vector?**
- Simple sequential storage
- Efficient iteration when needed
- Indexes point to Route objects in this vector

---

## Data Loading Process

### Step 1: CSV File Parsing

The application reads three `.dat` files from OpenFlights:

1. **airlines.dat** - Contains airline information
2. **airports.dat** - Contains airport information  
3. **routes.dat** - Contains route information

#### CSV Parser Implementation

The `CSVParser` class handles the complex CSV format:

```cpp
// Handles quoted fields, escaped quotes, and null values
std::vector<std::string> CSVParser::parseLine(const std::string& line) {
    // Parses CSV line handling:
    // - Quoted fields: "American Airlines"
    // - Escaped quotes: "He said ""Hello"""
    // - Null values: \N
    // - Commas within quoted fields
}
```

**Example CSV Line:**
```
24,"American Airlines",\N,"AA","AAL","AMERICAN","United States","Y"
```

**Parsed into:**
- ID: 24
- Name: "American Airlines"
- Alias: null
- IATA: "AA"
- ICAO: "AAL"
- Callsign: "AMERICAN"
- Country: "United States"
- Active: "Y"

### Step 2: Data Loading into Database

#### Loading Airlines
```cpp
bool Database::loadAirlines(const std::string& filename) {
    // 1. Open file
    // 2. Read each line
    // 3. Parse CSV line into Airline struct
    // 4. Store in THREE places:
    //    - airlines_by_iata[airline.iata] = airline  (for IATA lookup)
    //    - airlines_by_id[airline.id] = airline      (for ID lookup)
    //    - airlines_sorted_by_iata[airline.iata] = airline  (for sorted reports)
}
```

**Result:** ~6,000+ airlines loaded into hash maps

#### Loading Airports
```cpp
bool Database::loadAirports(const std::string& filename) {
    // Similar process:
    // 1. Parse each line
    // 2. Store in airports_by_iata, airports_by_id, airports_sorted_by_iata
}
```

**Result:** ~7,000+ airports loaded with GPS coordinates

#### Loading Routes
```cpp
bool Database::loadRoutes(const std::string& filename) {
    // 1. Parse each route line
    // 2. Store in routes vector
    // 3. Build indexes:
    //    - routes_by_source[route.source_iata].push_back(&route)
    //    - routes_by_dest[route.dest_iata].push_back(&route)
    //    - routes_by_airline[route.airline_iata].push_back(&route)
}
```

**Result:** ~67,000+ routes loaded and indexed

### Step 3: Index Building

After loading routes, indexes are built:

```cpp
void Database::buildIndexes() {
    // For each route:
    // 1. Add pointer to routes_by_source[source_iata]
    // 2. Add pointer to routes_by_dest[dest_iata]
    // 3. Add pointer to routes_by_airline[airline_iata]
}
```

**Why Pointers?**
- Saves memory (don't duplicate Route objects)
- Updates to routes reflect in all indexes
- Fast access without copying

---

## How Searches Work

### 1. Finding an Airline by IATA Code

**User Action:** Types "AA" in search box

**Process:**
```cpp
// Frontend JavaScript
const response = await fetch('/airline/AA');
const airline = await response.json();

// Backend C++ (main.cpp)
svr.Get("/airline/:iata", [&db](const httplib::Request& req, httplib::Response& res) {
    std::string iata = req.path_params.at("iata");  // "AA"
    Airline airline = db.getAirlineByIATA(iata);    // O(1) hash map lookup
    
    // Convert to JSON and return
    res.set_content(airlineToJSON(airline), "application/json");
});

// Database lookup (database.cpp)
Airline Database::getAirlineByIATA(const std::string& iata) const {
    auto it = airlines_by_iata.find(iata);  // O(1) hash map lookup
    if (it != airlines_by_iata.end()) {
        return it->second;  // Found!
    }
    return Airline();  // Not found (returns Airline with id = -1)
}
```

**Time Complexity:** O(1) - Constant time lookup

### 2. Finding an Airport by IATA Code

**User Action:** Types "SFO" in search box

**Process:**
```cpp
// Similar to airline lookup
Airport Database::getAirportByIATA(const std::string& iata) const {
    auto it = airports_by_iata.find(iata);  // O(1) lookup
    return (it != airports_by_iata.end()) ? it->second : Airport();
}
```

**Time Complexity:** O(1)

### 3. Autocomplete Dropdown Search

**User Action:** Types "san" in airport search field

**Frontend Process:**
```javascript
async function filterAirports(query) {
    // 1. Call server-side search endpoint
    const response = await fetch('/airports/search?q=san');
    const filtered = await response.json();
    
    // 2. Sort results (exact match first, then starts with, etc.)
    filtered.sort((a, b) => {
        // Exact IATA match first: "SAN" before "SFO"
        // Then IATA starts with: "SAN" before "SAT"
        // Then name starts with: "San Diego" before "San Francisco"
    });
    
    // 3. Display top 15 results in dropdown
}
```

**Backend Process:**
```cpp
svr.Get("/airports/search", [&db](const httplib::Request& req, httplib::Response& res) {
    std::string query = req.get_param_value("q");  // "san"
    
    // 1. Get all airports (sorted by IATA)
    auto allAirports = db.getAllAirportsSorted();
    
    // 2. Filter matching airports
    for (const auto& airport : allAirports) {
        // Check if query matches IATA code or name
        if (iata.find("san") != std::string::npos ||
            name.find("san") != std::string::npos) {
            results.push_back(airport);
        }
    }
    
    // 3. Sort by relevance (exact match first)
    std::sort(results.begin(), results.end(), [&query](...) {
        // Exact IATA match first
        // Then IATA starts with query
        // Then name starts with query
    });
    
    // 4. Limit to top 20 and return JSON
});
```

**Results for "san":**
1. **SAN** - San Diego International Airport (exact IATA match)
2. **SFO** - San Francisco International Airport (name contains "San")
3. **SAT** - San Antonio International Airport (name contains "San")

**Time Complexity:** O(n) where n = number of airports (but limited to 20 results)

---

## How Routes Are Found

### 1. Direct Routes (Non-Stop Flights)

**User Action:** Searches for routes from SFO to JFK

**Process:**
```cpp
std::vector<Database::DirectRoute> Database::getDirectRoutes(
    const std::string& source_iata,  // "SFO"
    const std::string& dest_iata     // "JFK"
) const {
    // 1. Get source and destination airports
    Airport source = getAirportByIATA(source_iata);  // O(1)
    Airport dest = getAirportByIATA(dest_iata);      // O(1)
    
    // 2. Find all routes FROM source airport (using index)
    auto source_routes = routes_by_source.find(source_iata);  // O(1)
    // This gives us a vector of Route* pointers
    
    // 3. Filter routes that go TO destination
    for (const Route* route : source_routes->second) {
        if (route->dest_iata == dest_iata) {  // Found direct route!
            DirectRoute direct;
            direct.airline_iata = route->airline_iata;
            direct.airline_name = getAirlineByIATA(route->airline_iata).name;
            direct.stops = route->stops;
            
            // 4. Calculate distance using GPS coordinates
            direct.distance = calculateDistance(source, dest);
            
            result.push_back(direct);
        }
    }
    
    // 5. Sort by distance (shortest first)
    std::sort(result.begin(), result.end(), 
        [](const DirectRoute& a, const DirectRoute& b) {
            return a.distance < b.distance;
        });
    
    return result;
}
```

**Distance Calculation (Haversine Formula):**
```cpp
double Database::calculateDistance(const Airport& a1, const Airport& a2) const {
    // Haversine formula calculates great-circle distance between two GPS points
    const double R = 3959.0; // Earth radius in miles
    
    // Convert degrees to radians
    double lat1 = a1.latitude * M_PI / 180.0;
    double lat2 = a2.latitude * M_PI / 180.0;
    double dlat = (a2.latitude - a1.latitude) * M_PI / 180.0;
    double dlon = (a2.longitude - a1.longitude) * M_PI / 180.0;
    
    // Haversine formula
    double a = sin(dlat/2) * sin(dlat/2) +
               cos(lat1) * cos(lat2) *
               sin(dlon/2) * sin(dlon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return R * c;  // Distance in miles
}
```

**Example Result:**
- **AA (American Airlines)**: SFO → JFK, 2,585 miles, 0 stops
- **UA (United Airlines)**: SFO → JFK, 2,585 miles, 0 stops
- **DL (Delta)**: SFO → JFK, 2,585 miles, 0 stops

**Time Complexity:** 
- O(1) to find routes from source (hash map lookup)
- O(k) where k = number of routes from source (typically small, < 100)
- O(m log m) to sort m results
- **Overall:** O(k + m log m) where k << total routes

### 2. One-Hop Routes (Connecting Flights)

**User Action:** Searches for one-hop routes from SFO to LHR (London)

**Process:**
```cpp
std::vector<Database::OneHopRoute> Database::getOneHopRoutes(
    const std::string& source_iata,  // "SFO"
    const std::string& dest_iata     // "LHR"
) const {
    // 1. Get source and destination airports
    Airport source = getAirportByIATA(source_iata);
    Airport dest = getAirportByIATA(dest_iata);
    
    // 2. Find all routes FROM source (e.g., all routes from SFO)
    auto source_routes = routes_by_source.find(source_iata);
    
    // 3. Find all routes TO destination (e.g., all routes to LHR)
    auto dest_routes = routes_by_dest.find(dest_iata);
    
    // 4. Create set of intermediate airports that have routes TO destination
    std::unordered_set<std::string> intermediate_airports;
    for (const Route* route : dest_routes->second) {
        // If there's a route JFK → LHR, then JFK is an intermediate
        intermediate_airports.insert(route->source_iata);
    }
    
    // 5. Find routes from source that connect to intermediate airports
    for (const Route* route : source_routes->second) {
        // If route is SFO → JFK, and JFK is in intermediate_airports
        if (intermediate_airports.find(route->dest_iata) != intermediate_airports.end()) {
            // Found a connection! SFO → JFK → LHR
            
            OneHopRoute hop;
            hop.intermediate = route->dest_iata;  // "JFK"
            hop.airline = route->airline_iata;    // "AA"
            
            // 6. Calculate total distance
            Airport intermediate = getAirportByIATA(route->dest_iata);
            hop.distance = calculateDistance(source, intermediate) +  // SFO → JFK
                          calculateDistance(intermediate, dest);      // JFK → LHR
            
            result.push_back(hop);
        }
    }
    
    // 7. Sort by total distance (shortest first)
    std::sort(result.begin(), result.end(), 
        [](const OneHopRoute& a, const OneHopRoute& b) {
            return a.distance < b.distance;
        });
    
    return result;
}
```

**Example Result:**
- **AA**: SFO → JFK → LHR, 5,345 miles total
- **UA**: SFO → EWR → LHR, 5,412 miles total
- **DL**: SFO → ATL → LHR, 5,678 miles total

**Time Complexity:**
- O(1) to find routes from source
- O(1) to find routes to destination
- O(k) to build intermediate set where k = routes to destination
- O(m) to check connections where m = routes from source
- O(n log n) to sort n results
- **Overall:** O(k + m + n log n) - Very efficient!

**Why This is Fast:**
- Uses hash map indexes (O(1) lookups)
- Only examines routes from source and to destination
- Doesn't scan all 67,000+ routes

### 3. Airline Routes Report

**User Action:** Requests all airports served by American Airlines (AA)

**Process:**
```cpp
std::vector<AirportRouteCount> Database::getAirportsByAirline(
    const std::string& airline_iata  // "AA"
) const {
    // 1. Find all routes by this airline (using index)
    auto airline_routes = routes_by_airline.find(airline_iata);  // O(1)
    
    // 2. Count routes per destination airport
    std::unordered_map<std::string, int> airport_counts;
    for (const Route* route : airline_routes->second) {
        airport_counts[route->dest_iata]++;  // Count routes to each airport
    }
    
    // 3. Build result with airport info and route counts
    std::vector<AirportRouteCount> result;
    for (const auto& pair : airport_counts) {
        Airport airport = getAirportByIATA(pair.first);  // O(1)
        result.push_back(AirportRouteCount(airport, pair.second));
    }
    
    // 4. Sort by route count (descending)
    std::sort(result.begin(), result.end(),
        [](const AirportRouteCount& a, const AirportRouteCount& b) {
            return a.route_count > b.route_count;
        });
    
    return result;
}
```

**Example Result:**
1. **DFW** (Dallas/Fort Worth) - 150 routes
2. **ORD** (Chicago O'Hare) - 120 routes
3. **LAX** (Los Angeles) - 95 routes
...

**Time Complexity:** O(k + m log m) where k = routes by airline, m = unique airports

### 4. Airport Airlines Report

**User Action:** Requests all airlines serving SFO

**Process:**
```cpp
std::vector<AirlineRouteCount> Database::getAirlinesByAirport(
    const std::string& airport_iata  // "SFO"
) const {
    // 1. Find all routes TO this airport (using index)
    auto dest_routes = routes_by_dest.find(airport_iata);  // O(1)
    
    // 2. Count routes per airline
    std::unordered_map<std::string, int> airline_counts;
    for (const Route* route : dest_routes->second) {
        airline_counts[route->airline_iata]++;  // Count routes by each airline
    }
    
    // 3. Build result with airline info and route counts
    std::vector<AirlineRouteCount> result;
    for (const auto& pair : airline_counts) {
        Airline airline = getAirlineByIATA(pair.first);  // O(1)
        result.push_back(AirlineRouteCount(airline, pair.second));
    }
    
    // 4. Sort by route count (descending)
    std::sort(result.begin(), result.end(),
        [](const AirlineRouteCount& a, const AirlineRouteCount& b) {
            return a.route_count > b.route_count;
        });
    
    return result;
}
```

**Time Complexity:** O(k + m log m) where k = routes to airport, m = unique airlines

---

## API Endpoints

### Basic Entity Retrieval

#### GET /airline/{iata}
- **Purpose:** Get airline information by IATA code
- **Example:** `GET /airline/AA`
- **Returns:** JSON object with airline details
- **Time Complexity:** O(1) - Hash map lookup

#### GET /airport/{iata}
- **Purpose:** Get airport information by IATA code
- **Example:** `GET /airport/SFO`
- **Returns:** JSON object with airport details including GPS coordinates
- **Time Complexity:** O(1) - Hash map lookup

#### GET /airlines
- **Purpose:** Get all airlines sorted by IATA code
- **Returns:** JSON array of all airlines
- **Time Complexity:** O(n) where n = number of airlines

#### GET /airports
- **Purpose:** Get all airports sorted by IATA code
- **Returns:** JSON array of all airports
- **Time Complexity:** O(n) where n = number of airports

### Reports

#### GET /airline/{iata}/routes
- **Purpose:** Get all airports served by an airline, ordered by route count
- **Example:** `GET /airline/AA/routes`
- **Returns:** JSON array of airports with route counts
- **Time Complexity:** O(k + m log m) where k = routes, m = airports

#### GET /airport/{iata}/airlines
- **Purpose:** Get all airlines serving an airport, ordered by route count
- **Example:** `GET /airport/SFO/airlines`
- **Returns:** JSON array of airlines with route counts
- **Time Complexity:** O(k + m log m) where k = routes, m = airlines

### Advanced Route Finding

#### GET /direct/{source}/{dest}
- **Purpose:** Find all direct (non-stop) routes between two airports
- **Example:** `GET /direct/SFO/JFK`
- **Returns:** JSON array of direct routes with distances
- **Process:**
  1. Look up routes from source using `routes_by_source` index
  2. Filter routes where destination matches
  3. Calculate distance using Haversine formula
  4. Sort by distance
- **Time Complexity:** O(k + m log m) where k = routes from source, m = results

#### GET /onehop/{source}/{dest}
- **Purpose:** Find all one-hop (connecting) routes between two airports
- **Example:** `GET /onehop/SFO/LHR`
- **Returns:** JSON array of one-hop routes with intermediate airports and total distances
- **Process:**
  1. Find all routes FROM source (using index)
  2. Find all routes TO destination (using index)
  3. Find intermediate airports that connect source to destination
  4. Calculate total distance (source → intermediate → destination)
  5. Sort by total distance
- **Time Complexity:** O(k + m + n log n) where k = routes to dest, m = routes from source, n = results

### Search Endpoints

#### GET /airports/search?q={query}
- **Purpose:** Search airports by IATA code or name (for autocomplete)
- **Example:** `GET /airports/search?q=san`
- **Returns:** JSON array of up to 20 matching airports, sorted by relevance
- **Process:**
  1. Get all airports
  2. Filter by IATA code or name containing query
  3. Sort: exact IATA match first, then IATA starts with, then name starts with
  4. Limit to 20 results
- **Time Complexity:** O(n) where n = number of airports

### Data Update Endpoints (Extra Credit)

#### POST /airline/add
- **Purpose:** Add a new airline
- **Body:** JSON with airline details
- **Process:** Validates, generates ID, adds to all data structures

#### POST /airline/update
- **Purpose:** Update an existing airline
- **Body:** JSON with fields to update
- **Process:** Updates in-place, maintains all indexes

#### DELETE /airline/{iata}
- **Purpose:** Delete an airline and all its routes
- **Process:** Removes from all data structures, deletes associated routes, rebuilds indexes

Similar endpoints exist for airports and routes.

### Utility Endpoints

#### GET /student
- **Purpose:** Return student ID and name
- **Returns:** JSON with student information

#### GET /code
- **Purpose:** Return source code (extra credit)
- **Returns:** JSON with escaped source code

#### GET /airports/top?limit={n}
- **Purpose:** Get top airports by traffic (route count)
- **Process:** Calculates route counts for all airports, sorts, returns top N

#### GET /airports/geographic
- **Purpose:** Get geographic statistics (countries by airport count)
- **Process:** Counts airports per country, sorts, returns top 20

#### GET /airlines/list?page={n}&size={s}
- **Purpose:** Get paginated list of airlines
- **Returns:** JSON with total, page info, and airlines array

#### GET /airports/list?page={n}&size={s}
- **Purpose:** Get paginated list of airports
- **Returns:** JSON with total, page info, and airports array

---

## Frontend Implementation

### Architecture

The frontend is embedded directly in the C++ code as a raw string literal in `main.cpp`. This includes:
- Complete HTML structure
- CSS styling (embedded in `<style>` tags)
- JavaScript functionality (embedded in `<script>` tags)
- Leaflet.js integration for maps (loaded from CDN)

### Key Frontend Features

#### 1. Tabbed Navigation
```javascript
function showTab(tabName, button) {
    // Hide all tab contents
    // Show selected tab
    // Update active button styling
}
```

#### 2. Autocomplete Dropdowns
```javascript
async function filterAirports(query) {
    // 1. Call /airports/search?q={query}
    // 2. Receive filtered results
    // 3. Sort by relevance
    // 4. Display in dropdown
    // 5. Handle click to select
}
```

**Features:**
- Real-time filtering as user types
- Server-side search for performance
- Sorted by relevance (exact match first)
- Limited to 15 visible results

#### 3. Interactive Maps (Leaflet.js)

**Direct Routes Map:**
```javascript
function displayDirectRoutes(routes, source, dest) {
    // 1. Initialize Leaflet map
    // 2. Add source airport marker (blue)
    // 3. Add destination airport marker (red)
    // 4. For each route:
    //    - Draw line from source to destination
    //    - Color by airline
    //    - Add popup with route details
}
```

**One-Hop Routes Map:**
```javascript
function displayOneHopRoutes(routes, source, dest) {
    // 1. Initialize map
    // 2. Add source marker
    // 3. Add destination marker
    // 4. For each one-hop route:
    //    - Add intermediate airport marker (yellow)
    //    - Draw line: source → intermediate (blue)
    //    - Draw line: intermediate → destination (green)
    //    - Add popup with route details and total distance
}
```

#### 4. Data Display

Results are displayed in formatted HTML cards:
- Airline/Airport information
- Route counts
- Geographic information
- Interactive elements (maps, buttons)

#### 5. Pagination

For large datasets (all airlines, all airports):
```javascript
async function getAllAirlines(page = 1) {
    const response = await fetch(`/airlines/list?page=${page}&size=100`);
    const data = await response.json();
    
    // Display airlines
    // Show pagination controls (Previous/Next)
}
```

---

## Deployment

### Docker Containerization

The application is containerized using Docker:

```dockerfile
# Base image: Ubuntu 22.04
FROM ubuntu:22.04

# Install build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git

# Copy source code
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
COPY *.dat ./

# Build application
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make

# Run application
CMD ["./air_travel_db"]
```

### Render.com Deployment

1. **GitHub Integration:** Code is pushed to GitHub
2. **Render Service:** Render pulls from GitHub
3. **Docker Build:** Render builds Docker image
4. **Deployment:** Container runs on Render's infrastructure
5. **Public URL:** Application accessible at `https://shreeshs-air-travel-db.onrender.com`

### Environment Configuration

The server uses the `PORT` environment variable (set by Render):
```cpp
const char* port_env = std::getenv("PORT");
int port = port_env ? std::atoi(port_env) : 8080;
svr.listen("0.0.0.0", port);  // Listen on all interfaces
```

---

## Performance Characteristics

### Time Complexities

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| Get Airline by IATA | O(1) | Hash map lookup |
| Get Airport by IATA | O(1) | Hash map lookup |
| Get Direct Routes | O(k + m log m) | k = routes from source, m = results |
| Get One-Hop Routes | O(k + m + n log n) | k = routes to dest, m = routes from source, n = results |
| Airline Routes Report | O(k + m log m) | k = routes by airline, m = airports |
| Airport Airlines Report | O(k + m log m) | k = routes to airport, m = airlines |
| Airport Search | O(n) | n = number of airports (limited to 20 results) |

### Space Complexity

- **Airlines:** ~6,000 entries × ~200 bytes = ~1.2 MB
- **Airports:** ~7,000 entries × ~300 bytes = ~2.1 MB
- **Routes:** ~67,000 entries × ~100 bytes = ~6.7 MB
- **Indexes:** Additional ~10-15 MB for pointers and hash maps
- **Total:** ~20-25 MB in memory

### Optimization Strategies

1. **Hash Maps for O(1) Lookups:** Primary access method
2. **Indexes for Route Queries:** Avoid scanning all routes
3. **Server-Side Search:** Limits results to 20, reduces network traffic
4. **Pagination:** Handles large datasets without loading everything
5. **Client-Side Caching:** Caches autocomplete data in browser

---

## Data Flow Examples

### Example 1: User Searches for Airline "AA"

```
1. User types "AA" in search box
   ↓
2. JavaScript: fetch('/airline/AA')
   ↓
3. HTTP Server receives GET /airline/AA
   ↓
4. Database::getAirlineByIATA("AA")
   ↓
5. Hash map lookup: airlines_by_iata["AA"] → Airline object
   ↓
6. Convert Airline to JSON
   ↓
7. HTTP Response: {"id":24,"name":"American Airlines",...}
   ↓
8. JavaScript displays airline information
```

**Total Time:** ~10-50ms (mostly network latency)

### Example 2: User Finds Direct Routes SFO → JFK

```
1. User selects SFO and JFK, clicks "Find Direct Routes"
   ↓
2. JavaScript: fetch('/direct/SFO/JFK')
   ↓
3. HTTP Server receives GET /direct/SFO/JFK
   ↓
4. Database::getDirectRoutes("SFO", "JFK")
   ↓
5. Lookup routes_by_source["SFO"] → vector of Route* pointers
   ↓
6. Filter routes where dest_iata == "JFK"
   ↓
7. For each route:
   - Get airline info: getAirlineByIATA(route->airline_iata)
   - Calculate distance: calculateDistance(SFO, JFK)
   ↓
8. Sort routes by distance
   ↓
9. Convert to JSON array
   ↓
10. HTTP Response: [{"airline_iata":"AA","distance":2585.2,...},...]
   ↓
11. JavaScript displays routes and draws map
```

**Total Time:** ~50-200ms (depending on number of routes)

### Example 3: User Finds One-Hop Routes SFO → LHR

```
1. User selects SFO and LHR, clicks "Find One-Hop Routes"
   ↓
2. JavaScript: fetch('/onehop/SFO/LHR')
   ↓
3. HTTP Server receives GET /onehop/SFO/LHR
   ↓
4. Database::getOneHopRoutes("SFO", "LHR")
   ↓
5. Lookup routes_by_source["SFO"] → routes FROM SFO
   ↓
6. Lookup routes_by_dest["LHR"] → routes TO LHR
   ↓
7. Build set of intermediate airports (airports that have routes TO LHR)
   ↓
8. For each route FROM SFO:
   - Check if destination is in intermediate set
   - If yes: Calculate total distance (SFO → intermediate → LHR)
   ↓
9. Sort by total distance
   ↓
10. Convert to JSON
   ↓
11. HTTP Response: [{"intermediate":"JFK","airline":"AA","distance":5345.1,...},...]
   ↓
12. JavaScript displays routes and draws map with intermediate stops
```

**Total Time:** ~100-500ms (depending on number of connections)

---

## Key Design Decisions

### 1. Why Hash Maps for Primary Lookups?

**Decision:** Use `std::unordered_map` for IATA code lookups

**Reasoning:**
- O(1) average case lookup time
- Most common operation (searching by IATA code)
- Fast enough for real-time web requests

**Alternative Considered:** Binary search on sorted array
- Would be O(log n) - slower
- Hash maps are simpler and faster for this use case

### 2. Why Indexes for Routes?

**Decision:** Build separate indexes for routes by source, destination, and airline

**Reasoning:**
- Without indexes: Finding routes from SFO would require scanning all 67,000+ routes (O(n))
- With indexes: O(1) to find routes from SFO, then O(k) where k << n
- Trade-off: Uses more memory, but dramatically faster queries

**Memory Trade-off:**
- Indexes use ~10-15 MB additional memory
- Worth it for 100x+ speed improvement on route queries

### 3. Why In-Memory Storage?

**Decision:** Load all data into memory at startup

**Reasoning:**
- Fast access (no disk I/O during queries)
- Total data size (~25 MB) fits easily in memory
- Modern servers have plenty of RAM

**Alternative Considered:** Database (SQLite, PostgreSQL)
- Would require additional dependencies
- Slower for simple lookups
- Overkill for this dataset size

### 4. Why Server-Side Search?

**Decision:** Implement `/airports/search` endpoint instead of loading all airports client-side

**Reasoning:**
- Airport dataset is large (~7,000 airports)
- Loading all airports in browser would be slow
- Server-side search allows filtering before sending data
- Reduces network traffic

### 5. Why Haversine Formula for Distance?

**Decision:** Use Haversine formula to calculate distances

**Reasoning:**
- Accounts for Earth's curvature
- Accurate for great-circle distances
- Standard formula for GPS coordinate calculations
- Fast enough for real-time calculations

**Formula:**
- Calculates distance along Earth's surface (not straight line through Earth)
- Returns distance in miles
- Accurate to within ~0.5% for most distances

---

## Summary

This project demonstrates:

1. **Efficient Data Structures:** Hash maps for O(1) lookups, indexes for fast route queries
2. **Algorithm Implementation:** Graph algorithms for route finding, sorting for reports
3. **Web Development:** RESTful API design, modern frontend with interactive maps
4. **System Design:** In-memory database, efficient indexing, server-side processing
5. **Deployment:** Docker containerization, cloud deployment on Render

The application successfully handles:
- ~6,000 airlines
- ~7,000 airports
- ~67,000 routes
- Real-time searches and route finding
- Interactive map visualizations
- All with sub-second response times for most operations

**Total Development Approach:** "Vibe Coding" - collaborative development with AI assistance to create a fully functional, production-ready web application.

