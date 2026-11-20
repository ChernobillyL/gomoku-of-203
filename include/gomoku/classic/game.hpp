#pragma once

#include "gomoku/core/board.hpp"
#include "gomoku/core/game_base.hpp"

#include <optional>
#include <vector>

namespace gomoku {

// 经典五子棋：15 路棋盘，黑白轮流落子，先连成五子者胜。
// 数据成员：board_ 保存棋盘；currentPlayer_ 表示当前行棋方；
// result_ 缓存对局结果；moveHistory_ 按时间顺序保存棋子与坐标。
// 关系：公有继承 GameBase，并以组合方式持有一个 Board。
class Game : public GameBase {
public:
    Game() = default;

    // 以下为 GameBase 只读接口的具体实现：只查询状态，不修改棋盘或棋谱。
    const Board& board() const override { return board_; }
    Stone currentPlayer() const override { return currentPlayer_; }
    Result result() const { return result_; }
    bool finished() const override { return result_ != Result::Ongoing; }
    std::optional<Stone> winner() const override;
    const std::vector<MoveRecord>& moveHistory() const override { return moveHistory_; }
    // 落子流程：检查对局状态 -> Board::place -> 写棋谱 -> resultAfter 判胜 -> 换手。
    // 返回 false 时棋盘、棋谱和当前方均保持不变。
    bool play(Move move) override;

private:
    Board board_;
    Stone currentPlayer_ = Stone::Black;
    Result result_ = Result::Ongoing;
    std::vector<MoveRecord> moveHistory_;
};

// 经典五子棋的启发式电脑玩家：极大极小搜索 + Alpha-Beta 剪枝。
// 无可变数据成员，只依赖传入的棋盘和己方颜色；这种无状态设计使同一 AI
// 对象可以服务多局，也便于在测试中重复调用并检查确定性。
class AiPlayer {
public:
    // 当棋盘已满时返回空。
    std::optional<Move> chooseMove(const Board& board, Stone aiStone) const;

private:
    static long long scoreMove(const Board& board, Move move, Stone stone);
};

}  // namespace gomoku
