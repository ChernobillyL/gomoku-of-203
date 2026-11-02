# C++17 五子棋

提供 **Win32 原生图形界面**（Windows）和控制台两种入口，共用相同的规则与 AI 核心。游戏为 15×15 棋盘，黑棋先手；横、竖、两条斜线任意方向连成五个或以上棋子获胜。棋盘下满且无人获胜时为平局。

PvC 中玩家执黑先手，电脑执白后手。AI 会优先完成自己的五连或阻挡对手的直接胜点，再按连子和开放端等局面评分选点。无需第三方运行库。

## 图形界面（Windows）

使用 Windows 自带的 Win32 API 和 GDI 绘制，不需要额外安装 UI 库。

### 直接双击启动

构建完成后，项目根目录会生成 **`五子棋.exe`**。以后直接在文件资源管理器中双击这个文件即可打开游戏窗口，不必打开命令行，也不需要另外安装 MinGW 的 C++ 运行库。

首次生成或重新生成程序时，开发者可在项目根目录运行以下构建命令：

```powershell
cmake -S . -B build
cmake --build build --config Release
```

生成后双击项目根目录的 `五子棋.exe` 即可游玩。控制台版仍单独保留为 `gomoku.exe`。

### 操作方式

- 主菜单点击 **双人对战 PvP** 或 **人机对战 PvC** 开始；也可点退出。
- 在棋盘交叉点单击落子，PvP 双方轮流操作；PvC 中电脑在玩家落子后自动回应。
- 右侧显示轮到哪一方、棋子说明和状态；已占用交叉点不能重复落子。
- 对局过程中可点击“重新开始”“返回主菜单”或“退出游戏”。五连结束后可以再来一局或返回菜单。
- 按 **Esc** 从对局返回菜单；在主菜单按 Esc 关闭窗口。点击标题栏关闭按钮也可退出。
- 拖动窗口边缘可以调整大小，棋盘布局会随窗口重新绘制。

## 控制台版

原控制台目标 `gomoku` 保留，支持 Windows 和 POSIX 终端：

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude src/main.cpp src/game.cpp -o gomoku
./gomoku
```

Windows MinGW：

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude src/main.cpp src/game.cpp -o gomoku.exe
.\gomoku.exe
```

输入主菜单数字后按 Enter；落子使用“行 列”格式，如 `8 8`。等待输入时按 Esc 可退出。

## 构建与运行测试

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

核心测试不依赖第三方框架，覆盖落子边界、输入解析、四方向胜负、回合切换、AI 必胜/阻挡与满盘处理。非 Windows 平台不会创建 Win32 GUI 目标，控制台版和核心测试仍可照常构建。
