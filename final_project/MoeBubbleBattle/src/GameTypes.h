#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <graphics.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

// 集中保存窗口、网格、HUD 和关卡平衡常量，避免在各类中散落“魔法数字”。
namespace GameConfig
{
    constexpr int WindowWidth = 960;
    constexpr int WindowHeight = 720;
    constexpr int MapColumns = 15;
    constexpr int MapRows = 13;
    constexpr int TileSize = 44;
    constexpr int MapLeft = 24;
    constexpr int MapTop = 76;
    constexpr int MapWidth = MapColumns * TileSize;
    constexpr int MapHeight = MapRows * TileSize;
    constexpr int HudLeft = 708;
    constexpr int HudTop = 24;
    constexpr int HudWidth = 228;
    constexpr int HudHeight = 672;
    constexpr int TotalLevels = 3;
    // 课程演示时只需修改这里，就能集中调整第三关首领强度。
    constexpr int BossMaxHealth = 10;
    constexpr int MaxPlayerLives = 4;
    constexpr float Pi = 3.14159265358979323846f;
}

// 全项目共用的低饱和配色表，界面类只引用语义颜色，不重复书写 RGB。
namespace Palette
{
    constexpr COLORREF Paper = RGB(255, 248, 237);
    constexpr COLORREF Ink = RGB(52, 76, 90);
    constexpr COLORREF Aqua = RGB(102, 213, 232);
    constexpr COLORREF AquaDark = RGB(40, 155, 176);
    constexpr COLORREF Coral = RGB(255, 143, 130);
    constexpr COLORREF Mint = RGB(145, 216, 191);
    constexpr COLORREF Honey = RGB(245, 199, 104);
    constexpr COLORREF Danger = RGB(231, 93, 114);
    constexpr COLORREF Shadow = RGB(220, 205, 189);
    constexpr COLORREF White = RGB(255, 253, 248);
    constexpr COLORREF Stone = RGB(142, 170, 184);
    constexpr COLORREF StoneDark = RGB(91, 121, 137);
    constexpr COLORREF Purple = RGB(147, 112, 190);
    constexpr COLORREF PurpleDark = RGB(89, 67, 126);
    constexpr COLORREF GrassA = RGB(239, 246, 224);
    constexpr COLORREF GrassB = RGB(228, 241, 218);
    constexpr COLORREF Crate = RGB(210, 151, 76);
    constexpr COLORREF CrateDark = RGB(139, 91, 52);
}

// 二维世界坐标与速度向量；网格坐标另用 GridPos 表示，防止单位混用。
struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;

    Vec2 operator+(const Vec2& other) const { return { x + other.x, y + other.y }; }
    Vec2 operator-(const Vec2& other) const { return { x - other.x, y - other.y }; }
    Vec2 operator*(float scalar) const { return { x * scalar, y * scalar }; }
    Vec2& operator+=(const Vec2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
};

inline float lengthSquared(const Vec2& value)
{
    return value.x * value.x + value.y * value.y;
}

inline Vec2 normalized(const Vec2& value)
{
    const float length = std::sqrt(lengthSquared(value));
    return length > 0.0001f ? Vec2{ value.x / length, value.y / length } : Vec2{};
}

// 浮点轴对齐碰撞矩形，适合逐帧移动后再做精确边界判断。
struct RectF
{
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

inline bool intersects(const RectF& first, const RectF& second)
{
    return first.left < second.right && first.right > second.left
        && first.top < second.bottom && first.bottom > second.top;
}

// 地图格坐标。row/column 只描述逻辑网格，不直接参与像素绘制。
struct GridPos
{
    int row = 0;
    int column = 0;

    bool operator==(const GridPos& other) const
    {
        return row == other.row && column == other.column;
    }
};

// 逻辑网格与 EasyX 世界像素之间的统一换算入口。
inline Vec2 cellCenter(const GridPos& cell)
{
    return {
        static_cast<float>(GameConfig::MapLeft + cell.column * GameConfig::TileSize + GameConfig::TileSize / 2),
        static_cast<float>(GameConfig::MapTop + cell.row * GameConfig::TileSize + GameConfig::TileSize / 2)
    };
}

inline GridPos worldToCell(const Vec2& position)
{
    return {
        static_cast<int>((position.y - GameConfig::MapTop) / GameConfig::TileSize),
        static_cast<int>((position.x - GameConfig::MapLeft) / GameConfig::TileSize)
    };
}

inline RectF cellBounds(const GridPos& cell, float inset = 0.0f)
{
    const float left = static_cast<float>(GameConfig::MapLeft + cell.column * GameConfig::TileSize) + inset;
    const float top = static_cast<float>(GameConfig::MapTop + cell.row * GameConfig::TileSize) + inset;
    return { left, top, left + GameConfig::TileSize - inset * 2.0f,
        top + GameConfig::TileSize - inset * 2.0f };
}

// 输入状态封装：down 用于连续移动，pressed 用于菜单等“只触发一次”的操作。
// 三个固定长度 array 的下标直接对应 Windows 虚拟键码，访问复杂度为 O(1)。
class InputManager
{
public:
    void poll()
    {
        previous_ = current_;
        for (int key = 0; key < static_cast<int>(current_.size()); ++key)
        {
            const SHORT state = GetAsyncKeyState(key);
            // 高位表示当前按住，低位用于补获两帧轮询之间发生的快速点击。
            current_[key] = (state & 0x8000) != 0;
            pressedSinceLastPoll_[key] = (state & 0x0001) != 0;
        }
    }

    bool down(int key) const
    {
        return key >= 0 && key < static_cast<int>(current_.size()) && current_[key];
    }

    bool pressed(int key) const
    {
        return key >= 0 && key < static_cast<int>(current_.size())
            && ((current_[key] && !previous_[key]) || pressedSinceLastPoll_[key]);
    }

private:
    std::array<bool, 256> current_{};
    std::array<bool, 256> previous_{};
    std::array<bool, 256> pressedSinceLastPoll_{};
};

// 可选角色外观；实际数值由 CharacterProfile 集中配置。
enum class CharacterStyle
{
    Bear,
    Rabbit,
    Cat,
    Dog
};

// 角色初始属性值对象。Player 读取该结构，而不在类内判断具体角色类型。
struct CharacterProfile
{
    int lives = 3;
    int bubbleCapacity = 1;
    int blastRange = 2;
    int shieldCharges = 0;
    float moveSpeed = 148.0f;
    const wchar_t* role = L"全能队员";
};

// 单一配置入口便于课堂演示时平衡生命、速度、容量、威力和护盾。
inline CharacterProfile characterProfile(CharacterStyle style)
{
    switch (style)
    {
    case CharacterStyle::Bear:
        return { 3, 2, 2, 0, 150.0f, L"全能队员" };
    case CharacterStyle::Rabbit:
        return { 3, 1, 2, 0, 178.0f, L"疾风侦察" };
    case CharacterStyle::Cat:
        return { 2, 2, 3, 0, 152.0f, L"爆破专家" };
    case CharacterStyle::Dog:
        return { 4, 1, 2, 1, 138.0f, L"守护先锋" };
    }
    return {};
}

enum class PowerUpType
{
    BlastRange,
    BubbleCapacity,
    Speed,
    Shield
};

// 多态受击结果用于区分普通伤害和最终淘汰，主循环据此计分和播放反馈。
enum class EnemyHitResult
{
    Ignored,
    Damaged,
    Defeated
};

// 游戏场景状态机；输入、更新和绘制均以该枚举进行明确分派。
enum class SceneState
{
    MainMenu,
    CharacterSelect,
    Instructions,
    Playing,
    Paused,
    LevelClear,
    GameOver,
    Victory,
    ExitConfirm
};

// 每个地图格的封装类型：地板可通行，固定墙和木箱会阻挡角色与水浪。
enum class TileType
{
    Floor,
    SolidWall,
    Crate
};
