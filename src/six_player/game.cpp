#include "gomoku/six_player/game.hpp"

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

// 参数：board 为落子后的棋盘，lastMove 为最后落点，stone 为该手颜色。
// 返回：四个方向中任一方向连续同色棋子达到 6（或更多）时为 true。
// 实现：从 lastMove 向正反方向累计同色棋子，总长度即为该线连子数。
bool formsSixInRow(const Board& board, Move lastMove, Stone stone) {
    if (stone == Stone::Empty || !board.inBounds(lastMove) || board.at(lastMove) != stone) return false;
    for (const Move direction : kDirections) {
        const int length = 1 + countDirection(board, lastMove, stone, direction) +
                           countDirection(board, lastMove, stone, {-direction.row, -direction.col});
        if (length >= kSixPlayerWinLength) return true;
    }
    return false;
}

// 参数：move 为当前方落子坐标。
// 返回：成功为 true；终局或落点非法为 false。
// 实现：每次落子后立即判六连，第一子成六也会结束，不必等第二子；未结束时
// 递减 movesRemaining_，减到 0 才换手并重置为 2（黑方首回合初始为 1）。
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
