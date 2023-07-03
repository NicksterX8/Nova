cmake_minimum_required(VERSION 3.23.3)

project(faketorio VERSION 1.0)

add_compile_options(-g -Wall -std=c++17
    -Wno-macro-redefined -Wno-unused-variable -Wno-char-subscripts -Wno-comment
    -Wliteral-conversion
    -D_GLIBCXX_DEBUG -D_GLIBXX_DEBUG_PEDANTIC)

add_link_options(-framework OpenGL)

set(SD src)

set(SRC_FILES 
    llvm/SmallVector.cpp
    ${SD}/main.cpp
    ${SD}/global.cpp
    ${SD}/sdl.cpp
    ${SD}/memory.cpp
    ${SD}/utils/Log.cpp
    ${SD}/utils/Metadata.cpp
    ${SD}/utils/Debug.cpp
    ${SD}/utils/FileSystem.cpp
    ${SD}/My/Vec.cpp
    ${SD}/My/HashMap.cpp
    ${SD}/GUI/Gui.cpp
    ${SD}/items/items.cpp
    ${SD}/Chunks.cpp
    ${SD}/Tiles.cpp
    ${SD}/Game.cpp
    ${SD}/PlayerControls.cpp
    ${SD}/commands.cpp
    ${SD}/GameState.cpp
    ${SD}/rendering/textures.cpp
    ${SD}/rendering/text.cpp
    ${SD}/rendering/Drawing.cpp
    ${SD}/rendering/utils.cpp
    ${SD}/rendering/Shader.cpp
    ${SD}/rendering/rendering.cpp
    ${SD}/rendering/TexturePacker.cpp
    ${SD}/GameSave/main.cpp
    ${SD}/ECS/ECS.cpp
    ${SD}/ECS/EntityManager.cpp
    ${SD}/ECS/EntitySystem.cpp
    ${SD}/ECS/SECS.cpp
    ${SD}/ECS/components/components.cpp
    ${SD}/ECS/entities/entities.cpp
    ${SD}/ECS/entities/methods.cpp
)

#find_package (glog 0.6.0 REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_FILES})
#target_link_libraries (${PROJECT_NAME} glog::glog)

set(HLB /opt/homebrew/Cellar)
set(VLB /Users/nick/vcpkg/packages)

target_link_directories(${PROJECT_NAME} PUBLIC 
    ${HLB}/sdl2/2.0.22/lib
    ${HLB}/sdl2_image/2.6.0/lib
    ${HLB}/freetype/2.12.1/lib

    #${VLB}/glog_arm64-osx/lib
)

#${VLB}/glog_arm64-osx/include

target_link_libraries(${PROJECT_NAME} sdl2 freetype sdl2_image)
target_include_directories(${PROJECT_NAME} PUBLIC
    ../faketorio/include
    ${HLB}/sdl2/2.0.22/include/sdl2 # have to do this because of sdl_image 
    ${HLB}/sdl2/2.0.22/include
    ${HLB}/sdl2_image/2.6.0/include
    ${HLB}/glm/0.9.9.8/include

    ${HLB}/freetype/2.12.1/include/freetype2 # Freetype is weird so you have to include it like this
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND codesign -s - -v -f --entitlements /Users/nick/debug.plist ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Signing executable..."
)