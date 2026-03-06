## CMake file for the JourneyClient from-scratch Maplestory client ##
cmake_minimum_required (VERSION 3.8)
project (JourneyClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DEBUG_SYMBOLS "Enable debug symbols (-g)" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(ERROR_ON_UNUSED_VARIABLES "Fail the build on unused variable warnings" ON)
option(ENFORCE_UNUSED_INCLUDES "Run include-what-you-use to catch unused includes" ON)

if(EMSCRIPTEN)
    add_compile_definitions(MS_PLATFORM_WASM=1)
    add_compile_options("-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=/")
    add_compile_options(
        --use-port=freetype
        -Wno-c++11-narrowing
        -Wno-deprecated-declarations
        -fexceptions
    )
    # Emscripten -s flags are linker settings; passing them at compile time emits warnings.
    add_link_options(
        -sUSE_GLFW=3 
        -sUSE_WEBGL2=1 
        -sFULL_ES3=1
        -sWASM=1 
        -sALLOW_MEMORY_GROWTH=1
        -sFORCE_FILESYSTEM=1
        -fexceptions
        -sUSE_ZLIB=1
        --use-port=freetype
        -lwebsocket.js 
        -sASYNCIFY=1
        -sASYNCIFY_IMPORTS=['emscripten_asm_const_int','emscripten_asm_const_double','emscripten_sleep']
        -sASYNCIFY_STACK_SIZE=16777216
        -sSTACK_SIZE=16777216
        -sINITIAL_MEMORY=268435456  
        -sEXPORTED_RUNTIME_METHODS=FS,ccall,cwrap,setCanvasSize
        "--embed-file=${CMAKE_CURRENT_SOURCE_DIR}/fonts/Arial/Arial.ttf@fonts/Arial/Arial.ttf"
        "--embed-file=${CMAKE_CURRENT_SOURCE_DIR}/fonts/Arial/Arial-Bold.ttf@fonts/Arial/Arial-Bold.ttf"
        --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/LazyFS/lazyfs.js
        -sASSERTIONS=1
    )
    
    if(DEBUG_SYMBOLS)
        add_compile_options(-g)
        add_link_options(-g3 -gsource-map)
    endif()
else()
    # Native Linux flags
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O -g -DASIO_STANDALONE -DGLEW_NO_GLU -lglut -lGL -ldl -lXinerama -lXext -lXrandr -lXi -lXcursor -lX11 -lXxf86vm -pthread -ftrapv -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Werror-implicit-function-declaration -Wformat=2 -Wnull-dereference -Wstrict-overflow=5 -Wno-unused-private-field -Wno-unused-command-line-argument -Wno-narrowing -Wno-sign-compare")
endif()

file(GLOB_RECURSE SOURCES
    "Audio/*.cpp" "Audio/*.c"
    "Character/*.cpp" "Character/*.c"
    "Data/*.cpp" "Data/*.c"
    "Gameplay/*.cpp" "Gameplay/*.c"
    "Graphics/*.cpp" "Graphics/*.c"
    "IO/*.cpp" "IO/*.c"
    "LazyFS/*.cpp" "LazyFS/*.c"
    "Net/*.cpp" "Net/*.c"
    "Template/*.cpp" "Template/*.c"
    "Util/*.cpp" "Util/*.c"
)

list(APPEND SOURCES
    "Configuration.cpp"
    "Journey.cpp"
    "includes/lz4.c"
    "includes/stb/stb_vorbis.c"
    "../nlnx/bitmap.cpp"
    "../nlnx/node.cpp"
    "../nlnx/audio.cpp"
    "../nlnx/file.cpp"
    "../nlnx/nx.cpp"
)

add_executable(JourneyClient ${SOURCES})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    if(WARNINGS_AS_ERRORS)
        target_compile_options(JourneyClient PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-Werror>
            $<$<COMPILE_LANGUAGE:CXX>:-Werror>
        )
    endif()

    if(ERROR_ON_UNUSED_VARIABLES)
        target_compile_options(JourneyClient PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-Werror=unused-variable>
            $<$<COMPILE_LANGUAGE:CXX>:-Werror=unused-variable>
            $<$<COMPILE_LANGUAGE:C>:-Werror=unused-but-set-variable>
            $<$<COMPILE_LANGUAGE:CXX>:-Werror=unused-but-set-variable>
            $<$<COMPILE_LANGUAGE:CXX>:-Werror=unused-const-variable>
        )
    endif()
endif()

if(ENFORCE_UNUSED_INCLUDES)
    find_program(IWYU_EXECUTABLE NAMES include-what-you-use iwyu)
    if(IWYU_EXECUTABLE)
        set_target_properties(JourneyClient PROPERTIES
            CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXECUTABLE};-Xiwyu;--error=1"
        )
    else()
        message(STATUS "ENFORCE_UNUSED_INCLUDES is ON, but include-what-you-use is not installed. Unused include checks are skipped.")
    endif()
endif()

# Include directories
include_directories(".")
include_directories("..")
include_directories("../nlnx")
include_directories("includes")
include_directories("includes/stb")
include_directories("includes/GL")
