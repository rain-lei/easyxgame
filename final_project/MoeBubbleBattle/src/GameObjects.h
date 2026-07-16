#pragma once

#include "GameTypes.h"

#include <filesystem>

class WaterBubble;

// 封装固定大小的 15×13 地图、随机生成、绘制与碰撞查询。
// tiles_ 保持私有，外部只能通过语义化方法读取或破坏木箱。
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

// 所有可更新、可绘制对象的抽象基类，也是运行时多态的共同接口。
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

// Player 与 Enemy 的共同父类，复用位置、速度、碰撞盒和分轴移动逻辑。
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

// 玩家对象封装输入移动、生命、角色属性、水泡计数、强化与行走动画。
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
    // 精灵图为 4 行角色 × 3 列步态，运行时按角色和动画帧裁切。
    static constexpr int SpriteColumns = 3;
    static constexpr int SpriteRows = 4;
    static constexpr int SpriteCellSize = 96;
    static constexpr int SpriteDrawSize = GameConfig::TileSize - 4;

    // 外观和运动状态。
    CharacterStyle style_ = CharacterStyle::Bear;
    Vec2 facing_{ 0.0f, 1.0f };
    bool walking_ = false;
    IMAGE spriteSheet_;
    bool spriteSheetLoaded_ = false;
    // 战斗数值全部私有，通过受伤/强化/放泡行为维护上限与不变量。
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

// 三格透明敌人图集统一普通怪、追踪怪和 Boss 的美术资源及缩放规则。
class EnemySpriteAtlas
{
public:
    bool load(const std::filesystem::path& imagePath);
    bool loaded() const { return loaded_; }
    void draw(int column, int centerX, int centerY, int size) const;

private:
    static constexpr int CellSize = 256;
    IMAGE atlas_;
    bool loaded_ = false;
};

// 敌人抽象基类负责公共移动与避险，派生类只重写决策、绘制或受击差异。
class Enemy : public Character
{
public:
    Enemy(int id, Vec2 position, float speed, const EnemySpriteAtlas* sprites);

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
    const EnemySpriteAtlas* sprites_ = nullptr;
    Vec2 direction_{ 0.0f, 1.0f };
    float decisionTimer_ = 0.0f;
    float animationTime_ = 0.0f;

    // 纯虚函数是敌人多态的核心：主循环不需要判断实际派生类型。
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

// 巡逻敌人：从安全可走方向中随机选择，行为不直接追踪玩家。
class PatrolEnemy final : public Enemy
{
public:
    PatrolEnemy(int id, Vec2 position, float speed, const EnemySpriteAtlas* sprites);
    void draw() const override;

protected:
    Vec2 chooseDirection(const GameMap& map,
        const std::vector<WaterBubble>& bubbles,
        const std::vector<GridPos>& dangerousCells,
        const Vec2& playerPosition,
        std::mt19937& randomEngine) override;
};

// 追踪敌人：在安全方向中优先选择能缩短与玩家距离的方向。
class HunterEnemy final : public Enemy
{
public:
    HunterEnemy(int id, Vec2 position, float speed, const EnemySpriteAtlas* sprites);
    void draw() const override;

protected:
    Vec2 chooseDirection(const GameMap& map,
        const std::vector<WaterBubble>& bubbles,
        const std::vector<GridPos>& dangerousCells,
        const Vec2& playerPosition,
        std::mt19937& randomEngine) override;
};

// 第三关首领：在 Enemy 接口上扩展多血量、受击冷却和更高得分。
class BossEnemy final : public Enemy
{
public:
    BossEnemy(int id, Vec2 position, float speed, int maxHealth,
        const EnemySpriteAtlas* sprites);

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

// 水泡保存所有者、威力和倒计时；刚放置时允许所有者从本格离开一次。
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

// 单个网格水浪片段，短暂存在并参与玩家/敌人的碰撞检测。
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

// 四格透明道具图集，只加载一次并在地图、说明页和 HUD 中复用。
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

// 有限生命周期的强化道具，具体效果由 Player::applyPowerUp 统一处理。
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

// 轻量粒子对象，用速度和剩余寿命生成爆炸、拾取与结算反馈。
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
