#include "gomoku/game.hpp"
#include "gomoku/input.hpp"

#include <iostream>
#include <string>
#include <utility>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace {

using gomoku::AiPlayer;
using gomoku::Board;
using gomoku::Game;
using gomoku::Move;
using gomoku::Result;
using gomoku::Stone;

enum class ReadStatus { Line, Escape, EndOfInput };
struct ReadResult {
    ReadStatus status;
    std::string text;
};

#ifdef _WIN32
int readCharacter() {
    const int ch = _getch();
    if (ch == 0 || ch == 224) {
        (void)_getch();
        return 0;
    }
    return ch;
}
#else
class TerminalMode {
public:
    TerminalMode() {
        if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &original_) == 0) {
            termios raw = original_;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            active_ = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
        }
    }

    ~TerminalMode() {
        restore();
    }

    void restore() {
        if (active_) {
            tcsetattr(STDIN_FILENO, TCSANOW, &original_);
            active_ = false;
        }
    }

private:
    termios original_{};
    bool active_ = false;
};

int readCharacter() {
    unsigned char ch = 0;
    const ssize_t bytes = ::read(STDIN_FILENO, &ch, 1);
    if (bytes != 1) return -1;
    return ch;
}
#endif

ReadResult readLine(const std::string& prompt) {
#ifndef _WIN32
    TerminalMode terminalMode;
    (void)terminalMode;
#endif
    std::cout << prompt << std::flush;
    std::string line;
    while (true) {
        const int ch = readCharacter();
        if (ch < 0) return {ReadStatus::EndOfInput, {}};
        if (ch == 27) {
            std::cout << '\n';
            return {ReadStatus::Escape, {}};
        }
        if (ch == '\r' || ch == '\n') {
            std::cout << '\n';
            return {ReadStatus::Line, std::move(line)};
        }
        if (ch == 8 || ch == 127) {
            if (!line.empty()) {
                line.pop_back();
                std::cout << "\b \b" << std::flush;
            }
            continue;
        }
        if (ch >= 32 && ch <= 126) {
            line.push_back(static_cast<char>(ch));
            std::cout << static_cast<char>(ch) << std::flush;
        }
    }
}

std::string stoneSymbol(Stone stone) {
    if (stone == Stone::Black) return "●";
    if (stone == Stone::White) return "○";
    return "+";
}

void drawBoard(const Board& board) {
    std::cout << "\n    ";
    for (int col = 1; col <= gomoku::kBoardSize; ++col) {
        if (col < 10) std::cout << ' ';
        std::cout << col << ' ';
    }
    std::cout << '\n';
    for (int row = 0; row < gomoku::kBoardSize; ++row) {
        if (row + 1 < 10) std::cout << ' ';
        std::cout << row + 1 << "  ";
        for (int col = 0; col < gomoku::kBoardSize; ++col) {
            std::cout << stoneSymbol(board.at({row, col})) << "  ";
        }
        std::cout << '\n';
    }
    std::cout << "\n符号：黑棋 ●  白棋 ○\n";
}

const char* playerName(Stone stone, bool pvc) {
    if (pvc && stone == Stone::White) return "电脑（白棋）";
    return stone == Stone::Black ? "玩家1（黑棋）" : "玩家2（白棋）";
}

bool getHumanMove(const Game& game, Move& move) {
    const std::string coordinateRange = "1-" + std::to_string(gomoku::kBoardSize);
    const std::string prompt = "输入行 列（" + coordinateRange + "），如：8 8（按 Esc 退出）：";
    while (true) {
        const ReadResult input = readLine(prompt);
        if (input.status != ReadStatus::Line) return false;
        const gomoku::MoveInputStatus status = gomoku::parseMoveInput(input.text, move);
        if (status == gomoku::MoveInputStatus::InvalidFormat) {
            std::cout << "输入格式错误，请输入两个整数，例如：8 8。\n";
            continue;
        }
        if (status == gomoku::MoveInputStatus::OutOfRange) {
            std::cout << "坐标越界，行和列必须在 1 到 15 之间。\n";
            continue;
        }
        if (!game.board().isEmpty(move)) {
            std::cout << "该位置已有棋子，请选择空位。\n";
            continue;
        }
        return true;
    }
}

void announceResult(Result result, bool pvc) {
    switch (result) {
        case Result::BlackWins:
            std::cout << (pvc ? "玩家（黑棋）获胜！\n" : "玩家1（黑棋）获胜！\n");
            break;
        case Result::WhiteWins:
            std::cout << (pvc ? "电脑（白棋）获胜！\n" : "玩家2（白棋）获胜！\n");
            break;
        case Result::Draw:
            std::cout << "棋盘下满，平局！\n";
            break;
        case Result::Ongoing:
            break;
    }
}

bool playGame(bool pvc) {
    Game game;
    AiPlayer ai;
    std::cout << "\n=== " << (pvc ? "PvC 人机对战" : "PvP 玩家对战") << " ===\n"
              << "黑棋先手；输入行 列落子。等待输入时按 Esc 可立即退出程序。\n";
    drawBoard(game.board());

    while (game.result() == Result::Ongoing) {
        const Stone turn = game.currentPlayer();
        std::cout << "轮到：" << playerName(turn, pvc) << '\n';
        Move move;
        if (pvc && turn == Stone::White) {
            const auto choice = ai.chooseMove(game.board(), Stone::White);
            if (!choice || !game.play(*choice)) {
                std::cerr << "AI 无法生成合法落子，游戏已安全结束。\n";
                return false;
            }
            std::cout << "电脑落子：" << choice->row + 1 << ' ' << choice->col + 1 << '\n';
        } else {
            if (!getHumanMove(game, move)) return false;
            if (!game.play(move)) {
                // The board is single-threaded; this is a defensive fallback.
                std::cout << "落子失败，请重试。\n";
                continue;
            }
        }
        drawBoard(game.board());
    }

    announceResult(game.result(), pvc);
    return true;
}

int readMenuChoice(const std::string& prompt, int minimum, int maximum) {
    while (true) {
        const ReadResult input = readLine(prompt);
        if (input.status != ReadStatus::Line) return 0;
        int choice = 0;
        if (gomoku::parseMenuInput(input.text, choice) && choice >= minimum && choice <= maximum) return choice;
        std::cout << "选项无效，请输入 " << minimum << " 到 " << maximum << " 之间的单个数字。\n";
    }
}

bool runMode(bool pvc) {
    while (true) {
        if (!playGame(pvc)) return false;
        const int choice = readMenuChoice("本局结束：1 重新开始  2 返回主菜单  3 退出（Esc 退出）：", 1, 3);
        if (choice == 0 || choice == 3) return false;
        if (choice == 2) return true;
    }
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << "欢迎来到五子棋！\n";
    while (true) {
        std::cout << "\n========== 主菜单 ==========\n"
                  << "1：PvP 玩家对战\n"
                  << "2：PvC 人机对战（玩家执黑先手）\n"
                  << "3：退出游戏\n";
        const int choice = readMenuChoice("请选择（按 Esc 退出）：", 1, 3);
        if (choice == 0 || choice == 3) break;
        if (!runMode(choice == 2)) break;
    }
    std::cout << "感谢游玩，再见！\n";
    return 0;
}
