cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0091 NEW)

project(OpenGromada VERSION 0.2.0 LANGUAGES C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "OpenGromada requires a 32-bit or 64-bit target")
endif()
math(EXPR ALIEN_POINTER_BITS "${CMAKE_SIZEOF_VOID_P} * 8")

include(GNUInstallDirs)

file(GLOB_RECURSE ALIEN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
list(LENGTH ALIEN_SOURCES ALIEN_SOURCE_COUNT)
message(STATUS
    "OpenGromada: ${ALIEN_SOURCE_COUNT} sources (${ALIEN_POINTER_BITS}-bit target ABI)")

find_package(SDL3 3.4 REQUIRED CONFIG)

set(ALIEN_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
set(ALIEN_LIB_SOURCES ${ALIEN_SOURCES})
list(REMOVE_ITEM ALIEN_LIB_SOURCES "${ALIEN_MAIN}")

add_library(alien_core STATIC ${ALIEN_LIB_SOURCES})
target_link_libraries(alien_core PUBLIC SDL3::SDL3)

set(ALIEN_MOVIES "AUTO" CACHE STRING "Movie playback: AUTO, ON, or OFF (FFmpeg 6+)")
set_property(CACHE ALIEN_MOVIES PROPERTY STRINGS AUTO ON OFF)
if(NOT ALIEN_MOVIES MATCHES "^(AUTO|ON|OFF)$")
    message(FATAL_ERROR "ALIEN_MOVIES must be AUTO, ON, or OFF")
endif()
if(NOT ALIEN_MOVIES STREQUAL "OFF")
    unset(FFMPEG_FOUND CACHE)
    unset(FFMPEG_FOUND)
    find_package(FFMPEG QUIET)
    set(ALIEN_FFMPEG_FOUND FALSE)
    set(ALIEN_FFMPEG_ERROR "Movie playback requires FFmpeg 6+ development libraries (vcpkg or pkg-config)")
    if(FFMPEG_FOUND)
        if(NOT FFMPEG_libavformat_FOUND OR NOT FFMPEG_libavformat_VERSION OR FFMPEG_libavformat_VERSION VERSION_LESS 60
           OR NOT FFMPEG_libavcodec_FOUND OR NOT FFMPEG_libavcodec_VERSION OR FFMPEG_libavcodec_VERSION VERSION_LESS 60
           OR NOT FFMPEG_libavutil_FOUND OR NOT FFMPEG_libavutil_VERSION OR FFMPEG_libavutil_VERSION VERSION_LESS 58
           OR NOT FFMPEG_libswscale_FOUND OR NOT FFMPEG_libswscale_VERSION OR FFMPEG_libswscale_VERSION VERSION_LESS 7
           OR NOT FFMPEG_libswresample_FOUND OR NOT FFMPEG_libswresample_VERSION OR FFMPEG_libswresample_VERSION VERSION_LESS 4)
            set(ALIEN_FFMPEG_ERROR "FFMPEG package is incomplete or older than FFmpeg 6: avformat, avcodec, avutil, swscale and swresample are required")
        else()
            target_include_directories(alien_core SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
            target_link_libraries(alien_core PUBLIC ${FFMPEG_LIBRARIES})
            set(ALIEN_FFMPEG_FOUND TRUE)
        endif()
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(ALIEN_FFMPEG_PKG QUIET IMPORTED_TARGET
                libavformat>=60 libavcodec>=60 libavutil>=58 libswscale>=7 libswresample>=4)
            if(ALIEN_FFMPEG_PKG_FOUND AND TARGET PkgConfig::ALIEN_FFMPEG_PKG)
                target_link_libraries(alien_core PUBLIC PkgConfig::ALIEN_FFMPEG_PKG)
                set(ALIEN_FFMPEG_FOUND TRUE)
            endif()
        endif()
    endif()
    if(ALIEN_FFMPEG_FOUND)
        target_compile_definitions(alien_core PRIVATE OPENGROMADA_HAVE_FFMPEG=1)
        message(STATUS "Movie playback: FFmpeg enabled (local ASF/AVI)")
    elseif(ALIEN_MOVIES STREQUAL "ON")
        message(FATAL_ERROR "${ALIEN_FFMPEG_ERROR}")
    else()
        message(STATUS "Movie playback: unavailable; SDL-only build")
    endif()
else()
    message(STATUS "Movie playback: disabled")
endif()

target_compile_features(alien_core PUBLIC cxx_std_20)
set_target_properties(alien_core PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(alien_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(WIN32)
    target_compile_definitions(alien_core PUBLIC NOMINMAX)
endif()

if(MSVC)
    target_compile_options(alien_core PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:/FIutil/compat.h>")
else()
    target_compile_options(alien_core PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:-includeutil/compat.h>")
endif()

if(NOT MSVC)
    target_compile_options(alien_core PRIVATE -fno-strict-aliasing -Wall -Wextra
        -Wno-unused-parameter -Wno-missing-field-initializers)

    target_compile_options(alien_core PRIVATE
        -Werror=format -Werror=format-extra-args -Werror=format-security
        -Werror=nonnull -Wno-error=format-zero-length)
    if(CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
        target_compile_options(alien_core PRIVATE
            "$<$<COMPILE_LANGUAGE:CXX>:-Wno-delete-non-abstract-non-virtual-dtor>")
    endif()
endif()

set(ALIEN_STEAMWORKS_SDK "" CACHE PATH
    "Steamworks SDK path ('' = auto-detect ../SteamworksSDK, OFF = disable)")
if(ALIEN_STEAMWORKS_SDK STREQUAL "" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../SteamworksSDK/public/steam/steam_api.h")
    set(ALIEN_STEAMWORKS_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../SteamworksSDK")
endif()

set(ALIEN_STEAM_LIB "")
set(ALIEN_STEAM_RUNTIME "")
if(ALIEN_STEAMWORKS_SDK AND NOT ALIEN_STEAMWORKS_SDK STREQUAL "OFF")
    if(NOT EXISTS "${ALIEN_STEAMWORKS_SDK}/public/steam/steam_api.h")
        message(FATAL_ERROR "Not a Steamworks SDK: ${ALIEN_STEAMWORKS_SDK}")
    endif()
    set(ALIEN_STEAM_BIN "${ALIEN_STEAMWORKS_SDK}/redistributable_bin")
    if(WIN32)
        if(NOT MSVC)
            message(STATUS "Steamworks skipped (MSVC only on Windows)")
        elseif(CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "ARM64"
               OR CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            message(STATUS "Steamworks: no Windows arm64 redistributable. Building without Steam...")
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(ALIEN_STEAM_LIB "${ALIEN_STEAM_BIN}/win64/steam_api64.lib")
            set(ALIEN_STEAM_RUNTIME "${ALIEN_STEAM_BIN}/win64/steam_api64.dll")
        else()
            set(ALIEN_STEAM_LIB "${ALIEN_STEAM_BIN}/steam_api.lib")
            set(ALIEN_STEAM_RUNTIME "${ALIEN_STEAM_BIN}/steam_api.dll")
        endif()
    elseif(APPLE)
        set(ALIEN_STEAM_LIB "${ALIEN_STEAM_BIN}/osx/libsteam_api.dylib")
        set(ALIEN_STEAM_RUNTIME "${ALIEN_STEAM_LIB}")
    else()
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(ALIEN_STEAM_ARCH linuxarm64)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(ALIEN_STEAM_ARCH linux64)
        else()
            set(ALIEN_STEAM_ARCH linux32)
        endif()
        set(ALIEN_STEAM_LIB "${ALIEN_STEAM_BIN}/${ALIEN_STEAM_ARCH}/libsteam_api.so")
        set(ALIEN_STEAM_RUNTIME "${ALIEN_STEAM_LIB}")
    endif()
endif()

if(ALIEN_STEAM_LIB)
    message(STATUS "Steamworks: enabled (${ALIEN_STEAM_LIB})")
    target_compile_definitions(alien_core PRIVATE ALIEN_HAVE_STEAMWORKS=1)
    target_include_directories(alien_core SYSTEM PRIVATE "${ALIEN_STEAMWORKS_SDK}/public")
    if(MSVC)
        target_compile_options(alien_core PRIVATE "/Zc:__cplusplus")
    endif()
else()
    message(STATUS "Steamworks: disabled. Store is local only.")
endif()

add_executable(alien_portable "${ALIEN_MAIN}")
target_link_libraries(alien_portable PRIVATE alien_core)
set_target_properties(alien_portable PROPERTIES OUTPUT_NAME "OpenGromada")
target_compile_definitions(alien_core PUBLIC
    "OPENGROMADA_VERSION=\"${PROJECT_VERSION}\"")

add_executable(res_probe "${CMAKE_CURRENT_SOURCE_DIR}/tools/res_probe.cpp")
target_compile_features(res_probe PRIVATE cxx_std_20)

if(ALIEN_STEAM_RUNTIME)
    add_custom_command(TARGET alien_portable POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${ALIEN_STEAM_RUNTIME}" "$<TARGET_FILE_DIR:alien_portable>"
        VERBATIM)
    if(UNIX AND NOT APPLE)
        set_target_properties(alien_portable PROPERTIES
            BUILD_RPATH "\$ORIGIN"
            INSTALL_RPATH "\$ORIGIN")
    endif()
    if(APPLE)
        install(FILES "${ALIEN_STEAM_RUNTIME}"
            DESTINATION "OpenGromada.app/Contents/MacOS")
    else()
        install(FILES "${ALIEN_STEAM_RUNTIME}"
            DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
endif()

set(ALIEN_APP_ICON_PNG
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/OpenGromada.png")
if(NOT EXISTS "${ALIEN_APP_ICON_PNG}")
    message(FATAL_ERROR "Icon is missing from resources/")
endif()

if(APPLE)
    set(ALIEN_MACOS_ICON
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/OpenGromada.icns")
    if(NOT EXISTS "${ALIEN_MACOS_ICON}")
        message(FATAL_ERROR "macOS application icon is missing from resources/")
    endif()
    set_source_files_properties("${ALIEN_MACOS_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(alien_portable PRIVATE "${ALIEN_MACOS_ICON}")
    set_target_properties(alien_portable PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "OpenGromada"
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.siohaza.OpenGromada"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "OpenGromada.icns"
        MACOSX_BUNDLE_INFO_PLIST
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in")
    target_link_options(alien_portable PRIVATE "-Wl,-headerpad_max_install_names")
elseif(UNIX)
    set_property(TARGET alien_portable APPEND PROPERTY LINK_DEPENDS
        "${ALIEN_APP_ICON_PNG}")
    add_custom_command(TARGET alien_portable POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${ALIEN_APP_ICON_PNG}"
            "$<TARGET_FILE_DIR:alien_portable>/OpenGromada.png"
        VERBATIM)
endif()

if(WIN32)
    set(ALIEN_WINDOWS_RC
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/OpenGromada.rc")
    set(ALIEN_WINDOWS_RES
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/OpenGromada.res")
    if(NOT EXISTS "${ALIEN_WINDOWS_RC}" OR NOT EXISTS "${ALIEN_WINDOWS_RES}")
        message(FATAL_ERROR
            "Checked-in Windows resources are missing from resources/")
    endif()
    set_source_files_properties("${ALIEN_WINDOWS_RC}" PROPERTIES
        HEADER_FILE_ONLY TRUE)
    set_source_files_properties("${ALIEN_WINDOWS_RES}" PROPERTIES
        EXTERNAL_OBJECT TRUE)
    target_sources(alien_portable PRIVATE
        "${ALIEN_WINDOWS_RC}"
        "${ALIEN_WINDOWS_RES}")
endif()

install(TARGETS alien_portable
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE DESTINATION ".")
if(UNIX AND NOT APPLE)
    install(FILES "${ALIEN_APP_ICON_PNG}"
        DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES "${ALIEN_APP_ICON_PNG}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
        RENAME "io.github.siohaza.OpenGromada.png")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/OpenGromada-32.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps"
        RENAME "io.github.siohaza.OpenGromada.png")
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/io.github.siohaza.OpenGromada.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
endif()
