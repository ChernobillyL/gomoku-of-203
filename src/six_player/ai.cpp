#include "gomoku/six_player/ai.hpp"

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
// 节点预算用于兜底，保证复杂中局下界面不会被同步搜索长时间卡住。
constexpr long long kNodeBudget = 12'000;

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

// 参数：board 为落子前局面，move 为候选点，stone 为假设落子方。
// 返回：只经过该落点的六格窗口总分；非法点返回负无穷。
// 实现：复制棋盘模拟落子后，沿四个方向枚举所有包含 move 的六格窗口，避免为
// 每个候选点重新扫描整个 19 路棋盘。该函数只用于候选排序，不用于终局判定。
long long localScore(const Board& board, Move move, Stone stone) {
    Board simulated = board;
    if (!simulated.place(move, stone)) return -kInfinity;
    long long score = 0;
    for (const Move direction : kDirections) {
        for (int offset = -(kSixPlayerWinLength - 1); offset <= 0; ++offset) {
            const Move start{move.row + offset * direction.row, move.col + offset * direction.col};
            const Move end{start.row + (kSixPlayerWinLength - 1) * direction.row,
                           start.col + (kSixPlayerWinLength - 1) * direction.col};
            if (simulated.inBounds(start) && simulated.inBounds(end)) {
                score += scoreWindow(simulated, start, direction, stone);
            }
        }
    }
    return score;
}

long long movePriority(const Board& board, Move move, Stone side) {
    const long long attack = localScore(board, move, side);
    if (attack == -kInfinity) return -kInfinity;
    const long long defense = localScore(board, move, opponent(side));
    const int center = board.size() / 2;
    const long long centerBias = std::abs(move.row - center) + std::abs(move.col - center);
    return attack * 2 + defense - centerBias;
}

// 找出所有一步成六的点，并优先选择靠近棋盘中心的点，保证同一局面的
// 选择可复现，也避免在多个胜点之间随机落到边缘。
std::optional<Move> immediateWinner(const Board& board, Stone stone) {
    std::optional<Move> best;
    int bestDistance = std::numeric_limits<int>::max();
    const int center = board.size() / 2;
    for (const Move move : board.emptyMoves()) {
        Board after = board;
        if (!after.place(move, stone) || !formsSixInRow(after, move, stone)) continue;
        const int distance = std::abs(move.row - center) + std::abs(move.col - center);
        if (!best || distance < bestDistance) {
            best = move;
            bestDistance = distance;
        }
    }
    return best;
}

std::vector<Move> orderedCandidates(const Board& board, Stone side) {
    // 任何成六点都必然紧邻已有棋子，因此只需检查局部候选点，无需扫描全盘。
    std::vector<Move> wins;
    std::vector<std::pair<long long, Move>> rest;
    for (const Move move : candidateMoves(board)) {
        Board after = board;
        if (!after.place(move, side)) continue;
        if (formsSixInRow(after, move, side)) {
            wins.push_back(move);
        } else {
            rest.emplace_back(movePriority(board, move, side), move);
        }
    }
    std::stable_sort(rest.begin(), rest.end(),
                     [](const std::pair<long long, Move>& lhs,
                        const std::pair<long long, Move>& rhs) {
                         return lhs.first > rhs.first;
                     });

    // 胜着永远排在前面，避免被候选宽度截断；其余按启发式分数取前若干。
    std::vector<Move> ordered = wins;
    for (const auto& entry : rest) {
        if (ordered.size() >= kMaxBranching) break;
        ordered.push_back(entry.second);
    }
    if (ordered.size() > kMaxBranching) {
        ordered.resize(kMaxBranching);
    }
    return ordered;
}

// 参数：board 为当前局面；side 为轮到的一方；aiStone 为电脑颜色；depth 为剩余
//       单子层数；turnRemaining 为该方本回合还剩几子；alpha/beta 为剪枝窗口；
//       lastMove/lastStone 为上一手；nodes 为跨整棵搜索树共享的节点计数器。
// 返回：从 aiStone 视角的估值；节点数超过 kNodeBudget 时直接返回静态评估。
// 实现：先判上一手是否成六/满盘，再按候选顺序递归；同一方剩余两子时下一层
// 仍由该方行动，第二子落完才换手。这是带节点预算的标准 Alpha-Beta。
long long minimax(const Board& board, Stone side, Stone aiStone, int depth,
                  int turnRemaining, long long alpha, long long beta, Move lastMove,
                  Stone lastStone, long long& nodes) {
    if (++nodes > kNodeBudget) return evaluateBoard(board, aiStone);
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
                                        alpha, beta, move, side, nodes);
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

// 参数：board 为当前局面；aiStone 为电脑颜色；movesRemaining 为当前方本回合
//       剩余落子数（1 或 2，默认 2）。
// 返回：电脑落点；棋盘已满、颜色非法或剩余子数非法时为空。
// 实现：先找自己一步成六点，再堵对手一步成六点；随后对排序后的候选逐个调用
// minimax，根层也维护 alpha 以剪掉不可能更优的分支。
std::optional<Move> SixPlayerAi::chooseMove(const Board& board, Stone aiStone,
                                            int movesRemaining) const {
    if (aiStone == Stone::Empty || board.isFull() || movesRemaining < 1 || movesRemaining > 2)
        return std::nullopt;

    // 自己的直接胜点优先；没有时先堵住对手下一步的成六点。
    if (const auto win = immediateWinner(board, aiStone)) return win;
    if (const auto block = immediateWinner(board, opponent(aiStone))) return block;

    const auto moves = orderedCandidates(board, aiStone);
    if (moves.empty()) return std::nullopt;

    Move bestMove = moves.front();
    long long best = -kInfinity;
    long long alpha = -kInfinity;
    long long nodes = 0;
    for (const Move move : moves) {
        Board child = board;
        if (!child.place(move, aiStone)) continue;
        const int remainingAfter = movesRemaining - 1;
        const Stone nextSide = remainingAfter == 0 ? opponent(aiStone) : aiStone;
        const int nextRemaining = remainingAfter == 0 ? 2 : remainingAfter;
        const long long value = minimax(child, nextSide, aiStone, kSearchDepth - 1,
                                        nextRemaining, alpha, kInfinity, move, aiStone, nodes);
        if (value > best) {
            best = value;
            bestMove = move;
        }
        alpha = std::max(alpha, best);
    }
    return bestMove;
}

}  // namespace gomoku
