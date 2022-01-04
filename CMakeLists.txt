#############################################################################
#Copyright (c) 2022,                                          
#                                                                         
#Distributed under the terms of the GNU General Public License v3.                 
#                                                                         
#The full license is in the file LICENSE, distributed with this software. 
#############################################################################

cmake_minimum_required(VERSION 3.4.3)
project(xeus-r)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(XEUS_R_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Versionning
# ===========

file(STRINGS "${XEUS_R_INCLUDE_DIR}/xeus-r/xeus_r_config.hpp" version_defines
     REGEX "#define XEUS_R_VERSION_(MAJOR|MINOR|PATCH)")
foreach (ver ${version_defines})
    if (ver MATCHES "#define XEUS_R_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XEUS_R_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif ()
endforeach ()
set(${PROJECT_NAME}_VERSION
    ${XEUS_R_VERSION_MAJOR}.${XEUS_R_VERSION_MINOR}.${XEUS_R_VERSION_PATCH})
message(STATUS "Building xeus-r v${${PROJECT_NAME}_VERSION}")

# Configuration
# =============

include(GNUInstallDirs)

if (NOT DEFINED XEUS_R_KERNELSPEC_PATH)
    set(XEUS_R_KERNELSPEC_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/")
endif ()

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xr/kernel.json.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xr/kernel.json"
)

# Build options
# =============

# Compilation options
OPTION(XEUS_R_DISABLE_ARCH_NATIVE "disable -march=native flag" OFF)
OPTION(XEUS_R_DISABLE_TUNE_GENERIC "disable -mtune=generic flag" OFF)

option(XEUS_R_BUILD_STATIC "Build xeus-r static library" ON)
OPTION(XEUS_R_BUILD_SHARED "Split xr build into executable and library" ON)
OPTION(XEUS_R_BUILD_EXECUTABLE "Build the xr executable" ON)

OPTION(XEUS_R_USE_SHARED_XEUS "Link xr  with the xeus shared library (instead of the static library)" ON)
OPTION(XEUS_R_USE_SHARED_XEUS_R "Link xr  with the xeus shared library (instead of the static library)" ON)
OPTION(XEUS_R_EMSCRIPTEN_WASM_BUILD "Build for wasm with emscripten" OFF)


if(XEUS_R_EMSCRIPTEN_WASM_BUILD)
    add_compile_definitions(XEUS_R_EMSCRIPTEN_WASM_BUILD)
    message("Build with emscripten")
    SET(XEUS_R_DISABLE_ARCH_NATIVE ON)
    SET(XEUS_R_DISABLE_TUNE_GENERIC ON)
    SET(XEUS_R_BUILD_STATIC ON)
    SET(XEUS_R_BUILD_SHARED OFF)
    SET(XEUS_R_BUILD_EXECUTABLE OFF)
    SET(XEUS_R_USE_SHARED_XEUS OFF)
    SET(XEUS_R_USE_SHARED_XEUS_R OFF)
    SET(XEUS_R_BUILD_TESTS OFF)
endif()


# Dependencies
# ============

set(xtl_REQUIRED_VERSION 0.6.23)
set(xeus_REQUIRED_VERSION 2.0.0)

if (NOT TARGET xtl)
    find_package(xtl ${xtl_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET xeus AND NOT TARGET xeus-static)
    find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
endif ()

find_package(R REQUIRED)
include_directories(${R_INCLUDE_DIR} SYSTEM)

# Locate Rcpp by invoking R
execute_process(
    COMMAND ${R_COMMAND} -q -e "find.package('Rcpp')"
    OUTPUT_VARIABLE RCPP_FIND_RESULT_STRING
    RESULT_VARIABLE RCPP_FIND_RESULT)

if(NOT RCPP_FIND_RESULT_STRING)
    message(SEND_ERROR "Rcpp not installed or not found!")
else()
    string(REGEX MATCH "\".+\"" RCPP_LOCATION_LINE ${RCPP_FIND_RESULT_STRING})
    string(REGEX REPLACE "\"" "" RCPP_LOCATION_LINE ${RCPP_LOCATION_LINE})
    string(CONCAT Rcpp_INCLUDE_DIRS ${RCPP_LOCATION_LINE} "/include/")
    message("RCpp Include Directory: " ${Rcpp_INCLUDE_DIRS})
    include_directories(SYSTEM ${Rcpp_INCLUDE_DIRS})
endif()

# Locate RInside by invoking R
execute_process(
    COMMAND ${R_COMMAND} -q -e "find.package('RInside')"
    OUTPUT_VARIABLE RINSIDE_FIND_RESULT_STRING
    RESULT_VARIABLE RINSIDE_FIND_RESULT)
if(NOT RINSIDE_FIND_RESULT_STRING)
    message(SEND_ERROR "RInside not installed or not found!")
else()
    string(REGEX MATCH "\".+\"" RINSIDE_LOCATION_LINE ${RINSIDE_FIND_RESULT_STRING})
    string(REGEX REPLACE "\"" "" RINSIDE_LOCATION_LINE ${RINSIDE_LOCATION_LINE})
    string(CONCAT RInside_INCLUDE_DIRS ${RINSIDE_LOCATION_LINE} "/include/")
    message("RInside Include Directory: " ${RInside_INCLUDE_DIRS})

    set(OLD_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES} ".dll")
    find_library(RINSIDE_LIBRARIES
        NAMES RInside
        HINTS ${RINSIDE_LOCATION_LINE}/lib ${RINSIDE_LOCATION_LINE}/libs/${R_LIB_ARCH}
    )
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${OLD_SUFFIXES})
    include_directories(SYSTEM ${RInside_INCLUDE_DIRS})
endif()

# Flags
# =====
include(CheckCXXCompilerFlag)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4141")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4267 /wd4715 /wd4146 /wd4129")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    if(NOT XEUS_R_EMSCRIPTEN_WASM_BUILD)    
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-parameter -Wextra -Wreorder")
    endif()
    if (XEUS_R_DISABLE_ARCH_NATIVE AND NOT XEUS_R_DISABLE_TUNE_GENERIC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic")
    elseif (XEUS_R_DISABLE_TUNE_GENERIC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif ()


    CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP_14_FLAG)
    if (HAS_CPP_14_FLAG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    else ()
        message(FATAL_ERROR "Unsupported compiler -- xeus requires C++14 support!")
    endif ()

endif ()


if(XEUS_R_EMSCRIPTEN_WASM_BUILD)

    include(XeusEmscriptenWasmFlags)

    # async
    set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} --bind")
    set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} -s ASYNCIFY=1")
    #set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} -s 'ASYNCIFY_IMPORTS=[\"async_get_input_function\"]'")
    set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} -s 'ASYNCIFY_STACK_SIZE=20480'")

    # ENV (https://github.com/emscripten-core/emscripten/commit/6d9681ad04f60b41ef6345ab06c29bbc9eeb84e0)
    set(EMSCRIPTEN_FEATURES "${EMSCRIPTEN_FEATURES} -s \"EXTRA_EXPORTED_RUNTIME_METHODS=[ENV']\"")

    # filesystem
    #file(COPY asset_dir DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    #set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} --embed-file asset_dir")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${XEUS_EMSCRIPTEN_WASM_FLAGS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS}  ${XEUS_EMSCRIPTEN_WASM_FLAGS} ---proxy-to-worker")
endif()



# Source files
# ============

set(XEUS_R_HEADERS
    include/xeus-r/xeus_r_config.hpp
    include/xeus-r/xinterpreter.hpp
)

set(XEUS_R_SRC
    src/xinterpreter.cpp 
)

set(XEUS_R_MAIN_SRC
    src/main.cpp
)


# Targets and link - Macros
# =========================

include(CheckCXXCompilerFlag)

string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib; ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

macro(xeus_r_set_common_options target_name)
    if (MSVC)
        target_compile_options(${target_name} PUBLIC /wd4251 /wd4141)
        target_compile_options(${target_name} PUBLIC /wd4018 /wd4267 /wd4715 /wd4146 /wd4129)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Intel")

        target_compile_options(${target_name} PUBLIC -Wunused-parameter -Wextra -Wreorder)

        # Mtune generic/native
        if (XEUS_R_DISABLE_ARCH_NATIVE AND NOT XEUS_R_DISABLE_TUNE_GENERIC)
            target_compile_options(${target_name} PUBLIC -mtune=generic)
        elseif (XEUS_R_DISABLE_TUNE_GENERIC)
        else ()
            target_compile_options(${target_name} PUBLIC -march=native)
        endif ()

        # C++14 flag
        CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP_14_FLAG)
        if (HAS_CPP_14_FLAG)
            target_compile_features(${target_name} PRIVATE cxx_std_14)
        else ()
            message(FATAL_ERROR "Unsupported compiler -- xeus-r requires C++14 support!")
        endif ()
    endif ()

    if (APPLE)
        set_target_properties(${target_name} PROPERTIES
            MACOSX_RPATH ON
        )
    else ()
        set_target_properties(${target_name} PROPERTIES
            BUILD_WITH_INSTALL_RPATH 1
            SKIP_BUILD_RPATH FALSE
        )
    endif ()

    set_target_properties(${target_name} PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endmacro()

# Common macro kernels (xr )
macro(xeus_r_set_kernel_options target_name)
    target_link_libraries(${target_name} PRIVATE ${R_LIBRARIES} ${RINSIDE_LIBRARIES}  )
    if (XEUS_R_USE_SHARED_XEUS_R)
        target_link_libraries(${target_name} PRIVATE xeus-r)
        if(CMAKE_DL_LIBS)
            target_link_libraries(${target_name} PRIVATE ${CMAKE_DL_LIBS} util)
        endif()
    else ()
        target_link_libraries(${target_name} PRIVATE xeus-r-static)
    endif()

    if(NOT XEUS_R_EMSCRIPTEN_WASM_BUILD)
        find_package(Threads) 
        target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT}  )
    else()
        # add something to link here
        # target_link_libraries(${target_name} PRIVATE  
        #     ${SOME_LIBRARIES})
    endif()

endmacro()

# Common macro for shared and static library
macro(xeus_r_create_target target_name linkage output_name)
    string(TOUPPER "${linkage}" linkage_upper)

    if (NOT ${linkage_upper} MATCHES "^(SHARED|STATIC)$")
        message(FATAL_ERROR "Invalid library linkage: ${linkage}")
    endif ()

    add_library(${target_name} ${linkage_upper} ${XEUS_R_SRC} ${XEUS_R_HEADERS})
    xeus_r_set_common_options(${target_name})

    set_target_properties(${target_name} PROPERTIES
                          PUBLIC_HEADER "${XEUS_R_HEADERS}"
                          PREFIX ""
                          VERSION ${${PROJECT_NAME}_VERSION}
                          SOVERSION ${XEUS_R_VERSION_MAJOR}
                          OUTPUT_NAME "lib${output_name}")

    target_compile_definitions(${target_name} PUBLIC "XEUS_R_EXPORTS")

    target_compile_features(${target_name} PRIVATE cxx_std_14)

    target_include_directories(${target_name}
                               PUBLIC
                               $<BUILD_INTERFACE:${XEUS_R_INCLUDE_DIR}>
                               $<INSTALL_INTERFACE:include>)

    if (XEUS_R_USE_SHARED_XEUS)
        set(XEUS_R_XEUS_TARGET xeus)
    else ()
        set(XEUS_R_XEUS_TARGET xeus-static)
    endif ()

    target_link_libraries(${target_name} PUBLIC ${XEUS_R_XEUS_TARGET} xtl )
    if (WIN32 OR CYGWIN)
        # 
    elseif (APPLE)
        target_link_libraries(${target_name} PRIVATE "-undefined dynamic_lookup")
    endif ()

    if(NOT XEUS_R_EMSCRIPTEN_WASM_BUILD)
        find_package(Threads) # TODO: add Threads as a dependence of xeus-static?
        target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
    else()
        # add some library here
        #target_link_libraries(${target_name} PRIVATE  ${SOME_LIBRARIES})
    endif()
endmacro()

# xeus-r
# ===========

set(XEUS_R_TARGETS "")

if (XEUS_R_BUILD_SHARED)
    # Build libraries
    xeus_r_create_target(xeus-r SHARED xeus-r)
    list(APPEND XEUS_R_TARGETS xeus-r)
endif ()

if (XEUS_R_BUILD_STATIC)
    # On Windows, a static library should use a different output name
    # to avoid the conflict with the import library of a shared one.
    if (CMAKE_HOST_WIN32)
        xeus_r_create_target(xeus-r-static STATIC xeus-r-static)
    else ()
        xeus_r_create_target(xeus-r-static STATIC xeus-r)
    endif ()
    list(APPEND XEUS_R_TARGETS xeus-r-static)
endif ()

# xeus-r
# =======
if (XEUS_R_BUILD_EXECUTABLE)
    add_executable(xr ${XEUS_R_MAIN_SRC})
    target_compile_features(xr PRIVATE cxx_std_14)
    xeus_r_set_common_options(xr)
    xeus_r_set_kernel_options(xr)
endif()


if(XEUS_R_EMSCRIPTEN_WASM_BUILD)
    add_executable(xeus_kernel src/main_emscripten_kernel.cpp )
    target_link_libraries(xeus_kernel  xeus-r-static)
endif()


# Installation
# ============

include(CMakePackageConfigHelpers)

set(XEUS_R_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING "install path for xeus-rConfig.cmake")

# Install xeus-r and xeus-r-static
if (XEUS_R_BUILD_SHARED)
    install(TARGETS ${XEUS_R_TARGETS}
            EXPORT ${PROJECT_NAME}-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xeus-r)

    # Makes the project importable from the build directory
    export(EXPORT ${PROJECT_NAME}-targets
           FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
endif ()

# Install xr
if (XEUS_R_BUILD_EXECUTABLE)
    install(TARGETS xr
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Configuration and data directories for jupyter and xeus-r
    set(XJUPYTER_DATA_DIR "share/jupyter"    CACHE STRING "Jupyter data directory")

    # Install xr Jupyter kernelspec
    set(KERNELSPEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels)
    install(DIRECTORY ${KERNELSPEC_DIR}
            DESTINATION ${XJUPYTER_DATA_DIR}
            PATTERN "*.in" EXCLUDE)


    # Extra path for installing Jupyter kernelspec
    if (XEXTRA_JUPYTER_DATA_DIR)
        install(DIRECTORY ${KERNELSPEC_DIR}
                DESTINATION ${XEXTRA_JUPYTER_DATA_DIR}
                PATTERN "*.in" EXCLUDE)
    endif ()
endif ()

# Configure 'xeus-rConfig.cmake' for a build tree
set(XEUS_R_CONFIG_CODE "####### Expanded from \@XEUS_R_CONFIG_CODE\@ #######\n")
set(XEUS_R_CONFIG_CODE "${XEUS_R_CONFIG_CODE}set(CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/cmake;\${CMAKE_MODULE_PATH}\")\n")
set(XEUS_R_CONFIG_CODE "${XEUS_R_CONFIG_CODE}##################################################")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${PROJECT_BINARY_DIR})

# Configure 'xeus-rConfig.cmake.in for an install tree
set(XEUS_R_CONFIG_CODE "")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${XEUS_R_CMAKECONFIG_INSTALL_DIR})

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${${PROJECT_NAME}_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${XEUS_R_CMAKECONFIG_INSTALL_DIR})

if (XEUS_R_BUILD_SHARED)
    install(EXPORT ${PROJECT_NAME}-targets
            FILE ${PROJECT_NAME}Targets.cmake
            DESTINATION ${XEUS_R_CMAKECONFIG_INSTALL_DIR})
endif ()
