#pragma once

#include "gomoku/six_player/game.hpp"

#include <optional>

namespace gomoku {

// 六子棋电脑玩家：Minimax + Alpha-Beta，带节点预算和局部窗口评估。
// 无可变数据成员，依赖 Board、己方颜色和本回合剩余落子数；双子回合
// 通过参数传入，AI 本身不修改对局状态。
class SixPlayerAi {
public:
    // 当轮到 aiStone 落子时选一步；额外落子规则由 SixPlayerGame 管理。
    // movesRemaining 为当前方本回合剩余子数（1 或 2，默认 2）。
    std::optional<Move> chooseMove(const Board& board, Stone aiStone,
                                   int movesRemaining = 2) const;
};

}  // namespace gomoku
