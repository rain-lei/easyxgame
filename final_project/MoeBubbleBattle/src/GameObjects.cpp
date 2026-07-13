#include "GameObjects.h"

#include <limits>

namespace
{
    int pixel(float value)
    {
        return static_cast<int>(std::lround(value));
    }

    bool containsCell(const std::vector<GridPos>& cells, const GridPos& target)
    {
        return std::find(cells.begin(), cells.end(), target) != cells.end();
    }

    bool isOpposite(const Vec2& first, const Vec2& second)
    {
        return first.x == -second.x && first.y == -second.y;
    }

    void drawSimpleStar(int centerX, int centerY, int radius, COLORREF color)
    {
        POINT points[10]{};
        for (int index = 0; index < 10; ++index)
        {
            const float angle = -GameConfig::Pi / 2.0f + index * GameConfig::Pi / 5.0f;
            const float pointRadius = index % 2 == 0 ? static_cast<float>(radius)
                : static_cast<float>(radius) * 0.46f;
            points[index].x = centerX + pixel(std::cos(angle) * pointRadius);
            points[index].y = centerY + pixel(std::sin(angle) * pointRadius);
        }
        setlinecolor(Palette::Ink);
        setfillcolor(color);
        fillpolygon(points, 10);
    }
}

GameMap::GameMap()
{
    tiles_.fill(TileType::Floor);
}

int GameMap::indexOf(const GridPos& cell) const
{
    return cell.row * GameConfig::MapColumns + cell.column;
}

void GameMap::setTile(const GridPos& cell, TileType tile)
{
    if (inside(cell))
    {
        tiles_[indexOf(cell)] = tile;
    }
}

bool GameMap::isReservedSpawnCell(const GridPos& cell) const
{
    constexpr std::array<GridPos, 12> reserved = {
        GridPos{ 1, 1 }, GridPos{ 1, 2 }, GridPos{ 2, 1 },
        GridPos{ 11, 13 }, GridPos{ 11, 12 }, GridPos{ 10, 13 },
        GridPos{ 1, 13 }, GridPos{ 1, 12 }, GridPos{ 2, 13 },
        GridPos{ 11, 1 }, GridPos{ 11, 2 }, GridPos{ 10, 1 }
    };
    return std::find(reserved.begin(), reserved.end(), cell) != reserved.end();
}

void GameMap::generate(int level)
{
    tiles_.fill(TileType::Floor);
    std::mt19937 randomEngine(20260713u + static_cast<std::uint32_t>(level * 101));
    std::uniform_int_distribution<int> chance(0, 99);
    const int crateChance = 38 + level * 5;

    for (int row = 0; row < GameConfig::MapRows; ++row)
    {
        for (int column = 0; column < GameConfig::MapColumns; ++column)
        {
            const GridPos cell{ row, column };
            const bool boundary = row == 0 || column == 0
                || row == GameConfig::MapRows - 1 || column == GameConfig::MapColumns - 1;
            const bool pillar = row % 2 == 0 && column % 2 == 0;

            if (boundary || pillar)
            {
                setTile(cell, TileType::SolidWall);
            }
            else if (!isReservedSpawnCell(cell) && chance(randomEngine) < crateChance)
            {
                setTile(cell, TileType::Crate);
            }
        }
    }
}

void GameMap::draw() const
{
    for (int row = 0; row < GameConfig::MapRows; ++row)
    {
        for (int column = 0; column < GameConfig::MapColumns; ++column)
        {
            const GridPos cell{ row, column };
            const RectF area = cellBounds(cell);
            const int left = pixel(area.left);
            const int top = pixel(area.top);
            const int right = pixel(area.right);
            const int bottom = pixel(area.bottom);

            setlinecolor(row % 2 == column % 2 ? Palette::GrassB : Palette::GrassA);
            setfillcolor(row % 2 == column % 2 ? Palette::GrassA : Palette::GrassB);
            fillrectangle(left, top, right, bottom);

            const TileType tile = tileAt(cell);
            if (tile == TileType::SolidWall)
            {
                setlinecolor(Palette::StoneDark);
                setfillcolor(Palette::Stone);
                fillroundrect(left + 2, top + 2, right - 2, bottom - 2, 10, 10);
                setlinecolor(RGB(188, 210, 218));
                line(left + 8, top + 9, right - 9, top + 9);
                arc(left + 8, top + 8, right - 8, bottom - 8, 0.25, 1.25);
            }
            else if (tile == TileType::Crate)
            {
                setlinecolor(Palette::CrateDark);
                setfillcolor(Palette::Crate);
                fillroundrect(left + 4, top + 4, right - 4, bottom - 4, 6, 6);
                setlinecolor(RGB(246, 199, 111));
                rectangle(left + 9, top + 9, right - 9, bottom - 9);
                setlinecolor(Palette::CrateDark);
                line(left + 8, top + 8, right - 8, bottom - 8);
                line(right - 8, top + 8, left + 8, bottom - 8);
            }
        }
    }

    setlinecolor(Palette::Ink);
    rectangle(GameConfig::MapLeft - 1, GameConfig::MapTop - 1,
        GameConfig::MapLeft + GameConfig::MapWidth + 1,
        GameConfig::MapTop + GameConfig::MapHeight + 1);
}

bool GameMap::inside(const GridPos& cell) const
{
    return cell.row >= 0 && cell.row < GameConfig::MapRows
        && cell.column >= 0 && cell.column < GameConfig::MapColumns;
}

TileType GameMap::tileAt(const GridPos& cell) const
{
    return inside(cell) ? tiles_[indexOf(cell)] : TileType::SolidWall;
}

bool GameMap::isWalkable(const GridPos& cell) const
{
    return tileAt(cell) == TileType::Floor;
}

bool GameMap::isRectWalkable(const RectF& bounds) const
{
    constexpr float edgeInset = 2.0f;
    const std::array<Vec2, 4> corners = {
        Vec2{ bounds.left + edgeInset, bounds.top + edgeInset },
        Vec2{ bounds.right - edgeInset, bounds.top + edgeInset },
        Vec2{ bounds.left + edgeInset, bounds.bottom - edgeInset },
        Vec2{ bounds.right - edgeInset, bounds.bottom - edgeInset }
    };
    return std::all_of(corners.begin(), corners.end(), [this](const Vec2& corner)
    {
        return isWalkable(worldToCell(corner));
    });
}

bool GameMap::destroyCrate(const GridPos& cell)
{
    if (tileAt(cell) != TileType::Crate)
    {
        return false;
    }
    setTile(cell, TileType::Floor);
    return true;
}

int GameMap::crateCount() const
{
    return static_cast<int>(std::count(tiles_.begin(), tiles_.end(), TileType::Crate));
}

Character::Character(int id, Vec2 position, float radius, float speed)
    : id_(id), position_(position), radius_(radius), speed_(speed)
{
}

RectF Character::bounds() const
{
    return { position_.x - radius_, position_.y - radius_,
        position_.x + radius_, position_.y + radius_ };
}

bool Character::canOccupy(const RectF& candidate, const GameMap& map,
    const std::vector<WaterBubble>& bubbles) const
{
    if (!map.isRectWalkable(candidate))
    {
        return false;
    }
    return std::none_of(bubbles.begin(), bubbles.end(), [this, &candidate](const WaterBubble& bubble)
    {
        return bubble.active() && bubble.blocksCharacter(id_, candidate);
    });
}

bool Character::move(const Vec2& direction, float deltaTime, const GameMap& map,
    const std::vector<WaterBubble>& bubbles)
{
    const Vec2 unit = normalized(direction);
    if (lengthSquared(unit) < 0.001f)
    {
        return false;
    }

    bool moved = false;
    const Vec2 delta = unit * (speed_ * deltaTime);
    Vec2 candidatePosition = position_;
    candidatePosition.x += delta.x;
    RectF candidateBounds{ candidatePosition.x - radius_, candidatePosition.y - radius_,
        candidatePosition.x + radius_, candidatePosition.y + radius_ };
    if (canOccupy(candidateBounds, map, bubbles))
    {
        position_.x = candidatePosition.x;
        moved = true;
    }

    candidatePosition = position_;
    candidatePosition.y += delta.y;
    candidateBounds = { candidatePosition.x - radius_, candidatePosition.y - radius_,
        candidatePosition.x + radius_, candidatePosition.y + radius_ };
    if (canOccupy(candidateBounds, map, bubbles))
    {
        position_.y = candidatePosition.y;
        moved = true;
    }
    return moved;
}

Player::Player()
    // The visible sprite is roughly one 44 px lane wide, but collision is a
    // compact 20 px foot box.  Hats and backpacks should never catch on walls.
    : Character(1, cellCenter({ 1, 1 }), 10.0f, 148.0f)
{
}

void Player::update(float deltaTime)
{
    invulnerableTimer_ = std::max(0.0f, invulnerableTimer_ - deltaTime);
    // Keep a clock running even while idle. The invulnerability blink used to
    // freeze on its hidden frame until the player moved for the first time.
    animationTime_ += deltaTime;
}

void Player::draw() const
{
    const bool hiddenFrame = invulnerable() && static_cast<int>(animationTime_ * 12.0f) % 2 == 0;
    if (!hiddenFrame)
    {
        if (spriteSheetLoaded_)
        {
            drawSpriteFrame();
        }
        else
        {
            drawChibiCharacter(style_, position_, 0.58f, walking_, animationTime_);
        }
    }

    if (shieldCharges_ > 0)
    {
        const int pulse = 2 + pixel(std::sin(animationTime_ * 6.0f) * 2.0f);
        setlinecolor(Palette::Mint);
        setlinestyle(PS_SOLID, 3);
        circle(pixel(position_.x), pixel(position_.y), pixel(radius_ + 9.0f + pulse));
        setlinestyle(PS_SOLID, 1);
    }
}

bool Player::loadSpriteSheet(const std::filesystem::path& imagePath)
{
    const std::wstring path = imagePath.wstring();
    const int result = loadimage(&spriteSheet_, path.c_str(),
        SpriteColumns * SpriteCellSize, SpriteRows * SpriteCellSize, true);
    spriteSheetLoaded_ = result == 0;
    return spriteSheetLoaded_;
}

void Player::drawSpriteFrame() const
{
    int frame = 1;
    if (walking_)
    {
        frame = static_cast<int>(animationTime_ * 9.0f) % 2 == 0 ? 0 : 2;
    }

    const int row = std::clamp(static_cast<int>(style_), 0, SpriteRows - 1);
    const int drawSize = SpriteCellSize;
    const int drawX = pixel(position_.x) - drawSize / 2;
    // Keep the feet close to the collision center while allowing the hood to
    // overlap the tile above. Transparent pixels do not cover the map.
    const int drawY = pixel(position_.y) - 47;

    DWORD* destination = GetImageBuffer();
    DWORD* source = GetImageBuffer(&spriteSheet_);
    if (destination == nullptr || source == nullptr)
    {
        return;
    }

    const int destinationWidth = getwidth();
    const int destinationHeight = getheight();
    const int sourceWidth = spriteSheet_.getwidth();
    const int sourceX = frame * SpriteCellSize;
    const int sourceY = row * SpriteCellSize;
    for (int localY = 0; localY < drawSize; ++localY)
    {
        const int destinationY = drawY + localY;
        if (destinationY < 0 || destinationY >= destinationHeight)
        {
            continue;
        }
        for (int localX = 0; localX < drawSize; ++localX)
        {
            const int destinationX = drawX + localX;
            if (destinationX < 0 || destinationX >= destinationWidth)
            {
                continue;
            }

            const DWORD sourcePixel = source[(sourceY + localY) * sourceWidth + sourceX + localX];
            const unsigned int alpha = (sourcePixel >> 24) & 0xffu;
            if (alpha == 0u)
            {
                continue;
            }

            DWORD& destinationPixel = destination[destinationY * destinationWidth + destinationX];
            if (alpha == 255u)
            {
                destinationPixel = (destinationPixel & 0xff000000u) | (sourcePixel & 0x00ffffffu);
                continue;
            }

            const unsigned int inverseAlpha = 255u - alpha;
            const unsigned int sourceBlue = sourcePixel & 0xffu;
            const unsigned int sourceGreen = (sourcePixel >> 8) & 0xffu;
            const unsigned int sourceRed = (sourcePixel >> 16) & 0xffu;
            const unsigned int destinationBlue = destinationPixel & 0xffu;
            const unsigned int destinationGreen = (destinationPixel >> 8) & 0xffu;
            const unsigned int destinationRed = (destinationPixel >> 16) & 0xffu;
            const unsigned int blue = (sourceBlue * alpha + destinationBlue * inverseAlpha) / 255u;
            const unsigned int green = (sourceGreen * alpha + destinationGreen * inverseAlpha) / 255u;
            const unsigned int red = (sourceRed * alpha + destinationRed * inverseAlpha) / 255u;
            destinationPixel = (destinationPixel & 0xff000000u)
                | (red << 16) | (green << 8) | blue;
        }
    }
}

void Player::handleMovement(const InputManager& input, float deltaTime,
    const GameMap& map, const std::vector<WaterBubble>& bubbles)
{
    // down() keeps long presses smooth; pressed() also catches a quick tap that
    // starts and ends between two game frames.  Without the second half, menu
    // navigation feels responsive while a light WASD tap can appear to do
    // nothing on faster machines.
    const auto movementKeyActive = [&](int key)
    {
        return input.down(key) || input.pressed(key);
    };

    Vec2 requested{};
    if (movementKeyActive('A') || movementKeyActive(VK_LEFT))
    {
        requested.x -= 1.0f;
    }
    if (movementKeyActive('D') || movementKeyActive(VK_RIGHT))
    {
        requested.x += 1.0f;
    }
    if (movementKeyActive('W') || movementKeyActive(VK_UP))
    {
        requested.y -= 1.0f;
    }
    if (movementKeyActive('S') || movementKeyActive(VK_DOWN))
    {
        requested.y += 1.0f;
    }

    if (lengthSquared(requested) < 0.001f)
    {
        walking_ = false;
        return;
    }

    Vec2 primary = requested;
    Vec2 secondary{};
    if (requested.x != 0.0f && requested.y != 0.0f)
    {
        if (facing_.x != 0.0f)
        {
            primary = { requested.x, 0.0f };
            secondary = { 0.0f, requested.y };
        }
        else
        {
            primary = { 0.0f, requested.y };
            secondary = { requested.x, 0.0f };
        }
    }
    else
    {
        primary = requested.x != 0.0f ? Vec2{ requested.x, 0.0f }
            : Vec2{ 0.0f, requested.y };
    }

    auto moveWithCornerAssist = [&](const Vec2& direction)
    {
        if (lengthSquared(direction) < 0.001f)
        {
            return false;
        }

        const Vec2 center = cellCenter(cell());
        // A 20 px foot box has about 12 px clearance inside a 44 px lane.
        // Cover that full clearance so an early turn naturally glides to the
        // centre line instead of stopping against the corner of a wall.
        const float correctionLimit = speed_ * deltaTime * 2.15f;
        constexpr float assistDistance = 15.0f;

        if (direction.x != 0.0f)
        {
            const float difference = center.y - position_.y;
            if (std::fabs(difference) <= assistDistance)
            {
                const float correction = std::clamp(difference, -correctionLimit, correctionLimit);
                const RectF candidate{ position_.x - radius_, position_.y + correction - radius_,
                    position_.x + radius_, position_.y + correction + radius_ };
                if (canOccupy(candidate, map, bubbles))
                {
                    position_.y += correction;
                }
            }
        }
        else
        {
            const float difference = center.x - position_.x;
            if (std::fabs(difference) <= assistDistance)
            {
                const float correction = std::clamp(difference, -correctionLimit, correctionLimit);
                const RectF candidate{ position_.x + correction - radius_, position_.y - radius_,
                    position_.x + correction + radius_, position_.y + radius_ };
                if (canOccupy(candidate, map, bubbles))
                {
                    position_.x += correction;
                }
            }
        }
        return move(direction, deltaTime, map, bubbles);
    };

    bool moved = moveWithCornerAssist(primary);
    if (!moved && lengthSquared(secondary) > 0.001f)
    {
        moved = moveWithCornerAssist(secondary);
        if (moved)
        {
            primary = secondary;
        }
    }

    walking_ = moved;
    if (moved)
    {
        facing_ = normalized(primary);
    }
}

void Player::resetForNewGame(CharacterStyle style)
{
    style_ = style;
    lives_ = 3;
    bubbleCapacity_ = 1;
    activeBubbles_ = 0;
    blastRange_ = 2;
    speedLevel_ = 0;
    shieldCharges_ = 0;
    speed_ = 148.0f;
    invulnerableTimer_ = 0.0f;
    facing_ = { 0.0f, 1.0f };
    walking_ = false;
    animationTime_ = 0.0f;
    active_ = true;
    prepareForLevel({ 1, 1 });
}

void Player::prepareForLevel(const GridPos& spawnCell)
{
    position_ = cellCenter(spawnCell);
    activeBubbles_ = 0;
    invulnerableTimer_ = 1.2f;
    walking_ = false;
    animationTime_ = 0.0f;
    active_ = true;
}

bool Player::takeDamage()
{
    if (invulnerableTimer_ > 0.0f || lives_ <= 0)
    {
        return false;
    }

    if (shieldCharges_ > 0)
    {
        --shieldCharges_;
        invulnerableTimer_ = 0.8f;
        return true;
    }

    --lives_;
    invulnerableTimer_ = 1.8f;
    if (lives_ <= 0)
    {
        active_ = false;
    }
    return true;
}

void Player::applyPowerUp(PowerUpType type)
{
    switch (type)
    {
    case PowerUpType::BlastRange:
        blastRange_ = std::min(6, blastRange_ + 1);
        break;
    case PowerUpType::BubbleCapacity:
        bubbleCapacity_ = std::min(5, bubbleCapacity_ + 1);
        break;
    case PowerUpType::Speed:
        speedLevel_ = std::min(4, speedLevel_ + 1);
        speed_ = 148.0f + speedLevel_ * 14.0f;
        break;
    case PowerUpType::Shield:
        shieldCharges_ = std::min(2, shieldCharges_ + 1);
        break;
    }
}

bool Player::canPlaceBubble() const
{
    return activeBubbles_ < bubbleCapacity_ && lives_ > 0;
}

void Player::onBubblePlaced()
{
    activeBubbles_ = std::min(bubbleCapacity_, activeBubbles_ + 1);
}

void Player::onBubbleExpired()
{
    activeBubbles_ = std::max(0, activeBubbles_ - 1);
}

Enemy::Enemy(int id, Vec2 position, float speed)
    : Character(id, position, 14.0f, speed)
{
}

void Enemy::update(float deltaTime)
{
    animationTime_ += deltaTime;
    decisionTimer_ -= deltaTime;
}

void Enemy::updateAI(float deltaTime, const GameMap& map,
    const std::vector<WaterBubble>& bubbles,
    const std::vector<GridPos>& dangerousCells,
    const Vec2& playerPosition,
    std::mt19937& randomEngine)
{
    update(deltaTime);
    const GridPos currentCell = cell();
    const Vec2 center = cellCenter(currentCell);
    const bool nearCenter = std::fabs(position_.x - center.x) < 3.5f
        && std::fabs(position_.y - center.y) < 3.5f;

    if (decisionTimer_ <= 0.0f && nearCenter)
    {
        position_ = center;
        direction_ = chooseDirection(map, bubbles, dangerousCells, playerPosition, randomEngine);
        decisionTimer_ = 0.30f;
    }

    if (!move(direction_, deltaTime, map, bubbles))
    {
        direction_ = chooseDirection(map, bubbles, dangerousCells, playerPosition, randomEngine);
        decisionTimer_ = 0.0f;
    }
}

std::vector<Vec2> Enemy::availableDirections(const GameMap& map,
    const std::vector<WaterBubble>& bubbles) const
{
    constexpr std::array<Vec2, 4> directions = {
        Vec2{ 1.0f, 0.0f }, Vec2{ -1.0f, 0.0f },
        Vec2{ 0.0f, 1.0f }, Vec2{ 0.0f, -1.0f }
    };
    std::vector<Vec2> result;
    const GridPos current = cell();
    for (const Vec2& direction : directions)
    {
        const GridPos next{ current.row + pixel(direction.y), current.column + pixel(direction.x) };
        if (!map.isWalkable(next))
        {
            continue;
        }
        const bool occupiedByBubble = std::any_of(bubbles.begin(), bubbles.end(), [&next](const WaterBubble& bubble)
        {
            return bubble.active() && bubble.cell() == next;
        });
        if (!occupiedByBubble)
        {
            result.push_back(direction);
        }
    }
    return result;
}

bool Enemy::isDangerous(const GridPos& cell,
    const std::vector<GridPos>& dangerousCells) const
{
    return containsCell(dangerousCells, cell);
}

PatrolEnemy::PatrolEnemy(int id, Vec2 position, float speed)
    : Enemy(id, position, speed)
{
}

void PatrolEnemy::draw() const
{
    const int x = pixel(position_.x);
    const int y = pixel(position_.y + std::sin(animationTime_ * 6.0f) * 1.6f);
    setlinecolor(Palette::PurpleDark);
    setfillcolor(Palette::Purple);
    solidellipse(x - 15, y - 12, x + 15, y + 14);
    solidcircle(x - 9, y - 12, 5);
    solidcircle(x + 9, y - 12, 5);
    setfillcolor(Palette::White);
    solidcircle(x - 5, y - 2, 3);
    solidcircle(x + 5, y - 2, 3);
    setfillcolor(Palette::Ink);
    solidcircle(x - 5, y - 2, 1);
    solidcircle(x + 5, y - 2, 1);
    arc(x - 5, y + 1, x + 5, y + 8, 3.45, 5.95);
}

Vec2 PatrolEnemy::chooseDirection(const GameMap& map,
    const std::vector<WaterBubble>& bubbles,
    const std::vector<GridPos>& dangerousCells,
    const Vec2& playerPosition,
    std::mt19937& randomEngine)
{
    (void)playerPosition;
    std::vector<Vec2> options = availableDirections(map, bubbles);
    if (options.empty())
    {
        return { -direction_.x, -direction_.y };
    }

    std::vector<Vec2> safe;
    for (const Vec2& option : options)
    {
        const GridPos next{ cell().row + pixel(option.y), cell().column + pixel(option.x) };
        if (!isDangerous(next, dangerousCells))
        {
            safe.push_back(option);
        }
    }
    if (!safe.empty())
    {
        options = safe;
    }

    if (options.size() > 1)
    {
        options.erase(std::remove_if(options.begin(), options.end(), [this](const Vec2& option)
        {
            return isOpposite(option, direction_);
        }), options.end());
    }
    std::uniform_int_distribution<std::size_t> choose(0, options.size() - 1);
    return options[choose(randomEngine)];
}

HunterEnemy::HunterEnemy(int id, Vec2 position, float speed)
    : Enemy(id, position, speed)
{
}

void HunterEnemy::draw() const
{
    const int x = pixel(position_.x);
    const int y = pixel(position_.y + std::sin(animationTime_ * 7.0f) * 1.8f);
    setlinecolor(RGB(123, 64, 77));
    setfillcolor(Palette::Coral);
    solidellipse(x - 15, y - 13, x + 15, y + 14);
    POINT leftEar[3] = { { x - 13, y - 7 }, { x - 20, y - 17 }, { x - 5, y - 12 } };
    POINT rightEar[3] = { { x + 13, y - 7 }, { x + 20, y - 17 }, { x + 5, y - 12 } };
    fillpolygon(leftEar, 3);
    fillpolygon(rightEar, 3);
    setfillcolor(Palette::White);
    solidcircle(x - 5, y - 3, 3);
    solidcircle(x + 5, y - 3, 3);
    setfillcolor(Palette::Ink);
    solidcircle(x - 4, y - 3, 1);
    solidcircle(x + 4, y - 3, 1);
    line(x - 5, y + 7, x, y + 4);
    line(x, y + 4, x + 5, y + 7);
}

Vec2 HunterEnemy::chooseDirection(const GameMap& map,
    const std::vector<WaterBubble>& bubbles,
    const std::vector<GridPos>& dangerousCells,
    const Vec2& playerPosition,
    std::mt19937& randomEngine)
{
    std::vector<Vec2> options = availableDirections(map, bubbles);
    if (options.empty())
    {
        return { -direction_.x, -direction_.y };
    }

    std::shuffle(options.begin(), options.end(), randomEngine);
    const GridPos targetCell = worldToCell(playerPosition);
    const GridPos current = cell();
    auto scoreDirection = [&](const Vec2& direction)
    {
        const GridPos next{ current.row + pixel(direction.y), current.column + pixel(direction.x) };
        const int distance = std::abs(next.row - targetCell.row) + std::abs(next.column - targetCell.column);
        const int dangerPenalty = isDangerous(next, dangerousCells) ? 100 : 0;
        const int reversePenalty = isOpposite(direction, direction_) ? 2 : 0;
        return distance + dangerPenalty + reversePenalty;
    };
    return *std::min_element(options.begin(), options.end(), [&](const Vec2& first, const Vec2& second)
    {
        return scoreDirection(first) < scoreDirection(second);
    });
}

WaterBubble::WaterBubble(GridPos cell, int ownerId, int blastRange)
    : cell_(cell), ownerId_(ownerId), blastRange_(blastRange)
{
}

void WaterBubble::update(float deltaTime)
{
    timer_ -= deltaTime;
    animationTime_ += deltaTime;
}

void WaterBubble::draw() const
{
    const Vec2 center = cellCenter(cell_);
    const float urgency = std::clamp((0.8f - timer_) / 0.8f, 0.0f, 1.0f);
    const float pulseSpeed = 4.0f + urgency * 12.0f;
    const int pulse = pixel(std::sin(animationTime_ * pulseSpeed) * (2.0f + urgency * 2.0f));
    const int radius = 15 + pulse;
    const COLORREF outline = timer_ < 0.7f ? Palette::Danger : Palette::AquaDark;

    setlinecolor(outline);
    setfillcolor(RGB(173, 235, 244));
    fillcircle(pixel(center.x), pixel(center.y), radius);
    setfillcolor(Palette::White);
    solidcircle(pixel(center.x - radius * 0.34f), pixel(center.y - radius * 0.34f),
        std::max(2, radius / 4));
    setlinecolor(RGB(90, 190, 211));
    arc(pixel(center.x - radius * 0.55f), pixel(center.y - radius * 0.55f),
        pixel(center.x + radius * 0.55f), pixel(center.y + radius * 0.55f), 3.8, 5.6);
}

RectF WaterBubble::bounds() const
{
    return cellBounds(cell_, 7.0f);
}

void WaterBubble::updateOwnerPassage(const RectF& ownerBounds)
{
    if (ownerMayPass_ && !intersects(bounds(), ownerBounds))
    {
        ownerMayPass_ = false;
    }
}

bool WaterBubble::blocksCharacter(int characterId, const RectF& candidate) const
{
    if (characterId == ownerId_ && ownerMayPass_)
    {
        return false;
    }
    return intersects(bounds(), candidate);
}

WaterWave::WaterWave(GridPos cell)
    : cell_(cell)
{
}

void WaterWave::update(float deltaTime)
{
    lifetime_ -= deltaTime;
    if (lifetime_ <= 0.0f)
    {
        active_ = false;
    }
}

void WaterWave::draw() const
{
    const RectF area = cellBounds(cell_, 4.0f);
    const Vec2 center = cellCenter(cell_);
    const int radius = 7 + pixel(std::sin(lifetime_ * 35.0f) * 2.0f);
    setlinecolor(Palette::AquaDark);
    setfillcolor(RGB(151, 229, 242));
    fillroundrect(pixel(area.left), pixel(area.top), pixel(area.right), pixel(area.bottom), 16, 16);
    setfillcolor(Palette::White);
    solidcircle(pixel(center.x - 9.0f), pixel(center.y - 8.0f), std::max(2, radius / 2));
    solidcircle(pixel(center.x + 8.0f), pixel(center.y + 6.0f), std::max(2, radius / 3));
    setlinecolor(Palette::White);
    line(pixel(area.left + 7.0f), pixel(center.y), pixel(area.right - 7.0f), pixel(center.y));
    line(pixel(center.x), pixel(area.top + 7.0f), pixel(center.x), pixel(area.bottom - 7.0f));
}

RectF WaterWave::bounds() const
{
    return cellBounds(cell_, 4.0f);
}

PowerUp::PowerUp(GridPos cell, PowerUpType type)
    : cell_(cell), type_(type)
{
}

void PowerUp::update(float deltaTime)
{
    lifetime_ -= deltaTime;
    animationTime_ += deltaTime;
    if (lifetime_ <= 0.0f)
    {
        active_ = false;
    }
}

void PowerUp::draw() const
{
    const Vec2 center = cellCenter(cell_);
    const int y = pixel(center.y + std::sin(animationTime_ * 5.0f) * 3.0f);
    COLORREF color = Palette::Aqua;
    wchar_t symbol[2] = L"+";
    switch (type_)
    {
    case PowerUpType::BlastRange:
        color = Palette::Coral;
        symbol[0] = L'R';
        break;
    case PowerUpType::BubbleCapacity:
        color = Palette::Aqua;
        symbol[0] = L'B';
        break;
    case PowerUpType::Speed:
        color = Palette::Honey;
        symbol[0] = L'S';
        break;
    case PowerUpType::Shield:
        color = Palette::Mint;
        symbol[0] = L'盾';
        break;
    }

    setlinecolor(Palette::Ink);
    setfillcolor(color);
    fillroundrect(pixel(center.x) - 15, y - 15, pixel(center.x) + 15, y + 15, 10, 10);
    setbkmode(TRANSPARENT);
    settextcolor(Palette::Ink);
    settextstyle(16, 0, L"Microsoft YaHei UI");
    outtextxy(pixel(center.x) - textwidth(symbol) / 2, y - textheight(symbol) / 2, symbol);
}

RectF PowerUp::bounds() const
{
    return cellBounds(cell_, 8.0f);
}

StarParticle::StarParticle(Vec2 position, Vec2 velocity, COLORREF color,
    float lifetime, float radius)
    : position_(position), velocity_(velocity), color_(color), lifetime_(lifetime),
      initialLifetime_(lifetime), radius_(radius)
{
}

void StarParticle::update(float deltaTime)
{
    lifetime_ -= deltaTime;
    if (lifetime_ <= 0.0f)
    {
        active_ = false;
        return;
    }
    position_ += velocity_ * deltaTime;
    velocity_.y += 35.0f * deltaTime;
}

void StarParticle::draw() const
{
    const float ratio = std::clamp(lifetime_ / initialLifetime_, 0.0f, 1.0f);
    drawSimpleStar(pixel(position_.x), pixel(position_.y),
        std::max(2, pixel(radius_ * ratio)), color_);
}

RectF StarParticle::bounds() const
{
    return { position_.x - radius_, position_.y - radius_,
        position_.x + radius_, position_.y + radius_ };
}

COLORREF styleColor(CharacterStyle style)
{
    switch (style)
    {
    case CharacterStyle::Bear: return Palette::Aqua;
    case CharacterStyle::Rabbit: return Palette::Coral;
    case CharacterStyle::Cat: return Palette::Mint;
    case CharacterStyle::Dog: return Palette::Honey;
    }
    return Palette::Aqua;
}

const wchar_t* styleName(CharacterStyle style)
{
    switch (style)
    {
    case CharacterStyle::Bear: return L"蓝熊少年";
    case CharacterStyle::Rabbit: return L"珊瑚兔少女";
    case CharacterStyle::Cat: return L"薄荷猫发明家";
    case CharacterStyle::Dog: return L"蜂蜜犬侦察员";
    }
    return L"泡泡搭档";
}

void drawChibiCharacter(CharacterStyle style, const Vec2& position, float scale,
    bool walking, float animationTime)
{
    const int x = pixel(position.x);
    const int bounce = walking ? pixel(std::sin(animationTime * 9.0f) * 2.0f * scale) : 0;
    const int y = pixel(position.y) + bounce;
    const int stride = walking ? pixel(std::sin(animationTime * 10.0f) * 5.0f * scale) : 0;
    const COLORREF primary = styleColor(style);
    const int headRadius = std::max(8, pixel(24.0f * scale));
    const int bodyWidth = std::max(10, pixel(31.0f * scale));
    const int bodyHeight = std::max(10, pixel(36.0f * scale));

    setlinestyle(PS_SOLID, std::max(1, pixel(2.0f * scale)));
    setlinecolor(Palette::Ink);
    setfillcolor(Palette::Shadow);
    solidellipse(x - bodyWidth / 2 + 3, y + headRadius / 2 + 5,
        x + bodyWidth / 2 + 3, y + headRadius / 2 + bodyHeight + 6);

    setfillcolor(primary);
    if (style == CharacterStyle::Bear)
    {
        solidcircle(x - headRadius + 3, y - headRadius + 4, std::max(3, headRadius / 3));
        solidcircle(x + headRadius - 3, y - headRadius + 4, std::max(3, headRadius / 3));
    }
    else if (style == CharacterStyle::Rabbit)
    {
        solidellipse(x - headRadius + 2, y - headRadius * 2 - 5,
            x - 3, y - headRadius + 6);
        solidellipse(x + 3, y - headRadius * 2 - 5,
            x + headRadius - 2, y - headRadius + 6);
    }
    else if (style == CharacterStyle::Cat)
    {
        POINT leftEar[3] = {
            { x - headRadius + 2, y - headRadius / 2 },
            { x - headRadius + 5, y - headRadius - 12 },
            { x - 4, y - headRadius + 2 }
        };
        POINT rightEar[3] = {
            { x + headRadius - 2, y - headRadius / 2 },
            { x + headRadius - 5, y - headRadius - 12 },
            { x + 4, y - headRadius + 2 }
        };
        fillpolygon(leftEar, 3);
        fillpolygon(rightEar, 3);
    }
    else
    {
        solidellipse(x - headRadius - 9, y - headRadius + 1,
            x - headRadius / 2, y + 2);
        solidellipse(x + headRadius / 2, y - headRadius + 1,
            x + headRadius + 9, y + 2);
    }

    setfillcolor(primary);
    fillroundrect(x - bodyWidth / 2, y + headRadius / 2,
        x + bodyWidth / 2, y + headRadius / 2 + bodyHeight,
        std::max(5, pixel(12.0f * scale)), std::max(5, pixel(12.0f * scale)));
    solidcircle(x - bodyWidth / 2 - pixel(4.0f * scale), y + headRadius + stride / 2,
        std::max(3, pixel(6.0f * scale)));
    solidcircle(x + bodyWidth / 2 + pixel(4.0f * scale), y + headRadius - stride / 2,
        std::max(3, pixel(6.0f * scale)));

    setfillcolor(Palette::Ink);
    const int footWidth = std::max(4, bodyWidth / 2 - 2);
    const int footHeight = std::max(4, pixel(7.0f * scale));
    solidellipse(x - bodyWidth / 2 + stride, y + headRadius / 2 + bodyHeight - 2,
        x - bodyWidth / 2 + stride + footWidth,
        y + headRadius / 2 + bodyHeight + footHeight);
    solidellipse(x + 2 - stride, y + headRadius / 2 + bodyHeight - 2,
        x + 2 - stride + footWidth,
        y + headRadius / 2 + bodyHeight + footHeight);

    setfillcolor(primary);
    solidcircle(x, y, headRadius + 2);
    setfillcolor(RGB(255, 231, 204));
    solidellipse(x - headRadius + 4, y - headRadius + 7,
        x + headRadius - 4, y + headRadius - 2);

    const int eyeOffset = std::max(3, pixel(7.0f * scale));
    const int eyeRadius = std::max(1, pixel(3.0f * scale));
    setfillcolor(Palette::Ink);
    solidcircle(x - eyeOffset, y - 1, eyeRadius);
    solidcircle(x + eyeOffset, y - 1, eyeRadius);
    setlinecolor(Palette::Ink);
    arc(x - std::max(3, pixel(6.0f * scale)), y + 2,
        x + std::max(3, pixel(6.0f * scale)), y + std::max(5, pixel(11.0f * scale)),
        3.4, 6.0);

    setlinecolor(primary);
    setfillcolor(RGB(194, 239, 246));
    const int tankRadius = std::max(3, pixel(6.0f * scale));
    solidcircle(x + bodyWidth / 2, y + headRadius / 2 + bodyHeight / 2, tankRadius);
    setlinestyle(PS_SOLID, 1);
}
