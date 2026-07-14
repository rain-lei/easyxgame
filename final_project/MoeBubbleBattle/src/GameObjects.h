#pragma once

#include "GameTypes.h"

#include <filesystem>

class WaterBubble;

class GameMap
{
public:
    GameMap();

    void generate(int level);
    void draw() const;
    bool inside(const GridPos& cell) const;
    TileType tileAt(const GridPos& cell) const;
    bool isWalkable(const GridPos& cell) const;
    bool isRectWalkable(const RectF& bounds) const;
    bool destroyCrate(const GridPos& cell);
    int crateCount() const;

private:
    std::array<TileType, GameConfig::MapRows * GameConfig::MapColumns> tiles_{};

    int indexOf(const GridPos& cell) const;
    void setTile(const GridPos& cell, TileType tile);
    bool isReservedSpawnCell(const GridPos& cell) const;
};

class GameObject
{
public:
    virtual ~GameObject() = default;
    virtual void update(float deltaTime) = 0;
    virtual void draw() const = 0;
    virtual RectF bounds() const = 0;

    bool active() const { return active_; }
    void deactivate() { active_ = false; }

protected:
    bool active_ = true;
};

class Character : public GameObject
{
public:
    Character(int id, Vec2 position, float radius, float speed);

    RectF bounds() const override;
    int id() const { return id_; }
    const Vec2& position() const { return position_; }
    GridPos cell() const { return worldToCell(position_); }

protected:
    int id_ = 0;
    Vec2 position_{};
    float radius_ = 14.0f;
    float speed_ = 150.0f;

    bool move(const Vec2& direction, float deltaTime, const GameMap& map,
        const std::vector<WaterBubble>& bubbles);
    bool canOccupy(const RectF& candidate, const GameMap& map,
        const std::vector<WaterBubble>& bubbles) const;
};

class Player final : public Character
{
public:
    Player();

    void update(float deltaTime) override;
    void draw() const override;
    bool loadSpriteSheet(const std::filesystem::path& imagePath);
    void handleMovement(const InputManager& input, float deltaTime,
        const GameMap& map, const std::vector<WaterBubble>& bubbles);
    void resetForNewGame(CharacterStyle style);
    void prepareForLevel(const GridPos& spawnCell);

    bool takeDamage();
    void applyPowerUp(PowerUpType type);
    bool canPlaceBubble() const;
    void onBubblePlaced();
    void onBubbleExpired();

    CharacterStyle style() const { return style_; }
    int lives() const { return lives_; }
    int bubbleCapacity() const { return bubbleCapacity_; }
    int activeBubbles() const { return activeBubbles_; }
    int blastRange() const { return blastRange_; }
    int speedLevel() const { return speedLevel_; }
    int movementSpeed() const { return static_cast<int>(std::lround(speed_)); }
    int shieldCharges() const { return shieldCharges_; }
    bool invulnerable() const { return invulnerableTimer_ > 0.0f; }

private:
    static constexpr int SpriteColumns = 3;
    static constexpr int SpriteRows = 4;
    static constexpr int SpriteCellSize = 72;

    CharacterStyle style_ = CharacterStyle::Bear;
    Vec2 facing_{ 0.0f, 1.0f };
    bool walking_ = false;
    IMAGE spriteSheet_;
    bool spriteSheetLoaded_ = false;
    int lives_ = 3;
    int bubbleCapacity_ = 1;
    int activeBubbles_ = 0;
    int blastRange_ = 2;
    int speedLevel_ = 0;
    int shieldCharges_ = 0;
    float baseSpeed_ = 148.0f;
    float invulnerableTimer_ = 0.0f;
    float animationTime_ = 0.0f;
    float walkAnimationTime_ = 0.0f;

    void drawSpriteFrame() const;
};

class Enemy : public Character
{
public:
    Enemy(int id, Vec2 position, float speed);

    void update(float deltaTime) override;
    void updateAI(float deltaTime, const GameMap& map,
        const std::vector<WaterBubble>& bubbles,
        const std::vector<GridPos>& dangerousCells,
        const Vec2& playerPosition,
        std::mt19937& randomEngine);
    virtual EnemyHitResult takeWaveHit();
    virtual int scoreValue() const { return 300; }
    virtual bool isBoss() const { return false; }
    virtual int health() const { return active_ ? 1 : 0; }
    virtual int maxHealth() const { return 1; }

protected:
    Vec2 direction_{ 0.0f, 1.0f };
    float decisionTimer_ = 0.0f;
    float animationTime_ = 0.0f;

    virtual Vec2 chooseDirection(const GameMap& map,
        const std::vector<WaterBubble>& bubbles,
        const std::vector<GridPos>& dangerousCells,
        const Vec2& playerPosition,
        std::mt19937& randomEngine) = 0;
    std::vector<Vec2> availableDirections(const GameMap& map,
        const std::vector<WaterBubble>& bubbles) const;
    bool isDangerous(const GridPos& cell,
        const std::vector<GridPos>& dangerousCells) const;
};

class PatrolEnemy final : public Enemy
{
public:
    PatrolEnemy(int id, Vec2 position, float speed);
    void draw() const override;

protected:
    Vec2 chooseDirection(const GameMap& map,
        const std::vector<WaterBubble>& bubbles,
        const std::vector<GridPos>& dangerousCells,
        const Vec2& playerPosition,
        std::mt19937& randomEngine) override;
};

class HunterEnemy final : public Enemy
{
public:
    HunterEnemy(int id, Vec2 position, float speed);
    void draw() const override;

protected:
    Vec2 chooseDirection(const GameMap& map,
        const std::vector<WaterBubble>& bubbles,
        const std::vector<GridPos>& dangerousCells,
        const Vec2& playerPosition,
        std::mt19937& randomEngine) override;
};

class BossEnemy final : public Enemy
{
public:
    BossEnemy(int id, Vec2 position, float speed, int maxHealth);

    void update(float deltaTime) override;
    void draw() const override;
    EnemyHitResult takeWaveHit() override;
    int scoreValue() const override { return 1500; }
    bool isBoss() const override { return true; }
    int health() const override { return health_; }
    int maxHealth() const override { return maxHealth_; }

protected:
    Vec2 chooseDirection(const GameMap& map,
        const std::vector<WaterBubble>& bubbles,
        const std::vector<GridPos>& dangerousCells,
        const Vec2& playerPosition,
        std::mt19937& randomEngine) override;

private:
    int health_ = 1;
    int maxHealth_ = 1;
    float hitCooldown_ = 0.0f;
};

class WaterBubble final : public GameObject
{
public:
    WaterBubble(GridPos cell, int ownerId, int blastRange);

    void update(float deltaTime) override;
    void draw() const override;
    RectF bounds() const override;

    const GridPos& cell() const { return cell_; }
    int ownerId() const { return ownerId_; }
    int blastRange() const { return blastRange_; }
    float remainingTime() const { return timer_; }
    bool readyToExplode() const { return timer_ <= 0.0f; }
    void trigger() { timer_ = 0.0f; }
    void updateOwnerPassage(const RectF& ownerBounds);
    bool blocksCharacter(int characterId, const RectF& candidate) const;

private:
    GridPos cell_{};
    int ownerId_ = 0;
    int blastRange_ = 2;
    float timer_ = 2.5f;
    float animationTime_ = 0.0f;
    bool ownerMayPass_ = true;
};

class WaterWave final : public GameObject
{
public:
    explicit WaterWave(GridPos cell);

    void update(float deltaTime) override;
    void draw() const override;
    RectF bounds() const override;
    const GridPos& cell() const { return cell_; }

private:
    GridPos cell_{};
    float lifetime_ = 0.52f;
};

class ItemIconAtlas
{
public:
    bool load(const std::filesystem::path& imagePath);
    bool loaded() const { return loaded_; }
    void draw(PowerUpType type, int centerX, int centerY, int size) const;

private:
    static constexpr int CellSize = 256;
    IMAGE atlas_;
    bool loaded_ = false;
};

class PowerUp final : public GameObject
{
public:
    PowerUp(GridPos cell, PowerUpType type, const ItemIconAtlas* icons = nullptr);

    void update(float deltaTime) override;
    void draw() const override;
    RectF bounds() const override;
    PowerUpType type() const { return type_; }

private:
    GridPos cell_{};
    PowerUpType type_ = PowerUpType::BlastRange;
    float lifetime_ = 14.0f;
    float animationTime_ = 0.0f;
    const ItemIconAtlas* icons_ = nullptr;
};

class StarParticle final : public GameObject
{
public:
    StarParticle(Vec2 position, Vec2 velocity, COLORREF color, float lifetime, float radius);

    void update(float deltaTime) override;
    void draw() const override;
    RectF bounds() const override;

private:
    Vec2 position_{};
    Vec2 velocity_{};
    COLORREF color_ = Palette::Honey;
    float lifetime_ = 0.5f;
    float initialLifetime_ = 0.5f;
    float radius_ = 4.0f;
};

COLORREF styleColor(CharacterStyle style);
const wchar_t* styleName(CharacterStyle style);
void drawChibiCharacter(CharacterStyle style, const Vec2& position, float scale,
    bool walking = false, float animationTime = 0.0f);
