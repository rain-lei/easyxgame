#pragma once

#include "GameObjects.h"

#include <filesystem>

// 同一角色原画按使用场景预裁切成三种尺寸。
enum class PortraitSize
{
    Large,
    Card,
    Hud
};

// 角色原画图集封装，避免菜单、角色卡和 HUD 重复加载图片。
class PortraitAtlas
{
public:
    bool load(const std::filesystem::path& imagePath);
    bool loaded() const { return loaded_; }
    void draw(CharacterStyle style, PortraitSize size, int x, int y) const;
    int width(PortraitSize size) const;
    int height(PortraitSize size) const;

private:
    IMAGE largeAtlas_;
    IMAGE cardAtlas_;
    IMAGE hudAtlas_;
    bool loaded_ = false;

    const IMAGE* imageFor(PortraitSize size) const;
};

// MCI 音频资源的 RAII 管理器，统一背景音乐状态与短音效播放。
class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    void playMenuMusic();
    void playGameMusic();
    void pauseMusic();
    void resumeMusic();
    void stopMusic();
    void playEffect(const wchar_t* filename) const;

private:
    enum class MusicTrack
    {
        None,
        Menu,
        Game
    };

    std::filesystem::path audioDirectory_;
    MusicTrack currentTrack_ = MusicTrack::None;
    bool paused_ = false;

    void playLoop(const wchar_t* filename, MusicTrack track);
    std::filesystem::path assetPath(const wchar_t* filename) const;
};

// 跨关卡累计的演示统计，结算页和报告截图使用同一份数据。
struct GameStatistics
{
    int cratesDestroyed = 0;
    int powerUpsCollected = 0;
    int enemiesDefeated = 0;
    float elapsedTime = 0.0f;

    void reset()
    {
        cratesDestroyed = 0;
        powerUpsCollected = 0;
        enemiesDefeated = 0;
        elapsedTime = 0.0f;
    }
};

// 游戏组合根：拥有场景状态、地图和全部对象容器，负责主循环的统一调度。
// 具体对象仍在各自类中维护状态，避免把所有逻辑堆在入口函数里。
class MoeBubbleGame
{
public:
    MoeBubbleGame();
    ~MoeBubbleGame();

    int run();
    // 复用正式 render() 生成确定性的报告证据，不改变正常启动流程。
    int captureReportScreenshots(const std::filesystem::path& outputDirectory);

private:
    // 长生命周期服务和核心实体。
    InputManager input_;
    AudioManager audio_;
    PortraitAtlas portraits_;
    ItemIconAtlas itemIcons_;
    EnemySpriteAtlas enemySprites_;
    GameMap map_;
    Player player_{ 1 };
    Player player2_{ 2 };
    // 动态对象使用 STL 容器；Enemy 通过基类智能指针实现异构多态和 RAII。
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<WaterBubble> bubbles_;
    std::vector<WaterWave> waves_;
    std::vector<PowerUp> powerUps_;
    std::vector<StarParticle> particles_;
    // 固定种子使地图、掉落和 AI 决策可复现，便于测试和录制。
    std::mt19937 randomEngine_{ 20260713u };

    // 场景状态机及各界面的当前选择。
    SceneState scene_ = SceneState::MainMenu;
    SceneState instructionReturnScene_ = SceneState::MainMenu;
    SceneState exitReturnScene_ = SceneState::MainMenu;
    GameMode gameMode_ = GameMode::SinglePlayer;
    CharacterStyle selectedStyle_ = CharacterStyle::Bear;
    CharacterStyle selectedStyle2_ = CharacterStyle::Rabbit;
    bool coopCharacterSelection_ = false;
    int selectingPlayer_ = 1;
    bool running_ = true;
    bool windowOpened_ = false;
    int mouseX_ = -1;
    int mouseY_ = -1;
    bool mouseLeftPressed_ = false;
    int menuIndex_ = 0;
    int characterIndex_ = 0;
    int pauseIndex_ = 0;
    int resultIndex_ = 0;
    int exitChoice_ = 0;
    // 关卡、分数和重开快照；重开本关时回滚到关卡起点统计。
    int level_ = 1;
    int score_ = 0;
    int highScore_ = 0;
    int levelStartScore_ = 0;
    int levelStartCrates_ = 0;
    int levelStartPowerUps_ = 0;
    int levelStartEnemies_ = 0;
    float sceneTime_ = 0.0f;
    float effectCooldown_ = 0.0f;
    GameStatistics statistics_{};

    // 主循环基础阶段：输入 → 更新 → 清理 → 绘制。
    void openWindow();
    void processWindowMessages();
    void processInput();
    void update(float deltaTime);
    void render() const;
    void transitionTo(SceneState state);

    // 各场景分别处理键盘和鼠标，避免一个超长输入分支。
    void handleMainMenuInput();
    void handleCharacterSelectInput();
    void handleInstructionsInput();
    void handlePlayingInput();
    void handlePausedInput();
    void handleResultInput();
    void handleExitConfirmInput();

    // 玩法更新与关卡生命周期。
    void startNewGame();
    void setupLevel(int level, bool restoreLevelScore = false);
    void restartCurrentLevel();
    void tryPlaceBubble(Player& player);
    void updatePlaying(float deltaTime);
    void updateBubbles(float deltaTime);
    void explodeBubble(std::size_t bubbleIndex);
    void updateEnemies(float deltaTime);
    void updateCollisions();
    void createPowerUp(const GridPos& cell);
    void createParticles(const Vec2& position, COLORREF color, int count);
    std::vector<GridPos> collectDangerousCells() const;
    void completeLevel();
    void finishGame(bool victory);
    void cleanupInactiveObjects();
    bool twoPlayerMode() const { return gameMode_ == GameMode::TwoPlayer; }
    bool allPlayersDefeated() const;
    int totalRemainingLives() const;

    // 各场景绘制函数只读取状态，不在绘制阶段修改游戏数据。
    void drawBackground() const;
    void drawMainMenu() const;
    void drawCharacterSelect() const;
    void drawInstructions() const;
    void drawGameplay() const;
    void drawHud() const;
    void drawPausedOverlay() const;
    void drawLevelClearOverlay() const;
    void drawGameOverOverlay() const;
    void drawVictoryOverlay() const;
    void drawExitConfirmOverlay() const;

    void moveMenuSelection(int& index, int itemCount, int direction);
    bool mouseInside(int left, int top, int right, int bottom) const;
    bool mouseSelects(int& index, int candidate, int left, int top, int right, int bottom);
};
