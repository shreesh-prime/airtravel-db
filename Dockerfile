# Use a base image with CMake and build tools
FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy project files
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
COPY *.dat ./

# Build the application
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make

# Copy the built binary and data files to a clean location
WORKDIR /app/runtime
RUN cp /app/build/air_travel_db . && \
    cp /app/*.dat .

# Expose port (will be overridden by PORT env var)
EXPOSE 8080

# Run the application
CMD ["./air_travel_db"]

