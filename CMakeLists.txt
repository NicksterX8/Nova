cmake_minimum_required(VERSION 3.23.3)

set(CMAKE_BUILD_TYPE Debug)
project(faketorio VERSION 1.0)
# -Wextra -pedantic

add_compile_options(-g -Wall -std=c++17
    -Wno-macro-redefined -Wno-unused-variable -Wno-char-subscripts -Wno-comment
    -Wliteral-conversion -fsanitize=undefined -fno-exceptions
    -DDEBUG
)

add_link_options(-framework OpenGL -fsanitize=address -fsanitize=undefined -fno-exceptions)

set(CMAKE_BUILD_TYPE Debug)
set(SD src)

set(SRC_FILES 
    ${SD}/llvm/SmallVector.cpp
    ${SD}/main.cpp
    ${SD}/global.cpp
    ${SD}/sdl.cpp
    ${SD}/memory.cpp
    ${SD}/threads.cpp
    ${SD}/actions.cpp
    ${SD}/utils/Log.cpp
    ${SD}/utils/Metadata.cpp
    ${SD}/utils/Debug.cpp
    ${SD}/utils/FileSystem.cpp
    ${SD}/utils/GlobalAllocators.cpp
    ${SD}/My/Vec.cpp
    ${SD}/My/HashMap.cpp
    ${SD}/GUI/Gui.cpp
    ${SD}/GUI/update.cpp
    ${SD}/items/items.cpp
    ${SD}/items/prototypes/prototypes.cpp
    ${SD}/Chunks.cpp
    ${SD}/Tiles.cpp
    ${SD}/Game.cpp
    ${SD}/PlayerControls.cpp
    ${SD}/commands.cpp
    ${SD}/GameState.cpp
    ${SD}/rendering/context.cpp
    ${SD}/rendering/textures.cpp
    ${SD}/rendering/shaders.cpp
    ${SD}/rendering/text/Font.cpp
    ${SD}/rendering/text/formatting.cpp
    ${SD}/rendering/text/text-rendering.cpp
    ${SD}/rendering/text/freetype.cpp
    ${SD}/rendering/drawing.cpp
    ${SD}/rendering/utils.cpp
    ${SD}/rendering/Shader.cpp
    ${SD}/rendering/rendering.cpp
    ${SD}/rendering/TexturePacker.cpp
    ${SD}/world/components/components.cpp
    ${SD}/world/functions.cpp
    ${SD}/world/entities/entities.cpp
    ${SD}/world/entities/methods.cpp
    ${SD}/world/EntityWorld.cpp
    ${SD}/ECS/system.cpp
    ${SD}/ECS/EntityManager.cpp
    ${SD}/physics/physics.cpp
)

#find_package (glog 0.6.0 REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_FILES})
#target_link_libraries (${PROJECT_NAME} glog::glog)

set(HLB /opt/homebrew/Cellar)
set(VLB /Users/nick/vcpkg/packages)

#find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

target_link_directories(${PROJECT_NAME} PUBLIC 
    ${HLB}/sdl3/3.2.16/lib
    ${HLB}/sdl3_image/3.2.4/lib
    ${HLB}/freetype/2.13.3/lib
)

target_link_libraries(${PROJECT_NAME} sdl3 freetype sdl3_image)
target_include_directories(${PROJECT_NAME} PUBLIC
    ../faketorio/include
    ${HLB}/sdl3/3.2.16/include/SDL3 # have to do this because of sdl_image 
    ${HLB}/sdl3/3.2.16/include
    ${HLB}/sdl3_image/3.2.4/include
    ${HLB}/glm/0.9.9.8/include
    ${HLB}/freetype/2.13.3/include/freetype2 # Freetype is weird so you have to include it like this
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND codesign -s - -v -f --entitlements /Users/nick/debug.plist ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Signing executable..."
)