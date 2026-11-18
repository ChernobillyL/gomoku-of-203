#pragma once

#include "gomoku/game.hpp"

#include <optional>

namespace gomoku {

constexpr int kSixPlayerBoardSize = 19;
constexpr int kSixPlayerWinLength = 6;

bool formsSixInRow(const Board& board, Move lastMove, Stone stone);

// 六子棋：黑方开局先落一子；此后双方各自每回合连续落两子。
class SixPlayerGame {
public:
    SixPlayerGame() : board_(kSixPlayerBoardSize) {}

    const Board& board() const { return board_; }
    Stone currentPlayer() const { return currentPlayer_; }
    int movesRemaining() const { return movesRemaining_; }
    bool finished() const { return winner_.has_value() || board_.isFull(); }
    std::optional<Stone> winner() const { return winner_; }
    const std::vector<MoveRecord>& moveHistory() const { return moveHistory_; }
    bool play(Move move);

private:
    Board board_;
    Stone currentPlayer_ = Stone::Black;
    int movesRemaining_ = 1;
    std::optional<Stone> winner_;
    std::vector<MoveRecord> moveHistory_;
};

}  // namespace gomoku
