cmake_minimum_required(VERSION 3.20)

project(warp_solidity
        VERSION 0.0.1)

include(ExternalProject)

set(CMAKE_CXX_STANDARD 20)

cmake_path(GET CMAKE_CURRENT_BINARY_DIR PARENT_PATH ROOT_DIR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -I${ROOT_DIR}/src/vendor/intx/include")

ExternalProject_Add(
        solidity-project

        GIT_REPOSITORY "https://github.com/ethereum/solidity.git"
        GIT_TAG "e28d00a76daa0747f00ffe47cea17862bca55771"

        PREFIX vendor

        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

        BUILD_COMMAND
            ${CMAKE_COMMAND} --build libsolidity

        INSTALL_COMMAND ""
)

set(SOLIDITY_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/vendor/src/solidity-project)
set(SOLIDITY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/vendor/src/solidity-project-build)

add_library(libsolidity SHARED IMPORTED)
set_target_properties(libsolidity PROPERTIES
        IMPORTED_LOCATION ${SOLIDITY_BINARY_DIR}/libsolidity/libsolidity.a)

add_library(liblangutil SHARED IMPORTED)
set_target_properties(liblangutil PROPERTIES
        IMPORTED_LOCATION ${SOLIDITY_BINARY_DIR}/liblangutil/liblangutil.a)

add_library(libsolutil SHARED IMPORTED)
set_target_properties(libsolutil PROPERTIES
        IMPORTED_LOCATION ${SOLIDITY_BINARY_DIR}/libsolutil/libsolutil.a)

add_library(libyul SHARED IMPORTED)
set_target_properties(libyul PROPERTIES
        IMPORTED_LOCATION ${SOLIDITY_BINARY_DIR}/libyul/libyul.a)

add_library(libevmasm SHARED IMPORTED)
set_target_properties(libevmasm PROPERTIES
        IMPORTED_LOCATION ${SOLIDITY_BINARY_DIR}/libevmasm/libevmasm.a)
target_link_libraries(libevmasm INTERFACE liblangutil)

add_library(solidity-deps SHARED IMPORTED)

add_library(full-solidity INTERFACE)
target_link_libraries(full-solidity INTERFACE
        libsolidity liblangutil libsolutil libyul libevmasm)

set (WARP_LIBS src/Parser.h src/library.h src/library.cpp src/AsmParser.h 
        src/AsmParser.cpp src/ObjectParser.h src/ObjectParser.cpp 
        src/vendor/intx/include/intx/int128.hpp
        src/vendor/intx/include/intx/intx.hpp)
        
add_library(warp-solidity ${WARP_LIBS})
add_dependencies(warp-solidity solidity-project)
target_include_directories(warp-solidity
        PUBLIC ${SOLIDITY_SOURCE_DIR} ${SOLIDITY_BINARY_DIR}/deps/include)
target_link_libraries(warp-solidity full-solidity)

add_executable(check check.cpp)
target_link_libraries(check warp-solidity)
