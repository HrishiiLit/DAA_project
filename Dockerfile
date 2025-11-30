# ==========================================
# Multi-Stage Build: Compile on Linux base
# ==========================================
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    git \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY CMakeLists.txt .
COPY main.cpp .

# Build the application
RUN cmake -B build -G "Unix Makefiles" \
    && cmake --build build --config Release

# ==========================================
# Runtime Stage: Minimal image with server
# ==========================================
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy compiled executable from builder
COPY --from=builder /build/build/server_app /app/server_app

# Expose the port
EXPOSE 18080

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:18080/health || exit 1

# Run the application
CMD ["/app/server_app"]
