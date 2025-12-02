cmake_minimum_required(VERSION 3.5)
project(MyCppBackend)

set(CMAKE_CXX_STANDARD 17)

# ==========================================
# 1. SETUP: Prevent Errors & Define Variables
# ==========================================
# Prevent Crow from building its own tests (fixes previous error)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Fix for Windows network compatibility
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

include(FetchContent)

# ==========================================
# 2. DOWNLOAD DEPENDENCY: ASIO (Networking)
# ==========================================
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-30-2  # Stable version
)
FetchContent_MakeAvailable(asio)

# CRITICAL: Tell Crow where to find the Asio we just downloaded
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "Path to Asio" FORCE)

# ==========================================
# 3. DOWNLOAD LIBRARY: CROW (Web Server)
# ==========================================
FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG master
)
FetchContent_MakeAvailable(Crow)

# ==========================================
# 4. DOWNLOAD LIBRARY: JSON
# ==========================================
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# ==========================================
# 5. BUILD THE APP
# ==========================================
add_executable(server_app main.cpp)

# Link everything together
if(WIN32)
    # Windows needs 'ws2_32' (Winsock) and 'mswsock'
    target_link_libraries(server_app PRIVATE Crow::Crow nlohmann_json::nlohmann_json ws2_32 mswsock)
else()
    target_link_libraries(server_app PRIVATE Crow::Crow nlohmann_json::nlohmann_json)
endif()