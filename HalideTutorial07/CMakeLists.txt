cmake_minimum_required(VERSION 3.21)

project(halide_tutorial07)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

find_package(Halide REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)

add_executable(
    ${PROJECT_NAME}
    main.cpp
)
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${JPEG_INCLUDE_DIR}
)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        Halide::Halide
        Halide::Tools
        PNG::PNG
        ${JPEG_LIBRARIES}
)