cmake_minimum_required(VERSION 3.5)
project(PcLp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall")

if(APPLE) # MacOS specific instructions
  link_directories("/usr/local/lib")
  include_directories("/usr/local/include")
endif()

# Verify that the BOOST library is installed
find_package(Boost 1.58 REQUIRED program_options)
include_directories("${Boost_INCLUDE_DIR}" "$ENV{HOME}/libraries/lemon/include")

link_directories($ENV{HOME}/libraries/lemon/lib)

add_executable(pclp
               src/link.cc
               src/path.cc
               src/flow.cc
               src/path_file_parser.cc
               src/lp_solver.cc
               src/xml_handler.cc
               src/main.cc)

target_link_libraries(pclp emon glpk tinyxml2 pthread
                      ${Boost_LIBRARIES})
