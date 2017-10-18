## CMake file for the JourneyClient from-scratch Maplestory client ##
cmake_minimum_required (VERSION 3.8)
project (JourneyClient)

# Set this variable to be the absolute path of the "root" directory you're
# building from
set(BUILD_DIR "/home/z/code/clone/JourneyClient")

# Set compiler flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DASIO_STANDALONE -lglut -lGL -ldl -lXinerama -lXext -lXrandr -lXi -lXcursor -lX11 -lXxf86vm -pthread -Wall -Wno-narrowing -Wno-unused-command-line-argument")

# Globbing together each src folder's top-level *.cpp files to each form
# their own libraries
FILE(GLOB Audio_CPP      "Audio/*.cpp")
FILE(GLOB Character_CPP  "Character/*.cpp")
FILE(GLOB Data_CPP       "Data/*.cpp")
FILE(GLOB Gameplay_CPP   "Gameplay/*.cpp")
FILE(GLOB Graphics_CPP   "Graphics/*.cpp")
FILE(GLOB IO_CPP         "IO/*.cpp")
FILE(GLOB Net_CPP        "Net/*.cpp")
FILE(GLOB Util_CPP       "Util/*.cpp")

# Glob each subdirectory as well
FILE(GLOB Inventory_CPP  "Character/Inventory/*.cpp")
FILE(GLOB Look_CPP       "Character/Look/*.cpp")
FILE(GLOB Combat_CPP     "Gameplay/Combat/*.cpp")
FILE(GLOB MapleMap_CPP   "Gameplay/MapleMap/*.cpp")
FILE(GLOB Physics_CPP    "Gameplay/Physics/*.cpp")
FILE(GLOB Components_CPP "IO/Components/*.cpp")
FILE(GLOB UITypes_CPP    "IO/UITypes/*.cpp")
FILE(GLOB Handlers_CPP   "Net/Handlers/*.cpp")

FILE(GLOB Helpers_CPP    "Net/Handlers/Helpers/*.cpp")

# Glob dependencies
FILE(GLOB nlnx_CPP       "include/nlnx/*.cpp")

# Adding file globs created above as libraries
add_library(Audio         ${Audio_CPP})
add_library(Character     ${Character_CPP})
add_library(Data          ${Data_CPP})
add_library(Gameplay      ${Gameplay_CPP})
add_library(Graphics      ${Graphics_CPP})
add_library(IO            ${IO_CPP})
add_library(Net           ${Net_CPP})
add_library(Util          ${Util_CPP})

add_library(Inventory     ${Inventory_CPP})
add_library(Look          ${Look_CPP})
add_library(Combat        ${Combat_CPP})
add_library(MapleMap      ${MapleMap_CPP})
add_library(Physics       ${Physics_CPP})
add_library(Components    ${Components_CPP})
add_library(UITypes       ${UITypes_CPP})
add_library(Handlers      ${Handlers_CPP})

add_library(Helpers       ${Helpers_CPP})

# Just the one file
add_library(Configuration "Configuration.cpp")

add_library(nlnx          ${nlnx_CPP})

# Target executable
add_executable(JourneyClient Journey.cpp)

# Linking between libraries
target_link_libraries(Inventory     Data)
target_link_libraries(MapleMap      Gameplay)
target_link_libraries(MapleMap      Graphics)
target_link_libraries(Components    Character)
target_link_libraries(UITypes       Components)
target_link_libraries(Handlers      UITypes)
target_link_libraries(Combat        IO)
target_link_libraries(UITypes       IO)
target_link_libraries(Character     MapleMap)
target_link_libraries(Gameplay      MapleMap)

# Linking executable to libraries
target_link_libraries(JourneyClient MapleMap)

target_link_libraries(JourneyClient Audio)
target_link_libraries(JourneyClient Character)
target_link_libraries(JourneyClient Data)
target_link_libraries(JourneyClient Gameplay)
target_link_libraries(JourneyClient Graphics)
target_link_libraries(JourneyClient IO)
target_link_libraries(JourneyClient Net)
target_link_libraries(JourneyClient Util)

target_link_libraries(JourneyClient Inventory)
target_link_libraries(JourneyClient Look)
target_link_libraries(JourneyClient Combat)
target_link_libraries(JourneyClient Physics)
target_link_libraries(JourneyClient Components)
target_link_libraries(JourneyClient UITypes)
target_link_libraries(JourneyClient Handlers)

target_link_libraries(JourneyClient Helpers)

target_link_libraries(JourneyClient Configuration)

target_link_libraries(JourneyClient nlnx)

# Link in shared object files
target_link_libraries(JourneyClient "${BUILD_DIR}/include/freetype/objs/.libs/libfreetype.so")
target_link_libraries(JourneyClient "${BUILD_DIR}/include/bass/x64/libbass.so")
target_link_libraries(JourneyClient "${BUILD_DIR}/include/glew/lib/libGLEW.so")
target_link_libraries(JourneyClient "${BUILD_DIR}/include/glfw/src/libglfw3.a")
target_link_libraries(JourneyClient "${BUILD_DIR}/include/glfw/src/libglfw3.a")

target_link_libraries(nlnx          "${BUILD_DIR}/include/lz4/lib/liblz4.so")

# Include directories for project dependencies
include_directories(".")
include_directories("./include")
include_directories("./include/freetype/include")
include_directories("./include/glfw/include")
include_directories("./include/asio/include")
