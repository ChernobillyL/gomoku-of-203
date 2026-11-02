#pragma once

#include "gomoku/game.hpp"

#include <charconv>
#include <cctype>
#include <string>
#include <system_error>

namespace gomoku {

enum class MoveInputStatus { Valid, InvalidFormat, OutOfRange };

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

inline MoveInputStatus parseMoveInput(const std::string& text, Move& move) {
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
    if (row < 1 || row > kBoardSize || col < 1 || col > kBoardSize) {
        return MoveInputStatus::OutOfRange;
    }
    move = {row - 1, col - 1};
    return MoveInputStatus::Valid;
}

}  // namespace gomoku
