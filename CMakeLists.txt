cmake_minimum_required(VERSION 3.16)
project(GomokuSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
    # 静态链接 MSVC 运行库，使 五子棋.exe 不依赖 VC++ Redistributable。
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

function(gomoku_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# 公共层：棋盘、棋子、着法、对局抽象接口。三个游戏模块共同依赖这一层。
# ---------------------------------------------------------------------------
add_library(gomoku_common src/core/board.cpp)
target_include_directories(gomoku_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
gomoku_set_warnings(gomoku_common)

# ---------------------------------------------------------------------------
# 三个可独立开发、独立测试的游戏模块。
# ---------------------------------------------------------------------------
add_library(gomoku_classic src/classic/game.cpp)
target_link_libraries(gomoku_classic PUBLIC gomoku_common)
gomoku_set_warnings(gomoku_classic)

add_library(gomoku_three_player
    src/three_player/game.cpp
    src/three_player/ai.cpp
)
target_link_libraries(gomoku_three_player PUBLIC gomoku_common)
gomoku_set_warnings(gomoku_three_player)

add_library(gomoku_six_player
    src/six_player/game.cpp
    src/six_player/ai.cpp
)
target_link_libraries(gomoku_six_player PUBLIC gomoku_common)
gomoku_set_warnings(gomoku_six_player)

# ---------------------------------------------------------------------------
# 组合后的系统入口：控制台程序与 Win32 图形界面共用三个游戏模块。
# ---------------------------------------------------------------------------
add_executable(gomoku src/frontend/console.cpp)
target_link_libraries(gomoku PRIVATE
    gomoku_classic
    gomoku_three_player
    gomoku_six_player
)
gomoku_set_warnings(gomoku)

if(WIN32)
    # 默认把可双击运行的 五子棋.exe 放在项目根目录；打包或验证时可覆盖此变量，
    # 避免覆盖用户正在运行的程序。
    set(GOMOKU_GUI_OUTPUT_DIR "${CMAKE_SOURCE_DIR}" CACHE PATH
        "Directory for the Win32 GUI executable")

    add_executable(gomoku_gui WIN32 src/frontend/win32_ui.cpp)
    target_link_libraries(gomoku_gui PRIVATE
        gomoku_classic
        gomoku_three_player
        gomoku_six_player
        user32
        gdi32
    )
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
        RUNTIME_OUTPUT_DIRECTORY "${GOMOKU_GUI_OUTPUT_DIR}"
    )
    foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_target_properties(gomoku_gui PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${config} "${GOMOKU_GUI_OUTPUT_DIR}"
        )
    endforeach()
    if(MINGW)
        target_link_options(gomoku_gui PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    gomoku_set_warnings(gomoku_gui)
endif()

# ---------------------------------------------------------------------------
# 每个游戏模块拥有独立测试；公共层由经典五子棋测试一并覆盖。
# ---------------------------------------------------------------------------
add_executable(gomoku_classic_tests tests/classic/test_game.cpp)
target_link_libraries(gomoku_classic_tests PRIVATE gomoku_classic)
gomoku_set_warnings(gomoku_classic_tests)

add_executable(gomoku_three_player_tests tests/three_player/test_three_player.cpp)
target_link_libraries(gomoku_three_player_tests PRIVATE gomoku_three_player)
gomoku_set_warnings(gomoku_three_player_tests)

add_executable(gomoku_six_player_tests tests/six_player/test_six_player.cpp)
target_link_libraries(gomoku_six_player_tests PRIVATE gomoku_six_player)
gomoku_set_warnings(gomoku_six_player_tests)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME classic_tests COMMAND gomoku_classic_tests)
    add_test(NAME three_player_tests COMMAND gomoku_three_player_tests)
    add_test(NAME six_player_tests COMMAND gomoku_six_player_tests)
endif()
