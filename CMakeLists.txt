cmake_minimum_required(VERSION 3.15...3.30)
project(MegaCute LANGUAGES CXX)

find_package(cpr REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(SQLite3 REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_subdirectory(cureMaid/lib/transactional)
add_subdirectory(cskill)


add_executable(cprTest main.cpp)
target_compile_features(cprTest PUBLIC cxx_std_23)
target_link_libraries(cprTest PUBLIC cpr::cpr  PkgConfig::GTKMM nlohmann_json::nlohmann_json SQLite::SQLite3 cskill) 
target_include_directories(cprTest PUBLIC cureMaid/lib/transactional)

set(CMAKE_BUILD_TYPE Debug)

