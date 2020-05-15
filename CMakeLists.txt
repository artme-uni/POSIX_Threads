cmake_minimum_required(VERSION 3.16)
project(POSIX_Threads)

SET(CMAKE_CXX_COMPILER /usr/local/bin/mpicxx)

include_directories(/usr/local/opt/mpich/include)
link_directories(/usr/local/opt/mpich/lib)

add_executable(POSIX_Threads main.cpp dynamic_distribution.cpp dynamic_distribution.h)
target_link_libraries(POSIX_Threads mpi)