set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(PROJECT_SOURCES
    main.cpp
    decoder.cpp
    decoder.h
    goofy_tc_reference.cpp
    goofy_tc_reference.h
    ../GoofyTC/goofy_tc.h
)


if(EMSCRIPTEN)
    alp_add_git_repository(qml_catch2_console URL https://github.com/AlpineMapsOrg/qml_catch2_console.git COMMITISH origin/main DO_NOT_ADD_SUBPROJECT)

    find_package(Qt6 REQUIRED COMPONENTS Core Gui)
    qt_standard_project_setup()

    include(${qml_catch2_console_SOURCE_DIR}/src/qml_catch2_console.cmake)

    add_qml_catch2_console_unittests(${PROJECT_NAME} ${PROJECT_SOURCES})
    set(INSTALL_FILES
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.js"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.wasm"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.html"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/qtloader.js"
    )
    install(FILES ${INSTALL_FILES} DESTINATION ${AA_WWW_INSTALL_DIR})

    qt_add_resources(${PROJECT_NAME} "test-data"
        BASE ".."
        FILES
        ../test-data/parrot_red.png
        ../test-data/basis/etc1/parrot_red_unpacked_rgb_ETC1_RGB_0000.png
    )
    if (AA_ENABLE_LONG_TEST_RUN)
        qt_add_resources(${PROJECT_NAME} "test-data"
            BASE ".."
            FILES
            ../test-data/parrot_red.png
            ../test-data/patterns.png
            ../test-data/pbr_bricks_albedo.png
            ../test-data/pbr_ground_albedo.png
            ../test-data/pbr_stones_albedo.png
            ../test-data/pbr_stones_normal.png
            ../test-data/pbr_head_albedo.png
            ../test-data/baboon.png
            ../test-data/lena.png
            ../test-data/monarch.png
            ../test-data/peppers.png
            ../test-data/sail.png
            ../test-data/tulips.png
            ../test-data/kodim01.png
            ../test-data/kodim02.png
            ../test-data/kodim03.png
            ../test-data/kodim04.png
            ../test-data/kodim05.png
            ../test-data/kodim06.png
            ../test-data/kodim07.png
            ../test-data/kodim08.png
            ../test-data/kodim09.png
            ../test-data/kodim10.png
            ../test-data/kodim11.png
            ../test-data/kodim12.png
            ../test-data/kodim13.png
            ../test-data/kodim14.png
            ../test-data/kodim15.png
            ../test-data/kodim16.png
            ../test-data/kodim17.png
            ../test-data/kodim18.png
            ../test-data/kodim19.png
            ../test-data/kodim20.png
            ../test-data/kodim21.png
            ../test-data/kodim22.png
            ../test-data/kodim23.png
            ../test-data/kodim24.png
            ../test-data/roblox01.png
            ../test-data/roblox02.png
            ../test-data/roblox03.png
            ../test-data/roblox04.png
            ../test-data/roblox05.png
            ../test-data/roblox06.png
            # ../test-data/basis/etc1/patterns_unpacked_rgb_ETC1_RGB_0000.png           // can't compile with that many files
            # ../test-data/basis/etc1/pbr_bricks_albedo_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/pbr_ground_albedo_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/pbr_stones_albedo_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/pbr_stones_normal_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/pbr_head_albedo_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/parrot_red_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/baboon_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/lena_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/monarch_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/peppers_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/sail_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/tulips_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim01_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim02_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim03_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim04_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim05_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim06_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim07_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim08_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim09_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim10_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim11_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim12_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim13_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim14_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim15_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim16_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim17_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim18_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim19_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim20_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim21_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim22_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim23_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/kodim24_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/roblox01_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/roblox02_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/roblox03_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/roblox04_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/roblox05_unpacked_rgb_ETC1_RGB_0000.png
            # ../test-data/basis/etc1/roblox06_unpacked_rgb_ETC1_RGB_0000.png
        )
    endif()

    # build standalone (no js code)
    # set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_compile_options(${PROJECT_NAME} PUBLIC -msimd128 -msse2)
    set_target_properties(${PROJECT_NAME} PROPERTIES QT_WASM_INITIAL_MEMORY "200MB")
    # target_link_options(${PROJECT_NAME} PUBLIC -s TOTAL_MEMORY=209715200)
else()
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)


if (AA_ENABLE_ADDRESS_SANITIZER)
    message(NOTICE "building with address sanitizer enabled")
    if(MSVC)
        target_compile_options (${PROJECT_NAME} INTERFACE /fsanitize=address)
        target_link_options(${PROJECT_NAME} INTERFACE /fsanitize=address)
    else()
        target_compile_options (${PROJECT_NAME} INTERFACE -fsanitize=address)
        target_link_options(${PROJECT_NAME} INTERFACE -fsanitize=address)
    endif()
endif()

if (AA_ENABLE_LONG_TEST_RUN)
    target_compile_definitions(${PROJECT_NAME} PUBLIC "ENABLE_LONG_TEST_RUN")
endif()
