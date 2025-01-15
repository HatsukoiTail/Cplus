cmake_minimum_required(VERSION 3.10)
project(CAIDA)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_executable(${PROJECT_NAME} main.cpp distribute.cpp caida.cpp)