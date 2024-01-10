cmake_minimum_required(VERSION 3.13)
project(GoofyTC)

option(AA_ENABLE_ADDRESS_SANITIZER "compiles atb with address sanitizer enabled (only debug, works only on g++ and clang)" OFF)
option(AA_ENABLE_LONG_TEST_RUN "Switch this off to have way shorter tests" ON)

if(EMSCRIPTEN)
    set(AA_WWW_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE PATH "path to the install directory (for webassembly files, i.e., www directory)")
endif()

include(cmake/alp_add_git_repository.cmake)

add_subdirectory(Src)

