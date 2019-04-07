cmake_minimum_required(VERSION 3.5)
project(lp-solver)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall")

if(APPLE)  # MacOS specific instructions
  link_directories("/usr/local/lib")
  include_directories("/usr/local/include")
endif()

# Verify that the BOOST library is installed
find_package(Boost 1.58 REQUIRED program_options graph)
include_directories("${Boost_INCLUDE_DIR}" "$ENV{HOME}/libraries/lemon/include")

link_directories($ENV{HOME}/libraries/lemon/lib)

add_executable(lp-solver
               src/lp-solver.cc
               src/flow-manager.cc
               src/flow-manager.h
               src/graph-manager.cc
               src/graph-manager.h
               src/xml-utilities.cc
               src/xml-utilities.h)

target_link_libraries(lp-solver emon glpk tinyxml2 pthread
		      ${Boost_LIBRARIES})
