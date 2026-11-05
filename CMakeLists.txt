# Project Gotham Racing (original Xbox), statically recompiled

cmake_minimum_required(VERSION 3.25)
project(game LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# the release, by assets/default.xbe: each disc is another compile (config/<prefix>*.toml, every address ported;
# PGR1_RELEASE picks the hooks' addresses, src/common/release.h). None of these: Europe 1.1, config/pgr1.toml
set(PGR1_PREFIX "")
set(PGR1_RELEASE 0)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/default.xbe)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS assets/default.xbe)  # another disc: another config
    file(SHA1 ${CMAKE_CURRENT_SOURCE_DIR}/assets/default.xbe PGR1_XBE_SHA1)
    if(PGR1_XBE_SHA1 STREQUAL "ba7763e6e946ab40aed6706f2c9bc1df3a990fa0")  # Europe and Australia 1.0
        set(PGR1_PREFIX "e10_")
        set(PGR1_RELEASE 1)
    elseif(PGR1_XBE_SHA1 STREQUAL "0d55fff29ceb34ab4dfcfc4820e823d967ca1469" OR PGR1_XBE_SHA1 STREQUAL "c14b35767fddc5a7af4b3aa13b53e374be23fe57")
        # USA, and Asia: its xbe is USA's code, every section the same, its certificate's version 6
        set(PGR1_PREFIX "usa_")
        set(PGR1_RELEASE 2)
    elseif(PGR1_XBE_SHA1 STREQUAL "4c38cf630f7dc92a845d2141020157e78e682e3d")  # Japan (Project Gotham: World Street Racer)
        set(PGR1_PREFIX "jpn_")
        set(PGR1_RELEASE 3)
    endif()
endif()
message(STATUS "PGR1 release: ${PGR1_RELEASE}")

# on Windows the game's code is a DLL built on the player's PC (sdk's cmake/XbeRecomp.cmake, XBE_GAME_DLL); the harness
# links it in. The package carries this disc's two configs
if(WIN32)
    set(XBE_GAME_DLL ON)
    set(XBE_GAME_CONFIGS ${CMAKE_CURRENT_SOURCE_DIR}/config/${PGR1_PREFIX}game.toml
                         ${CMAKE_CURRENT_SOURCE_DIR}/config/${PGR1_PREFIX}pgr1.toml)
endif()

# the SDK: sdk/ here, two folders up inside the SDK's templates, a checkout beside this repository, else downloaded
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../..)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBE-Recomp/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBE-Recomp)
else()
    include(FetchContent)
    FetchContent_Declare(xbe_sdk GIT_REPOSITORY https://github.com/hk21x/XboxXBERecomp.git GIT_TAG main)
    FetchContent_MakeAvailable(xbe_sdk)
    set(SDK ${xbe_sdk_SOURCE_DIR})
endif()
include(${SDK}/cmake/XbeRecomp.cmake)

file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
list(FILTER XBE_HOST_SOURCES EXCLUDE REGEX "/src/game/")


# partitions as xbe_symbols wrote them into config/game.toml
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS config/${PGR1_PREFIX}game.toml)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/${PGR1_PREFIX}game.toml)
    file(STRINGS config/${PGR1_PREFIX}game.toml GAME_PARTITIONS REGEX "^partitions *=")
    string(REGEX MATCH "[0-9]+" GAME_PARTITIONS "${GAME_PARTITIONS}")
endif()
# no config yet: build.bat writes it with xbe_symbols, the change reconfigures
if(NOT GAME_PARTITIONS)
    set(GAME_PARTITIONS 8)
endif()

# target name = GAME_EXE in build.bat, xbe = GAME_XBE
# config/pgr1.toml holds the hand-kept entries and includes the generated config/game.toml
add_xbe_image(PGR1 default.xbe ${PGR1_PREFIX}pgr1.toml ${GAME_PARTITIONS} game)
target_compile_definitions(PGR1 PRIVATE PGR1_RELEASE=${PGR1_RELEASE})

# launcher/icon.ico, when there is one: the game's icon too (resource 1, which the window class loads)
if(WIN32 AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/launcher/icon.ico)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/game_icon.rc "1 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/launcher/icon.ico\"\n")
    target_sources(PGR1 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/game_icon.rc)
endif()
