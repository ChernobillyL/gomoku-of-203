#include "gomoku/classic/game.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace gomoku {
namespace {

constexpr std::array<Move, 4> kDirections{{{0, 1}, {1, 0}, {1, 1}, {1, -1}}};

constexpr long long kWinScore = 1'000'000'000LL;
constexpr long long kInfinity = std::numeric_limits<long long>::max() / 4;
constexpr int kSearchDepth = 6;
constexpr std::size_t kMaxBranching = 8;

// 评分表按“连续五格窗口”统计，允许窗口内有空位，因此活三、冲四等
// 棋型不再因中间有空位而被漏掉。窗口两端的空位用于区分活棋与眠棋。
long long patternScore(int stones, int openEnds) {
    if (stones >= 5) return kWinScore;
    if (stones == 4) return openEnds == 2 ? 1'000'000LL : 100'000LL;
    if (stones == 3) return openEnds == 2 ? 20'000LL : 2'000LL;
    if (stones == 2) return openEnds == 2 ? 500LL : 50LL;
    if (stones == 1) return openEnds == 2 ? 10LL : 1LL;
    return 0;
}

long long scoreWindow(const Board& board, Move start, Move direction, Stone stone) {
    int stones = 0;
    for (int i = 0; i < 5; ++i) {
        const Move cell{start.row + i * direction.row, start.col + i * direction.col};
        const Stone occupant = board.at(cell);
        if (occupant != Stone::Empty && occupant != stone) return 0;
        if (occupant == stone) ++stones;
    }
    if (stones == 0) return 0;

    int open = 0;
    const Move before{start.row - direction.row, start.col - direction.col};
    const Move after{start.row + 5 * direction.row, start.col + 5 * direction.col};
    if (board.inBounds(before) && board.at(before) == Stone::Empty) ++open;
    if (board.inBounds(after) && board.at(after) == Stone::Empty) ++open;
    return patternScore(stones, open);
}

// 参数：board 为待评估局面，perspective 为评估视角（己方颜色）。
// 返回：己方窗口总分减去对方窗口总分，分数越高对 perspective 越有利。
// 实现：枚举四个方向上所有连续五格窗口，对每个窗口分别累计双方棋型，
// 因此同一局面只需扫描一次即可得到相对分。
long long evaluateBoard(const Board& board, Stone perspective) {
    long long score = 0;
    for (int row = 0; row < board.size(); ++row) {
        for (int col = 0; col < board.size(); ++col) {
            for (const Move direction : kDirections) {
                const Move end{row + 4 * direction.row, col + 4 * direction.col};
                if (!board.inBounds(end)) continue;
                const Move start{row, col};
                score += scoreWindow(board, start, direction, perspective);
                score -= scoreWindow(board, start, direction, opponent(perspective));
            }
        }
    }
    return score;
}

// 参数：board 为落子前局面，move 为候选点，stone 为假设落子的颜色。
// 返回：该点的局部棋型收益（含中心偏好修正）；非法点返回 long long 最小值。
// 实现：先复制棋盘模拟落子，再只检查经过 move 的五格窗口，避免全盘重算。
long long evaluateMove(const Board& board, Move move, Stone stone) {
    Board simulated = board;
    if (!simulated.place(move, stone)) return std::numeric_limits<long long>::min();
    long long score = 0;
    for (const Move direction : kDirections) {
        // 检查所有经过落点的五格窗口，识别跳棋型和不同方向的组合。
        for (int offset = -4; offset <= 0; ++offset) {
            const Move start{move.row + offset * direction.row,
                             move.col + offset * direction.col};
            const Move end{start.row + 4 * direction.row, start.col + 4 * direction.col};
            if (simulated.inBounds(start) && simulated.inBounds(end)) {
                score += scoreWindow(simulated, start, direction, stone);
            }
        }
    }
    const int center = board.size() / 2;
    score -= static_cast<long long>(std::abs(move.row - center) + std::abs(move.col - center));
    return score;
}

long long movePriority(const Board& board, Move move, Stone side) {
    return evaluateMove(board, move, side) * 2 + evaluateMove(board, move, opponent(side));
}

std::vector<Move> orderedCandidates(const Board& board, Stone side) {
    std::vector<Move> moves = candidateMoves(board);
    std::stable_sort(moves.begin(), moves.end(), [&](Move lhs, Move rhs) {
        return movePriority(board, lhs, side) > movePriority(board, rhs, side);
    });
    if (moves.size() > kMaxBranching) moves.resize(kMaxBranching);
    return moves;
}

// 参数：board 为当前局面；sideToMove 为轮到的一方；aiStone 为电脑颜色；
//       depth 为剩余搜索层数；alpha/beta 为剪枝窗口；lastMove 为进入本层前的一手。
// 返回：从 aiStone 视角看该局面的估值；己方获胜返回正大值，对方获胜返回负大值。
// 实现：先检查上一手是否终局，再在 depth==0 时调用 evaluateBoard；否则按启发式
// 顺序展开候选点，递归搜索并在 beta<=alpha 时剪枝。
long long minimax(Board board, Stone sideToMove, Stone aiStone, int depth,
                  long long alpha, long long beta, Move lastMove) {
    if (lastMove.row >= 0) {
        const Result result = board.resultAfter(lastMove);
        if (result == (aiStone == Stone::Black ? Result::BlackWins : Result::WhiteWins))
            return kWinScore + depth;
        if (result == (aiStone == Stone::Black ? Result::WhiteWins : Result::BlackWins))
            return -kWinScore - depth;
        if (result == Result::Draw) return 0;
    }
    if (depth == 0) return evaluateBoard(board, aiStone);

    const std::vector<Move> moves = orderedCandidates(board, sideToMove);
    if (moves.empty()) return evaluateBoard(board, aiStone);
    const bool maximizing = sideToMove == aiStone;
    long long best = maximizing ? -kInfinity : kInfinity;
    for (const Move move : moves) {
        Board child = board;
        if (!child.place(move, sideToMove)) continue;
        const long long value = minimax(std::move(child), opponent(sideToMove), aiStone,
                                        depth - 1, alpha, beta, move);
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

// 将内部的 Result 枚举转换为统一的“胜者”表示；平局或未结束返回空。
std::optional<Stone> Game::winner() const {
    if (result_ == Result::BlackWins) return Stone::Black;
    if (result_ == Result::WhiteWins) return Stone::White;
    return std::nullopt;
}

// 参数：move 为当前方要落子的坐标。
// 返回：成功为 true；对局已结束或落点非法为 false。
// 实现：Board::place 负责棋盘校验；成功后立即写入棋谱并判胜，只有对局仍在
// 进行时才切换行棋方，保证终局后 currentPlayer 不再变化。
bool Game::play(Move move) {
    if (result_ != Result::Ongoing || !board_.place(move, currentPlayer_)) return false;
    moveHistory_.push_back({move, currentPlayer_});
    result_ = board_.resultAfter(move);
    if (result_ == Result::Ongoing) currentPlayer_ = opponent(currentPlayer_);
    return true;
}

long long AiPlayer::scoreMove(const Board& board, Move move, Stone stone) {
    return evaluateMove(board, move, stone);
}

// 参数：board 为当前局面，aiStone 为电脑执棋颜色。
// 返回：电脑选定的落点；棋盘已满或无合法候选时返回空。
// 实现顺序：全盘检查己方一步必胜点 -> 检查并阻挡对方一步胜点 -> 按启发式
// 排序候选 -> 对每个根节点走法调用 minimax（含 Alpha-Beta 剪枝）取最优。
std::optional<Move> AiPlayer::chooseMove(const Board& board, Stone aiStone) const {
    if (aiStone == Stone::Empty || board.isFull()) return std::nullopt;
    const std::vector<Move> moves = candidateMoves(board);
    if (moves.empty()) return std::nullopt;

    const Stone enemy = opponent(aiStone);
    // 先检查必胜点，避免候选广度限制漏掉边缘上的直接胜着。
    for (const Move move : board.emptyMoves()) {
        Board simulated = board;
        if (simulated.place(move, aiStone) &&
            simulated.resultAfter(move) == (aiStone == Stone::Black ? Result::BlackWins : Result::WhiteWins))
            return move;
    }

    std::vector<Move> opponentWins;
    for (const Move move : board.emptyMoves()) {
        Board simulated = board;
        if (simulated.place(move, enemy) &&
            simulated.resultAfter(move) == (enemy == Stone::Black ? Result::BlackWins : Result::WhiteWins))
            opponentWins.push_back(move);
    }
    if (!opponentWins.empty()) {
        if (opponentWins.size() == 1) return opponentWins.front();
        return *std::max_element(opponentWins.begin(), opponentWins.end(), [&](Move lhs, Move rhs) {
            return scoreMove(board, lhs, enemy) < scoreMove(board, rhs, enemy);
        });
    }

    std::vector<Move> ordered = orderedCandidates(board, aiStone);
    if (ordered.empty()) return std::nullopt;
    Move bestMove = ordered.front();
    long long bestScore = -kInfinity;
    long long alpha = -kInfinity;
    for (const Move move : ordered) {
        Board child = board;
        if (!child.place(move, aiStone)) continue;
        const long long score = minimax(std::move(child), enemy, aiStone, kSearchDepth - 1,
                                        alpha, kInfinity, move);
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        alpha = std::max(alpha, bestScore);
    }
    return bestMove;
}

}  // namespace gomoku
