cmake_minimum_required(VERSION 3.5)
project(network-topology-lgf-to-xml)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall")

# Verify that the BOOST library is installed
find_package(Boost 1.58 REQUIRED program_options)
include_directories("${Boost_INCLUDE_DIR}" "$ENV{HOME}/libraries/lemon/include")

link_directories($ENV{HOME}/libraries/lemon/lib)

add_executable(ntltx
               src/lemon-graph.cc
               src/main.cc)

target_link_libraries(ntltx emon tinyxml2 pthread
		      ${Boost_LIBRARIES})
