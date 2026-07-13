# EasyX 随堂练习（Visual Studio）

打开 `EasyXExercises.sln`，选择一个项目并设为启动项目即可运行。解决方案按课程报告章节分为两组：

## 1.1 EasyX 基础

- `Basic01_GobangBoard`：绘制 15×15 五子棋棋盘和九个星位，按任意键退出。
- `Basic02_DynamicClock`：根据系统时间实时绘制时、分、秒针，按 Esc 退出。

## 1.2 EasyX 进阶

- `Advanced01_MouseKeyboardDrawing`：左键画正方形、右键画圆；按住 Ctrl 绘制大图形；R/G/B/W 切换颜色，C 清屏，Esc 退出。
- `Advanced02_BouncingBall`：A/D 或左右方向键移动挡板，P 暂停/继续，Esc 退出；小球落到底边后程序结束。

## 开发环境

- C++17
- Visual Studio / MSVC v143
- EasyX 图形库
- 支持 Win32 与 x64、Debug 与 Release 配置

## 面向对象设计

- `GobangBoard`：封装窗口初始化、棋盘网格、星位绘制和退出等待。
- `DynamicClock` 与 `ClockHand`：时钟负责程序流程和表盘，三个指针对象分别保存自身长度、宽度和颜色。
- `DrawingApp`：封装画布状态和消息处理；`Square`、`Circle` 继承抽象基类 `Shape`，通过虚函数实现多态绘制。
- `BouncingBallGame`：组织游戏循环；`Ball` 封装运动与碰撞，`Paddle` 封装移动、边界限制和绘制。

命令行编译产生的文件统一位于 `build` 目录。提交前应按作业要求删除 `build`、`.vs` 等生成目录。
