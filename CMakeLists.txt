cmake_minimum_required(VERSION 3.16)

project(bayan_lab7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS program_options NO_MODULE)

add_executable(bayan_lab7
    duplicate_finder.cpp
    main.cpp
    pool_allocator.h
    seq.h
    validate.h
    hasher.h
    lazy_file.h
    duplicate_finder.h
)

target_link_libraries(bayan_lab7 PRIVATE Boost::program_options)
target_compile_options(bayan_lab7 PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(bayan_lab7 PRIVATE -static -static-libgcc -static-libstdc++)