#include "gomoku/core/board.hpp"

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

int lineLength(const Board& board, Move move, Stone stone, Move direction) {
    return 1 + countDirection(board, move, stone, direction) +
           countDirection(board, move, stone, {-direction.row, -direction.col});
}

}  // namespace

Stone opponent(Stone stone) {
    return stone == Stone::Black ? Stone::White : Stone::Black;
}

Board::Board(int size) : size_(size >= 1 && size <= kMaxBoardSize ? size : kBoardSize) {
    cells_.fill(Stone::Empty);
}

bool Board::inBounds(Move move) const {
    return move.row >= 0 && move.row < size_ && move.col >= 0 && move.col < size_;
}

std::size_t Board::index(Move move) const {
    return static_cast<std::size_t>(move.row * size_ + move.col);
}

bool Board::isEmpty(Move move) const {
    return inBounds(move) && cells_[index(move)] == Stone::Empty;
}

Stone Board::at(Move move) const {
    if (!inBounds(move)) return Stone::Empty;
    return cells_[index(move)];
}

// 参数：move 为行列坐标，stone 为待落棋子。
// 返回：落子成功为 true；越界、重复落子或 stone 为 Empty 时返回 false。
// 实现：先用 inBounds/isEmpty 做完整校验，只有校验通过才写入并递增计数，
// 因此失败时棋盘状态不会发生部分修改。
bool Board::place(Move move, Stone stone) {
    if (!inBounds(move) || stone == Stone::Empty || !isEmpty(move)) return false;
    cells_[index(move)] = stone;
    ++moveCount_;
    return true;
}

bool Board::isFull() const {
    return moveCount_ == size_ * size_;
}

// 参数：lastMove 为刚落下的一手。
// 返回：经典五子棋的胜负/平局状态；坐标非法或该点为空时返回 Ongoing。
// 实现：只需检查经过最后落点的四条线，无需扫描整盘。每条线分别向正、反
// 方向累计同色棋子，总长度达到 5 即获胜；若无人五连且棋盘已满则为平局。
Result Board::resultAfter(Move lastMove) const {
    if (!inBounds(lastMove)) return Result::Ongoing;
    const Stone stone = at(lastMove);
    if (stone == Stone::Empty) return Result::Ongoing;
    for (const Move direction : kDirections) {
        if (lineLength(*this, lastMove, stone, direction) >= 5) {
            return stone == Stone::Black ? Result::BlackWins : Result::WhiteWins;
        }
    }
    return isFull() ? Result::Draw : Result::Ongoing;
}

std::vector<Move> Board::emptyMoves() const {
    std::vector<Move> moves;
    moves.reserve(static_cast<std::size_t>(size_ * size_ - moveCount_));
    for (int row = 0; row < size_; ++row) {
        for (int col = 0; col < size_; ++col) {
            Move move{row, col};
            if (isEmpty(move)) moves.push_back(move);
        }
    }
    return moves;
}

// 参数：board 为当前棋盘。
// 返回：所有已有棋子周围 2 格内的空点；空盘时只返回天元。
// 实现：用 seen 数组去重，避免同一空点被多个相邻棋子重复加入。该函数供三种
// 棋的 AI 共用，属于公共搜索工具，不包含任何棋种规则。
std::vector<Move> candidateMoves(const Board& board) {
    std::vector<Move> moves;
    if (board.moveCount() == 0) {
        return {{board.size() / 2, board.size() / 2}};
    }

    std::array<bool, kMaxBoardSize * kMaxBoardSize> seen{};
    for (int row = 0; row < board.size(); ++row) {
        for (int col = 0; col < board.size(); ++col) {
            const Move occupied{row, col};
            if (board.at(occupied) == Stone::Empty) continue;
            for (int dr = -2; dr <= 2; ++dr) {
                for (int dc = -2; dc <= 2; ++dc) {
                    const Move candidate{row + dr, col + dc};
                    if (!board.isEmpty(candidate)) continue;
                    const std::size_t idx =
                        static_cast<std::size_t>(candidate.row * board.size() + candidate.col);
                    if (!seen[idx]) {
                        seen[idx] = true;
                        moves.push_back(candidate);
                    }
                }
            }
        }
    }
    return moves;
}

}  // namespace gomoku
