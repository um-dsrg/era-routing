cmake_minimum_required(VERSION 3.5)
project(lp-solver)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall")

include_directories($ENV{HOME}/libraries/lemon/include)

if(APPLE)  # MacOS specific instructions
  link_directories(
    $ENV{HOME}/libraries/lemon/lib
    $ENV{HOME}/libraries/ILOG/CPLEX_Studio1271/cplex/lib/x86-64_osx/static_pic
  )
endif()

if(UNIX AND NOT APPLE)  # Linux specific instructions
  link_directories(
    $ENV{HOME}/libraries/lemon/lib
    $ENV{HOME}/libraries/ILOG/CPLEX_Studio1271/cplex/lib/x86-64_linux/static_pic
  )
endif()

add_executable(lp-solver
        src/lp-solver.cc
        src/flow-manager.cc
        src/flow-manager.h
        src/graph-manager.cc
        src/graph-manager.h
        src/xml-utilities.cc
        src/xml-utilities.h)

target_link_libraries(lp-solver emon cplex glpk tinyxml2 pthread)
