cmake_minimum_required(VERSION 3.20)
project(PackageTest CXX)

find_package(Entity CONFIG REQUIRED COMPONENTS entity)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE ad::entity)
