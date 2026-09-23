cmake_minimum_required(VERSION 3.21)
project(JourneyClientTests LANGUAGES C CXX)

if(EMSCRIPTEN)
    message(FATAL_ERROR "These tests use the native compiler. Build WASM with scripts/build_wasm.sh.")
endif()

enable_testing()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(TEST_ASSETS "Build tests that read the local NX assets" OFF)
option(TEST_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
get_filename_component(REPO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(TEST_ASSET_DIR "${REPO_DIR}/assets" CACHE PATH "Read-only NX asset directory")

include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
FetchContent_Declare(doctest
    URL https://codeload.github.com/doctest/doctest/tar.gz/refs/tags/v2.4.11
    URL_HASH SHA256=632ed2c05a7f53fa961381497bf8069093f0d6628c5f26286161fbd32a560186
    # Only the header and discovery script are needed, not upstream's project.
    SOURCE_SUBDIR doctest
)
FetchContent_MakeAvailable(doctest)
add_library(doctest INTERFACE)
add_library(doctest::doctest ALIAS doctest)
target_include_directories(doctest SYSTEM INTERFACE "${doctest_SOURCE_DIR}")
include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")

add_library(test_options INTERFACE)
target_include_directories(test_options INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${REPO_DIR}/src" "${REPO_DIR}/src/client/includes")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Match the client's existing allowances for legacy headers. Keep warnings fatal.
    target_compile_options(test_options INTERFACE -Werror -Wno-deprecated-declarations
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-narrowing> -ffunction-sections -fdata-sections)
    # Some production translation units contain unrelated graphics entrypoints.
    # Discard those unused functions instead of supplying fake implementations.
    if(APPLE)
        target_link_options(test_options INTERFACE -Wl,-dead_strip)
    else()
        target_link_options(test_options INTERFACE -Wl,--gc-sections)
    endif()
endif()
if(TEST_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "TEST_SANITIZERS requires Clang or GCC")
    endif()
    target_compile_options(test_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(test_options INTERFACE -fsanitize=address,undefined)
endif()

add_library(test_main STATIC support/main.cpp)
target_link_libraries(test_main PUBLIC test_options doctest::doctest)

set(CLIENT "${REPO_DIR}/src/client")
add_library(test_core STATIC
    "${CLIENT}/Net/InPacket.cpp"
    "${CLIENT}/Net/OutPacket.cpp"
    "${CLIENT}/Net/Handlers/Helpers/ItemParser.cpp"
    "${CLIENT}/Character/Inventory/Inventory.cpp"
    "${CLIENT}/Character/Inventory/InventoryType.cpp"
    "${CLIENT}/Character/Inventory/Equip.cpp"
    "${CLIENT}/Character/Inventory/Item.cpp"
    "${CLIENT}/Character/Inventory/Pet.cpp"
    "${CLIENT}/Character/Job.cpp"
    "${CLIENT}/IO/UIWindow.cpp"
    "${CLIENT}/IO/Components/NpcMenu.cpp"
)
target_link_libraries(test_core PUBLIC test_options)

function(client_test name suite)
    add_executable(${name} "${name}.cpp")
    target_link_libraries(${name} PRIVATE ${ARGN} test_main test_core)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/work/${name}")
    # Each case runs in its own process; legacy client singletons cannot leak
    # state between cases, and different executables cannot collide on output.
    doctest_discover_tests(${name}
        TEST_PREFIX "${suite}.${name}."
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/work/${name}"
        PROPERTIES LABELS "${suite}" TIMEOUT 120
    )
endfunction()

client_test(packet_test unit)
client_test(cash_shop_test unit)
client_test(ui_window_test unit)
client_test(job_requirements_test unit)
client_test(npc_menu_test unit)

if(TEST_ASSETS)
    foreach(asset Character Etc Item Npc Quest String UI UI_83)
        if(NOT EXISTS "${TEST_ASSET_DIR}/${asset}.nx")
            message(FATAL_ERROR "Asset tests require ${TEST_ASSET_DIR}/${asset}.nx. Set TEST_ASSET_DIR to an existing asset directory, or configure with TEST_ASSETS=OFF.")
        endif()
    endforeach()
    find_package(Freetype REQUIRED)
    add_library(test_graphics_headers INTERFACE)
    target_link_libraries(test_graphics_headers INTERFACE Freetype::Freetype)
    # The WASM GLEW shim needs native GL declarations when capturing draw calls.
    if(APPLE)
        target_compile_options(test_graphics_headers INTERFACE -include OpenGL/gl3.h)
    else()
        find_package(OpenGL REQUIRED)
        target_include_directories(test_graphics_headers INTERFACE ${OPENGL_INCLUDE_DIR})
        target_compile_options(test_graphics_headers INTERFACE -include GL/gl.h)
    endif()

    add_library(test_nx STATIC
        "${REPO_DIR}/src/nlnx/nx.cpp" "${REPO_DIR}/src/nlnx/node.cpp"
        "${REPO_DIR}/src/nlnx/file.cpp" "${REPO_DIR}/src/nlnx/audio.cpp"
        "${REPO_DIR}/src/nlnx/bitmap.cpp" "${CLIENT}/includes/lz4.c")
    target_link_libraries(test_nx PUBLIC test_options)

    add_library(test_asset_support STATIC support/assets.cpp support/render_capture.cpp)
    target_compile_definitions(test_asset_support PRIVATE TEST_ASSET_DIR="${TEST_ASSET_DIR}")
    target_link_libraries(test_asset_support PUBLIC test_main test_nx)

    add_library(test_asset_client STATIC
        "${CLIENT}/Data/CashShopCatalog.cpp" "${CLIENT}/Data/ItemData.cpp"
        "${CLIENT}/Data/EquipData.cpp" "${CLIENT}/Data/WeaponData.cpp" "${CLIENT}/Data/QuestData.cpp"
        "${CLIENT}/Character/CharStats.cpp" "${CLIENT}/Character/MapleStat.cpp"
        "${CLIENT}/Character/QuestLog.cpp" "${CLIENT}/Character/Inventory/Weapon.cpp"
        "${CLIENT}/Character/Look/CharLook.cpp" "${CLIENT}/Character/Look/CharEquips.cpp"
        "${CLIENT}/Character/Look/Clothing.cpp" "${CLIENT}/Character/Look/Face.cpp"
        "${CLIENT}/Character/Look/Hair.cpp" "${CLIENT}/Character/Look/Body.cpp"
        "${CLIENT}/Character/Look/BodyDrawInfo.cpp" "${CLIENT}/Character/Look/Stance.cpp"
        "${CLIENT}/Gameplay/QuestConversation.cpp" "${CLIENT}/Gameplay/MapleMap/Npc.cpp"
        "${CLIENT}/Gameplay/MapleMap/MapObject.cpp"
        "${CLIENT}/IO/Components/EquipInventoryLayout.cpp" "${CLIENT}/IO/Components/Icon.cpp"
        "${CLIENT}/IO/Components/Charset.cpp" "${CLIENT}/IO/Components/Button.cpp"
        "${CLIENT}/IO/Components/MapleButton.cpp" "${CLIENT}/IO/Components/TwoSpriteButton.cpp"
        "${CLIENT}/IO/Components/Slider.cpp" "${CLIENT}/IO/Components/NpcText.cpp"
        "${CLIENT}/IO/Components/QuestText.cpp" "${CLIENT}/IO/UIElement.cpp"
        "${CLIENT}/IO/UITypes/UIStatsInfo.cpp"
        "${CLIENT}/Graphics/Texture.cpp" "${CLIENT}/Graphics/Text.cpp"
        "${CLIENT}/Graphics/Sprite.cpp" "${CLIENT}/Graphics/Animation.cpp"
        "${CLIENT}/Util/Misc.cpp")
    target_link_libraries(test_asset_client PUBLIC test_core test_nx test_graphics_headers)

    # Boundary replacements must be linked even when callers are extracted from
    # a later archive; otherwise ELF linkers can select the production backend.
    add_library(test_inventory_stub OBJECT support/inventory_stub.cpp)
    target_link_libraries(test_inventory_stub PUBLIC test_options)
    add_library(test_texture_capture OBJECT support/texture_capture.cpp)
    target_link_libraries(test_texture_capture PUBLIC test_asset_support)

    foreach(name cash_shop_assets character_stats equip_requirements npc_buttons npc_dialog quest_ui quest_quiz)
        client_test(${name}_test assets test_asset_client test_asset_support)
    endforeach()
    foreach(name quest_rules quest_conversation npc_quest_markers)
        client_test(${name}_test assets test_inventory_stub test_asset_client test_asset_support)
    endforeach()
    foreach(name equip_inventory face_accessory)
        client_test(${name}_test assets test_texture_capture test_asset_client test_asset_support)
    endforeach()
endif()
