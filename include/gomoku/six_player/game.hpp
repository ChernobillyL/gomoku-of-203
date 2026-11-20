#pragma once

#include "gomoku/core/board.hpp"
#include "gomoku/core/game_base.hpp"

#include <optional>

namespace gomoku {

constexpr int kSixPlayerBoardSize = 19;
constexpr int kSixPlayerWinLength = 6;

bool formsSixInRow(const Board& board, Move lastMove, Stone stone);

// 六子棋：19 路棋盘，黑方开局先落一子；此后双方各自每回合连续落两子，
// 先连成六子者胜。
// 数据成员：board_ 为 19 路棋盘；currentPlayer_ 为当前方；movesRemaining_
// 表示本回合还可落几子；winner_ 缓存胜者；moveHistory_ 保存棋谱。
// 关系：公有继承 GameBase，组合一个 Board。
class SixPlayerGame : public GameBase {
public:
    SixPlayerGame() : board_(kSixPlayerBoardSize) {}

    // 以下为 GameBase 只读接口的具体实现：只查询状态，不修改棋盘或棋谱。
    const Board& board() const override { return board_; }
    Stone currentPlayer() const override { return currentPlayer_; }
    int movesRemaining() const { return movesRemaining_; }
    bool finished() const override { return winner_.has_value() || board_.isFull(); }
    std::optional<Stone> winner() const override { return winner_; }
    const std::vector<MoveRecord>& moveHistory() const override { return moveHistory_; }
    bool play(Move move) override;

private:
    Board board_;
    Stone currentPlayer_ = Stone::Black;
    int movesRemaining_ = 1;
    std::optional<Stone> winner_;
    std::vector<MoveRecord> moveHistory_;
};

}  // namespace gomoku
