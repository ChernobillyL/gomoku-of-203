#include "gomoku/six_player_ai.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

namespace gomoku {
namespace {

constexpr std::array<Move, 4> kDirections{{{0, 1}, {1, 0}, {1, 1}, {1, -1}}};
constexpr long long kWinScore = 1'000'000'000LL;
constexpr long long kInfinity = std::numeric_limits<long long>::max() / 4;
constexpr int kSearchDepth = 5;  // 每层表示一颗棋；双子回合由 turnRemaining 保持在同一方。
constexpr std::size_t kMaxBranching = 7;

long long scoreWindow(const Board& board, Move start, Move direction, Stone stone) {
    int count = 0;
    for (int i = 0; i < kSixPlayerWinLength; ++i) {
        const Stone occupant = board.at({start.row + i * direction.row, start.col + i * direction.col});
        if (occupant != Stone::Empty && occupant != stone) return 0;
        if (occupant == stone) ++count;
    }
    if (count == 0) return 0;

    int openEnds = 0;
    const Move before{start.row - direction.row, start.col - direction.col};
    const Move after{start.row + kSixPlayerWinLength * direction.row,
                     start.col + kSixPlayerWinLength * direction.col};
    if (board.inBounds(before) && board.at(before) == Stone::Empty) ++openEnds;
    if (board.inBounds(after) && board.at(after) == Stone::Empty) ++openEnds;

    if (count >= 6) return kWinScore;
    constexpr std::array<long long, 6> kOpen{{0, 8, 100, 2'000, 30'000, 500'000}};
    constexpr std::array<long long, 6> kClosed{{0, 2, 25, 400, 6'000, 100'000}};
    return openEnds == 2 ? kOpen[static_cast<std::size_t>(count)]
                         : kClosed[static_cast<std::size_t>(count)];
}

long long evaluateFor(const Board& board, Stone stone) {
    long long score = 0;
    for (int row = 0; row < board.size(); ++row) {
        for (int col = 0; col < board.size(); ++col) {
            for (const Move direction : kDirections) {
                const Move end{row + (kSixPlayerWinLength - 1) * direction.row,
                               col + (kSixPlayerWinLength - 1) * direction.col};
                if (board.inBounds(end)) score += scoreWindow(board, {row, col}, direction, stone);
            }
        }
    }
    return score;
}

long long evaluateBoard(const Board& board, Stone aiStone) {
    const Stone enemy = opponent(aiStone);
    return evaluateFor(board, aiStone) * 2 - evaluateFor(board, enemy);
}

long long movePriority(const Board& board, Move move, Stone side) {
    Board own = board;
    if (!own.place(move, side)) return -kInfinity;
    const Stone enemy = opponent(side);
    const long long attackGain = evaluateFor(own, side) - evaluateFor(board, side);
    const long long defenseGain = evaluateFor(board, enemy) - evaluateFor(own, enemy);
    const int center = board.size() / 2;
    const long long centerBias = std::abs(move.row - center) + std::abs(move.col - center);
    return attackGain * 2 + defenseGain - centerBias;
}

std::vector<Move> orderedCandidates(const Board& board, Stone side) {
    auto moves = AiPlayer::candidates(board);
    // 立即胜着不受候选宽度限制，避免战术胜点被静态排序裁掉。
    for (const Move move : board.emptyMoves()) {
        Board after = board;
        if (after.place(move, side) && formsSixInRow(after, move, side) &&
            std::find(moves.begin(), moves.end(), move) == moves.end()) {
            moves.push_back(move);
        }
    }
    std::stable_sort(moves.begin(), moves.end(), [&](Move lhs, Move rhs) {
        return movePriority(board, lhs, side) > movePriority(board, rhs, side);
    });
    if (moves.size() > kMaxBranching) {
        std::vector<Move> selected(moves.begin(), moves.begin() + static_cast<std::ptrdiff_t>(kMaxBranching));
        for (const Move move : board.emptyMoves()) {
            Board after = board;
            if (after.place(move, side) && formsSixInRow(after, move, side) &&
                std::find(selected.begin(), selected.end(), move) == selected.end()) {
                selected.back() = move;
            }
        }
        return selected;
    }
    return moves;
}

long long minimax(const Board& board, Stone side, Stone aiStone, int depth,
                  int turnRemaining, long long alpha, long long beta, Move lastMove,
                  Stone lastStone) {
    if (lastMove.row >= 0 && formsSixInRow(board, lastMove, lastStone)) {
        const Stone winner = lastStone;
        return winner == aiStone ? kWinScore + depth : -kWinScore - depth;
    }
    if (board.isFull()) return 0;
    if (depth == 0) return evaluateBoard(board, aiStone);

    const auto moves = orderedCandidates(board, side);
    if (moves.empty()) return evaluateBoard(board, aiStone);
    const bool maximizing = side == aiStone;
    long long best = maximizing ? -kInfinity : kInfinity;
    for (const Move move : moves) {
        Board child = board;
        if (!child.place(move, side)) continue;
        const int remainingAfter = turnRemaining - 1;
        const Stone nextSide = remainingAfter == 0 ? opponent(side) : side;
        const int nextRemaining = remainingAfter == 0 ? 2 : remainingAfter;
        const long long value = minimax(child, nextSide, aiStone, depth - 1, nextRemaining,
                                        alpha, beta, move, side);
        if (maximizing) {
            best = std::max(best, value);
            alpha = std::max(alpha, best);
        } else {
            best = std::min(best, value);
            beta = std::min(beta, best);
        }
        if (beta <= alpha) break;
    }
    return best;
}

}  // namespace

std::optional<Move> SixPlayerAi::chooseMove(const Board& board, Stone aiStone,
                                            int movesRemaining) const {
    if (aiStone == Stone::Empty || board.isFull() || movesRemaining < 1 || movesRemaining > 2)
        return std::nullopt;

    for (const Move move : board.emptyMoves()) {
        Board after = board;
        if (after.place(move, aiStone) && formsSixInRow(after, move, aiStone)) return move;
    }
    const auto moves = orderedCandidates(board, aiStone);
    if (moves.empty()) return std::nullopt;

    Move bestMove = moves.front();
    long long best = -kInfinity;
    long long alpha = -kInfinity;
    for (const Move move : moves) {
        Board child = board;
        if (!child.place(move, aiStone)) continue;
        const int remainingAfter = movesRemaining - 1;
        const Stone nextSide = remainingAfter == 0 ? opponent(aiStone) : aiStone;
        const int nextRemaining = remainingAfter == 0 ? 2 : remainingAfter;
        const long long value = minimax(child, nextSide, aiStone, kSearchDepth - 1,
                                        nextRemaining, alpha, kInfinity, move, aiStone);
        if (value > best) {
            best = value;
            bestMove = move;
        }
        alpha = std::max(alpha, best);
    }
    return bestMove;
}

}  // namespace gomoku
