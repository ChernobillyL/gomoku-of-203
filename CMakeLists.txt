cmake_minimum_required(VERSION 3.16)
project(GomokuConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(gomoku_core src/game.cpp)
target_include_directories(gomoku_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(gomoku_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(gomoku_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(gomoku src/main.cpp)
target_link_libraries(gomoku PRIVATE gomoku_core)

if(WIN32)
    add_executable(gomoku_gui WIN32 src/win32_ui.cpp)
    target_link_libraries(gomoku_gui PRIVATE gomoku_core user32 gdi32)
    set_target_properties(gomoku_gui PROPERTIES
        OUTPUT_NAME "五子棋"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_target_properties(gomoku_gui PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_SOURCE_DIR}"
        )
    endforeach()
    if(MSVC)
        target_compile_options(gomoku_gui PRIVATE /W4 /permissive-)
    elseif(MINGW)
        target_compile_options(gomoku_gui PRIVATE -Wall -Wextra -Wpedantic)
        target_link_options(gomoku_gui PRIVATE -static-libgcc -static-libstdc++)
    else()
        target_compile_options(gomoku_gui PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_executable(gomoku_tests tests/test_game.cpp)
target_link_libraries(gomoku_tests PRIVATE gomoku_core)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME gomoku_core_tests COMMAND gomoku_tests)
endif()
