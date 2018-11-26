cmake_minimum_required(VERSION 3.5)
project(path-constrained-mcfp-solver)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall")

# Verify that the BOOST library is installed
find_package(Boost 1.58 REQUIRED program_options graph)
include_directories("${Boost_INCLUDE_DIR}" "$ENV{HOME}/libraries/lemon/include")

link_directories($ENV{HOME}/libraries/lemon/lib)

add_executable(pc_mcfp_solver
               src/main.cc
               src/links.cc
               src/flows.cc)

target_link_libraries(pc_mcfp_solver emon glpk tinyxml2 pthread
		      ${Boost_LIBRARIES})
