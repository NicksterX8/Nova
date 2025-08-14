
cmake_minimum_required(VERSION 3.23.3)

set(CMAKE_BUILD_TYPE Debug)
project(nova VERSION 1.0)

# for extra optimization but slow:
# enable -flto. link time optimization
# or use unity builds for optimization
# but can break things
# -DCMAKE_UNITY_BUILD_ON

set(CMAKE_CXX_STANDARD 17)


add_compile_options(
    -g
    -Wall -Wextra -pedantic -Wliteral-conversion
    -Wno-macro-redefined -Wno-unused-variable -Wno-unused-parameter -Wno-char-subscripts -Wno-comment -Wno-sign-compare
    -Wno-gnu-zero-variadic-macro-arguments
    -Werror=format
    -fno-math-errno -fno-trapping-math -fassociative-math -freciprocal-math -fno-signed-zeros
    -fno-exceptions -fno-rtti
    -DMACOS
    -DDEBUG_LEVEL=2 -DMEMORY_DEBUG_LEVEL=0
)

add_link_options(-framework OpenGL)

# debug options
# doesn't work with lldb on mac. just freezes and stops without giving you any information
#add_compile_options(-fsanitize=undefined -fsanitize=address)
#add_link_options(-fsanitize=undefined -fsanitize=address)

set(SD src)
set(HLB /opt/homebrew/Cellar)
set(VLB /Users/nick/vcpkg/packages)

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
    ${SD}/GUI/systems.cpp
    ${SD}/items/items.cpp
    ${SD}/items/prototypes/prototypes.cpp
    ${SD}/Chunks.cpp
    ${SD}/Tiles.cpp
    ${SD}/Game.cpp
    ${SD}/PlayerControls.cpp
    ${SD}/commands.cpp
    ${SD}/GameState.cpp
    ${SD}/rendering/renderers.cpp
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
    ${SD}/world/entities/prototypes.cpp
    ${SD}/world/entities/entities.cpp
    ${SD}/world/entities/methods.cpp
    ${SD}/world/EntityWorld.cpp
    ${SD}/ECS/System.cpp
    ${SD}/ECS/EntityManager.cpp
    ${SD}/ECS/ArchetypePool.cpp
    ${SD}/ECS/ArchetypalComponentManager.cpp
)

add_library(nova_lib STATIC ${SRC_FILES})

add_executable(${PROJECT_NAME} ${SD}/mainmain.cpp)
target_link_libraries(${PROJECT_NAME} nova_lib)

find_package(SDL3 3.2.16 QUIET)
find_package(SDL3_image 3.2.4 QUIET)
find_package(Freetype 2.13.3 QUIET)
# find_package(glm QUIET)

target_link_libraries(nova_lib SDL3::SDL3 Freetype::Freetype SDL3_image::SDL3_image)
target_include_directories(nova_lib PUBLIC
    include
    ${SDL3_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    #${HLB}/glm/0.9.9.8/include
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND codesign -s - -v -f --entitlements /Users/nick/debug.plist ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Signing executable..."
)

enable_testing()
add_subdirectory(tests)