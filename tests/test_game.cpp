#include "gomoku/game.hpp"
#include "gomoku/input.hpp"

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

void testMoveValidation() {
    gomoku::Board board;
    check(board.place({0, 0}, gomoku::Stone::Black), "角落合法落子");
    check(!board.place({0, 0}, gomoku::Stone::White), "禁止重复落子");
    check(!board.place({-1, 0}, gomoku::Stone::White), "禁止负坐标");
    check(!board.place({gomoku::kBoardSize, 0}, gomoku::Stone::White), "禁止越界坐标");
    check(board.moveCount() == 1, "非法落子不改变棋子数");
}

void testWinDirections() {
    const int center = gomoku::kBoardSize / 2;
    const gomoku::Move directions[] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    for (const auto direction : directions) {
        gomoku::Board board;
        for (int offset = -2; offset <= 1; ++offset) {
            check(board.place({center + direction.row * offset, center + direction.col * offset},
                              gomoku::Stone::Black), "设置四连测试棋子");
        }
        const gomoku::Move winning{center + direction.row * 2, center + direction.col * 2};
        check(board.place(winning, gomoku::Stone::Black), "放置获胜棋子");
        check(board.resultAfter(winning) == gomoku::Result::BlackWins, "横竖斜方向五连判胜");
    }

    gomoku::Board four;
    for (int col = 0; col < 4; ++col) four.place({0, col}, gomoku::Stone::White);
    check(four.resultAfter({0, 3}) == gomoku::Result::Ongoing, "四连尚未获胜");

    gomoku::Board draw;
    gomoku::Move last;
    const gomoku::Stone pattern[] = {gomoku::Stone::Black, gomoku::Stone::Black,
                                     gomoku::Stone::White, gomoku::Stone::White};
    for (int row = 0; row < gomoku::kBoardSize; ++row) {
        for (int col = 0; col < gomoku::kBoardSize; ++col) {
            last = {row, col};
            draw.place(last, pattern[(row + 2 * col) % 4]);
        }
    }
    check(draw.resultAfter(last) == gomoku::Result::Draw, "满盘且没有五连判和棋");
}

void testInputParsing() {
    gomoku::Move move{4, 5};
    check(gomoku::parseMoveInput("8 8", move) == gomoku::MoveInputStatus::Valid &&
              move == gomoku::Move{7, 7}, "解析有效坐标");
    check(gomoku::parseMoveInput(" 1 15 ", move) == gomoku::MoveInputStatus::Valid &&
              move == gomoku::Move{0, 14}, "解析首尾边界坐标");
    check(gomoku::parseMoveInput("abc 2", move) == gomoku::MoveInputStatus::InvalidFormat,
          "拒绝非数字坐标");
    check(gomoku::parseMoveInput("8 8 xyz", move) == gomoku::MoveInputStatus::InvalidFormat,
          "拒绝尾随垃圾字符");
    check(gomoku::parseMoveInput("999999999999 2", move) == gomoku::MoveInputStatus::InvalidFormat,
          "拒绝整数溢出");
    check(gomoku::parseMoveInput("0 8", move) == gomoku::MoveInputStatus::OutOfRange,
          "拒绝越界坐标");
    int choice = 0;
    check(gomoku::parseMenuInput(" 2 ", choice) && choice == 2, "解析菜单选项");
    check(!gomoku::parseMenuInput("2 extra", choice), "拒绝多余菜单内容");
}

void testGameTurns() {
    gomoku::Game game;
    check(game.currentPlayer() == gomoku::Stone::Black, "黑棋先手");
    check(!game.play({-1, 0}), "非法落子被拒绝");
    check(game.currentPlayer() == gomoku::Stone::Black, "非法落子后不切换回合");
    check(game.play({7, 7}), "黑棋合法落子");
    check(game.currentPlayer() == gomoku::Stone::White, "合法落子后切换回合");
}

void testGameWinStopsFurtherMoves() {
    gomoku::Game game;
    const gomoku::Move sequence[] = {
        {7, 3}, {0, 0}, {7, 4}, {0, 1}, {7, 5}, {0, 2}, {7, 6}, {0, 3}, {7, 7}};
    for (const gomoku::Move move : sequence) {
        check(game.play(move), "通过 Game 完成合法对局落子");
    }
    check(game.result() == gomoku::Result::BlackWins, "Game 正确记录黑棋获胜");
    check(game.currentPlayer() == gomoku::Stone::Black, "对局结束后不再切换当前棋手");
    check(!game.play({8, 8}), "对局结束后拒绝继续落子");
}

void testAiWinsAndBlocks() {
    gomoku::AiPlayer ai;
    gomoku::Board winning;
    for (int col = 3; col <= 6; ++col) winning.place({7, col}, gomoku::Stone::White);
    const auto winningMove = ai.chooseMove(winning, gomoku::Stone::White);
    check(winningMove.has_value(), "AI 可以找到获胜点");
    if (winningMove) {
        check(winning.isEmpty(*winningMove), "AI 获胜点为空");
        gomoku::Board after = winning;
        after.place(*winningMove, gomoku::Stone::White);
        check(after.resultAfter(*winningMove) == gomoku::Result::WhiteWins, "AI 优先完成五连");
    }

    gomoku::Board threat;
    for (int col = 4; col <= 7; ++col) threat.place({6, col}, gomoku::Stone::Black);
    threat.place({6, 3}, gomoku::Stone::White);
    const auto block = ai.chooseMove(threat, gomoku::Stone::White);
    check(block && *block == gomoku::Move{6, 8}, "AI 阻挡对手唯一的一步胜利点");
}

void testAiLegalityAndDeterminism() {
    gomoku::AiPlayer ai;
    gomoku::Board empty;
    const auto opening = ai.chooseMove(empty, gomoku::Stone::White);
    check(opening && *opening == gomoku::Move{7, 7}, "空盘 AI 选择中心");

    empty.place({7, 7}, gomoku::Stone::Black);
    const auto first = ai.chooseMove(empty, gomoku::Stone::White);
    const auto second = ai.chooseMove(empty, gomoku::Stone::White);
    check(first && empty.isEmpty(*first), "AI 选择棋盘内空位");
    check(first && second && *first == *second, "同一棋面选择可复现");

    gomoku::Board full;
    for (int row = 0; row < gomoku::kBoardSize; ++row) {
        for (int col = 0; col < gomoku::kBoardSize; ++col) {
            full.place({row, col}, (row + col) % 2 == 0 ? gomoku::Stone::Black : gomoku::Stone::White);
        }
    }
    check(!ai.chooseMove(full, gomoku::Stone::White), "满盘 AI 返回无落点");
}

}  // namespace

int main() {
    testMoveValidation();
    testWinDirections();
    testInputParsing();
    testGameTurns();
    testGameWinStopsFurtherMoves();
    testAiWinsAndBlocks();
    testAiLegalityAndDeterminism();
    if (failures != 0) {
        std::cerr << failures << " 项测试失败。\n";
        return EXIT_FAILURE;
    }
    std::cout << "所有 Gomoku 核心测试均已通过。\n";
    return EXIT_SUCCESS;
}
