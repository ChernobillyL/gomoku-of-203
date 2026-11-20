#include "gomoku/six_player/game.hpp"
#include "gomoku/six_player/ai.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[失败] " << message << '\n';
        ++failures;
    }
}

void testOpeningAndTwoStoneTurns() {
    gomoku::SixPlayerGame game;
    check(game.currentPlayer() == gomoku::Stone::Black && game.movesRemaining() == 1,
          "开局黑方只下一子");
    check(game.play({7, 7}), "黑方开局落子");
    check(game.currentPlayer() == gomoku::Stone::White && game.movesRemaining() == 2,
          "黑方首子后白方获得两子回合");
    check(game.play({7, 8}), "白方回合第一子");
    check(game.currentPlayer() == gomoku::Stone::White && game.movesRemaining() == 1,
          "白方第一子后仍是白方");
    check(game.play({7, 9}), "白方回合第二子");
    check(game.currentPlayer() == gomoku::Stone::Black && game.movesRemaining() == 2,
          "白方第二子后黑方获得两子回合");
    check(game.moveHistory().size() == 3 && game.moveHistory()[0].stone == gomoku::Stone::Black &&
              game.moveHistory()[1].stone == gomoku::Stone::White &&
              game.moveHistory()[2].stone == gomoku::Stone::White,
          "棋谱按实际落子颜色记录双子回合");
}

void testIllegalAndTerminalMoves() {
    gomoku::SixPlayerGame game;
    check(!game.play({-1, 0}), "拒绝越界落子");
    check(game.play({7, 7}), "黑方合法开局");
    check(!game.play({7, 7}), "拒绝重复落子");
    check(game.currentPlayer() == gomoku::Stone::White && game.movesRemaining() == 2,
          "非法落子不消耗回合棋数");
}

void testSixLineDirections() {
    const gomoku::Move directions[] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    for (const auto direction : directions) {
        gomoku::Board board(gomoku::kSixPlayerBoardSize);
        const gomoku::Move start{9, 9};
        gomoku::Move last{};
        for (int i = 0; i < gomoku::kSixPlayerWinLength; ++i) {
            last = {start.row + i * direction.row, start.col + i * direction.col};
            check(board.place(last, gomoku::Stone::Black), "布置六连棋子");
        }
        check(gomoku::formsSixInRow(board, last, gomoku::Stone::Black), "横竖斜六连判胜");
    }
    gomoku::Board five(gomoku::kSixPlayerBoardSize);
    gomoku::Move last{};
    for (int col = 0; col < 5; ++col) {
        last = {0, col};
        five.place(last, gomoku::Stone::White);
    }
    check(!gomoku::formsSixInRow(five, last, gomoku::Stone::White), "五连尚未达到六子胜线");
}

void testGameEndsImmediatelyOnSixthStone() {
    gomoku::SixPlayerGame game;
    const gomoku::Move sequence[] = {
        {7, 3}, {0, 0}, {0, 2}, {7, 4}, {7, 5}, {0, 4},
        {0, 6}, {7, 6}, {7, 7}, {0, 8}, {0, 10}, {7, 8}};
    for (std::size_t i = 0; i + 1 < sizeof(sequence) / sizeof(sequence[0]); ++i) {
        check(game.play(sequence[i]), "六连获胜前落子合法");
        check(!game.finished(), "六连形成前对局继续");
    }
    check(game.play(sequence[11]), "完成第六子");
    check(game.winner() == gomoku::Stone::Black && game.finished(), "黑方第六子立即获胜");
    check(game.moveHistory().size() == 12, "获胜棋步写入棋谱");
    check(!game.play({8, 8}), "终局后拒绝落子");
}

void testAiFirstBlackMoveCanWinWithoutPlayingSecondStone() {
    gomoku::SixPlayerGame game;
    const gomoku::Move sequence[] = {
        {0, 0}, {14, 0}, {14, 2}, {7, 2}, {7, 3}, {14, 4},
        {14, 6}, {7, 4}, {7, 5}, {14, 8}, {14, 10}};
    for (const auto move : sequence) {
        check(game.play(move), "布置 AI 必胜局面");
    }
    check(game.currentPlayer() == gomoku::Stone::Black && game.movesRemaining() == 2,
          "局面轮到黑方双子回合");
    check(game.play({7, 6}), "黑方回合第一子形成五连");
    check(game.currentPlayer() == gomoku::Stone::Black && game.movesRemaining() == 1,
          "五连未获胜且黑方仍有本回合第二子");
    gomoku::SixPlayerAi ai;
    const auto win = ai.chooseMove(game.board(), gomoku::Stone::Black, game.movesRemaining());
    check(win && *win == gomoku::Move{7, 7} && game.play(*win), "黑方 AI 第二子完成六连");
    check(game.winner() == gomoku::Stone::Black, "第二子成六后立即获胜");
}

void testAiBlocksOpponentImmediateSix() {
    gomoku::SixPlayerAi ai;
    gomoku::Board threat(gomoku::kSixPlayerBoardSize);
    // 白棋在 10 行连成五子，左侧被黑棋封住，唯一成六点是 (10, 10)。
    check(threat.place({10, 4}, gomoku::Stone::Black), "布置封堵黑子");
    for (int col = 5; col <= 9; ++col) {
        check(threat.place({10, col}, gomoku::Stone::White), "布置白棋五连");
    }
    const auto block = ai.chooseMove(threat, gomoku::Stone::Black, 2);
    check(block && *block == gomoku::Move{10, 10}, "AI 阻挡对手唯一的成六点");
}

void testAiFindsImmediateWinAndLegalMove() {
    gomoku::SixPlayerAi ai;
    gomoku::Board winning(gomoku::kSixPlayerBoardSize);
    for (int col = 3; col < 8; ++col) winning.place({9, col}, gomoku::Stone::White);
    const auto win = ai.chooseMove(winning, gomoku::Stone::White, 2);
    check(win && winning.isEmpty(*win), "六子棋 AI 返回合法空位");
    if (win) {
        gomoku::Board after = winning;
        after.place(*win, gomoku::Stone::White);
        check(gomoku::formsSixInRow(after, *win, gomoku::Stone::White), "AI 优先补成六连");
    }

    gomoku::Board opening(gomoku::kSixPlayerBoardSize);
    const auto first = ai.chooseMove(opening, gomoku::Stone::White, 2);
    check(first && *first == gomoku::Move{9, 9}, "19 路空棋盘开局选中心点");
}

}  // namespace

int main() {
    testOpeningAndTwoStoneTurns();
    testIllegalAndTerminalMoves();
    testSixLineDirections();
    testGameEndsImmediatelyOnSixthStone();
    testAiFirstBlackMoveCanWinWithoutPlayingSecondStone();
    testAiBlocksOpponentImmediateSix();
    testAiFindsImmediateWinAndLegalMove();
    if (failures != 0) {
        std::cerr << failures << " 项六子棋测试失败。\n";
        return EXIT_FAILURE;
    }
    std::cout << "所有六子棋核心测试均已通过。\n";
    return EXIT_SUCCESS;
}
