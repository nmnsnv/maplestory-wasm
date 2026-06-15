## CMake file for the JourneyClient from-scratch Maplestory client ##
cmake_minimum_required (VERSION 3.8)
project (JourneyClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DEBUG_SYMBOLS "Enable debug symbols (-g)" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(ERROR_ON_UNUSED_VARIABLES "Fail the build on unused variable warnings" ON)
option(ENFORCE_UNUSED_INCLUDES "Run include-what-you-use to catch unused includes" ON)
option(BUILD_TESTS "Build the native host unit tests instead of the client" OFF)

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
elseif(NOT BUILD_TESTS)
    # Native Linux flags (full client build). The test build uses its own
    # portable flags below and must not inherit these GL/X11 link libraries.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O -g -DASIO_STANDALONE -DGLEW_NO_GLU -lglut -lGL -ldl -lXinerama -lXext -lXrandr -lXi -lXcursor -lX11 -lXxf86vm -pthread -ftrapv -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Werror-implicit-function-declaration -Wformat=2 -Wnull-dereference -Wstrict-overflow=5 -Wno-unused-private-field -Wno-unused-command-line-argument -Wno-narrowing -Wno-sign-compare")
endif()

if(NOT BUILD_TESTS)
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
endif() # NOT BUILD_TESTS

# Include directories
include_directories(".")
include_directories("..")
include_directories("../nlnx")
include_directories("includes")
include_directories("includes/stb")
include_directories("includes/GL")

# ---------------------------------------------------------------------------
# Native host unit tests (Plan 00 / Plan 07). Built with -DBUILD_TESTS=ON.
# Compiles only pure translation units (no GL context, network, or WZ assets)
# plus the doctest-based test sources. See Test/ and src/client/README.md.
# ---------------------------------------------------------------------------
if(BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()

    # FreeType provides CPU glyph layout for the headless GraphicsGL build.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FREETYPE REQUIRED freetype2)

    add_executable(tests
        # doctest entry point + characterization tests
        Test/test_main.cpp
        Test/test_packet_io.cpp
        Test/test_cryptography.cpp
        Test/test_util.cpp
        Test/test_template.cpp
        Test/test_keyboard.cpp
        Test/test_graphics_headless.cpp
        Test/test_nx_synthetic.cpp
        Test/test_texture_render.cpp
        Test/test_button_render.cpp
        # Test-only synthetic NX (PKG4) image builder (Phase D).
        Test/NxBuilder.cpp
        # Pure production translation units under test (no GL/network/assets).
        Net/InPacket.cpp
        Net/Cryptography.cpp
        # Tier 1 UI logic (uses GLFW key codes only, via the host shim below).
        IO/Keyboard.cpp
        # Headless graphics core (MS_HEADLESS: GL calls compiled out, FreeType
        # layout + quad/atlas bookkeeping retained). See Graphics/GLHeadless.h.
        Graphics/GraphicsGL.cpp
        Graphics/Text.cpp
        Graphics/Texture.cpp
        Graphics/Animation.cpp
        # Real UI component exercised headlessly (no UI.cpp hub dependency).
        IO/Components/Button.cpp
        IO/Components/MapleButton.cpp
        IO/Cursor.cpp
        Configuration.cpp
        # WZ asset reader (needed by the graphics core; drives Phase D/E).
        ../nlnx/bitmap.cpp
        ../nlnx/node.cpp
        ../nlnx/file.cpp
        ../nlnx/audio.cpp
        ../nlnx/nx.cpp
        includes/lz4.c
    )
    target_compile_features(tests PRIVATE cxx_std_17)

    # MS_HEADLESS strips the GL calls; ASIO_STANDALONE/GLEW_NO_GLU match the
    # client's native compile expectations for the shared headers.
    target_compile_definitions(tests PRIVATE MS_HEADLESS ASIO_STANDALONE GLEW_NO_GLU)

    target_include_directories(tests PRIVATE
        # Hermetic GLFW key-code shim so IO/Keyboard.cpp builds without GLFW.
        "${CMAKE_CURRENT_SOURCE_DIR}/Test/glfw_shim"
        ${FREETYPE_INCLUDE_DIRS}
    )
    target_link_directories(tests PRIVATE ${FREETYPE_LIBRARY_DIRS})
    target_link_libraries(tests PRIVATE ${FREETYPE_LIBRARIES})

    # Portable warning flags only -- deliberately not the Linux GL/X11 block.
    # The suppressions mirror the client's native build (CMAKE_CXX_FLAGS above):
    # the shared headers rely on narrowing being permitted, and the GraphicsGL
    # GL-handle fields are unused once MS_HEADLESS compiles the GL calls out.
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(tests PRIVATE
            -Wall -Wextra -Wno-narrowing -Wno-c++11-narrowing
            -Wno-sign-compare -Wno-unused-private-field -Wno-deprecated-declarations
        )

        # Same standard as the client: warnings fail the build (toggle with
        # -DWARNINGS_AS_ERRORS=OFF).
        if(WARNINGS_AS_ERRORS)
            target_compile_options(tests PRIVATE -Werror)
        endif()
    endif()

    # Register each doctest case as an individual ctest test so `ctest` can
    # enumerate, report, and filter them one-by-one (instead of one umbrella
    # entry wrapping the whole binary).
    include("${CMAKE_CURRENT_SOURCE_DIR}/includes/doctest/doctest.cmake")
    doctest_discover_tests(tests)
endif()
