cmake_minimum_required(VERSION 3.5)
project(PathSelector)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMY_DEBUG")

if(APPLE) # MacOS specific instructions
  link_directories("/usr/local/lib")
  include_directories("/usr/local/include")
endif()

# Verify that the BOOST library is installed
find_package(Boost 1.58 REQUIRED program_options graph)
include_directories("${Boost_INCLUDE_DIR}" "$ENV{HOME}/libraries/lemon/include")

link_directories($ENV{HOME}/libraries/lemon/lib)

add_executable(pathSelector
               src/custom_dijkstra_call.hpp
               src/yen_ksp.hpp
               src/definitions.hpp
               src/flow.hpp
               src/flow.cpp
               src/lemon-graph.hpp
               src/lemon-graph.cpp
               src/boost-graph.hpp
               src/boost-graph.cpp
               src/xml-handler.hpp
               src/xml-handler.cpp
               src/main.cpp)

# Link with the libraries
target_link_libraries(pathSelector emon tinyxml2 ${Boost_LIBRARIES})
