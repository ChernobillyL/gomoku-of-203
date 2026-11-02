#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace gomoku {

constexpr int kBoardSize = 15;

enum class Stone : char { Empty = '.', Black = 'X', White = 'O' };
enum class Result { Ongoing, BlackWins, WhiteWins, Draw };

struct Move {
    int row = 0;
    int col = 0;

    friend bool operator==(const Move& lhs, const Move& rhs) {
        return lhs.row == rhs.row && lhs.col == rhs.col;
    }
};

struct MoveRecord {
    Move move;
    Stone stone = Stone::Empty;
};

Stone opponent(Stone stone);

class Board {
public:
    Board();

    bool inBounds(Move move) const;
    bool isEmpty(Move move) const;
    Stone at(Move move) const;
    bool place(Move move, Stone stone);
    bool isFull() const;
    int moveCount() const { return moveCount_; }
    Result resultAfter(Move lastMove) const;
    std::vector<Move> emptyMoves() const;

private:
    std::array<Stone, kBoardSize * kBoardSize> cells_{};
    int moveCount_ = 0;
    std::size_t index(Move move) const;
};

class Game {
public:
    Game() = default;

    const Board& board() const { return board_; }
    Stone currentPlayer() const { return currentPlayer_; }
    Result result() const { return result_; }
    const std::vector<MoveRecord>& moveHistory() const { return moveHistory_; }
    bool play(Move move);

private:
    Board board_;
    Stone currentPlayer_ = Stone::Black;
    Result result_ = Result::Ongoing;
    std::vector<MoveRecord> moveHistory_;
};

class AiPlayer {
public:
    // Returns no move only when the board has no empty intersections.
    std::optional<Move> chooseMove(const Board& board, Stone aiStone) const;

private:
    static long long scoreMove(const Board& board, Move move, Stone stone);
    static std::vector<Move> candidates(const Board& board);
};

}  // namespace gomoku
