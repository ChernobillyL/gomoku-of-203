#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace gomoku {

constexpr int kBoardSize = 15;
constexpr int kMaxBoardSize = 19;

enum class Stone : char { Empty = '.', Black = 'X', White = 'O', Red = 'R' };

// 经典五子棋的胜负状态。三人局与六子棋使用各自的 winner() 表示胜者。
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

// 两人对局中另一方使用的棋子。三人局请使用 nextStone()。
Stone opponent(Stone stone);

// 所有棋种共用的棋盘。只负责落子、查询和“五连”这一经典规则的判定；
// 三人四子棋与六子棋的连子规则由各自的 Game 类负责。
class Board {
public:
    // size 超出 1..kMaxBoardSize 时回退到经典 15 路，避免非法尺寸导致越界。
    explicit Board(int size = kBoardSize);

    int size() const { return size_; }
    // 判断坐标是否落在当前棋盘内。
    bool inBounds(Move move) const;
    // 仅在坐标合法且该点为空时返回 true。
    bool isEmpty(Move move) const;
    // 越界坐标统一返回 Empty，调用方无需先做边界判断。
    Stone at(Move move) const;
    // 在空点放置 stone 并维护 moveCount_；越界、重复落子或 Empty 棋子返回 false。
    bool place(Move move, Stone stone);
    bool isFull() const;
    int moveCount() const { return moveCount_; }
    // 经典五子棋判胜：从最后落点检查四个方向是否达到五连；满盘无胜者为 Draw。
    // 三人四子棋和六子棋不使用此函数，而由各自模块的规则函数判定。
    Result resultAfter(Move lastMove) const;
    // 按行优先顺序返回所有空点，供“全盘找必胜点”等少量调用使用。
    std::vector<Move> emptyMoves() const;

private:
    std::array<Stone, kMaxBoardSize * kMaxBoardSize> cells_{};
    int size_ = kBoardSize;
    int moveCount_ = 0;
    std::size_t index(Move move) const;
};

// 生成落子点附近的候选空位，供三种棋的电脑玩家共用的搜索排序与剪枝。
// 棋盘为空时返回天元。
std::vector<Move> candidateMoves(const Board& board);

}  // namespace gomoku
