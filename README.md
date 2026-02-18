# OpenFlights Air Travel Database - Capstone Project

A C++ web application front-end for the OpenFlights Air Travel Database.

## Project Overview

This project implements a web-based front-end for querying airline, airport, and route data from the OpenFlights database. The application is built using C++ and serves data via HTTP endpoints.

## Requirements

- C++17 or later
- A C++ HTTP server library (cpp-httplib recommended)
- CMake 3.10+ (for building)

## Data Files

The following data files should be in the project root:
- `airlines.dat` - Airline data
- `airports.dat` - Airport data  
- `routes.dat` - Route data

## Building the Project

```bash
mkdir build
cd build
cmake ..
make
```

## Running the Application

Access it at: http:shreeshs-air-travel-db.onrender.com/

## API Endpoints

### Basic Functionality

1. **Get Airline by IATA Code**
   - `GET /airline/{iata_code}`
   - Returns airline entity for given IATA code

2. **Get Airport by IATA Code**
   - `GET /airport/{iata_code}`
   - Returns airport entity for given IATA code

3. **Airline Routes Report**
   - `GET /airline/{iata_code}/routes`
   - Returns all airports reached by airline, ordered by route count

4. **Airport Airlines Report**
   - `GET /airport/{iata_code}/airlines`
   - Returns all airlines serving airport, ordered by route count

5. **All Airlines Report**
   - `GET /airlines`
   - Returns all airlines ordered by IATA code

6. **All Airports Report**
   - `GET /airports`
   - Returns all airports ordered by IATA code

7. **Get Student ID**
   - `GET /student`
   - Returns student ID and name

## Data Structures

The application uses the following C++ data structures:
- **Hash Maps** for O(1) lookup by IATA codes
- **Vectors** for storing collections
- **Ordered structures** for sorted reports








