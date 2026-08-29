cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0091 NEW)

project(AlienShooter C CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/reccmp.cmake")

set(VC6_FOR_RECCMP FALSE)
if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
    set(VC6_FOR_RECCMP TRUE)
endif()

if(VC6_FOR_RECCMP)
    set(CMAKE_CXX_FLAGS "/W3 /G6 /DWIN32 /D_WINDOWS /DENABLE_DECOMP_ASSERTS")
    # /FAs is needed for reccmp/get_stack_slots.py.
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Z7 /FAs /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Z7 /D_DEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

    set(CMAKE_EXE_LINKER_FLAGS "/MACHINE:IX86")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /INCREMENTAL:NO /OPT:NOREF")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG /INCREMENTAL:NO /OPT:NOREF")

    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
    endif()
endif()

file(GLOB_RECURSE ALIEN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

set(NAMED_LIST_LOGICVAR_PROVIDER
    "${CMAKE_CURRENT_SOURCE_DIR}/src/util/named_list_logicvar_expand.cpp")
list(REMOVE_ITEM ALIEN_SOURCES "${NAMED_LIST_LOGICVAR_PROVIDER}")
list(INSERT ALIEN_SOURCES 0 "${NAMED_LIST_LOGICVAR_PROVIDER}")

set(PICTURE_MAKEVID_GETPIXELT_PROVIDER
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/picture_makevid_getpixelt.cpp")
list(REMOVE_ITEM ALIEN_SOURCES "${PICTURE_MAKEVID_GETPIXELT_PROVIDER}")
list(INSERT ALIEN_SOURCES 0 "${PICTURE_MAKEVID_GETPIXELT_PROVIDER}")

set(NAMED_LIST_STRING_PROVIDER
    "${CMAKE_CURRENT_SOURCE_DIR}/src/util/named_list_string.cpp")
list(REMOVE_ITEM ALIEN_SOURCES "${NAMED_LIST_STRING_PROVIDER}")
list(INSERT ALIEN_SOURCES 0 "${NAMED_LIST_STRING_PROVIDER}")

add_executable(alien WIN32 ${ALIEN_SOURCES})

set(ALIEN_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(ALIEN_RESOURCE_RES "${ALIEN_RESOURCE_DIR}/AlienShooter.res")
if(NOT EXISTS "${ALIEN_RESOURCE_RES}")
    message(FATAL_ERROR "Checked-in resource is missing: ${ALIEN_RESOURCE_RES}")
endif()
set_source_files_properties("${ALIEN_RESOURCE_RES}" PROPERTIES
    EXTERNAL_OBJECT TRUE
)
target_sources(alien PRIVATE "${ALIEN_RESOURCE_RES}")

target_include_directories(alien PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx8/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx8/inc/dxsdk"
)
target_link_directories(alien PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dx8/lib/x86/dxsdk"
)
target_link_libraries(alien PRIVATE
    kernel32 user32 gdi32 winmm advapi32 ole32 comdlg32 shell32
    d3d8 d3dx8 dsound
)

set_property(TARGET alien PROPERTY OUTPUT_NAME "AlienShooter")
set_property(TARGET alien PROPERTY SUFFIX ".exe")

if(VC6_FOR_RECCMP)
    set_property(TARGET alien PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    # VC6 cl cannot compile in parallel
    set_property(GLOBAL PROPERTY JOB_POOLS vc6_single=1)
    set_property(TARGET alien PROPERTY JOB_POOL_COMPILE vc6_single)
endif()

reccmp_add_target(alien ID ALIEN)
reccmp_configure()
