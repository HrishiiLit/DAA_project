cmake_minimum_required(VERSION 3.20)
project(MyCppBackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)

# ==========================================
# PATCH: Set policy for old CMake versions in dependencies
# ==========================================
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

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
  URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.zip
)
FetchContent_MakeAvailable(asio)

# CRITICAL: Tell Crow where to find the Asio we just downloaded
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "Path to Asio" FORCE)

# ==========================================
# 3. DOWNLOAD LIBRARY: CROW (Web Server)
# ==========================================
FetchContent_Declare(
  Crow
  URL https://github.com/CrowCpp/Crow/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(Crow)

# ==========================================
# 4. DOWNLOAD LIBRARY: JSON
# ==========================================
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.zip
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