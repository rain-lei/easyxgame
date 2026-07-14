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

struct GridPos
{
    int row = 0;
    int column = 0;

    bool operator==(const GridPos& other) const
    {
        return row == other.row && column == other.column;
    }
};

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

class InputManager
{
public:
    void poll()
    {
        previous_ = current_;
        for (int key = 0; key < static_cast<int>(current_.size()); ++key)
        {
            const SHORT state = GetAsyncKeyState(key);
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

enum class CharacterStyle
{
    Bear,
    Rabbit,
    Cat,
    Dog
};

struct CharacterProfile
{
    int lives = 3;
    int bubbleCapacity = 1;
    int blastRange = 2;
    int shieldCharges = 0;
    float moveSpeed = 148.0f;
    const wchar_t* role = L"全能队员";
};

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

enum class EnemyHitResult
{
    Ignored,
    Damaged,
    Defeated
};

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

enum class TileType
{
    Floor,
    SolidWall,
    Crate
};
