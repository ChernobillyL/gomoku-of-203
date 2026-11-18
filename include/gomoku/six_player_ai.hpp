#pragma once

#include "gomoku/six_player.hpp"

#include <optional>

namespace gomoku {

class SixPlayerAi {
public:
    // 当轮到 aiStone 落子时选一步；额外落子规则由 SixPlayerGame 管理。
    std::optional<Move> chooseMove(const Board& board, Stone aiStone,
                                   int movesRemaining = 2) const;
};

}  // namespace gomoku
