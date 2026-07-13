# 萌泡大作战（单人版）

《萌泡大作战》是使用 C++17、Visual Studio、EasyX 开发的原创 Q 版单人网格闯关游戏。玩家放置水泡产生十字水浪，破坏木箱、收集强化道具并淘汰 AI 敌人，连续完成三关后获胜。

## 打开和运行

1. 使用 Visual Studio 打开 `MoeBubbleBattle.sln`。
2. 平台选择 `x64`，配置选择 `Debug` 或 `Release`。
3. 将 `MoeBubbleBattle` 设为启动项目。
4. 按 `Ctrl+F5` 运行。

项目使用 MSVC `v143` 工具集，EasyX 需要安装到 Visual Studio。编译后音频、角色原画和透明行走精灵会自动复制到程序输出目录，不需要手动配置绝对路径。

## 操作

| 操作 | 按键 |
| --- | --- |
| 菜单选择 | 方向键或 `W/A/S/D` |
| 确认 | `Enter` 或 `Space` |
| 角色移动 | 方向键或 `W/A/S/D` |
| 放置水泡 | `Space` |
| 暂停/继续 | `P` 或 `Esc` |
| 结果界面重开 | `R` |

## 已实现功能

- 主菜单、角色选择、操作说明、暂停、退出确认。
- 三个单人关卡和逐步增加的 AI 敌人。
- 固定墙、可破坏木箱和安全出生区。
- 水泡容量、2.5 秒倒计时、十字水浪、连锁触发。
- 巡逻型与追踪型两类 AI 敌人。
- 范围、容量、速度、护盾四类随机道具。
- 生命、分数、关卡、敌人数量、累计统计 HUD。
- 关卡完成、挑战失败、最终通关和重新开始。
- 原创菜单音乐、关卡音乐和七个操作/战斗音效。
- 四个可选择的原创 Q 版角色主题，生成的角色原画已实际用于主菜单、角色选择、HUD、失败和通关界面。
- 地图角色使用四角色三帧透明行走精灵，站立与左右脚步交替播放，不再使用圆形占位小人。
- 移动同时支持长按连续行走与轻点步进，并包含四方向优先、全通道转角辅助和脚底碰撞盒。

## 面向对象设计

- `MoeBubbleGame`：游戏循环、场景状态、关卡和对象协作。
- `GameMap`：地图生成、绘制、碰撞和木箱破坏。
- `GameObject`：可更新、可绘制对象的抽象基类。
- `Character`：玩家与敌人的共同父类。
- `Player`：输入、生命、强化能力、水泡数量、三帧精灵动画和脚底碰撞盒。
- `Enemy`：AI 敌人抽象类。
- `PatrolEnemy`、`HunterEnemy`：通过重写决策函数实现运行时多态。
- `WaterBubble`、`WaterWave`、`PowerUp`、`StarParticle`：独立游戏对象。
- `InputManager`：区分按住和单次按下。
- `AudioManager`：背景音乐和音效播放。
- `PortraitAtlas`：加载同一张原创角色设定图并按角色裁切为菜单、卡片和 HUD 三种尺寸。

## 目录

```text
MoeBubbleBattle
├─ assets
│  ├─ audio       原创音乐与音效
│  ├─ concepts    角色设定图
│  ├─ sprites     四角色三帧透明行走精灵
│  └─ ui          UI视觉总览图
├─ src             C++源代码
├─ tools           音频生成脚本
├─ DESIGN.md       游戏设计基线
├─ UI_DESIGN.md    完整UI规范
├─ AUDIO_DESIGN.md 音频规范
├─ TEST_PLAN.md    测试清单
└─ MoeBubbleBattle.sln
```

## 编译验证

以下配置均已成功生成程序，编译日志仅包含 EasyX 头文件自身的非标准匿名联合警告：

- Debug x64
- Release x64
- Debug Win32
- Release Win32

提交前按照课程要求删除 `build`、`.vs` 等生成目录。保留 `assets`，否则程序没有音乐和设计资料。
