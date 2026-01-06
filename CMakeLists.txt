cmake_minimum_required(VERSION 3.10)
project(AirTravelDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find or include cpp-httplib
# Option 1: If installed via package manager
# find_package(httplib REQUIRED)

# Option 2: Include as header-only (recommended for this project)
include(FetchContent)
FetchContent_Declare(
  cpp-httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(cpp-httplib)

# Add executable
add_executable(air_travel_db
    src/main.cpp
    src/database.cpp
    src/csv_parser.cpp
)

# Link libraries
target_link_libraries(air_travel_db
    PRIVATE
    httplib::httplib
)

# Include directories
target_include_directories(air_travel_db
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

