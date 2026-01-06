# Project Structure

## Overview

This C++ web application implements a front-end for the OpenFlights Air Travel Database. It provides HTTP endpoints to query airline, airport, and route data.

## Directory Structure

```
capstone project/
├── README.md              # Main project documentation
├── QUICKSTART.md          # Quick start guide
├── CMakeLists.txt         # CMake build configuration
├── build.sh               # Build script
├── .gitignore            # Git ignore file
├── airlines.dat          # Airline data (downloaded)
├── airports.dat          # Airport data (downloaded)
├── routes.dat            # Route data (downloaded)
├── include/              # Header files
│   ├── models.h          # Data structures (Airline, Airport, Route)
│   ├── csv_parser.h      # CSV parsing utilities
│   └── database.h        # Database class for data management
└── src/                  # Source files
    ├── main.cpp          # HTTP server and API endpoints
    ├── database.cpp      # Database implementation
    └── csv_parser.cpp    # CSV parsing implementation
```

## Data Structures Used

### Collections

1. **Airlines**
   - `std::unordered_map<std::string, Airline>` - Hash table for O(1) lookup by IATA code
   - `std::unordered_map<int, Airline>` - Hash table for lookup by ID
   - `std::map<std::string, Airline>` - Ordered map for sorted reports (sorted by IATA code)

2. **Airports**
   - `std::unordered_map<std::string, Airport>` - Hash table for O(1) lookup by IATA code
   - `std::unordered_map<int, Airport>` - Hash table for lookup by ID
   - `std::map<std::string, Airport>` - Ordered map for sorted reports (sorted by IATA code)

3. **Routes**
   - `std::vector<Route>` - Vector storing all route entities
   - `std::unordered_map<std::string, std::vector<Route*>>` - Index by source airport IATA
   - `std::unordered_map<std::string, std::vector<Route*>>` - Index by destination airport IATA
   - `std::unordered_map<std::string, std::vector<Route*>>` - Index by airline IATA

### Linkages

- Routes link to Airlines via `airline_iata` (string) and `airline_id` (int)
- Routes link to Airports via `source_iata`/`dest_iata` (strings) and `source_id`/`dest_id` (ints)
- All linkages use direct field references (no pointers between entities)

### Sorting

- **By IATA Code**: Uses `std::map` which maintains sorted order automatically (sorted at insertion)
- **By Route Count**: Uses `std::sort` on vectors (sorted on-demand when generating reports)

## API Endpoints

All endpoints return JSON data.

### Basic Functionality

1. `GET /airline/{iata}` - Get airline by IATA code
2. `GET /airport/{iata}` - Get airport by IATA code
3. `GET /airline/{iata}/routes` - Get airports served by airline (ordered by route count)
4. `GET /airport/{iata}/airlines` - Get airlines serving airport (ordered by route count)
5. `GET /airlines` - Get all airlines (ordered by IATA code)
6. `GET /airports` - Get all airports (ordered by IATA code)
7. `GET /student` - Get student ID and name

## Implementation Details

### CSV Parsing
- Handles quoted fields and escaped commas
- Handles NULL values represented as `\N`
- Parses numeric fields (int, double) with error handling

### Data Loading
- Loads data from .dat files on startup
- Builds indexes for efficient querying
- Validates data (skips invalid entries)

### HTTP Server
- Uses cpp-httplib (header-only library)
- CORS enabled for browser access
- JSON responses
- Error handling (404 for not found)

## Next Steps

1. **Install CMake** (required for building)
   ```bash
   # macOS
   brew install cmake
   
   # Linux
   sudo apt-get install cmake
   ```

2. **Build the project**
   ```bash
   ./build.sh
   ```

3. **Run and test locally**
   ```bash
   cd build
   ./air_travel_db
   ```

4. **Update student info** in `src/database.cpp`

5. **Deploy externally** (see deployment options in README.md)

## Design Decisions

### Why Hash Tables for Lookups?
- O(1) average case lookup time
- IATA codes are unique identifiers
- Fast retrieval for individual entity queries

### Why Ordered Maps for Sorted Reports?
- Automatically maintains sorted order
- Efficient for generating sorted lists
- No need to sort on every request

### Why Vectors for Routes?
- Simple storage for all routes
- Efficient iteration for building indexes
- Indexes provide fast access patterns

### Why String-based Linkages?
- IATA codes are human-readable
- Easy to debug and inspect
- No pointer management issues
- Works well with hash table lookups






