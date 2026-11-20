#include "gomoku/core/board.hpp"
#include "gomoku/three_player/game.hpp"
#include "gomoku/three_player/ai.hpp"

#include <array>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[失败] " << message << '\n';
        ++failures;
    }
}

void placeStones(gomoku::Board& board, gomoku::Stone stone,
                 std::initializer_list<gomoku::Move> moves) {
    for (const auto move : moves) board.place(move, stone);
}

bool completesFour(const gomoku::Board& board, gomoku::Move move, gomoku::Stone stone) {
    gomoku::Board simulated = board;
    return simulated.place(move, stone) && gomoku::formsFourInRow(simulated, move, stone);
}

bool playAll(gomoku::ThreePlayerGame& game, std::initializer_list<gomoku::Move> moves) {
    for (const auto move : moves) {
        if (!game.play(move)) return false;
    }
    return true;
}

void testNextStoneCycle() {
    check(gomoku::nextStone(gomoku::Stone::Black) == gomoku::Stone::White, "黑棋之后轮到白棋");
    check(gomoku::nextStone(gomoku::Stone::White) == gomoku::Stone::Red, "白棋之后轮到红棋");
    check(gomoku::nextStone(gomoku::Stone::Red) == gomoku::Stone::Black, "红棋之后轮到黑棋");
}

void testTurnsAndHistory() {
    gomoku::ThreePlayerGame game;
    check(game.currentPlayer() == gomoku::Stone::Black, "三人局黑棋先手");
    check(!game.finished() && !game.winner(), "空棋盘对局进行中");

    check(game.play({7, 7}), "黑棋合法落子");
    check(game.currentPlayer() == gomoku::Stone::White, "黑棋之后轮到白棋");
    check(game.play({7, 8}), "白棋合法落子");
    check(game.currentPlayer() == gomoku::Stone::Red, "白棋之后轮到红棋");
    check(game.play({7, 9}), "红棋合法落子");
    check(game.currentPlayer() == gomoku::Stone::Black, "红棋之后回到黑棋");

    check(game.moveHistory().size() == 3, "三次落子写入三条棋谱");
    check(game.moveHistory()[0].stone == gomoku::Stone::Black &&
              game.moveHistory()[0].move == gomoku::Move{7, 7},
          "棋谱按顺序记录黑棋");
    check(game.moveHistory()[1].stone == gomoku::Stone::White, "棋谱按顺序记录白棋");
    check(game.moveHistory()[2].stone == gomoku::Stone::Red, "棋谱按顺序记录红棋");

    check(!game.play({7, 7}), "拒绝重复落子");
    check(!game.play({-1, 0}), "拒绝越界落子");
    check(game.currentPlayer() == gomoku::Stone::Black, "非法落子不切换回合");
    check(game.moveHistory().size() == 3, "非法落子不写入棋谱");
}

void testWinInAllDirections() {
    const gomoku::Move directions[] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    const gomoku::Move whiteFillers[] = {{0, 0}, {0, 2}, {0, 4}};
    const gomoku::Move redFillers[] = {{14, 14}, {14, 12}, {14, 10}};

    for (const auto direction : directions) {
        gomoku::ThreePlayerGame game;
        const gomoku::Move start{7, 7};
        for (int offset = 0; offset < 3; ++offset) {
            check(game.play({start.row + direction.row * offset,
                             start.col + direction.col * offset}),
                  "布置黑棋三连");
            check(game.play(whiteFillers[offset]), "白棋填充空位");
            check(game.play(redFillers[offset]), "红棋填充空位");
            check(!game.finished() && !game.winner(), "三连尚未获胜");
        }
        const gomoku::Move winning{start.row + direction.row * 3,
                                   start.col + direction.col * 3};
        check(game.play(winning), "黑棋补上第四颗棋子");
        check(game.winner() == gomoku::Stone::Black, "黑棋四连获胜");
        check(game.finished(), "获胜后对局结束");
        check(game.currentPlayer() == gomoku::Stone::Black, "获胜后回合停在获胜方");
        check(game.moveHistory().size() == 10, "获胜棋步写入棋谱");
        check(!game.play({0, 14}), "终局后拒绝继续落子");
        check(game.moveHistory().size() == 10, "终局后的落子不写入棋谱");
    }
}

void testWhiteWins() {
    gomoku::ThreePlayerGame game;
    const gomoku::Move blackFillers[] = {{14, 0}, {14, 2}, {14, 4}, {14, 6}};
    const gomoku::Move redFillers[] = {{0, 14}, {0, 12}, {0, 10}};
    for (int i = 0; i < 3; ++i) {
        check(game.play(blackFillers[i]), "黑棋填充空位");
        check(game.play({7, 5 + i}), "布置白棋");
        check(game.play(redFillers[i]), "红棋填充空位");
        check(!game.finished(), "白棋三连尚未获胜");
    }
    check(game.play(blackFillers[3]), "黑棋填充空位");
    check(game.play({7, 8}), "白棋补上第四颗棋子");
    check(game.winner() == gomoku::Stone::White, "白棋四连获胜");
    check(game.currentPlayer() == gomoku::Stone::White, "获胜后回合停在白棋");
}

void testRedWins() {
    gomoku::ThreePlayerGame game;
    const gomoku::Move blackFillers[] = {{14, 0}, {14, 2}, {14, 4}, {14, 6}};
    const gomoku::Move whiteFillers[] = {{0, 14}, {0, 12}, {0, 10}, {0, 8}};
    for (int i = 0; i < 3; ++i) {
        check(game.play(blackFillers[i]), "黑棋填充空位");
        check(game.play(whiteFillers[i]), "白棋填充空位");
        check(game.play({5, 5 + i}), "布置红棋");
        check(!game.finished(), "红棋三连尚未获胜");
    }
    check(game.play(blackFillers[3]), "黑棋填充空位");
    check(game.play(whiteFillers[3]), "白棋填充空位");
    check(game.play({5, 8}), "红棋补上第四颗棋子");
    check(game.winner() == gomoku::Stone::Red, "红棋四连获胜");
    check(game.currentPlayer() == gomoku::Stone::Red, "获胜后回合停在红棋");
}

void testOverlineWins() {
    gomoku::ThreePlayerGame game;
    const gomoku::Move black[] = {{7, 3}, {7, 5}, {7, 6}, {7, 7}};
    const gomoku::Move whiteFillers[] = {{0, 0}, {0, 2}, {0, 4}, {0, 6}};
    const gomoku::Move redFillers[] = {{14, 0}, {14, 2}, {14, 4}, {14, 6}};
    for (int i = 0; i < 4; ++i) {
        check(game.play(black[i]), "布置分散的黑棋");
        check(game.play(whiteFillers[i]), "白棋填充空位");
        check(game.play(redFillers[i]), "红棋填充空位");
    }
    check(!game.finished() && !game.winner(), "最长三连不算获胜");
    check(game.play({7, 4}), "黑棋补成五连");
    check(game.winner() == gomoku::Stone::Black, "五连及以上同样获胜");
    check(game.finished(), "五连后对局结束");
}

void testFullBoardDraw() {
    // 三色棋盘着色，保证任意方向都不存在四连；按黑/白/红顺序填满棋盘即为平局。
    const auto colorIndex = [](int row, int col) { return (row + 2 * col + row / 2) % 3; };
    std::array<std::vector<gomoku::Move>, 3> groups;
    for (int row = 0; row < gomoku::kBoardSize; ++row) {
        for (int col = 0; col < gomoku::kBoardSize; ++col) {
            groups[static_cast<std::size_t>(colorIndex(row, col))].push_back({row, col});
        }
    }
    check(groups[0].size() == 75 && groups[1].size() == 75 && groups[2].size() == 75,
          "平局着色均匀分布");

    gomoku::ThreePlayerGame game;
    for (std::size_t i = 0; i < groups[0].size(); ++i) {
        check(game.play(groups[0][i]), "平局棋盘落黑棋");
        check(game.play(groups[1][i]), "平局棋盘落白棋");
        check(game.play(groups[2][i]), "平局棋盘落红棋");
    }
    check(game.board().isFull(), "棋盘已下满");
    check(game.finished(), "满盘后对局结束");
    check(!game.winner(), "满盘且无四连判平局");
    check(game.moveHistory().size() == 225, "平局棋谱包含所有落子");
    check(!game.play({0, 0}), "平局后拒绝继续落子");
}

void testOpenThreeRuleBlocksNewOpenThree() {
    gomoku::ThreePlayerGame game(true);
    // 黑棋先成活三；随后白棋试图在另一条线上形成活三。
    const bool setup = playAll(game, {
        {7, 3}, {5, 5}, {0, 0},
        {7, 4}, {5, 6}, {0, 1},
        {7, 5}, {12, 12}, {12, 14},
        {14, 0},
    });
    check(setup, "同屏活三限制局面布置成功");
    check(game.currentPlayer() == gomoku::Stone::White, "活三限制局面轮到白棋");

    const std::size_t before = game.moveHistory().size();
    check(game.violatesOpenThreeRule({5, 4}), "识别出会形成活三的禁手");
    check(!game.play({5, 4}), "其他玩家有活三时禁止再成活三");
    check(game.board().isEmpty({5, 4}), "禁手不会落在棋盘上");
    check(game.currentPlayer() == gomoku::Stone::White, "禁手后仍轮到白棋");
    check(game.moveHistory().size() == before, "禁手不写入棋谱");
    check(game.play({10, 10}), "不形成活三的落子仍然允许");
}

void testOpenThreeRuleCountsDeadThree() {
    gomoku::ThreePlayerGame game(true);
    // 红棋在 (7,2) 封住黑棋一端，使黑棋只有唯一成四点，形成死三。
    const bool setup = playAll(game, {
        {7, 3}, {5, 5}, {7, 2},
        {7, 4}, {5, 6}, {0, 0},
        {7, 5}, {12, 12}, {12, 14},
        {14, 0},
    });
    check(setup, "死三局面布置成功");
    check(game.currentPlayer() == gomoku::Stone::White, "死三局面轮到白棋");
    check(game.violatesOpenThreeRule({5, 4}), "死三同样触发活三限制");
    check(!game.play({5, 4}), "其他玩家有死三时禁止形成活三");
}

void testOpenThreeRuleOffByDefault() {
    gomoku::ThreePlayerGame game;
    const bool setup = playAll(game, {
        {7, 3}, {5, 5}, {0, 0},
        {7, 4}, {5, 6}, {0, 1},
        {7, 5}, {12, 12}, {12, 14},
        {14, 0},
    });
    check(setup, "标准模式局面布置成功");
    check(!game.violatesOpenThreeRule({5, 4}), "标准模式不检查同屏活三限制");
    check(game.play({5, 4}), "标准模式允许形成活三");
}

void testAiTakesImmediateWin() {
    gomoku::Board board;
    placeStones(board, gomoku::Stone::White, {{7, 3}, {7, 4}, {7, 5}});
    placeStones(board, gomoku::Stone::Black, {{0, 0}, {0, 1}});
    placeStones(board, gomoku::Stone::Red, {{14, 0}, {14, 1}});

    gomoku::ThreePlayerAi ai;
    const auto move = ai.chooseMove(board, gomoku::Stone::White);
    check(move && completesFour(board, *move, gomoku::Stone::White), "AI 优先完成自己的四连");
}

void testAiBlocksNextOpponent() {
    // 白棋之后轮到红棋；红棋左下角被黑棋封住，唯一胜点是 (7,6)。
    gomoku::Board board;
    placeStones(board, gomoku::Stone::Red, {{7, 3}, {7, 4}, {7, 5}});
    placeStones(board, gomoku::Stone::Black, {{7, 2}, {0, 0}});
    placeStones(board, gomoku::Stone::White, {{14, 14}});

    gomoku::ThreePlayerAi ai;
    const auto move = ai.chooseMove(board, gomoku::Stone::White);
    check(move && *move == gomoku::Move{7, 6}, "AI 阻挡下一手对手的唯一胜点");
}

void testAiBlocksHuman() {
    // 白棋之后轮到红棋，红棋没有威胁；此时应处理真人黑棋的胜点。
    gomoku::Board board;
    placeStones(board, gomoku::Stone::Black, {{7, 3}, {7, 4}, {7, 5}});
    placeStones(board, gomoku::Stone::Red, {{7, 2}});
    placeStones(board, gomoku::Stone::White, {{14, 14}});

    gomoku::ThreePlayerAi ai;
    const auto move = ai.chooseMove(board, gomoku::Stone::White);
    check(move && *move == gomoku::Move{7, 6}, "AI 阻挡真人玩家的直接胜点");
}

void testAiBlocksHumanWhenHumanMovesNext() {
    // 红棋落子后轮到真人黑棋；黑棋有唯一胜点，电脑必须封堵。
    gomoku::Board board;
    placeStones(board, gomoku::Stone::Black, {{7, 3}, {7, 4}, {7, 5}});
    placeStones(board, gomoku::Stone::White, {{7, 2}, {14, 14}});
    placeStones(board, gomoku::Stone::Red, {{0, 0}});

    gomoku::ThreePlayerAi ai;
    const auto move = ai.chooseMove(board, gomoku::Stone::Red);
    check(move && *move == gomoku::Move{7, 6}, "电脑阻挡下一手真人玩家的胜点");
}

void testAiPrefersOwnWin() {
    gomoku::Board board;
    placeStones(board, gomoku::Stone::White, {{7, 3}, {7, 4}, {7, 5}});
    placeStones(board, gomoku::Stone::Red, {{9, 3}, {9, 4}, {9, 5}});
    placeStones(board, gomoku::Stone::Black, {{0, 0}});

    gomoku::ThreePlayerAi ai;
    const auto move = ai.chooseMove(board, gomoku::Stone::White);
    check(move && completesFour(board, *move, gomoku::Stone::White),
          "AI 自己的胜点优先于阻挡其他玩家");
}

void testAiRoundOrder() {
    gomoku::ThreePlayerGame game;
    gomoku::ThreePlayerAi ai;

    check(game.play({7, 7}), "真人黑棋落子");
    check(game.currentPlayer() == gomoku::Stone::White, "真人落子后轮到电脑1");

    const auto white = ai.chooseMove(game.board(), game.currentPlayer());
    check(white && game.play(*white), "电脑1生成并完成合法落子");
    check(game.currentPlayer() == gomoku::Stone::Red, "电脑1落子后轮到电脑2");

    const auto red = ai.chooseMove(game.board(), game.currentPlayer());
    check(red && game.play(*red), "电脑2生成并完成合法落子");
    check(game.currentPlayer() == gomoku::Stone::Black, "两台电脑落子后回到真人");
}

void testAiLegalAndDeterministic() {
    gomoku::Board board;
    placeStones(board, gomoku::Stone::Black, {{7, 7}, {7, 8}, {8, 7}});
    placeStones(board, gomoku::Stone::White, {{7, 9}, {8, 8}});
    placeStones(board, gomoku::Stone::Red, {{9, 9}});

    gomoku::ThreePlayerAi ai;
    const auto first = ai.chooseMove(board, gomoku::Stone::White);
    const auto second = ai.chooseMove(board, gomoku::Stone::White);
    check(first && board.isEmpty(*first), "AI 返回棋盘内的空位");
    check(first && second && *first == *second, "同一棋面 AI 选择可复现");
}

}  // namespace

int main() {
    testNextStoneCycle();
    testTurnsAndHistory();
    testWinInAllDirections();
    testWhiteWins();
    testRedWins();
    testOverlineWins();
    testFullBoardDraw();
    testOpenThreeRuleBlocksNewOpenThree();
    testOpenThreeRuleCountsDeadThree();
    testOpenThreeRuleOffByDefault();
    testAiTakesImmediateWin();
    testAiBlocksNextOpponent();
    testAiBlocksHuman();
    testAiBlocksHumanWhenHumanMovesNext();
    testAiPrefersOwnWin();
    testAiRoundOrder();
    testAiLegalAndDeterministic();
    if (failures != 0) {
        std::cerr << failures << " 项三人四子棋测试失败。\n";
        return EXIT_FAILURE;
    }
    std::cout << "所有三人四子棋核心测试均已通过。\n";
    return EXIT_SUCCESS;
}
