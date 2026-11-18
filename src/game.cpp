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

constexpr long long kWinScore = 1'000'000'000LL;
constexpr long long kInfinity = std::numeric_limits<long long>::max() / 4;
constexpr int kSearchDepth = 5;
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

long long evaluateBoard(const Board& board, Stone perspective) {
    long long score = 0;
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
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
    const int center = kBoardSize / 2;
    score -= static_cast<long long>(std::abs(move.row - center) + std::abs(move.col - center));
    return score;
}

long long movePriority(const Board& board, Move move, Stone side) {
    return evaluateMove(board, move, side) * 2 + evaluateMove(board, move, opponent(side));
}

std::vector<Move> orderedCandidates(const Board& board, Stone side) {
    std::vector<Move> moves = AiPlayer::candidates(board);
    std::stable_sort(moves.begin(), moves.end(), [&](Move lhs, Move rhs) {
        return movePriority(board, lhs, side) > movePriority(board, rhs, side);
    });
    if (moves.size() > kMaxBranching) moves.resize(kMaxBranching);
    return moves;
}

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
    return evaluateMove(board, move, stone);
}

std::optional<Move> AiPlayer::chooseMove(const Board& board, Stone aiStone) const {
    if (aiStone == Stone::Empty || board.isFull()) return std::nullopt;
    const std::vector<Move> moves = candidates(board);
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
