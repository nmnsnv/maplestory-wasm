## CMake file for the JourneyClient from-scratch Maplestory client ##
cmake_minimum_required (VERSION 3.8)
project (JourneyClient)

# Set compiler flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O -g -DASIO_STANDALONE -DGLEW_NO_GLU -lglut -lGL -ldl -lXinerama -lXext -lXrandr -lXi -lXcursor -lX11 -lXxf86vm -pthread -ftrapv -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Werror-implicit-function-declaration -Wformat=2 -Wnull-dereference -Wstrict-overflow=5 -Wno-unused-private-field -Wno-unused-command-line-argument -Wno-narrowing -Wno-sign-compare")

# Globbing together each src folder's top-level *.cpp files to each form
# their own libraries
FILE(GLOB Root_CPP       "./*.cpp")

FILE(GLOB Audio_CPP      "Audio/*.cpp")
FILE(GLOB Character_CPP  "Character/*.cpp")
FILE(GLOB Data_CPP       "Data/*.cpp")
FILE(GLOB Gameplay_CPP   "Gameplay/*.cpp")
FILE(GLOB Graphics_CPP   "Graphics/*.cpp")
FILE(GLOB IO_CPP         "IO/*.cpp")
FILE(GLOB Net_CPP        "Net/*.cpp")
FILE(GLOB Util_CPP       "Util/*.cpp")

# Headers as well
FILE(GLOB Root_H         "./*.h")

FILE(GLOB Audio_H        "Audio/*.h")
FILE(GLOB Character_H    "Character/*.h")
FILE(GLOB Data_H         "Data/*.h")
FILE(GLOB Gameplay_H     "Gameplay/*.h")
FILE(GLOB Graphics_H     "Graphics/*.h")
FILE(GLOB IO_H           "IO/*.h")
FILE(GLOB Net_H          "Net/*.h")
FILE(GLOB Util_H         "Util/*.h")
FILE(GLOB Template_H     "Template/*.h")

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

# Those headers too
FILE(GLOB Inventory_H    "Character/Inventory/*.h")
FILE(GLOB Look_H         "Character/Look/*.h")
FILE(GLOB Combat_H       "Gameplay/Combat/*.h")
FILE(GLOB MapleMap_H     "Gameplay/MapleMap/*.h")
FILE(GLOB Physics_H      "Gameplay/Physics/*.h")
FILE(GLOB Components_H   "IO/Components/*.h")
FILE(GLOB UITypes_H      "IO/UITypes/*.h")
FILE(GLOB Handlers_H     "Net/Handlers/*.h")
FILE(GLOB Packets_H      "Net/Packets/*.h")

FILE(GLOB Helpers_H      "Net/Handlers/Helpers/*.h")

# Glob dependencies
FILE(GLOB nlnx_CPP       "../nlnx/*.cpp")

# Glob dependency headers
FILE(GLOB nlnx_H         "../nlnx/*.hpp")

# Add all source files to $SOURCE_FILES for easy access
SET(SOURCE_FILES ${Root_CPP}
                 ${Audio_CPP}
                 ${Character_CPP}
                 ${Data_CPP}
                 ${Gameplay_CPP}
                 ${Graphics_CPP}
                 ${IO_CPP}
                 ${Net_CPP}
                 ${Util_CPP}
                 ${Inventory_CPP}
                 ${Look_CPP}
                 ${Combat_CPP}
                 ${MapleMap_CPP}
                 ${Physics_CPP}
                 ${Components_CPP}
                 ${UITypes_CPP}
                 ${Handlers_CPP}
                 ${Helpers_CPP}
                 ${nlnx_CPP}
                 "Configuration.cpp"
                 ${Root_H}
                 ${Audio_H}
                 ${Character_H}
                 ${Data_H}
                 ${Gameplay_H}
                 ${Graphics_H}
                 ${IO_H}
                 ${Net_H}
                 ${Util_H}
                 ${Template_H}
                 ${Inventory_H}
                 ${Look_H}
                 ${Combat_H}
                 ${MapleMap_H}
                 ${Physics_H}
                 ${Components_H}
                 ${UITypes_H}
                 ${Handlers_H}
                 ${Packets_H}
                 ${Helpers_H}
                 ${nlnx_H}
                 "gsl/gsl-lite.h")

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
add_executable(JourneyClient ${SOURCE_FILES})

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
target_link_libraries(JourneyClient "${CMAKE_CURRENT_SOURCE_DIR}/../freetype/objs/.libs/libfreetype.so")
target_link_libraries(JourneyClient "${CMAKE_CURRENT_SOURCE_DIR}/../bass/x64/libbass.so")
target_link_libraries(JourneyClient "${CMAKE_CURRENT_SOURCE_DIR}/../glew/lib/libGLEW.so")
target_link_libraries(JourneyClient "${CMAKE_CURRENT_SOURCE_DIR}/../glfw/build/src/libglfw3.a")

target_link_libraries(nlnx          "${CMAKE_CURRENT_SOURCE_DIR}/../lz4/lib/liblz4.so")

# Include directories for project dependencies
include_directories(".")
include_directories("..")
include_directories("../freetype/include")
include_directories("../glew/include")
include_directories("../glfw/include")
include_directories("../asio/include")
include_directories("../lz4/lib")
