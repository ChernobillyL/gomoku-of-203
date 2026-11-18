#include "gomoku/six_player.hpp"

#include <array>

namespace gomoku {
namespace {

constexpr std::array<Move, 4> kDirections{{{0, 1}, {1, 0}, {1, 1}, {1, -1}}};

int countDirection(const Board& board, Move origin, Stone stone, Move direction) {
    int count = 0;
    Move cursor{origin.row + direction.row, origin.col + direction.col};
    while (board.inBounds(cursor) && board.at(cursor) == stone) {
        ++count;
        cursor.row += direction.row;
        cursor.col += direction.col;
    }
    return count;
}

}  // namespace

bool formsSixInRow(const Board& board, Move lastMove, Stone stone) {
    if (stone == Stone::Empty || !board.inBounds(lastMove) || board.at(lastMove) != stone) return false;
    for (const Move direction : kDirections) {
        const int length = 1 + countDirection(board, lastMove, stone, direction) +
                           countDirection(board, lastMove, stone, {-direction.row, -direction.col});
        if (length >= kSixPlayerWinLength) return true;
    }
    return false;
}

bool SixPlayerGame::play(Move move) {
    if (finished() || !board_.place(move, currentPlayer_)) return false;
    const Stone placed = currentPlayer_;
    moveHistory_.push_back({move, placed});
    if (formsSixInRow(board_, move, placed)) {
        winner_ = placed;
        return true;
    }
    if (board_.isFull()) return true;

    --movesRemaining_;
    if (movesRemaining_ == 0) {
        currentPlayer_ = opponent(currentPlayer_);
        // 黑棋开局的第一手结束后，白棋及后续双方每回合各下两子。
        movesRemaining_ = 2;
    }
    return true;
}

}  // namespace gomoku
