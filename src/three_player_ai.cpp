#include "gomoku/three_player_ai.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

namespace gomoku {
namespace {

constexpr std::array<Move, 4> kDirections{{{0, 1}, {1, 0}, {1, 1}, {1, -1}}};
constexpr std::array<Stone, 3> kStoneOrder{{Stone::Black, Stone::White, Stone::Red}};
constexpr long long kWinScore = 1'000'000'000LL;
constexpr long long kInfinity = std::numeric_limits<long long>::max() / 4;
constexpr int kSearchDepth = 4;
constexpr int kMaxBranching = 5;

using Scores = std::array<long long, 3>;

int stoneIndex(Stone stone) {
    for (int i = 0; i < static_cast<int>(kStoneOrder.size()); ++i) {
        if (kStoneOrder[static_cast<std::size_t>(i)] == stone) return i;
    }
    return -1;
}

// 四格窗口评分。任何一方都是先连成四子获胜，因此窗口内三颗己方棋子
// 已经构成下一手必胜的威胁。
long long patternScore(int stones, int openEnds) {
    if (stones >= 4) return kWinScore;
    if (stones == 3) return openEnds == 2 ? 80'000LL : (openEnds == 1 ? 40'000LL : 20'000LL);
    if (stones == 2) return openEnds == 2 ? 1'500LL : (openEnds == 1 ? 250LL : 25LL);
    if (stones == 1) return openEnds == 2 ? 30LL : (openEnds == 1 ? 5LL : 1LL);
    return 0;
}

long long scoreWindow(const Board& board, Move start, Move direction, Stone stone) {
    int stones = 0;
    for (int i = 0; i < 4; ++i) {
        const Move cell{start.row + i * direction.row, start.col + i * direction.col};
        const Stone occupant = board.at(cell);
        if (occupant != Stone::Empty && occupant != stone) return 0;
        if (occupant == stone) ++stones;
    }
    if (stones == 0) return 0;

    int open = 0;
    const Move before{start.row - direction.row, start.col - direction.col};
    const Move after{start.row + 4 * direction.row, start.col + 4 * direction.col};
    if (board.inBounds(before) && board.at(before) == Stone::Empty) ++open;
    if (board.inBounds(after) && board.at(after) == Stone::Empty) ++open;
    return patternScore(stones, open);
}

long long evaluateStone(const Board& board, Stone stone) {
    long long score = 0;
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            for (const Move direction : kDirections) {
                const Move end{row + 3 * direction.row, col + 3 * direction.col};
                if (!board.inBounds(end)) continue;
                score += scoreWindow(board, {row, col}, direction, stone);
            }
        }
    }
    return score;
}

// 每名玩家只看自己的收益：进攻价值越高越好，另外两名对手的棋型会削弱收益。
Scores evaluateBoard(const Board& board) {
    std::array<long long, 3> own{};
    for (int i = 0; i < static_cast<int>(kStoneOrder.size()); ++i) {
        own[static_cast<std::size_t>(i)] = evaluateStone(board, kStoneOrder[static_cast<std::size_t>(i)]);
    }
    Scores value{};
    for (int i = 0; i < static_cast<int>(kStoneOrder.size()); ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        const std::size_t other1 = static_cast<std::size_t>((i + 1) % 3);
        const std::size_t other2 = static_cast<std::size_t>((i + 2) % 3);
        value[index] = own[index] * 2 - own[other1] - own[other2];
    }
    return value;
}

long long evaluateMoveForStone(const Board& board, Move move, Stone stone) {
    if (!board.isEmpty(move)) return -kInfinity;
    Board simulated = board;
    if (!simulated.place(move, stone)) return -kInfinity;

    long long score = 0;
    for (const Move direction : kDirections) {
        for (int offset = -3; offset <= 0; ++offset) {
            const Move start{move.row + offset * direction.row,
                             move.col + offset * direction.col};
            const Move end{start.row + 3 * direction.row, start.col + 3 * direction.col};
            if (simulated.inBounds(start) && simulated.inBounds(end)) {
                score += scoreWindow(simulated, start, direction, stone);
            }
        }
    }
    const int center = kBoardSize / 2;
    score -= static_cast<long long>(std::abs(move.row - center) + std::abs(move.col - center));
    return score;
}

long long movePriority(const Board& board, Move move, Stone side) {
    long long priority = evaluateMoveForStone(board, move, side) * 2;
    long long strongestThreat = 0;
    for (const Stone stone : kStoneOrder) {
        if (stone != side) {
            strongestThreat = std::max(strongestThreat, evaluateMoveForStone(board, move, stone));
        }
    }
    return priority + strongestThreat;
}

std::vector<Move> orderedCandidates(const Board& board, Stone side) {
    std::vector<std::pair<long long, Move>> scored;
    for (const Move move : AiPlayer::candidates(board)) {
        scored.emplace_back(movePriority(board, move, side), move);
    }
    std::stable_sort(scored.begin(), scored.end(),
                     [](const std::pair<long long, Move>& lhs,
                        const std::pair<long long, Move>& rhs) {
                         return lhs.first > rhs.first;
                     });
    const std::size_t count = std::min(scored.size(), static_cast<std::size_t>(kMaxBranching));
    std::vector<Move> moves;
    moves.reserve(count);
    for (std::size_t i = 0; i < count; ++i) moves.push_back(scored[i].second);
    return moves;
}

std::vector<Move> winningMoves(const Board& board, Stone stone) {
    std::vector<Move> wins;
    for (const Move move : board.emptyMoves()) {
        Board simulated = board;
        if (simulated.place(move, stone) && formsFourInRow(simulated, move, stone)) {
            wins.push_back(move);
        }
    }
    return wins;
}

Move bestBlockingMove(const Board& board, const std::vector<Move>& blocks, Stone aiStone) {
    Move best = blocks.front();
    long long bestScore = -kInfinity;
    for (const Move move : blocks) {
        const long long score = movePriority(board, move, aiStone);
        if (score > bestScore) {
            bestScore = score;
            best = move;
        }
    }
    return best;
}

// 自利式多玩家搜索（max-n）：轮到哪一方，就选择让该方收益最大的分支，
// 因此两台电脑不会把彼此的胜负当成共同目标。
Scores search(const Board& board, Stone sideToMove, int depth, Move lastMove) {
    if (lastMove.row >= 0) {
        const Stone lastStone = board.at(lastMove);
        const int winnerIndex = stoneIndex(lastStone);
        if (winnerIndex >= 0 && formsFourInRow(board, lastMove, lastStone)) {
            Scores value;
            value.fill(-kWinScore - depth);
            value[static_cast<std::size_t>(winnerIndex)] = kWinScore + depth;
            return value;
        }
    }
    if (board.isFull()) return Scores{};
    if (depth <= 0) return evaluateBoard(board);

    const int sideIndex = stoneIndex(sideToMove);
    if (sideIndex < 0) return evaluateBoard(board);
    const std::size_t side = static_cast<std::size_t>(sideIndex);

    const std::vector<Move> moves = orderedCandidates(board, sideToMove);
    if (moves.empty()) return evaluateBoard(board);

    Scores best;
    best.fill(-kInfinity);
    for (const Move move : moves) {
        Board child = board;
        if (!child.place(move, sideToMove)) continue;
        const Scores value = search(child, nextStone(sideToMove), depth - 1, move);
        if (value[side] > best[side]) best = value;
    }
    if (best[side] == -kInfinity) return evaluateBoard(board);
    return best;
}

}  // namespace

std::optional<Move> ThreePlayerAi::chooseMove(const Board& board, Stone aiStone) const {
    if (aiStone == Stone::Empty || board.isFull()) return std::nullopt;

    const std::vector<Move> ownWins = winningMoves(board, aiStone);
    if (!ownWins.empty()) return ownWins.front();

    // 下一手就是对手。若不堵住它，对手会立刻获胜；另一名对手的胜点
    // 则在其后处理，因为下一手对手也会为自己的胜负考虑。
    const Stone nextPlayer = nextStone(aiStone);
    const Stone followingPlayer = nextStone(nextPlayer);

    const std::vector<Move> nextWins = winningMoves(board, nextPlayer);
    if (!nextWins.empty()) return bestBlockingMove(board, nextWins, aiStone);

    const std::vector<Move> followingWins = winningMoves(board, followingPlayer);
    if (!followingWins.empty()) return bestBlockingMove(board, followingWins, aiStone);

    std::vector<Move> moves = orderedCandidates(board, aiStone);
    if (moves.empty()) {
        const std::vector<Move> empty = board.emptyMoves();
        if (empty.empty()) return std::nullopt;
        return empty.front();
    }

    const std::size_t aiIndex = static_cast<std::size_t>(stoneIndex(aiStone));
    Move bestMove = moves.front();
    long long bestValue = -kInfinity;
    for (const Move move : moves) {
        Board child = board;
        if (!child.place(move, aiStone)) continue;
        const Scores value = search(child, nextPlayer, kSearchDepth - 1, move);
        if (value[aiIndex] > bestValue) {
            bestValue = value[aiIndex];
            bestMove = move;
        }
    }
    return bestMove;
}

}  // namespace gomoku
