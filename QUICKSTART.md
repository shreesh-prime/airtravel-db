# Quick Start Guide

## Step 1: Update Your Student Information

Edit `src/database.cpp` and update the `getStudentInfo()` function with your actual student ID and name:

```cpp
std::string Database::getStudentInfo() const {
    return "Student ID: [YOUR_ID], Name: [YOUR_NAME]";
}
```

Replace `[YOUR_ID]` and `[YOUR_NAME]` with your actual information.

## Step 2: Install Dependencies

### macOS
```bash
# Install CMake if not already installed
brew install cmake
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install cmake build-essential
```

### Windows
- Download and install CMake from https://cmake.org/download/
- Install Visual Studio or MinGW for C++ compilation

## Step 3: Build the Project

```bash
./build.sh
```

Or manually:
```bash
mkdir build
cd build
cmake ..
make
```

## Step 4: Run the Server

From the project root directory:
```bash
cd build
./air_travel_db
```

Or if you're already in the build directory:
```bash
./air_travel_db
```

The server will start on port 8080. You should see:
```
Loading airlines...
Loading airports...
Loading routes...
Data loaded successfully!
Starting server on http://localhost:8080
```

## Step 5: Test the API

Open your browser and visit:
- http://localhost:8080 - Main page with API documentation
- http://localhost:8080/airline/AA - Get American Airlines info
- http://localhost:8080/airport/SFO - Get San Francisco Airport info
- http://localhost:8080/airlines - Get all airlines
- http://localhost:8080/airports - Get all airports
- http://localhost:8080/student - Get student info

## Troubleshooting

### Build Errors

1. **CMake not found**: Install CMake using your package manager
2. **C++ compiler not found**: Install build tools (Xcode Command Line Tools on macOS, build-essential on Linux)
3. **cpp-httplib download fails**: Check your internet connection, CMake will download it automatically

### Runtime Errors

1. **"Could not load airlines.dat"**: Make sure you're running from the project root directory (where the .dat files are located)
2. **Port 8080 already in use**: 
   - Find and stop the process using port 8080
   - Or modify the port in `src/main.cpp` (change `8080` to another port)

### Testing with curl

```bash
# Get airline
curl http://localhost:8080/airline/AA

# Get airport
curl http://localhost:8080/airport/SFO

# Get all airlines (this will be a large response)
curl http://localhost:8080/airlines | head -100
```

## Next Steps

1. Test all endpoints
2. Deploy externally (see deployment section in README.md)
3. Add additional functionality for extra credit






