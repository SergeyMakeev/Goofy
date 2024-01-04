
set(SOURCE_FILES
    main.cpp
    decoder.cpp
    decoder.h
    goofy_tc_reference.cpp
    goofy_tc_reference.h
    ../GoofyTC/goofy_tc.h
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(MSVC)
    target_compile_options(${PROJECT_NAME} INTERFACE /arch:SSE4.1)
else()
    target_compile_options(${PROJECT_NAME} INTERFACE -msse4.1)
endif()

if (ENABLE_ADDRESS_SANITIZER)
    message(NOTICE "building with address sanitizer enabled")
    if(MSVC)
        target_compile_options (${PROJECT_NAME} INTERFACE /fsanitize=address)
        target_link_options(${PROJECT_NAME} INTERFACE /fsanitize=address)
    else()
        target_compile_options (${PROJECT_NAME} INTERFACE -fsanitize=address)
        target_link_options(${PROJECT_NAME} INTERFACE -fsanitize=address)
    endif()
endif()

if (ENABLE_LONG_TEST_RUN)
    target_compile_definitions(${PROJECT_NAME} PUBLIC "ENABLE_LONG_TEST_RUN")
endif()
