#pragma once

#include "gomoku/core/board.hpp"

#include <charconv>
#include <cctype>
#include <string>
#include <system_error>

namespace gomoku {

enum class MoveInputStatus { Valid, InvalidFormat, OutOfRange };

// 参数：text 为用户输入的一行文本，choice 用于输出解析出的整数。
// 返回：整行只包含一个整数（允许前后空白）时为 true。
// 实现：用 from_chars 解析，并检查解析指针是否到达行尾，拒绝 "1 2" 之类输入。
inline bool parseMenuInput(const std::string& text, int& choice) {
    const char* cursor = text.data();
    const char* const end = cursor + text.size();
    while (cursor < end && std::isspace(static_cast<unsigned char>(*cursor)) != 0) ++cursor;
    const char* begin = cursor;
    while (cursor < end && std::isspace(static_cast<unsigned char>(*cursor)) == 0) ++cursor;
    if (begin == cursor) return false;
    int parsedValue = 0;
    const auto parsed = std::from_chars(begin, cursor, parsedValue);
    if (parsed.ec != std::errc{} || parsed.ptr != cursor) return false;
    while (cursor < end && std::isspace(static_cast<unsigned char>(*cursor)) != 0) ++cursor;
    if (cursor != end) return false;
    choice = parsedValue;
    return true;
}

// 参数：text 为“行 列”格式的 1 基输入，move 输出 0 基坐标，boardSize 为棋盘路数。
// 返回：合法 / 格式错误 / 越界三种状态，调用方可据此给出不同提示。
// 实现：严格解析两个整数，要求二者之间至少一个空白且行尾无多余字符；
// 通过范围检查后再统一减 1，避免把越界值写入 move。
inline MoveInputStatus parseMoveInput(const std::string& text, Move& move,
                                      int boardSize = kBoardSize) {
    const char* cursor = text.data();
    const char* const end = cursor + text.size();
    const auto skipWhitespace = [&cursor, end]() {
        while (cursor < end && std::isspace(static_cast<unsigned char>(*cursor)) != 0) ++cursor;
    };
    const auto parseNumber = [&cursor, end](int& value) {
        const char* begin = cursor;
        while (cursor < end && std::isspace(static_cast<unsigned char>(*cursor)) == 0) ++cursor;
        if (begin == cursor) return false;
        const auto parsed = std::from_chars(begin, cursor, value);
        return parsed.ec == std::errc{} && parsed.ptr == cursor;
    };

    skipWhitespace();
    int row = 0;
    int col = 0;
    if (!parseNumber(row)) return MoveInputStatus::InvalidFormat;
    if (cursor == end || std::isspace(static_cast<unsigned char>(*cursor)) == 0) {
        return MoveInputStatus::InvalidFormat;
    }
    skipWhitespace();
    if (!parseNumber(col)) return MoveInputStatus::InvalidFormat;
    skipWhitespace();
    if (cursor != end) return MoveInputStatus::InvalidFormat;
    if (row < 1 || row > boardSize || col < 1 || col > boardSize) {
        return MoveInputStatus::OutOfRange;
    }
    move = {row - 1, col - 1};
    return MoveInputStatus::Valid;
}

}  // namespace gomoku
