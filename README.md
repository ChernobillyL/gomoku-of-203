# C++17 多棋种游戏系统

一个由三个独立游戏模块组合而成的 C++17 课程设计项目：**经典五子棋、三人四子棋、六子棋**。三个模块共用棋盘与对局抽象层，最终组合在同一个 **Win32 原生图形界面**（Windows）和同一个控制台入口中。五子棋和三人四子棋使用 15×15 棋盘；六子棋使用 19×19 棋盘。

## 项目结构（三个子系统 + 公共层）

```text
include/gomoku/
  core/                 公共层：Stone、Move、MoveRecord、Board、GameBase
    board.hpp
    game_base.hpp
    input.hpp           控制台输入解析
  classic/              子系统一：经典五子棋
    game.hpp            Game、AiPlayer
  three_player/         子系统二：三人四子棋
    game.hpp            ThreePlayerGame
    ai.hpp              ThreePlayerAi
  six_player/           子系统三：六子棋
    game.hpp            SixPlayerGame
    ai.hpp              SixPlayerAi

src/
  core/board.cpp        公共棋盘实现与候选点生成
  classic/game.cpp      五子棋规则 + Minimax AI
  three_player/         三人局规则 + Max-N AI
  six_player/           六子棋规则 + Minimax AI
  frontend/console.cpp  组合后的控制台入口
  frontend/win32_ui.cpp 组合后的 Win32 图形界面

tests/
  classic/              子系统一测试
  three_player/         子系统二测试
  six_player/           子系统三测试
```

三名成员可以各自负责一个 `include/gomoku/<模块>` + `src/<模块>` + `tests/<模块>`；公共层、界面和构建脚本由小组共同维护。`GameBase` 定义三种棋统一的 `board()`、`currentPlayer()`、`finished()`、`winner()`、`moveHistory()`、`play()` 接口，三个对局类分别公有继承并实现，界面只依赖抽象基类。

CMake 目标与模块一一对应：`gomoku_common`、`gomoku_classic`、`gomoku_three_player`、`gomoku_six_player`；可执行目标 `gomoku`（控制台）和 `gomoku_gui`（Win32 界面）链接三个模块。

PvC 中玩家执黑先手，电脑执白后手。经典五子棋 AI 使用六层 Minimax 搜索和 Alpha-Beta 剪枝，并依据进攻/防守启发式排序候选点；局面评分通过四个方向的五格窗口识别连五、活四/眠四、活三/眠三等棋型（含跳棋型），优先完成必胜棋并阻挡对手的直接胜点。三人四子棋使用自利式 Max-N，六子棋使用带节点预算的五层 Minimax，详见下文。无需第三方运行库。

## 图形界面（Windows）

使用 Windows 自带的 Win32 API 和 GDI 绘制，不需要额外安装 UI 库。

### 直接双击启动

构建完成后，项目根目录会生成 **`五子棋.exe`**。支持五子棋、三人四子棋及六子棋 PvP/PvC。以后可直接双击启动游戏窗口。

首次生成或重新生成程序时，开发者可在项目根目录运行以下构建命令：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
```

生成后双击项目根目录的 `五子棋.exe` 即可游玩。控制台版仍单独保留为 `gomoku.exe`。

### 操作方式

- 开始界面分两步：先选择 **五子棋 / 三人四子棋 / 六子棋**，再选择 **PvP 玩家对战 / PvC 人机对战**；模式页可返回重新选择棋种。
- 在棋盘交叉点单击落子，PvP 双方轮流操作；PvC 中电脑在玩家落子后自动回应。
- 三人四子棋 PvP 由三名玩家共用鼠标，按黑 → 白 → 红顺序轮流落子；启用同屏活三限制。PvC 中玩家执黑，电脑1（白棋）与电脑2（红棋）各自为战并依次回应。
- 六子棋 PvP 双方轮流操作；黑方开局只下一子，此后双方每回合连续下两子。PvC 中玩家执黑，电脑执白；双方遵守相同的单子开局/双子回合规则。每次落子后若连成六子及以上立即获胜。
- 右侧显示当前回合、状态和落子记录；棋谱格式为“手数 棋色 坐标”，例如 `01 黑 H8`。电脑落子会标注“电脑”；鼠标在记录框内滚轮可浏览较早棋步。三人局的记录使用“黑 / 白 / 红”区分棋色。
- 已占用交叉点不能重复落子。
- 点击“悔棋”可撤回操作：PvP 和三人同屏撤回最近一颗棋子；PvC 和三人人机撤回玩家与所有电脑最近这一整轮。对局结束后也可以悔棋；棋谱会随悔棋同步回退，新局开始后记录清空。
- 对局过程中可点击“重新开始”“返回主菜单”或“退出游戏”。对局结束后可以再来一局或返回菜单。
- 按 **Esc**：对局中返回游戏选择页；PvP/PvC 模式页返回棋种选择页；棋种选择页按 Esc 关闭窗口。标题栏关闭按钮也可退出。
- 拖动窗口边缘可以调整大小，棋盘布局会随窗口重新绘制。

## 控制台版

原控制台目标 `gomoku` 保留，支持 Windows 和 POSIX 终端：

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude \
    src/core/board.cpp \
    src/classic/game.cpp \
    src/three_player/game.cpp src/three_player/ai.cpp \
    src/six_player/game.cpp src/six_player/ai.cpp \
    src/frontend/console.cpp -o gomoku
./gomoku
```

Windows MinGW：

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude src/core/board.cpp src/classic/game.cpp src/three_player/game.cpp src/three_player/ai.cpp src/six_player/game.cpp src/six_player/ai.cpp src/frontend/console.cpp -o gomoku.exe
.\gomoku.exe
```

输入主菜单数字后按 Enter：`1/2` 为五子棋 PvP/PvC，`3/4` 为三人四子棋 PvP/PvC，`5/6` 为六子棋 PvP/PvC，`7` 退出。落子使用“行 列”格式，如 `8 8`。三人四子棋 PvP 执行同屏活三限制，PvC 不启用；六子棋采用黑方首回合一子、此后每方每回合两子的规则。等待输入时按 Esc 可退出。

## 构建与运行测试

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
ctest --test-dir build --output-on-failure
```

三个测试目标分别链接自己的游戏模块，不依赖另外两个模块：

- `gomoku_classic_tests` → `classic_tests`：棋盘落子、四方向五连、输入解析、回合/历史、胜后禁止落子、AI 必胜/必防/确定性。
- `gomoku_three_player_tests` → `three_player_tests`：三人轮换、四方向四连、满盘平局、同屏活三规则、AI 必胜/阻挡/轮转。
- `gomoku_six_player_tests` → `six_player_tests`：单子开局、双子回合、四方向六连、即时终局、AI 必胜/阻挡。

测试不依赖第三方框架。非 Windows 平台不会创建 Win32 GUI 目标，控制台版和三个模块测试仍可照常构建。

### 图形界面输出目录

GUI 默认输出到项目根目录的 `五子棋.exe`。如果该文件正在运行、需要保留旧版本，或只想做构建验证，可以指定其他输出目录：

```powershell
cmake -S . -B build-check -G Ninja -DCMAKE_BUILD_TYPE=Release -DGOMOKU_GUI_OUTPUT_DIR="$PWD/build-check/out"
cmake --build build-check --target gomoku_gui
```

## 三人四子棋

三人四子棋提供两种玩法，均为 15×15 棋盘，任意方向连成四个或以上棋子获胜，满盘无胜者为平局：

- **PvP**：三名玩家轮流执黑、白、红棋。
- **PvC**：玩家执黑，电脑1执白、电脑2执红。两台电脑各自为战，只追求自己获胜，会阻挡任何一方的直接胜点，不会互相配合。

### 三人四子棋 PvP 附加规则

- 规则检测把“他人有三”定义为：至少存在一个四格线段，其中有三枚同色棋子、一格空位且没有第三方棋子；线段可以包含跳位，也不要求成四点唯一。
- 禁手检查把“新活三”定义为：当前落子后形成连续三子，且两端均为空。若已有其他颜色的上述三棋型，本手禁止形成这种新活三；违反时落子无效。
- 如果这手直接连成四子获胜，则不受此限制。
- 该规则只在三人四子棋 PvP 模式启用；PvC 模式不使用。

### 三人四子棋评分与 AI

四子棋评估器枚举横、竖、两条斜线上的四格窗口；窗口含对手棋子则记零，否则按己方棋子数和窗口紧邻两端的空位数打分。实际评分表为：四子 `1,000,000,000`；三子两端空 `80,000`、单端空 `40,000`、无空端 `20,000`；二子分别为 `1,500 / 250 / 25`；单子分别为 `30 / 5 / 1`；空窗口为 `0`。同屏规则的代码定义是：其他颜色存在“任一无对手棋子的四格窗口含三枚己子和一空位”时，当前落子不能新造成连续且两端为空的三子；直接成四获胜不受限。这是项目自定义规则，代码并没有完整判定所有棋论定义下的死三/活三。三人 AI 按窗口分数排序候选并限制分支，使用自利式 **Max-N**：每层轮到的颜色最大化自身收益向量分量，搜索深度 4、每层最多 5 个候选，并单独检查直接胜点和轮到下一家的威胁。三人是多方非零和博弈，因此它不是传统双人 Minimax/Alpha-Beta；评分和搜索均为启发式。

## 六子棋

六子棋使用 19×19 棋盘，横、竖、两条斜线任意方向连成六子或以上获胜。

- **PvP**：黑方开局先下一子；之后黑白双方每回合各连续落两子。第一子落下后仍由同一方落第二子；若第一子已形成六连则立即结束。
- **PvC**：玩家执黑先手、电脑执白，开局和后续每回合落子数量规则相同。
- AI 使用五子棋相同的双人 Minimax + Alpha-Beta 结构，对六格窗口中的连子数、开放端进行启发式评估，并按局部攻击/防守价值排序、限制候选分支；搜索深度为 5 个单子层，搜索状态会继续模拟当前方本回合剩余的第二子。
