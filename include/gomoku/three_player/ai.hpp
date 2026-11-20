#pragma once

#include "gomoku/three_player/game.hpp"

#include <optional>

namespace gomoku {

// 三人四子棋电脑玩家。
// 每台电脑只追求自己获胜：会阻挡任何一方（包括另一台电脑）的直接胜点，
// 但不会与其他电脑协同作战。
class ThreePlayerAi {
public:
    // 返回当前轮到 aiStone 时的落子；仅当棋盘已满时返回空。
    std::optional<Move> chooseMove(const Board& board, Stone aiStone) const;
};

}  // namespace gomoku
