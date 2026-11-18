cmake_minimum_required(VERSION 3.16)
project(GomokuConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
    # 静态链接 MSVC 运行库，使 五子棋.exe 不依赖 VC++ Redistributable。
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(gomoku_core src/game.cpp src/three_player.cpp src/three_player_ai.cpp)
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
    if(MINGW)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libwinpthread.a
            OUTPUT_VARIABLE MINGW_STATIC_PTHREAD
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(EXISTS "${MINGW_STATIC_PTHREAD}")
            target_link_libraries(gomoku_gui PRIVATE "${MINGW_STATIC_PTHREAD}")
        endif()
    endif()
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
        target_link_options(gomoku_gui PRIVATE -static -static-libgcc -static-libstdc++)
    else()
        target_compile_options(gomoku_gui PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_executable(gomoku_tests tests/test_game.cpp)
target_link_libraries(gomoku_tests PRIVATE gomoku_core)

add_executable(gomoku_three_player_tests tests/test_three_player.cpp)
target_link_libraries(gomoku_three_player_tests PRIVATE gomoku_core)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME gomoku_core_tests COMMAND gomoku_tests)
    add_test(NAME gomoku_three_player_tests COMMAND gomoku_three_player_tests)
endif()
