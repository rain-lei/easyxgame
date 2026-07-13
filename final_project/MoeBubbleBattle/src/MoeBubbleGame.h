#pragma once

#include "GameObjects.h"

#include <filesystem>

enum class PortraitSize
{
    Large,
    Card,
    Hud
};

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

class MoeBubbleGame
{
public:
    MoeBubbleGame();
    ~MoeBubbleGame();

    int run();

private:
    InputManager input_;
    AudioManager audio_;
    PortraitAtlas portraits_;
    GameMap map_;
    Player player_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<WaterBubble> bubbles_;
    std::vector<WaterWave> waves_;
    std::vector<PowerUp> powerUps_;
    std::vector<StarParticle> particles_;
    std::mt19937 randomEngine_{ 20260713u };

    SceneState scene_ = SceneState::MainMenu;
    SceneState instructionReturnScene_ = SceneState::MainMenu;
    SceneState exitReturnScene_ = SceneState::MainMenu;
    CharacterStyle selectedStyle_ = CharacterStyle::Bear;
    bool running_ = true;
    bool windowOpened_ = false;
    int menuIndex_ = 0;
    int characterIndex_ = 0;
    int pauseIndex_ = 0;
    int resultIndex_ = 0;
    int exitChoice_ = 0;
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

    void openWindow();
    void processWindowMessages();
    void processInput();
    void update(float deltaTime);
    void render() const;
    void transitionTo(SceneState state);

    void handleMainMenuInput();
    void handleCharacterSelectInput();
    void handleInstructionsInput();
    void handlePlayingInput();
    void handlePausedInput();
    void handleResultInput();
    void handleExitConfirmInput();

    void startNewGame();
    void setupLevel(int level, bool restoreLevelScore = false);
    void restartCurrentLevel();
    void tryPlaceBubble();
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
};
