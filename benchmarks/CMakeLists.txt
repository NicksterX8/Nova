cmake_minimum_required(VERSION 3.23.3)

project(benchmarks LANGUAGES CXX)

set(BENCHMARK_FILES 
    ECS/ecs-benchmark.cpp
    ECS/create-entity.cpp)

foreach(src ${BENCHMARK_FILES})
    get_filename_component(exe ${src} NAME_WE)
    add_executable(${exe} ${src})
    target_link_libraries(${exe} nova_lib)
    target_include_directories(${exe} PRIVATE
        ../include
        include
    )
endforeach()