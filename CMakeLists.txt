cmake_minimum_required(VERSION 3.15...3.30)
project(MegaCute LANGUAGES CXX)

find_package(cpr REQUIRED)
find_package(jsoncpp REQUIRED)
include_directories(${JSON_INC_PATH})


add_executable(cprTest cpr.cpp)
set_property(TARGET cprTest PROPERTY CXX_STANDARD 23)
target_link_libraries(cprTest PUBLIC cpr::cpr jsoncpp)
