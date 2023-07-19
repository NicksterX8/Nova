cmake_minimum_required(VERSION 3.23.3)

project(testing VERSION 1.0)

add_compile_options(-g -Wall -std=c++17 -O2
    -Wno-macro-redefined -Wno-unused-variable
    -Wliteral-conversion)

add_link_options(-framework OpenGL)

#find_package (glog 0.6.0 REQUIRED)

#target_link_libraries (${PROJECT_NAME} glog::glog)

set(HLB /opt/homebrew/Cellar)
set(VLB /Users/nick/vcpkg/packages)

set(SD src)

set(SRC_FILES 
    ${SD}/main.cpp
    ../src/My/Vec.cpp
    ../src/memory.cpp
    ../src/utils/Log.cpp
    ../src/Chunks.cpp
    ../src/utils/Metadata.cpp
)

#find_package (glog 0.6.0 REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_directories(${PROJECT_NAME} PUBLIC 
    ${HLB}/sdl2/2.0.22/lib
    ${HLB}/sdl2_image/2.6.0/lib
    ${HLB}/freetype/2.12.1/lib
)

target_link_libraries(${PROJECT_NAME} sdl2 freetype sdl2_image)
target_include_directories(${PROJECT_NAME} PUBLIC
    ../include
    ${HLB}/sdl2/2.0.22/include/sdl2 # have to do this because of sdl_image 
    ${HLB}/sdl2/2.0.22/include
    ${HLB}/sdl2_image/2.6.0/include
    ${HLB}/glm/0.9.9.8/include

    ${HLB}/freetype/2.12.1/include/freetype2 # Freetype is weird so you have to include it like this
)
