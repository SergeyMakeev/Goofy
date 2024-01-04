cmake_minimum_required(VERSION 3.13)
project(GoofyTC)

set(PROJECT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT})
set(CMAKE_INSTALL_PREFIX ${PROJECT_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
option(ENABLE_ADDRESS_SANITIZER "compiles atb with address sanitizer enabled (only debug, works only on g++ and clang)" OFF)
option(ENABLE_LONG_TEST_RUN "Switch this off to have way shorter tests" ON)
option(ENABLE_WASM_STANDALONE "Switch this on to generate a standalone wasm file (no js glue, good for running in e.g. wasmer)" ON)

if (ENABLE_WASM_STANDALONE)
    set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
endif()

add_subdirectory(Src)

