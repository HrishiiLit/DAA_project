cmake_minimum_required(VERSION 3.20)
project(FlightBackend LANGUAGES CXX)

# ============================================================
# Compiler settings
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force static linking for MinGW (Prevents missing DLL errors on other PCs)
if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif()

find_package(Threads REQUIRED)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601) # Target Windows 7 or later
endif()

# ============================================================
# FetchContent Setup
# ============================================================
include(FetchContent)

# 1. ASIO (Standalone, no Boost)
FetchContent_Declare(
    asio
    URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.zip
)
FetchContent_MakeAvailable(asio)

# 2. JSON (Nlohmann)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
)
FetchContent_MakeAvailable(json)

# 3. CROW (Web Framework)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CROW_USE_BOOST OFF CACHE BOOL "" FORCE) # Ensure we use ASIO, not Boost

# Tell Crow where to find ASIO before fetching it
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE STRING "ASIO include directory" FORCE)

FetchContent_Declare(
    Crow
    # Using v1.2.0 (Latest stable)
    URL https://github.com/CrowCpp/Crow/archive/refs/tags/v1.2.0.zip
)
FetchContent_MakeAvailable(Crow)

# ============================================================
# Build the final executable
# ============================================================
# NOTE: Ensure the file name here matches exactly what is on your disk
add_executable(server_app main.cpp jsondb.cpp) 

# Include ASIO headers explicitly if Crow doesn't pick them up automatically
target_include_directories(server_app PRIVATE
    ${asio_SOURCE_DIR}/asio/include
)

# ============================================================
# Link libraries
# ============================================================
if(WIN32)
    target_link_libraries(server_app PRIVATE 
        Crow::Crow 
        nlohmann_json::nlohmann_json
        Threads::Threads 
        ws2_32 
        mswsock
    )
else()
    target_link_libraries(server_app PRIVATE 
        Crow::Crow 
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
endif()