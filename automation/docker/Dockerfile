# Dockerfile for NetMon Plugins build environment

FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Copy source
COPY . .

# Build
RUN make build

# Default command
CMD ["make", "test"]

