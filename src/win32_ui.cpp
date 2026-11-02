#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>

#include "gomoku/game.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>

namespace {

constexpr wchar_t kWindowClass[] = L"GomokuNativeWindow";
constexpr wchar_t kWindowTitle[] = L"五子棋 · Gomoku";
constexpr COLORREF kBackground{RGB(17, 24, 39)};
constexpr COLORREF kPanel{RGB(29, 39, 57)};
constexpr COLORREF kPanelLight{RGB(38, 51, 72)};
constexpr COLORREF kText{RGB(239, 244, 250)};
constexpr COLORREF kMuted{RGB(157, 173, 194)};
constexpr COLORREF kAccent{RGB(82, 211, 177)};
constexpr COLORREF kAccentDark{RGB(35, 151, 123)};
constexpr COLORREF kBoardWood{RGB(223, 179, 111)};
constexpr COLORREF kGrid{RGB(94, 68, 43)};

using gomoku::Game;
using gomoku::Move;
using gomoku::Result;
using gomoku::Stone;

struct Rect {
    int left;
    int top;
    int right;
    int bottom;

    int width() const { return right - left; }
    int height() const { return bottom - top; }
    bool contains(POINT point) const {
        return point.x >= left && point.x < right && point.y >= top && point.y < bottom;
    }
};

enum class Screen { Menu, Playing, Finished };
enum class ButtonAction { None, PvP, PvC, Restart, MainMenu, Exit };

struct Layout {
    Rect boardCard{};
    Rect boardGrid{};
    Rect sidePanel{};
    std::array<Rect, 3> buttons{};
    std::array<Rect, 2> menuButtons{};
    Rect menuExit{};
    UINT dpi = 96;
};

struct AppState {
    Screen screen = Screen::Menu;
    bool pvc = false;
    UINT dpi = 96;
    Game game;
    gomoku::AiPlayer ai;
    std::optional<Move> lastMove;
    std::wstring status = L"选择一种模式，开始一局五子棋。";
    ButtonAction hovered = ButtonAction::None;
};

int px(int logicalSize, UINT dpi) {
    return MulDiv(logicalSize, static_cast<int>(dpi), 96);
}

Rect makeRect(int left, int top, int width, int height) {
    return {left, top, left + width, top + height};
}

Rect scaledRect(int left, int top, int width, int height, UINT dpi) {
    return makeRect(px(left, dpi), px(top, dpi), px(width, dpi), px(height, dpi));
}

int gridPixel(int origin, int index, int extent) {
    return origin + MulDiv(index, extent, gomoku::kBoardSize - 1);
}

void fillRect(HDC dc, Rect rect, COLORREF color) {
    const RECT nativeRect{rect.left, rect.top, rect.right, rect.bottom};
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &nativeRect, brush);
    DeleteObject(brush);
}

void drawRoundRect(HDC dc, Rect rect, COLORREF fill, COLORREF outline, int radius, UINT dpi) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, px(1, dpi)), outline);
    const HGDIOBJ oldBrush = SelectObject(dc, brush);
    const HGDIOBJ oldPen = SelectObject(dc, pen);
    const int scaledRadius = px(radius, dpi);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, scaledRadius, scaledRadius);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void drawText(HDC dc, const std::wstring& text, Rect rect, int logicalSize, COLORREF color,
              UINT dpi, UINT alignment = DT_LEFT, int weight = FW_NORMAL) {
    const int fontHeight = std::max(1, px(logicalSize, dpi));
    HFONT font = CreateFontW(-fontHeight, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    if (font == nullptr) return;
    const HGDIOBJ oldFont = SelectObject(dc, font);
    const int oldMode = SetBkMode(dc, TRANSPARENT);
    const COLORREF oldColor = SetTextColor(dc, color);
    const RECT nativeRect{rect.left, rect.top, rect.right, rect.bottom};
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), const_cast<RECT*>(&nativeRect),
              alignment | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SetTextColor(dc, oldColor);
    SetBkMode(dc, oldMode);
    SelectObject(dc, oldFont);
    DeleteObject(font);
}

void drawCenteredText(HDC dc, const std::wstring& text, Rect rect, int size, COLORREF color,
                      UINT dpi, int weight = FW_NORMAL) {
    drawText(dc, text, rect, size, color, dpi, DT_CENTER, weight);
}

void drawButton(HDC dc, Rect rect, const std::wstring& label, ButtonAction action,
                ButtonAction hovered, UINT dpi, bool primary = false, bool danger = false) {
    const bool isHovered = action == hovered;
    COLORREF fill = primary ? kAccentDark : kPanelLight;
    COLORREF outline = primary ? kAccent : RGB(61, 77, 101);
    if (danger) {
        fill = isHovered ? RGB(151, 61, 67) : RGB(116, 50, 59);
        outline = RGB(196, 87, 91);
    } else if (primary && isHovered) {
        fill = RGB(42, 174, 143);
    } else if (isHovered) {
        fill = RGB(52, 70, 97);
        outline = kAccent;
    }
    drawRoundRect(dc, rect, fill, outline, 14, dpi);
    drawCenteredText(dc, label, rect, 19, kText, dpi, FW_SEMIBOLD);
}

Layout calculateLayout(int width, int height, UINT dpi) {
    Layout layout;
    layout.dpi = dpi;
    const int buttonWidth = std::min(px(330, dpi), std::max(px(240, dpi), width / 3));
    const int buttonHeight = px(62, dpi);
    const int centerX = width / 2;
    layout.menuButtons[0] = makeRect(centerX - buttonWidth / 2, height / 2 - px(2, dpi), buttonWidth, buttonHeight);
    layout.menuButtons[1] = makeRect(centerX - buttonWidth / 2, height / 2 + px(78, dpi), buttonWidth, buttonHeight);
    layout.menuExit = makeRect(centerX - buttonWidth / 2, height / 2 + px(156, dpi), buttonWidth, buttonHeight);

    const int margin = std::max(px(22, dpi), std::min(width, height) / 30);
    const int headerHeight = px(104, dpi);
    const int availableHeight = std::max(px(300, dpi), height - px(150, dpi));
    const int availableWidth = std::max(px(300, dpi), width - px(390, dpi));
    const int boardSize = std::max(px(300, dpi), std::min(availableHeight, availableWidth));
    layout.boardCard = makeRect(margin, headerHeight, boardSize, boardSize);
    const int labelMargin = std::max(px(36, dpi), boardSize / 16);
    layout.boardGrid = {layout.boardCard.left + labelMargin,
                        layout.boardCard.top + labelMargin,
                        layout.boardCard.right - labelMargin,
                        layout.boardCard.bottom - labelMargin};

    const int panelX = layout.boardCard.right + margin;
    const int panelWidth = std::max(px(260, dpi), width - panelX - margin);
    layout.sidePanel = makeRect(panelX, headerHeight, panelWidth, boardSize);
    const int sideButtonHeight = std::max(px(42, dpi), std::min(px(50, dpi), layout.sidePanel.height() / 10));
    const int gap = px(8, dpi);
    const int buttonY = layout.sidePanel.bottom - sideButtonHeight * 3 - gap * 2 - px(24, dpi);
    for (int i = 0; i < 3; ++i) {
        layout.buttons[static_cast<std::size_t>(i)] = makeRect(
            panelX + px(18, dpi), buttonY + i * (sideButtonHeight + gap), panelWidth - px(36, dpi), sideButtonHeight);
    }
    return layout;
}

void drawBackground(HDC dc, int width, int height, UINT dpi) {
    fillRect(dc, makeRect(0, 0, width, height), kBackground);
    fillRect(dc, makeRect(0, 0, width, px(5, dpi)), kAccentDark);
}

void drawMenu(HDC dc, int width, int height, const AppState& app, const Layout& layout) {
    const UINT dpi = layout.dpi;
    drawBackground(dc, width, height, dpi);
    // Center menu headings relative to the window in physical pixels.
    drawCenteredText(dc, L"GOMOKU", makeRect(0, height / 2 - px(220, dpi), width, px(58, dpi)),
                     42, kText, dpi, FW_BOLD);
    drawCenteredText(dc, L"五子棋", makeRect(0, height / 2 - px(160, dpi), width, px(42, dpi)),
                     25, kAccent, dpi, FW_SEMIBOLD);
    drawCenteredText(dc, L"经典棋局 · 与朋友或电脑对弈",
                     makeRect(0, height / 2 - px(112, dpi), width, px(36, dpi)), 16, kMuted, dpi);

    drawRoundRect(dc, makeRect(width / 2 - px(260, dpi), height / 2 - px(24, dpi), px(520, dpi), px(330, dpi)),
                  kPanel, RGB(50, 66, 91), 22, dpi);
    drawButton(dc, layout.menuButtons[0], L"双人对战  ·  PvP", ButtonAction::PvP, app.hovered, dpi, true);
    drawButton(dc, layout.menuButtons[1], L"人机对战  ·  PvC", ButtonAction::PvC, app.hovered, dpi);
    drawButton(dc, layout.menuExit, L"退出游戏", ButtonAction::Exit, app.hovered, dpi, false, true);
    drawCenteredText(dc, L"15 × 15 棋盘    黑棋先手    五子连线获胜",
                     makeRect(0, height - px(52, dpi), width, px(26, dpi)), 14, kMuted, dpi);
}

std::wstring playerText(const AppState& app) {
    if (app.screen == Screen::Finished) {
        switch (app.game.result()) {
            case Result::BlackWins: return app.pvc ? L"你赢了！" : L"黑棋获胜！";
            case Result::WhiteWins: return app.pvc ? L"电脑获胜" : L"白棋获胜！";
            case Result::Draw: return L"平局，再来一局？";
            case Result::Ongoing: break;
        }
    }
    if (app.pvc) return app.game.currentPlayer() == Stone::Black ? L"轮到你（黑棋）" : L"电脑思考中（白棋）";
    return app.game.currentPlayer() == Stone::Black ? L"轮到黑棋" : L"轮到白棋";
}

void drawHeader(HDC dc, int width, const AppState& app) {
    const UINT dpi = app.dpi;
    drawText(dc, L"五子棋", scaledRect(28, 22, 180, 48, dpi), 27, kText, dpi, DT_LEFT, FW_BOLD);
    const std::wstring modeTitle = app.pvc ? L"人机对战  /  PvC" : L"双人对战  /  PvP";
    drawText(dc, modeTitle, scaledRect(210, 24, 250, 44, dpi), 16, kMuted, dpi);
    drawText(dc, L"ESC  返回菜单", makeRect(width - px(210, dpi), px(24, dpi), px(182, dpi), px(42, dpi)),
             14, kMuted, dpi, DT_RIGHT);
}

void drawStone(HDC dc, int x, int y, int radius, Stone stone, bool isLast, UINT dpi) {
    if (isLast) {
        HBRUSH markerBrush = CreateSolidBrush(kAccentDark);
        HPEN markerPen = CreatePen(PS_SOLID, std::max(1, px(1, dpi)), kAccentDark);
        const HGDIOBJ oldBrush = SelectObject(dc, markerBrush);
        const HGDIOBJ oldPen = SelectObject(dc, markerPen);
        const int markerRadius = radius + px(5, dpi);
        Ellipse(dc, x - markerRadius, y - markerRadius, x + markerRadius + 1, y + markerRadius + 1);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
        DeleteObject(markerPen);
        DeleteObject(markerBrush);
    }

    const COLORREF stoneColor = stone == Stone::Black ? RGB(25, 31, 41) : RGB(247, 247, 239);
    const COLORREF outlineColor = stone == Stone::Black ? RGB(8, 12, 18) : RGB(185, 186, 177);
    HBRUSH brush = CreateSolidBrush(stoneColor);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, px(1, dpi)), outlineColor);
    const HGDIOBJ oldBrush = SelectObject(dc, brush);
    const HGDIOBJ oldPen = SelectObject(dc, pen);
    Ellipse(dc, x - radius, y - radius, x + radius + 1, y + radius + 1);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void drawBoard(HDC dc, const AppState& app, const Layout& layout) {
    const UINT dpi = layout.dpi;
    drawRoundRect(dc, layout.boardCard, kBoardWood, RGB(160, 119, 70), 18, dpi);
    const int spacing = layout.boardGrid.width() / (gomoku::kBoardSize - 1);
    const int radius = std::max(px(4, dpi), static_cast<int>(spacing * 0.38));
    const int left = layout.boardGrid.left;
    const int top = layout.boardGrid.top;
    const int right = layout.boardGrid.right;
    const int bottom = layout.boardGrid.bottom;

    HPEN gridPen = CreatePen(PS_SOLID, std::max(1, px(1, dpi)), kGrid);
    const HGDIOBJ oldPen = SelectObject(dc, gridPen);
    for (int i = 0; i < gomoku::kBoardSize; ++i) {
        const int x = gridPixel(left, i, layout.boardGrid.width());
        const int y = gridPixel(top, i, layout.boardGrid.height());
        MoveToEx(dc, left, y, nullptr);
        LineTo(dc, right + 1, y);
        MoveToEx(dc, x, top, nullptr);
        LineTo(dc, x, bottom + 1);

        const wchar_t columnLabel[] = {static_cast<wchar_t>(L'A' + i), L'\0'};
        const Rect colRect{x - px(12, dpi), layout.boardCard.top + px(8, dpi),
                           x + px(12, dpi), top - px(2, dpi)};
        drawCenteredText(dc, columnLabel, colRect, 11, RGB(109, 78, 47), dpi, FW_SEMIBOLD);
        const std::wstring rowLabel = std::to_wstring(i + 1);
        const Rect rowRect{layout.boardCard.left + px(4, dpi), y - px(9, dpi),
                           left - px(5, dpi), y + px(10, dpi)};
        drawCenteredText(dc, rowLabel, rowRect, 11, RGB(109, 78, 47), dpi, FW_SEMIBOLD);
    }
    SelectObject(dc, oldPen);
    DeleteObject(gridPen);

    const std::array<int, 3> starPoints{{3, 7, 11}};
    HBRUSH starBrush = CreateSolidBrush(kGrid);
    const HGDIOBJ oldBrush = SelectObject(dc, starBrush);
    const int starRadius = std::max(2, px(3, dpi));
    for (const int row : starPoints) {
        for (const int col : starPoints) {
            const int x = gridPixel(left, col, layout.boardGrid.width());
            const int y = gridPixel(top, row, layout.boardGrid.height());
            Ellipse(dc, x - starRadius, y - starRadius, x + starRadius + 1, y + starRadius + 1);
        }
    }
    SelectObject(dc, oldBrush);
    DeleteObject(starBrush);

    for (int row = 0; row < gomoku::kBoardSize; ++row) {
        for (int col = 0; col < gomoku::kBoardSize; ++col) {
            const Move move{row, col};
            const Stone stone = app.game.board().at(move);
            if (stone == Stone::Empty) continue;
            const int x = gridPixel(left, col, layout.boardGrid.width());
            const int y = gridPixel(top, row, layout.boardGrid.height());
            drawStone(dc, x, y, radius, stone, app.lastMove && *app.lastMove == move, dpi);
        }
    }
}

void drawSidePanel(HDC dc, const AppState& app, const Layout& layout) {
    const UINT dpi = layout.dpi;
    const int left = layout.sidePanel.left;
    const int top = layout.sidePanel.top;
    const int width = layout.sidePanel.width();
    drawRoundRect(dc, layout.sidePanel, kPanel, RGB(48, 64, 87), 20, dpi);
    drawText(dc, L"当前对局", scaledRect(left + 22, top + 18, width - 44, 28, dpi),
             15, kMuted, dpi, DT_LEFT, FW_SEMIBOLD);
    drawText(dc, playerText(app), scaledRect(left + 22, top + 54, width - 44, 48, dpi),
             23, app.screen == Screen::Finished ? kAccent : kText, dpi, DT_LEFT, FW_BOLD);

    drawRoundRect(dc, scaledRect(left + 18, top + 120, width - 36, 120, dpi),
                  kPanelLight, RGB(61, 77, 101), 14, dpi);
    drawText(dc, L"棋子说明", scaledRect(left + 34, top + 134, width - 68, 22, dpi),
             13, kMuted, dpi, DT_LEFT, FW_SEMIBOLD);
    drawText(dc, L"●  黑棋 · 先手", scaledRect(left + 34, top + 162, width - 68, 28, dpi), 16, kText, dpi);
    drawText(dc, L"○  白棋 · 后手", scaledRect(left + 34, top + 196, width - 68, 28, dpi), 16, kText, dpi);

    const std::wstring modeHint = app.pvc ? L"点击棋盘交叉点落子，电脑自动应对。" : L"点击空交叉点轮流落子。";
    drawText(dc, modeHint, scaledRect(left + 22, top + 258, width - 44, 26, dpi), 13, kMuted, dpi);
    drawText(dc, app.status, scaledRect(left + 22, top + 286, width - 44, 32, dpi), 12, kAccent, dpi);
    drawButton(dc, layout.buttons[0], app.screen == Screen::Finished ? L"再来一局" : L"重新开始",
               ButtonAction::Restart, app.hovered, dpi, true);
    drawButton(dc, layout.buttons[1], L"返回主菜单", ButtonAction::MainMenu, app.hovered, dpi);
    drawButton(dc, layout.buttons[2], L"退出游戏", ButtonAction::Exit, app.hovered, dpi, false, true);
}

void drawGame(HDC dc, int width, int height, const AppState& app, const Layout& layout) {
    drawBackground(dc, width, height, layout.dpi);
    drawHeader(dc, width, app);
    drawBoard(dc, app, layout);
    drawSidePanel(dc, app, layout);
}

void paintWindow(HWND window, AppState& app) {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(window, &paint);
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    const Layout layout = calculateLayout(width, height, app.dpi);

    // Paint directly to the window DC so ClearType text is not rasterized into
    // an intermediate bitmap and then scaled by BitBlt or Windows DPI virtualization.
    if (app.screen == Screen::Menu) drawMenu(dc, width, height, app, layout);
    else drawGame(dc, width, height, app, layout);
    EndPaint(window, &paint);
}

void startGame(AppState& app, bool pvc) {
    app.screen = Screen::Playing;
    app.pvc = pvc;
    app.game = Game{};
    app.lastMove.reset();
    app.status = pvc ? L"你执黑棋先手。" : L"黑棋先手，请先落子。";
}

ButtonAction hitTest(const POINT point, const AppState& app, const Layout& layout) {
    if (app.screen == Screen::Menu) {
        if (layout.menuButtons[0].contains(point)) return ButtonAction::PvP;
        if (layout.menuButtons[1].contains(point)) return ButtonAction::PvC;
        if (layout.menuExit.contains(point)) return ButtonAction::Exit;
        return ButtonAction::None;
    }
    for (std::size_t i = 0; i < layout.buttons.size(); ++i) {
        if (layout.buttons[i].contains(point)) {
            return i == 0 ? ButtonAction::Restart : i == 1 ? ButtonAction::MainMenu : ButtonAction::Exit;
        }
    }
    return ButtonAction::None;
}

int nearestGridIndex(int coordinate, int origin, int extent) {
    int bestIndex = 0;
    int bestDistance = (std::numeric_limits<int>::max)();
    for (int index = 0; index < gomoku::kBoardSize; ++index) {
        const int distance = std::abs(coordinate - gridPixel(origin, index, extent));
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = index;
        }
    }
    return bestDistance <= extent / (gomoku::kBoardSize - 1) / 2 ? bestIndex : -1;
}

bool placeHumanMove(HWND window, AppState& app, const Layout& layout, POINT point) {
    if (app.screen != Screen::Playing || (app.pvc && app.game.currentPlayer() != Stone::Black)) return false;
    const int col = nearestGridIndex(point.x, layout.boardGrid.left, layout.boardGrid.width());
    const int row = nearestGridIndex(point.y, layout.boardGrid.top, layout.boardGrid.height());
    if (row < 0 || col < 0) return false;
    const Move move{row, col};
    if (!app.game.board().isEmpty(move)) {
        app.status = L"这个交叉点已有棋子，请选择空位。";
        InvalidateRect(window, nullptr, FALSE);
        return true;
    }
    if (!app.game.play(move)) return true;
    app.lastMove = move;
    app.status = L"已落子。";
    if (app.game.result() != Result::Ongoing) {
        app.screen = Screen::Finished;
    } else if (app.pvc) {
        const auto aiMove = app.ai.chooseMove(app.game.board(), Stone::White);
        if (aiMove && app.game.play(*aiMove)) {
            app.lastMove = *aiMove;
            app.status = L"电脑已落子。轮到你了。";
        } else {
            app.status = L"电脑没有可用落点。";
        }
        if (app.game.result() != Result::Ongoing) app.screen = Screen::Finished;
    } else {
        app.status = L"落子成功。";
    }
    InvalidateRect(window, nullptr, FALSE);
    return true;
}

void performAction(HWND window, AppState& app, ButtonAction action) {
    switch (action) {
        case ButtonAction::PvP:
            startGame(app, false);
            break;
        case ButtonAction::PvC:
            startGame(app, true);
            break;
        case ButtonAction::Restart:
            startGame(app, app.pvc);
            break;
        case ButtonAction::MainMenu:
            app.screen = Screen::Menu;
            app.status = L"选择一种模式，开始一局五子棋。";
            break;
        case ButtonAction::Exit:
            DestroyWindow(window);
            return;
        case ButtonAction::None:
            return;
    }
    InvalidateRect(window, nullptr, FALSE);
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    AppState* app = reinterpret_cast<AppState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* createInfo = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        app = static_cast<AppState*>(createInfo->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (app == nullptr) return DefWindowProcW(window, message, wParam, lParam);

    switch (message) {
        case WM_PAINT:
            paintWindow(window, *app);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_GETMINMAXINFO: {
            auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
            limits->ptMinTrackSize.x = MulDiv(900, static_cast<int>(app->dpi), 96);
            limits->ptMinTrackSize.y = MulDiv(650, static_cast<int>(app->dpi), 96);
            return 0;
        }
        case WM_DPICHANGED: {
            app->dpi = HIWORD(wParam);
            const auto* suggested = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(window, nullptr, suggested->left, suggested->top,
                         suggested->right - suggested->left, suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        }
        case WM_MOUSEMOVE: {
            RECT client{};
            GetClientRect(window, &client);
            const Layout layout = calculateLayout(client.right - client.left, client.bottom - client.top, app->dpi);
            const POINT point{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
            const ButtonAction hovered = hitTest(point, *app, layout);
            if (hovered != app->hovered) {
                app->hovered = hovered;
                InvalidateRect(window, nullptr, FALSE);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            RECT client{};
            GetClientRect(window, &client);
            const Layout layout = calculateLayout(client.right - client.left, client.bottom - client.top, app->dpi);
            const POINT point{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
            const ButtonAction action = hitTest(point, *app, layout);
            if (action != ButtonAction::None) {
                performAction(window, *app, action);
            } else if (app->screen == Screen::Playing) {
                (void)placeHumanMove(window, *app, layout, point);
            }
            return 0;
        }
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                if (app->screen == Screen::Menu) DestroyWindow(window);
                else {
                    app->screen = Screen::Menu;
                    app->status = L"已返回主菜单。选择模式开始新局。";
                    InvalidateRect(window, nullptr, FALSE);
                }
                return 0;
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window, message, wParam, lParam);
    }
}

}  // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const WNDCLASSEXW windowClass{
        sizeof(WNDCLASSEXW), CS_HREDRAW | CS_VREDRAW, windowProcedure, 0, 0, instance,
        LoadIconW(nullptr, MAKEINTRESOURCEW(32512)), LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)),
        CreateSolidBrush(kBackground), nullptr, kWindowClass, LoadIconW(nullptr, MAKEINTRESOURCEW(32512))};
    if (RegisterClassExW(&windowClass) == 0) return 1;

    const UINT dpi = GetDpiForSystem();
    RECT windowRect{0, 0, MulDiv(1220, static_cast<int>(dpi), 96),
                    MulDiv(850, static_cast<int>(dpi), 96)};
    AdjustWindowRectExForDpi(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
    AppState app;
    app.dpi = dpi;
    HWND window = CreateWindowExW(0, kWindowClass, kWindowTitle, WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  windowRect.right - windowRect.left,
                                  windowRect.bottom - windowRect.top,
                                  nullptr, nullptr, instance, &app);
    if (window == nullptr) return 1;
    app.dpi = GetDpiForWindow(window);

    ShowWindow(window, showCommand);
    UpdateWindow(window);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
