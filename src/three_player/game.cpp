#include "gomoku/three_player/game.hpp"

#include <array>

namespace gomoku {
namespace {

constexpr std::array<Move, 4> kDirections{{{0, 1}, {1, 0}, {1, 1}, {1, -1}}};
constexpr std::array<Stone, 3> kAllStones{{Stone::Black, Stone::White, Stone::Red}};

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

// 四格窗口内恰好有三颗己方棋子和一个空位，即构成“下一手可成四”的三连。
// 若窗口紧邻的位置还有己方棋子，则该形状实际是带缺口的四连，不算三连。
bool windowHasThree(const Board& board, Move start, Move direction, Stone stone) {
    int stones = 0;
    int empties = 0;
    for (int i = 0; i < kThreePlayerWinLength; ++i) {
        const Move cell{start.row + i * direction.row, start.col + i * direction.col};
        const Stone occupant = board.at(cell);
        if (occupant == Stone::Empty) {
            ++empties;
        } else if (occupant == stone) {
            ++stones;
        } else {
            return false;
        }
    }
    if (stones != 3 || empties != 1) return false;

    const Move before{start.row - direction.row, start.col - direction.col};
    const Move after{start.row + kThreePlayerWinLength * direction.row,
                     start.col + kThreePlayerWinLength * direction.col};
    if (board.inBounds(before) && board.at(before) == stone) return false;
    if (board.inBounds(after) && board.at(after) == stone) return false;
    return true;
}

bool hasThree(const Board& board, Stone stone) {
    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            for (const Move direction : kDirections) {
                const Move end{row + (kThreePlayerWinLength - 1) * direction.row,
                               col + (kThreePlayerWinLength - 1) * direction.col};
                if (!board.inBounds(end)) continue;
                if (windowHasThree(board, {row, col}, direction, stone)) return true;
            }
        }
    }
    return false;
}

// 棋盘上已经落下 move 后，检查它是否形成三颗连续棋子且两端皆空。
bool formsOpenThree(const Board& board, Move move, Stone stone) {
    for (const Move direction : kDirections) {
        const int backward =
            countDirection(board, move, stone, {-direction.row, -direction.col});
        const int forward = countDirection(board, move, stone, direction);
        if (backward + forward + 1 != 3) continue;
        const Move leftEnd{move.row - (backward + 1) * direction.row,
                           move.col - (backward + 1) * direction.col};
        const Move rightEnd{move.row + (forward + 1) * direction.row,
                            move.col + (forward + 1) * direction.col};
        const bool leftOpen = board.inBounds(leftEnd) && board.at(leftEnd) == Stone::Empty;
        const bool rightOpen = board.inBounds(rightEnd) && board.at(rightEnd) == Stone::Empty;
        if (leftOpen && rightOpen) return true;
    }
    return false;
}

}  // namespace

// 参数：stone 为当前行棋方。
// 返回：三人局的下一方（黑->白->红->黑）；Empty 视为黑方。
Stone nextStone(Stone stone) {
    switch (stone) {
        case Stone::Black: return Stone::White;
        case Stone::White: return Stone::Red;
        case Stone::Red: return Stone::Black;
        case Stone::Empty: break;
    }
    return Stone::Black;
}

// 参数：board 为落子后的棋盘，lastMove 为最后落点，stone 为该手颜色。
// 返回：四个方向中任一方向连续同色棋子达到 4（或更多）时为 true。
// 实现：只从最后落点向正反两个方向计数，长度 >= kThreePlayerWinLength 即胜。
bool formsFourInRow(const Board& board, Move lastMove, Stone stone) {
    if (stone == Stone::Empty || !board.inBounds(lastMove) || board.at(lastMove) != stone) {
        return false;
    }
    for (const Move direction : kDirections) {
        if (lineLength(board, lastMove, stone, direction) >= kThreePlayerWinLength) return true;
    }
    return false;
}

// 参数：move 为当前方准备落子的坐标。
// 返回：启用同屏限制且该手会形成新的活三、同时其他玩家已有三棋型时为 true。
// 实现：先模拟落子；直接成四的获胜手不受限制；否则判断是否形成两端皆空的
// 连续三子，再扫描另外两方是否已有“三子一空”的棋型。规则为本项目自定义。
bool ThreePlayerGame::violatesOpenThreeRule(Move move) const {
    if (!restrictOpenThree_ || finished() || !board_.isEmpty(move)) return false;

    Board simulated = board_;
    if (!simulated.place(move, currentPlayer_)) return false;
    if (formsFourInRow(simulated, move, currentPlayer_)) return false;
    if (!formsOpenThree(simulated, move, currentPlayer_)) return false;

    for (const Stone stone : kAllStones) {
        if (stone != currentPlayer_ && hasThree(board_, stone)) return true;
    }
    return false;
}

// 参数：move 为当前方落子坐标。
// 返回：成功为 true；终局、违反同屏活三限制或落点非法时为 false。
// 实现：规则检查 -> place -> 记录棋谱 -> 四连判胜 -> 未结束则 nextStone 换手。
bool ThreePlayerGame::play(Move move) {
    if (finished() || violatesOpenThreeRule(move) || !board_.place(move, currentPlayer_)) {
        return false;
    }
    const Stone placed = currentPlayer_;
    moveHistory_.push_back({move, placed});
    if (formsFourInRow(board_, move, placed)) {
        winner_ = placed;
        return true;
    }
    if (!board_.isFull()) currentPlayer_ = nextStone(placed);
    return true;
}

}  // namespace gomoku
