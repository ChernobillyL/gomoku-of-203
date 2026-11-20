#pragma once

#include "gomoku/core/board.hpp"

#include <optional>
#include <vector>

namespace gomoku {

// 三种棋的统一接口。控制台与 Win32 界面只依赖此抽象类，
// 新增棋种时无需修改界面代码。
class GameBase {
public:
    virtual ~GameBase() = default;

    // 以下纯虚函数构成三种棋的共同协议；派生类用 override 提供具体规则。
    virtual const Board& board() const = 0;       // 当前棋盘（只读）
    virtual Stone currentPlayer() const = 0;      // 当前轮到哪一方
    virtual bool finished() const = 0;            // 是否已分胜负或满盘
    virtual std::optional<Stone> winner() const = 0;  // 胜者；未结束/平局为空
    virtual const std::vector<MoveRecord>& moveHistory() const = 0;  // 完整棋谱
    // 尝试落子；非法落子返回 false，且不得改变对局状态。
    virtual bool play(Move move) = 0;
};

}  // namespace gomoku
