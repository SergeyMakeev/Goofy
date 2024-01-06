set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME}
    main.cpp
    decoder.cpp
    decoder.h
    goofy_tc_reference.cpp
    goofy_tc_reference.h
    ../GoofyTC/goofy_tc.h
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

if(EMSCRIPTEN)
    target_compile_options(${PROJECT_NAME} PUBLIC -msimd128 -msse2)
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
