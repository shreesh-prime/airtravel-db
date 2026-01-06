#!/bin/bash

# Build script for Air Travel Database project

echo "Creating build directory..."
mkdir -p build
cd build

echo "Running CMake..."
cmake ..

echo "Building project..."
make

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo "To run the server:"
    echo "  cd build"
    echo "  ./air_travel_db"
    echo ""
    echo "Or from project root:"
    echo "  cd build && ./air_travel_db"
else
    echo "Build failed!"
    exit 1
fi






