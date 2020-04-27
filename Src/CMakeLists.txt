
set(SOURCE_FILES
    "main.cpp"
    "decoder.cpp"
    "decoder.h"
    "goofy_tc_reference.cpp"
    "goofy_tc_reference.h"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

