#include "gomoku/game.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>

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

int openEnds(const Board& board, Move move, Stone stone, Move direction) {
    int open = 0;
    for (const int sign : {-1, 1}) {
        Move cursor{move.row + sign * direction.row, move.col + sign * direction.col};
        while (board.inBounds(cursor) && board.at(cursor) == stone) {
            cursor.row += sign * direction.row;
            cursor.col += sign * direction.col;
        }
        if (board.inBounds(cursor) && board.at(cursor) == Stone::Empty) {
            ++open;
        }
    }
    return open;
}

long long shapeScore(int length, int open) {
    if (length >= 5) return 1'000'000'000LL;
    if (open == 0) return 0;
    if (length == 4) return open == 2 ? 100'000'000LL : 10'000'000LL;
    if (length == 3) return open == 2 ? 1'000'000LL : 100'000LL;
    if (length == 2) return open == 2 ? 10'000LL : 1'000LL;
    return open == 2 ? 100LL : 10LL;
}

}  // namespace

Stone opponent(Stone stone) {
    return stone == Stone::Black ? Stone::White : Stone::Black;
}

Board::Board() {
    cells_.fill(Stone::Empty);
}

bool Board::inBounds(Move move) const {
    return move.row >= 0 && move.row < kBoardSize && move.col >= 0 && move.col < kBoardSize;
}

std::size_t Board::index(Move move) const {
    return static_cast<std::size_t>(move.row * kBoardSize + move.col);
}

bool Board::isEmpty(Move move) const {
    return inBounds(move) && cells_[index(move)] == Stone::Empty;
}

Stone Board::at(Move move) const {
    if (!inBounds(move)) return Stone::Empty;
    return cells_[index(move)];
}

bool Board::place(Move move, Stone stone) {
    if (!inBounds(move) || stone == Stone::Empty || !isEmpty(move)) return false;
    cells_[index(move)] = stone;
    ++moveCount_;
    return true;
}

bool Board::isFull() const {
    return moveCount_ == kBoardSize * kBoardSize;
}

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
    moves.reserve(static_cast<std::size_t>(kBoardSize * kBoardSize - moveCount_));
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            Move move{row, col};
            if (isEmpty(move)) moves.push_back(move);
        }
    }
    return moves;
}

bool Game::play(Move move) {
    if (result_ != Result::Ongoing || !board_.place(move, currentPlayer_)) return false;
    moveHistory_.push_back({move, currentPlayer_});
    result_ = board_.resultAfter(move);
    if (result_ == Result::Ongoing) currentPlayer_ = opponent(currentPlayer_);
    return true;
}

std::vector<Move> AiPlayer::candidates(const Board& board) {
    std::vector<Move> moves;
    if (board.moveCount() == 0) {
        return {{kBoardSize / 2, kBoardSize / 2}};
    }

    std::array<bool, kBoardSize * kBoardSize> seen{};
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            const Move occupied{row, col};
            if (board.at(occupied) == Stone::Empty) continue;
            for (int dr = -2; dr <= 2; ++dr) {
                for (int dc = -2; dc <= 2; ++dc) {
                    const Move candidate{row + dr, col + dc};
                    if (board.isEmpty(candidate)) {
                        const std::size_t idx = static_cast<std::size_t>(candidate.row * kBoardSize + candidate.col);
                        if (!seen[idx]) {
                            seen[idx] = true;
                            moves.push_back(candidate);
                        }
                    }
                }
            }
        }
    }
    return moves;
}

long long AiPlayer::scoreMove(const Board& board, Move move, Stone stone) {
    Board simulated = board;
    if (!simulated.place(move, stone)) return std::numeric_limits<long long>::min();

    long long score = 0;
    for (const Move direction : kDirections) {
        const int length = lineLength(simulated, move, stone, direction);
        const int open = openEnds(simulated, move, stone, direction);
        score += shapeScore(length, open);
    }
    // Resolve equal tactical scores predictably toward the center.
    const int center = kBoardSize / 2;
    score -= static_cast<long long>(std::abs(move.row - center) + std::abs(move.col - center));
    return score;
}

std::optional<Move> AiPlayer::chooseMove(const Board& board, Stone aiStone) const {
    if (aiStone == Stone::Empty || board.isFull()) return std::nullopt;
    const std::vector<Move> moves = candidates(board);
    if (moves.empty()) return std::nullopt;

    // Take a winning move before considering defense.
    for (const Move move : moves) {
        Board simulated = board;
        if (simulated.place(move, aiStone) &&
            simulated.resultAfter(move) == (aiStone == Stone::Black ? Result::BlackWins : Result::WhiteWins)) {
            return move;
        }
    }

    const Stone enemy = opponent(aiStone);
    std::vector<Move> opponentWins;
    for (const Move move : board.emptyMoves()) {
        Board simulated = board;
        if (simulated.place(move, enemy) &&
            simulated.resultAfter(move) == (enemy == Stone::Black ? Result::BlackWins : Result::WhiteWins)) {
            opponentWins.push_back(move);
        }
    }
    if (!opponentWins.empty()) {
        // A single immediate threat can be blocked at its winning intersection.
        if (opponentWins.size() == 1) return opponentWins.front();
        // If the opponent has multiple independent winning moves, block the most valuable one.
        return *std::max_element(opponentWins.begin(), opponentWins.end(), [&](Move lhs, Move rhs) {
            return scoreMove(board, lhs, enemy) < scoreMove(board, rhs, enemy);
        });
    }

    Move bestMove = moves.front();
    long long bestScore = std::numeric_limits<long long>::min();
    for (const Move move : moves) {
        const long long attack = scoreMove(board, move, aiStone);
        const long long defense = scoreMove(board, move, enemy);
        const long long score = attack + defense * 9 / 10;
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }
    return bestMove;
}

}  // namespace gomoku
