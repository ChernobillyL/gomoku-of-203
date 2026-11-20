#pragma once

#include "gomoku/core/board.hpp"
#include "gomoku/core/game_base.hpp"

#include <optional>
#include <vector>

namespace gomoku {

constexpr int kThreePlayerWinLength = 4;

// 三人局轮换顺序：黑 -> 白 -> 红 -> 黑。
Stone nextStone(Stone stone);

// 检查最近一手是否让 stone 形成四子及以上连线。
bool formsFourInRow(const Board& board, Move lastMove, Stone stone);

// 三人四子棋：黑白红三方依次落子，先连成四子者胜。
// 数据成员：board_ 为共用棋盘；currentPlayer_ 为当前方；winner_ 缓存胜者；
// moveHistory_ 保存棋谱；restrictOpenThree_ 控制是否启用同屏活三限制。
// 关系：公有继承 GameBase，组合一个 Board；轮换规则由 nextStone() 提供。
class ThreePlayerGame : public GameBase {
public:
    // restrictOpenThree 启用同屏规则：其他玩家已有死三或活三时，
    // 当前玩家不得再形成新的活三（直接成四获胜的手除外）。
    explicit ThreePlayerGame(bool restrictOpenThree = false)
        : restrictOpenThree_(restrictOpenThree) {}

    // 以下为 GameBase 只读接口的具体实现：只查询状态，不修改棋盘或棋谱。
    const Board& board() const override { return board_; }
    Stone currentPlayer() const override { return currentPlayer_; }
    bool finished() const override { return winner_.has_value() || board_.isFull(); }
    std::optional<Stone> winner() const override { return winner_; }
    const std::vector<MoveRecord>& moveHistory() const override { return moveHistory_; }
    bool play(Move move) override;
    bool violatesOpenThreeRule(Move move) const;

private:
    Board board_;
    Stone currentPlayer_ = Stone::Black;
    std::optional<Stone> winner_;
    std::vector<MoveRecord> moveHistory_;
    bool restrictOpenThree_ = false;
};

}  // namespace gomoku
