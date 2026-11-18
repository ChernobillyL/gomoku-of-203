#pragma once

#include "gomoku/game.hpp"

#include <optional>
#include <vector>

namespace gomoku {

constexpr int kThreePlayerWinLength = 4;

// 三人局轮换顺序：黑 -> 白 -> 红 -> 黑。
Stone nextStone(Stone stone);

// 检查最近一手是否让 stone 形成四子及以上连线。
bool formsFourInRow(const Board& board, Move lastMove, Stone stone);

// 三人四子棋对局。封装三人轮换、四子连线判胜、满盘平局与棋谱记录。
class ThreePlayerGame {
public:
    // restrictOpenThree 启用同屏规则：其他玩家已有死三或活三时，
    // 当前玩家不得再形成新的活三（直接成四获胜的手除外）。
    explicit ThreePlayerGame(bool restrictOpenThree = false)
        : restrictOpenThree_(restrictOpenThree) {}

    const Board& board() const { return board_; }
    Stone currentPlayer() const { return currentPlayer_; }
    bool finished() const { return winner_.has_value() || board_.isFull(); }
    std::optional<Stone> winner() const { return winner_; }
    const std::vector<MoveRecord>& moveHistory() const { return moveHistory_; }
    bool play(Move move);
    bool violatesOpenThreeRule(Move move) const;

private:
    Board board_;
    Stone currentPlayer_ = Stone::Black;
    std::optional<Stone> winner_;
    std::vector<MoveRecord> moveHistory_;
    bool restrictOpenThree_ = false;
};

}  // namespace gomoku
