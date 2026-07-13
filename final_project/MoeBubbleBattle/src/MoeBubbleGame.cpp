#include "MoeBubbleGame.h"

#include <imm.h>
#include <mmsystem.h>

#include <iomanip>
#include <sstream>

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "winmm.lib")

namespace
{
    int pixel(float value)
    {
        return static_cast<int>(std::lround(value));
    }

    void setUiFont(int height, bool bold = false)
    {
        LOGFONT font{};
        gettextstyle(&font);
        font.lfHeight = height;
        font.lfWidth = 0;
        font.lfWeight = bold ? FW_BOLD : FW_NORMAL;
        font.lfQuality = ANTIALIASED_QUALITY;
        wcscpy_s(font.lfFaceName, L"Microsoft YaHei UI");
        settextstyle(&font);
        setbkmode(TRANSPARENT);
    }

    void drawTextAt(const std::wstring& text, int x, int y, int size,
        COLORREF color, bool bold = false)
    {
        setUiFont(size, bold);
        settextcolor(color);
        outtextxy(x, y, text.c_str());
    }

    void drawCenteredText(const std::wstring& text, int centerX, int y, int size,
        COLORREF color, bool bold = false)
    {
        setUiFont(size, bold);
        settextcolor(color);
        outtextxy(centerX - textwidth(text.c_str()) / 2, y, text.c_str());
    }

    void drawPanel(int left, int top, int right, int bottom, COLORREF fill,
        int radius = 24, bool shadow = true)
    {
        if (shadow)
        {
            setlinecolor(Palette::Shadow);
            setfillcolor(Palette::Shadow);
            fillroundrect(left + 6, top + 6, right + 6, bottom + 6, radius, radius);
        }
        setlinecolor(Palette::Ink);
        setfillcolor(fill);
        fillroundrect(left, top, right, bottom, radius, radius);
    }

    void drawButton(int left, int top, int right, int bottom, const wchar_t* label,
        bool selected, COLORREF accent = Palette::Aqua)
    {
        const int offset = selected ? -2 : 0;
        const COLORREF fill = selected ? accent : Palette::White;
        if (selected)
        {
            setlinecolor(Palette::Shadow);
            setfillcolor(Palette::Shadow);
            fillroundrect(left + 5, top + 7, right + 5, bottom + 7, 18, 18);
        }
        setlinecolor(selected ? Palette::Ink : Palette::Shadow);
        setfillcolor(fill);
        fillroundrect(left, top + offset, right, bottom + offset, 18, 18);
        setUiFont(24, selected);
        settextcolor(Palette::Ink);
        outtextxy((left + right - textwidth(label)) / 2,
            (top + bottom - textheight(label)) / 2 + offset, label);
    }

    void drawKeyCap(int x, int y, int width, const wchar_t* label)
    {
        setlinecolor(Palette::Ink);
        setfillcolor(Palette::White);
        fillroundrect(x, y, x + width, y + 36, 8, 8);
        drawCenteredText(label, x + width / 2, y + 6, 18, Palette::Ink, true);
    }

    void drawBubbleDecoration(int x, int y, int radius, COLORREF outline = Palette::AquaDark)
    {
        setlinecolor(outline);
        setfillcolor(RGB(205, 242, 248));
        fillcircle(x, y, radius);
        setfillcolor(Palette::White);
        solidcircle(x - radius / 3, y - radius / 3, std::max(2, radius / 4));
    }

    void drawHeart(int x, int y, bool filled)
    {
        const COLORREF color = filled ? Palette::Coral : Palette::Shadow;
        setlinecolor(Palette::Ink);
        setfillcolor(color);
        solidcircle(x - 6, y - 3, 7);
        solidcircle(x + 6, y - 3, 7);
        POINT tip[3] = { { x - 13, y }, { x + 13, y }, { x, y + 16 } };
        fillpolygon(tip, 3);
    }

    void drawSmallIcon(int x, int y, PowerUpType type, int value)
    {
        COLORREF color = Palette::Aqua;
        wchar_t symbol[2] = L"B";
        switch (type)
        {
        case PowerUpType::BlastRange: color = Palette::Coral; symbol[0] = L'R'; break;
        case PowerUpType::BubbleCapacity: color = Palette::Aqua; symbol[0] = L'B'; break;
        case PowerUpType::Speed: color = Palette::Honey; symbol[0] = L'S'; break;
        case PowerUpType::Shield: color = Palette::Mint; symbol[0] = L'盾'; break;
        }
        setlinecolor(Palette::Ink);
        setfillcolor(color);
        fillroundrect(x, y, x + 52, y + 48, 12, 12);
        drawCenteredText(symbol, x + 20, y + 7, 18, Palette::Ink, true);
        drawTextAt(std::to_wstring(value), x + 34, y + 24, 16, Palette::Ink, true);
    }

    std::wstring formatTime(float seconds)
    {
        const int totalSeconds = static_cast<int>(seconds);
        std::wostringstream output;
        output << std::setfill(L'0') << std::setw(2) << totalSeconds / 60
            << L":" << std::setw(2) << totalSeconds % 60;
        return output.str();
    }

    CharacterStyle styleFromIndex(int index)
    {
        return static_cast<CharacterStyle>(std::clamp(index, 0, 3));
    }

    bool isMenuScene(SceneState state)
    {
        return state == SceneState::MainMenu || state == SceneState::CharacterSelect
            || state == SceneState::Instructions || state == SceneState::ExitConfirm;
    }
}

const IMAGE* PortraitAtlas::imageFor(PortraitSize size) const
{
    switch (size)
    {
    case PortraitSize::Large: return &largeAtlas_;
    case PortraitSize::Card: return &cardAtlas_;
    case PortraitSize::Hud: return &hudAtlas_;
    }
    return &cardAtlas_;
}

bool PortraitAtlas::load(const std::filesystem::path& imagePath)
{
    const std::wstring path = imagePath.wstring();
    const int largeResult = loadimage(&largeAtlas_, path.c_str(), 660, 363, true);
    const int cardResult = loadimage(&cardAtlas_, path.c_str(), 520, 286, true);
    const int hudResult = loadimage(&hudAtlas_, path.c_str(), 260, 143, true);
    loaded_ = largeResult == 0 && cardResult == 0 && hudResult == 0;
    return loaded_;
}

int PortraitAtlas::width(PortraitSize size) const
{
    return imageFor(size)->getwidth() / 4;
}

int PortraitAtlas::height(PortraitSize size) const
{
    return imageFor(size)->getheight();
}

void PortraitAtlas::draw(CharacterStyle style, PortraitSize size, int x, int y) const
{
    if (!loaded_)
    {
        return;
    }
    const IMAGE* atlas = imageFor(size);
    const int portraitWidth = atlas->getwidth() / 4;
    const int portraitHeight = atlas->getheight();
    const int sourceX = static_cast<int>(style) * portraitWidth;
    putimage(x, y, portraitWidth, portraitHeight, atlas, sourceX, 0, SRCCOPY);
}

AudioManager::AudioManager()
{
    wchar_t executablePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    audioDirectory_ = std::filesystem::path(executablePath).parent_path() / L"assets" / L"audio";
}

AudioManager::~AudioManager()
{
    stopMusic();
    PlaySoundW(nullptr, nullptr, 0);
}

std::filesystem::path AudioManager::assetPath(const wchar_t* filename) const
{
    return audioDirectory_ / filename;
}

void AudioManager::playLoop(const wchar_t* filename, MusicTrack track)
{
    if (currentTrack_ == track)
    {
        if (paused_)
        {
            resumeMusic();
        }
        return;
    }

    stopMusic();
    const std::wstring path = assetPath(filename).wstring();
    const std::wstring openCommand = L"open \"" + path + L"\" type waveaudio alias moe_bgm";
    if (mciSendStringW(openCommand.c_str(), nullptr, 0, nullptr) == 0)
    {
        mciSendStringW(L"setaudio moe_bgm volume to 320", nullptr, 0, nullptr);
        mciSendStringW(L"play moe_bgm repeat", nullptr, 0, nullptr);
        currentTrack_ = track;
        paused_ = false;
    }
}

void AudioManager::playMenuMusic()
{
    playLoop(L"bgm_menu.wav", MusicTrack::Menu);
}

void AudioManager::playGameMusic()
{
    playLoop(L"bgm_game.wav", MusicTrack::Game);
}

void AudioManager::pauseMusic()
{
    if (currentTrack_ != MusicTrack::None && !paused_)
    {
        mciSendStringW(L"pause moe_bgm", nullptr, 0, nullptr);
        paused_ = true;
    }
}

void AudioManager::resumeMusic()
{
    if (currentTrack_ != MusicTrack::None && paused_)
    {
        mciSendStringW(L"resume moe_bgm", nullptr, 0, nullptr);
        paused_ = false;
    }
}

void AudioManager::stopMusic()
{
    if (currentTrack_ != MusicTrack::None)
    {
        mciSendStringW(L"stop moe_bgm", nullptr, 0, nullptr);
        mciSendStringW(L"close moe_bgm", nullptr, 0, nullptr);
    }
    currentTrack_ = MusicTrack::None;
    paused_ = false;
}

void AudioManager::playEffect(const wchar_t* filename) const
{
    const std::filesystem::path path = assetPath(filename);
    PlaySoundW(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

MoeBubbleGame::MoeBubbleGame() = default;

MoeBubbleGame::~MoeBubbleGame()
{
    if (windowOpened_)
    {
        EndBatchDraw();
        closegraph();
    }
}

int MoeBubbleGame::run()
{
    openWindow();
    audio_.playMenuMusic();
    input_.poll();
    auto previousFrame = std::chrono::steady_clock::now();

    while (running_)
    {
        const auto frameStart = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(frameStart - previousFrame).count();
        previousFrame = frameStart;
        deltaTime = std::clamp(deltaTime, 0.0f, 0.05f);

        processWindowMessages();
        input_.poll();
        processInput();
        update(deltaTime);
        render();
        FlushBatchDraw();

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - frameStart).count();
        if (elapsed < 16)
        {
            Sleep(static_cast<DWORD>(16 - elapsed));
        }
    }
    return 0;
}

void MoeBubbleGame::openWindow()
{
    initgraph(GameConfig::WindowWidth, GameConfig::WindowHeight, EW_NOCLOSE);
    windowOpened_ = true;
    SetWindowTextW(GetHWnd(), L"萌泡大作战 - 单人闯关");
    ImmAssociateContext(GetHWnd(), HIMC{});
    setbkcolor(Palette::Paper);
    setbkmode(TRANSPARENT);
    wchar_t executablePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    const std::filesystem::path portraitPath = std::filesystem::path(executablePath).parent_path()
        / L"assets" / L"concepts" / L"chibi_character_concepts.png";
    portraits_.load(portraitPath);
    const std::filesystem::path spritePath = std::filesystem::path(executablePath).parent_path()
        / L"assets" / L"sprites" / L"player_walk_sheet.png";
    player_.loadSpriteSheet(spritePath);
    BeginBatchDraw();
}

void MoeBubbleGame::processWindowMessages()
{
    ExMessage message{};
    while (peekmessage(&message, EX_WINDOW))
    {
        if (message.message == WM_CLOSE)
        {
            running_ = false;
        }
    }
}

void MoeBubbleGame::processInput()
{
    switch (scene_)
    {
    case SceneState::MainMenu: handleMainMenuInput(); break;
    case SceneState::CharacterSelect: handleCharacterSelectInput(); break;
    case SceneState::Instructions: handleInstructionsInput(); break;
    case SceneState::Playing: handlePlayingInput(); break;
    case SceneState::Paused: handlePausedInput(); break;
    case SceneState::LevelClear:
    case SceneState::GameOver:
    case SceneState::Victory: handleResultInput(); break;
    case SceneState::ExitConfirm: handleExitConfirmInput(); break;
    }
}

void MoeBubbleGame::update(float deltaTime)
{
    sceneTime_ += deltaTime;
    effectCooldown_ = std::max(0.0f, effectCooldown_ - deltaTime);

    if (scene_ == SceneState::Playing)
    {
        updatePlaying(deltaTime);
    }
    else
    {
        for (StarParticle& particle : particles_)
        {
            particle.update(deltaTime);
        }
        particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
            [](const StarParticle& particle) { return !particle.active(); }), particles_.end());
    }
}

void MoeBubbleGame::transitionTo(SceneState state)
{
    const SceneState previous = scene_;
    scene_ = state;
    sceneTime_ = 0.0f;

    if (state == SceneState::MainMenu || state == SceneState::CharacterSelect
        || (state == SceneState::Instructions && instructionReturnScene_ == SceneState::MainMenu))
    {
        audio_.playMenuMusic();
    }
    else if (state == SceneState::Playing)
    {
        if (previous == SceneState::Paused)
        {
            audio_.resumeMusic();
        }
        else
        {
            audio_.playGameMusic();
        }
    }
    else if (state == SceneState::Paused || state == SceneState::LevelClear
        || state == SceneState::GameOver || state == SceneState::Victory)
    {
        audio_.pauseMusic();
    }

    if (state == SceneState::LevelClear || state == SceneState::Victory)
    {
        audio_.playEffect(L"sfx_level_clear.wav");
    }
}

void MoeBubbleGame::moveMenuSelection(int& index, int itemCount, int direction)
{
    index = (index + direction + itemCount) % itemCount;
    audio_.playEffect(L"sfx_menu_move.wav");
}

void MoeBubbleGame::handleMainMenuInput()
{
    if (input_.pressed(VK_UP) || input_.pressed('W'))
    {
        moveMenuSelection(menuIndex_, 4, -1);
    }
    if (input_.pressed(VK_DOWN) || input_.pressed('S'))
    {
        moveMenuSelection(menuIndex_, 4, 1);
    }
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE))
    {
        audio_.playEffect(L"sfx_menu_confirm.wav");
        if (menuIndex_ == 0)
        {
            startNewGame();
        }
        else if (menuIndex_ == 1)
        {
            characterIndex_ = static_cast<int>(selectedStyle_);
            transitionTo(SceneState::CharacterSelect);
        }
        else if (menuIndex_ == 2)
        {
            instructionReturnScene_ = SceneState::MainMenu;
            transitionTo(SceneState::Instructions);
        }
        else
        {
            exitReturnScene_ = SceneState::MainMenu;
            exitChoice_ = 0;
            transitionTo(SceneState::ExitConfirm);
        }
    }
    if (input_.pressed(VK_ESCAPE))
    {
        exitReturnScene_ = SceneState::MainMenu;
        exitChoice_ = 0;
        transitionTo(SceneState::ExitConfirm);
    }
}

void MoeBubbleGame::handleCharacterSelectInput()
{
    if (input_.pressed(VK_LEFT) || input_.pressed('A'))
    {
        moveMenuSelection(characterIndex_, 4, -1);
    }
    if (input_.pressed(VK_RIGHT) || input_.pressed('D'))
    {
        moveMenuSelection(characterIndex_, 4, 1);
    }
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE))
    {
        selectedStyle_ = styleFromIndex(characterIndex_);
        audio_.playEffect(L"sfx_menu_confirm.wav");
        transitionTo(SceneState::MainMenu);
    }
    if (input_.pressed(VK_ESCAPE))
    {
        transitionTo(SceneState::MainMenu);
    }
}

void MoeBubbleGame::handleInstructionsInput()
{
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE) || input_.pressed(VK_ESCAPE))
    {
        transitionTo(instructionReturnScene_);
    }
}

void MoeBubbleGame::handlePlayingInput()
{
    if (input_.pressed(VK_SPACE))
    {
        tryPlaceBubble();
    }
    if (input_.pressed('P') || input_.pressed(VK_ESCAPE))
    {
        pauseIndex_ = 0;
        transitionTo(SceneState::Paused);
    }
}

void MoeBubbleGame::handlePausedInput()
{
    if (input_.pressed('P') || input_.pressed(VK_ESCAPE))
    {
        transitionTo(SceneState::Playing);
        return;
    }
    if (input_.pressed(VK_UP) || input_.pressed('W'))
    {
        moveMenuSelection(pauseIndex_, 4, -1);
    }
    if (input_.pressed(VK_DOWN) || input_.pressed('S'))
    {
        moveMenuSelection(pauseIndex_, 4, 1);
    }
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE))
    {
        audio_.playEffect(L"sfx_menu_confirm.wav");
        if (pauseIndex_ == 0)
        {
            transitionTo(SceneState::Playing);
        }
        else if (pauseIndex_ == 1)
        {
            restartCurrentLevel();
        }
        else if (pauseIndex_ == 2)
        {
            instructionReturnScene_ = SceneState::Paused;
            transitionTo(SceneState::Instructions);
        }
        else
        {
            transitionTo(SceneState::MainMenu);
        }
    }
}

void MoeBubbleGame::handleResultInput()
{
    if (input_.pressed(VK_UP) || input_.pressed(VK_LEFT) || input_.pressed('W') || input_.pressed('A'))
    {
        moveMenuSelection(resultIndex_, 2, -1);
    }
    if (input_.pressed(VK_DOWN) || input_.pressed(VK_RIGHT) || input_.pressed('S') || input_.pressed('D'))
    {
        moveMenuSelection(resultIndex_, 2, 1);
    }
    if (input_.pressed('R'))
    {
        startNewGame();
        return;
    }
    if (input_.pressed(VK_ESCAPE))
    {
        transitionTo(SceneState::MainMenu);
        return;
    }
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE))
    {
        audio_.playEffect(L"sfx_menu_confirm.wav");
        if (resultIndex_ == 1)
        {
            transitionTo(SceneState::MainMenu);
        }
        else if (scene_ == SceneState::LevelClear)
        {
            setupLevel(level_ + 1);
            transitionTo(SceneState::Playing);
        }
        else
        {
            startNewGame();
        }
    }
}

void MoeBubbleGame::handleExitConfirmInput()
{
    if (input_.pressed(VK_LEFT) || input_.pressed(VK_RIGHT)
        || input_.pressed(VK_UP) || input_.pressed(VK_DOWN))
    {
        exitChoice_ = 1 - exitChoice_;
        audio_.playEffect(L"sfx_menu_move.wav");
    }
    if (input_.pressed(VK_ESCAPE))
    {
        transitionTo(exitReturnScene_);
    }
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE))
    {
        if (exitChoice_ == 0)
        {
            transitionTo(exitReturnScene_);
        }
        else
        {
            running_ = false;
        }
    }
}

void MoeBubbleGame::startNewGame()
{
    score_ = 0;
    level_ = 1;
    statistics_.reset();
    player_.resetForNewGame(selectedStyle_);
    setupLevel(level_);
    transitionTo(SceneState::Playing);
}

void MoeBubbleGame::setupLevel(int level, bool restoreLevelScore)
{
    if (restoreLevelScore)
    {
        score_ = levelStartScore_;
        statistics_.cratesDestroyed = levelStartCrates_;
        statistics_.powerUpsCollected = levelStartPowerUps_;
        statistics_.enemiesDefeated = levelStartEnemies_;
    }

    level_ = std::clamp(level, 1, GameConfig::TotalLevels);
    map_.generate(level_);
    bubbles_.clear();
    waves_.clear();
    powerUps_.clear();
    particles_.clear();
    enemies_.clear();
    player_.prepareForLevel({ 1, 1 });

    int enemyId = 100;
    const float patrolSpeed = 66.0f + level_ * 8.0f;
    const float hunterSpeed = 72.0f + level_ * 9.0f;
    enemies_.push_back(std::make_unique<PatrolEnemy>(enemyId++, cellCenter({ 11, 13 }), patrolSpeed));
    enemies_.push_back(std::make_unique<PatrolEnemy>(enemyId++, cellCenter({ 1, 13 }), patrolSpeed));
    if (level_ >= 2)
    {
        enemies_.push_back(std::make_unique<HunterEnemy>(enemyId++, cellCenter({ 11, 1 }), hunterSpeed));
    }
    if (level_ >= 3)
    {
        enemies_.push_back(std::make_unique<HunterEnemy>(enemyId++, cellCenter({ 9, 13 }), hunterSpeed + 6.0f));
    }

    if (!restoreLevelScore)
    {
        levelStartScore_ = score_;
        levelStartCrates_ = statistics_.cratesDestroyed;
        levelStartPowerUps_ = statistics_.powerUpsCollected;
        levelStartEnemies_ = statistics_.enemiesDefeated;
    }
    resultIndex_ = 0;
}

void MoeBubbleGame::restartCurrentLevel()
{
    setupLevel(level_, true);
    transitionTo(SceneState::Playing);
}

void MoeBubbleGame::tryPlaceBubble()
{
    if (!player_.canPlaceBubble())
    {
        return;
    }
    const GridPos cell = player_.cell();
    const bool occupied = std::any_of(bubbles_.begin(), bubbles_.end(), [&cell](const WaterBubble& bubble)
    {
        return bubble.active() && bubble.cell() == cell;
    });
    if (!map_.isWalkable(cell) || occupied)
    {
        return;
    }
    bubbles_.emplace_back(cell, player_.id(), player_.blastRange());
    player_.onBubblePlaced();
    audio_.playEffect(L"sfx_bubble_place.wav");
}

void MoeBubbleGame::updatePlaying(float deltaTime)
{
    statistics_.elapsedTime += deltaTime;
    player_.update(deltaTime);
    player_.handleMovement(input_, deltaTime, map_, bubbles_);

    for (WaterBubble& bubble : bubbles_)
    {
        if (bubble.active() && bubble.ownerId() == player_.id())
        {
            bubble.updateOwnerPassage(player_.bounds());
        }
    }

    updateBubbles(deltaTime);
    for (WaterWave& wave : waves_)
    {
        wave.update(deltaTime);
    }
    for (PowerUp& powerUp : powerUps_)
    {
        powerUp.update(deltaTime);
    }
    for (StarParticle& particle : particles_)
    {
        particle.update(deltaTime);
    }
    updateEnemies(deltaTime);
    updateCollisions();
    cleanupInactiveObjects();

    if (scene_ == SceneState::Playing && enemies_.empty())
    {
        completeLevel();
    }
}

void MoeBubbleGame::updateBubbles(float deltaTime)
{
    for (WaterBubble& bubble : bubbles_)
    {
        if (bubble.active())
        {
            bubble.update(deltaTime);
        }
    }

    bool exploded = true;
    while (exploded)
    {
        exploded = false;
        for (std::size_t index = 0; index < bubbles_.size(); ++index)
        {
            if (bubbles_[index].active() && bubbles_[index].readyToExplode())
            {
                explodeBubble(index);
                exploded = true;
            }
        }
    }
}

void MoeBubbleGame::explodeBubble(std::size_t bubbleIndex)
{
    WaterBubble& bubble = bubbles_[bubbleIndex];
    if (!bubble.active())
    {
        return;
    }

    const GridPos origin = bubble.cell();
    const int range = bubble.blastRange();
    const int ownerId = bubble.ownerId();
    bubble.deactivate();
    if (ownerId == player_.id())
    {
        player_.onBubbleExpired();
    }

    waves_.emplace_back(origin);
    createParticles(cellCenter(origin), Palette::Aqua, 7);
    constexpr std::array<GridPos, 4> directions = {
        GridPos{ 0, 1 }, GridPos{ 0, -1 }, GridPos{ 1, 0 }, GridPos{ -1, 0 }
    };

    for (const GridPos& direction : directions)
    {
        for (int distance = 1; distance <= range; ++distance)
        {
            const GridPos cell{ origin.row + direction.row * distance,
                origin.column + direction.column * distance };
            const TileType tile = map_.tileAt(cell);
            if (tile == TileType::SolidWall)
            {
                break;
            }

            waves_.emplace_back(cell);
            for (WaterBubble& other : bubbles_)
            {
                if (other.active() && other.cell() == cell)
                {
                    other.trigger();
                }
            }

            if (tile == TileType::Crate)
            {
                if (map_.destroyCrate(cell))
                {
                    score_ += 20;
                    ++statistics_.cratesDestroyed;
                    createParticles(cellCenter(cell), Palette::Honey, 8);
                    createPowerUp(cell);
                }
                break;
            }
        }
    }

    if (effectCooldown_ <= 0.0f)
    {
        audio_.playEffect(L"sfx_bubble_explode.wav");
        effectCooldown_ = 0.08f;
    }
}

std::vector<GridPos> MoeBubbleGame::collectDangerousCells() const
{
    std::vector<GridPos> danger;
    for (const WaterWave& wave : waves_)
    {
        if (wave.active() && std::find(danger.begin(), danger.end(), wave.cell()) == danger.end())
        {
            danger.push_back(wave.cell());
        }
    }

    constexpr std::array<GridPos, 4> directions = {
        GridPos{ 0, 1 }, GridPos{ 0, -1 }, GridPos{ 1, 0 }, GridPos{ -1, 0 }
    };
    for (const WaterBubble& bubble : bubbles_)
    {
        if (!bubble.active() || bubble.remainingTime() > 0.85f)
        {
            continue;
        }
        danger.push_back(bubble.cell());
        for (const GridPos& direction : directions)
        {
            for (int distance = 1; distance <= bubble.blastRange(); ++distance)
            {
                const GridPos cell{ bubble.cell().row + direction.row * distance,
                    bubble.cell().column + direction.column * distance };
                const TileType tile = map_.tileAt(cell);
                if (tile == TileType::SolidWall)
                {
                    break;
                }
                danger.push_back(cell);
                if (tile == TileType::Crate)
                {
                    break;
                }
            }
        }
    }
    return danger;
}

void MoeBubbleGame::updateEnemies(float deltaTime)
{
    const std::vector<GridPos> danger = collectDangerousCells();
    for (const std::unique_ptr<Enemy>& enemy : enemies_)
    {
        if (enemy->active())
        {
            enemy->updateAI(deltaTime, map_, bubbles_, danger, player_.position(), randomEngine_);
        }
    }
}

void MoeBubbleGame::updateCollisions()
{
    for (const WaterWave& wave : waves_)
    {
        if (!wave.active())
        {
            continue;
        }
        if (player_.active() && intersects(wave.bounds(), player_.bounds()) && player_.takeDamage())
        {
            audio_.playEffect(L"sfx_hit.wav");
            createParticles(player_.position(), Palette::Danger, 10);
            if (!player_.active())
            {
                finishGame(false);
                return;
            }
        }

        for (const std::unique_ptr<Enemy>& enemy : enemies_)
        {
            if (enemy->active() && intersects(wave.bounds(), enemy->bounds()))
            {
                enemy->deactivate();
                score_ += 300;
                ++statistics_.enemiesDefeated;
                createParticles(enemy->position(), Palette::Purple, 12);
            }
        }
    }

    for (const std::unique_ptr<Enemy>& enemy : enemies_)
    {
        if (enemy->active() && player_.active()
            && intersects(enemy->bounds(), player_.bounds()) && player_.takeDamage())
        {
            audio_.playEffect(L"sfx_hit.wav");
            createParticles(player_.position(), Palette::Danger, 8);
            if (!player_.active())
            {
                finishGame(false);
                return;
            }
        }
    }

    for (PowerUp& powerUp : powerUps_)
    {
        if (powerUp.active() && intersects(powerUp.bounds(), player_.bounds()))
        {
            player_.applyPowerUp(powerUp.type());
            powerUp.deactivate();
            score_ += 50;
            ++statistics_.powerUpsCollected;
            audio_.playEffect(L"sfx_power_up.wav");
            createParticles(player_.position(), Palette::Mint, 10);
        }
    }
}

void MoeBubbleGame::createPowerUp(const GridPos& cell)
{
    std::uniform_int_distribution<int> chance(0, 99);
    if (chance(randomEngine_) >= 34)
    {
        return;
    }
    std::uniform_int_distribution<int> type(0, 3);
    powerUps_.emplace_back(cell, static_cast<PowerUpType>(type(randomEngine_)));
}

void MoeBubbleGame::createParticles(const Vec2& position, COLORREF color, int count)
{
    std::uniform_real_distribution<float> angle(0.0f, GameConfig::Pi * 2.0f);
    std::uniform_real_distribution<float> speed(45.0f, 125.0f);
    std::uniform_real_distribution<float> life(0.35f, 0.75f);
    for (int index = 0; index < count; ++index)
    {
        const float direction = angle(randomEngine_);
        const float velocity = speed(randomEngine_);
        particles_.emplace_back(position,
            Vec2{ std::cos(direction) * velocity, std::sin(direction) * velocity },
            color, life(randomEngine_), 5.0f);
    }
}

void MoeBubbleGame::completeLevel()
{
    score_ += 500 * level_ + player_.lives() * 100;
    highScore_ = std::max(highScore_, score_);
    resultIndex_ = 0;
    createParticles({ GameConfig::WindowWidth / 2.0f, 210.0f }, Palette::Honey, 28);
    if (level_ >= GameConfig::TotalLevels)
    {
        transitionTo(SceneState::Victory);
    }
    else
    {
        transitionTo(SceneState::LevelClear);
    }
}

void MoeBubbleGame::finishGame(bool victory)
{
    highScore_ = std::max(highScore_, score_);
    resultIndex_ = 0;
    transitionTo(victory ? SceneState::Victory : SceneState::GameOver);
}

void MoeBubbleGame::cleanupInactiveObjects()
{
    bubbles_.erase(std::remove_if(bubbles_.begin(), bubbles_.end(),
        [](const WaterBubble& bubble) { return !bubble.active(); }), bubbles_.end());
    waves_.erase(std::remove_if(waves_.begin(), waves_.end(),
        [](const WaterWave& wave) { return !wave.active(); }), waves_.end());
    powerUps_.erase(std::remove_if(powerUps_.begin(), powerUps_.end(),
        [](const PowerUp& powerUp) { return !powerUp.active(); }), powerUps_.end());
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
        [](const StarParticle& particle) { return !particle.active(); }), particles_.end());
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
        [](const std::unique_ptr<Enemy>& enemy) { return !enemy->active(); }), enemies_.end());
}

void MoeBubbleGame::render() const
{
    switch (scene_)
    {
    case SceneState::MainMenu:
        drawMainMenu();
        break;
    case SceneState::CharacterSelect:
        drawCharacterSelect();
        break;
    case SceneState::Instructions:
        drawInstructions();
        break;
    case SceneState::Playing:
        drawGameplay();
        break;
    case SceneState::Paused:
        drawGameplay();
        drawPausedOverlay();
        break;
    case SceneState::LevelClear:
        drawGameplay();
        drawLevelClearOverlay();
        break;
    case SceneState::GameOver:
        drawGameplay();
        drawGameOverOverlay();
        break;
    case SceneState::Victory:
        drawBackground();
        drawVictoryOverlay();
        break;
    case SceneState::ExitConfirm:
        if (exitReturnScene_ == SceneState::MainMenu)
        {
            drawMainMenu();
        }
        else
        {
            drawGameplay();
        }
        drawExitConfirmOverlay();
        break;
    }
}

void MoeBubbleGame::drawBackground() const
{
    cleardevice();
    setfillcolor(Palette::Paper);
    solidrectangle(0, 0, GameConfig::WindowWidth, GameConfig::WindowHeight);
    for (int index = 0; index < 11; ++index)
    {
        const int x = 38 + (index * 109) % 900;
        const int y = 52 + (index * 83) % 640;
        const int radius = 6 + index % 4 * 3;
        drawBubbleDecoration(x, y, radius, RGB(159, 205, 215));
    }
    setlinecolor(RGB(235, 224, 209));
    for (int y = 12; y < GameConfig::WindowHeight; y += 18)
    {
        line(0, y, GameConfig::WindowWidth, y + 2);
    }
}

void MoeBubbleGame::drawMainMenu() const
{
    drawBackground();
    drawPanel(24, 20, 166, 58, Palette::White, 16, false);
    drawCenteredText(L"单人闯关", 95, 28, 18, Palette::Ink, true);

    drawCenteredText(L"萌泡大作战", 480, 72, 60, Palette::AquaDark, true);
    drawCenteredText(L"放置水泡 · 打通三关 · 成为泡泡达人", 480, 142, 20, Palette::Ink);
    drawBubbleDecoration(315, 92, 13);
    drawBubbleDecoration(652, 77, 18);
    drawBubbleDecoration(690, 128, 8);

    drawPanel(72, 190, 480, 625, Palette::White);
    if (portraits_.loaded())
    {
        portraits_.draw(selectedStyle_, PortraitSize::Large,
            276 - portraits_.width(PortraitSize::Large) / 2, 198);
    }
    else
    {
        drawChibiCharacter(selectedStyle_, { 276.0f, 356.0f }, 2.65f,
            true, sceneTime_);
    }
    drawCenteredText(styleName(selectedStyle_), 276, 568, 24, styleColor(selectedStyle_), true);
    drawCenteredText(L"当前泡泡搭档", 276, 598, 17, Palette::Ink);

    constexpr std::array<const wchar_t*, 4> labels = {
        L"开始游戏", L"角色选择", L"操作说明", L"退出游戏"
    };
    for (int index = 0; index < static_cast<int>(labels.size()); ++index)
    {
        drawButton(558, 236 + index * 78, 868, 294 + index * 78,
            labels[index], index == menuIndex_, index == 3 ? Palette::Coral : Palette::Aqua);
    }
    drawCenteredText(L"↑ ↓ 选择    Enter 确认    Esc 退出", 713, 588, 18, Palette::Ink);
    drawCenteredText(L"C++ · Visual Studio · EasyX", 480, 680, 16, RGB(108, 127, 137));
}

void MoeBubbleGame::drawCharacterSelect() const
{
    drawBackground();
    drawCenteredText(L"选择你的泡泡搭档", 480, 46, 42, Palette::Ink, true);
    drawCenteredText(L"四位角色能力相同，仅外观与主题色不同", 480, 100, 18, RGB(92, 113, 124));

    for (int index = 0; index < 4; ++index)
    {
        const int left = 30 + index * 232;
        const int right = left + 204;
        const bool selected = index == characterIndex_;
        const CharacterStyle style = styleFromIndex(index);
        if (selected)
        {
            setlinecolor(styleColor(style));
            setfillcolor(styleColor(style));
            fillroundrect(left - 5, 142, right + 5, 574, 26, 26);
        }
        drawPanel(left, 148, right, 568, Palette::White, 22, true);
        if (portraits_.loaded())
        {
            portraits_.draw(style, PortraitSize::Card,
                (left + right - portraits_.width(PortraitSize::Card)) / 2, 168);
        }
        else
        {
            drawChibiCharacter(style, { static_cast<float>((left + right) / 2), 310.0f },
                1.55f, selected, sceneTime_);
        }
        drawCenteredText(styleName(style), (left + right) / 2, 462, 19,
            selected ? styleColor(style) : Palette::Ink, true);

        drawSmallIcon(left + 18, 500, PowerUpType::BubbleCapacity, 1);
        drawSmallIcon(left + 76, 500, PowerUpType::BlastRange, 2);
        drawSmallIcon(left + 134, 500, PowerUpType::Speed, 0);
    }

    drawButton(282, 618, 678, 674, L"确认选择", true, styleColor(styleFromIndex(characterIndex_)));
    drawCenteredText(L"← → 切换    Enter 确认    Esc 返回", 480, 687, 17, Palette::Ink);
}

void MoeBubbleGame::drawInstructions() const
{
    drawBackground();
    drawPanel(54, 38, 906, 682, Palette::White, 28, true);
    drawCenteredText(L"操作说明", 480, 68, 42, Palette::AquaDark, true);
    drawCenteredText(L"单人闯关：淘汰本关所有AI敌人即可前进", 480, 122, 19, Palette::Ink);

    drawTextAt(L"键盘操作", 104, 174, 26, Palette::Ink, true);
    drawKeyCap(112, 228, 38, L"W");
    drawKeyCap(68, 270, 38, L"A");
    drawKeyCap(112, 270, 38, L"S");
    drawKeyCap(156, 270, 38, L"D");
    drawTextAt(L"移动角色", 218, 254, 20, Palette::Ink);
    drawKeyCap(68, 340, 126, L"SPACE");
    drawTextAt(L"放置水泡", 218, 346, 20, Palette::Ink);
    drawKeyCap(68, 410, 126, L"P / Esc");
    drawTextAt(L"暂停游戏", 218, 416, 20, Palette::Ink);

    drawTextAt(L"游戏规则", 508, 174, 26, Palette::Ink, true);
    const std::array<std::wstring, 4> rules = {
        L"1  水泡 2.5 秒后释放十字水浪",
        L"2  固定墙阻挡，木箱被破坏后停止传播",
        L"3  水浪可以连锁触发其他水泡",
        L"4  拾取道具可提升范围、容量、速度和护盾"
    };
    for (int index = 0; index < static_cast<int>(rules.size()); ++index)
    {
        drawPanel(488, 222 + index * 70, 856, 274 + index * 70,
            index % 2 == 0 ? RGB(239, 249, 250) : RGB(249, 242, 235), 14, false);
        drawTextAt(rules[index], 508, 237 + index * 70, 17, Palette::Ink);
    }

    drawSmallIcon(190, 526, PowerUpType::BubbleCapacity, 1);
    drawSmallIcon(286, 526, PowerUpType::BlastRange, 2);
    drawSmallIcon(382, 526, PowerUpType::Speed, 0);
    drawSmallIcon(478, 526, PowerUpType::Shield, 0);
    drawTextAt(L"容量", 188, 580, 17, Palette::Ink);
    drawTextAt(L"范围", 284, 580, 17, Palette::Ink);
    drawTextAt(L"速度", 380, 580, 17, Palette::Ink);
    drawTextAt(L"护盾", 476, 580, 17, Palette::Ink);
    drawCenteredText(L"按 Enter 或 Esc 返回", 480, 632, 20, Palette::AquaDark, true);
}

void MoeBubbleGame::drawGameplay() const
{
    cleardevice();
    setfillcolor(Palette::Paper);
    solidrectangle(0, 0, GameConfig::WindowWidth, GameConfig::WindowHeight);

    drawPanel(24, 16, 684, 64, Palette::White, 16, true);
    drawTextAt(L"第 " + std::to_wstring(level_) + L" / 3 关", 44, 27, 20, Palette::Ink, true);
    drawCenteredText(L"剩余敌人  " + std::to_wstring(enemies_.size()), 352, 27, 20, Palette::PurpleDark, true);
    const std::wstring scoreText = L"分数  " + std::to_wstring(score_);
    setUiFont(20, true);
    drawTextAt(scoreText, 664 - textwidth(scoreText.c_str()), 27, 20, Palette::Honey, true);

    map_.draw();
    for (const PowerUp& powerUp : powerUps_) powerUp.draw();
    for (const WaterBubble& bubble : bubbles_) bubble.draw();
    for (const std::unique_ptr<Enemy>& enemy : enemies_) enemy->draw();
    if (player_.active()) player_.draw();
    for (const WaterWave& wave : waves_) wave.draw();
    for (const StarParticle& particle : particles_) particle.draw();

    drawHud();
    drawPanel(24, 660, 684, 702, Palette::White, 12, false);
    drawCenteredText(L"WASD / 方向键移动     Space 放泡     P / Esc 暂停",
        354, 670, 17, Palette::Ink);
}

void MoeBubbleGame::drawHud() const
{
    drawPanel(GameConfig::HudLeft, GameConfig::HudTop,
        GameConfig::HudLeft + GameConfig::HudWidth,
        GameConfig::HudTop + GameConfig::HudHeight, Palette::White, 24, true);

    if (portraits_.loaded())
    {
        portraits_.draw(player_.style(), PortraitSize::Hud,
            822 - portraits_.width(PortraitSize::Hud) / 2, 36);
    }
    else
    {
        drawChibiCharacter(player_.style(), { 822.0f, 115.0f }, 1.05f, false, sceneTime_);
    }
    drawCenteredText(styleName(player_.style()), 822, 182, 19, styleColor(player_.style()), true);
    drawTextAt(L"生命", 730, 220, 18, Palette::Ink, true);
    for (int index = 0; index < 3; ++index)
    {
        drawHeart(800 + index * 38, 231, index < player_.lives());
    }

    drawTextAt(L"强化状态", 730, 274, 18, Palette::Ink, true);
    drawSmallIcon(730, 306, PowerUpType::BubbleCapacity, player_.bubbleCapacity());
    drawSmallIcon(794, 306, PowerUpType::BlastRange, player_.blastRange());
    drawSmallIcon(858, 306, PowerUpType::Speed, player_.speedLevel());
    drawSmallIcon(730, 364, PowerUpType::Shield, player_.shieldCharges());

    drawPanel(730, 432, 914, 508, RGB(239, 249, 250), 14, false);
    drawCenteredText(L"本关目标", 822, 442, 17, Palette::AquaDark, true);
    drawCenteredText(L"淘汰全部AI敌人", 822, 472, 17, Palette::Ink);

    drawTextAt(L"累计数据", 730, 536, 18, Palette::Ink, true);
    drawTextAt(L"木箱  " + std::to_wstring(statistics_.cratesDestroyed), 734, 568, 16, Palette::Ink);
    drawTextAt(L"道具  " + std::to_wstring(statistics_.powerUpsCollected), 734, 596, 16, Palette::Ink);
    drawTextAt(L"用时  " + formatTime(statistics_.elapsedTime), 734, 624, 16, Palette::Ink);
    drawCenteredText(L"最高分  " + std::to_wstring(std::max(highScore_, score_)), 822, 660, 16, Palette::Honey, true);
}

void MoeBubbleGame::drawPausedOverlay() const
{
    setlinecolor(RGB(104, 119, 126));
    for (int y = 0; y < GameConfig::WindowHeight; y += 8)
    {
        line(0, y, GameConfig::WindowWidth, y);
    }
    drawPanel(270, 120, 690, 610, Palette::Paper, 28, true);
    drawCenteredText(L"游戏暂停", 480, 154, 40, Palette::AquaDark, true);
    drawCenteredText(L"P / Esc 可立即继续", 480, 208, 17, Palette::Ink);
    constexpr std::array<const wchar_t*, 4> labels = {
        L"继续游戏", L"重新开始本关", L"操作说明", L"返回主菜单"
    };
    for (int index = 0; index < 4; ++index)
    {
        drawButton(330, 260 + index * 72, 630, 314 + index * 72,
            labels[index], index == pauseIndex_, index == 3 ? Palette::Coral : Palette::Aqua);
    }
}

void MoeBubbleGame::drawLevelClearOverlay() const
{
    drawPanel(220, 112, 740, 610, Palette::Paper, 30, true);
    drawCenteredText(L"关卡完成！", 480, 148, 46, Palette::AquaDark, true);
    for (int index = 0; index < 3; ++index)
    {
        const COLORREF starColor = index < std::min(3, player_.lives()) ? Palette::Honey : Palette::Shadow;
        setlinecolor(Palette::Ink);
        setfillcolor(starColor);
        POINT points[10]{};
        for (int point = 0; point < 10; ++point)
        {
            const float angle = -GameConfig::Pi / 2.0f + point * GameConfig::Pi / 5.0f;
            const float radius = point % 2 == 0 ? 32.0f : 14.0f;
            points[point] = { 400 + index * 80 + pixel(std::cos(angle) * radius),
                242 + pixel(std::sin(angle) * radius) };
        }
        fillpolygon(points, 10);
    }
    drawCenteredText(L"当前分数", 480, 304, 18, Palette::Ink);
    drawCenteredText(std::to_wstring(score_), 480, 334, 42, Palette::Honey, true);
    drawCenteredText(L"剩余生命  " + std::to_wstring(player_.lives())
        + L"    累计道具  " + std::to_wstring(statistics_.powerUpsCollected),
        480, 392, 18, Palette::Ink);
    drawButton(292, 458, 668, 514, L"进入下一关", resultIndex_ == 0, Palette::Aqua);
    drawButton(292, 528, 668, 584, L"返回主菜单", resultIndex_ == 1, Palette::Coral);
}

void MoeBubbleGame::drawGameOverOverlay() const
{
    drawPanel(220, 110, 740, 612, Palette::Paper, 30, true);
    drawCenteredText(L"挑战失败", 480, 146, 46, Palette::Coral, true);
    if (portraits_.loaded())
    {
        portraits_.draw(player_.style(), PortraitSize::Hud,
            400 - portraits_.width(PortraitSize::Hud) / 2, 216);
    }
    else
    {
        drawChibiCharacter(player_.style(), { 400.0f, 290.0f }, 1.45f, false, sceneTime_);
    }
    drawBubbleDecoration(566, 288, 34, Palette::Danger);
    drawCenteredText(L"别灰心，调整路线再来一次吧！", 480, 374, 20, Palette::Ink);
    drawCenteredText(L"最终分数  " + std::to_wstring(score_)
        + L"    到达第 " + std::to_wstring(level_) + L" 关", 480, 416, 20, Palette::Honey, true);
    drawButton(292, 468, 668, 524, L"重新挑战", resultIndex_ == 0, Palette::Aqua);
    drawButton(292, 538, 668, 594, L"返回主菜单", resultIndex_ == 1, Palette::Coral);
}

void MoeBubbleGame::drawVictoryOverlay() const
{
    for (const StarParticle& particle : particles_) particle.draw();
    drawCenteredText(L"全部通关！", 480, 62, 58, Palette::AquaDark, true);
    drawCenteredText(L"你已经成为真正的泡泡达人", 480, 132, 21, Palette::Ink);
    const std::array<CharacterStyle, 4> styles = {
        CharacterStyle::Bear, CharacterStyle::Rabbit, CharacterStyle::Cat, CharacterStyle::Dog
    };
    for (int index = 0; index < 4; ++index)
    {
        if (portraits_.loaded())
        {
            portraits_.draw(styles[index], PortraitSize::Hud,
                240 + index * 160 - portraits_.width(PortraitSize::Hud) / 2, 212);
        }
        else
        {
            drawChibiCharacter(styles[index], { 240.0f + index * 160.0f, 322.0f },
                styles[index] == selectedStyle_ ? 1.7f : 1.45f, true, sceneTime_ + index * 0.2f);
        }
    }
    drawPanel(212, 444, 748, 540, Palette::White, 18, true);
    drawCenteredText(L"最终分数  " + std::to_wstring(score_)
        + L"    总用时  " + formatTime(statistics_.elapsedTime), 480, 462, 21, Palette::Honey, true);
    drawCenteredText(L"木箱 " + std::to_wstring(statistics_.cratesDestroyed)
        + L"    道具 " + std::to_wstring(statistics_.powerUpsCollected)
        + L"    敌人 " + std::to_wstring(statistics_.enemiesDefeated), 480, 503, 18, Palette::Ink);
    drawButton(248, 580, 468, 638, L"再玩一次", resultIndex_ == 0, Palette::Aqua);
    drawButton(492, 580, 712, 638, L"返回主菜单", resultIndex_ == 1, Palette::Coral);
    drawCenteredText(L"R 可快速重新开始", 480, 662, 17, Palette::Ink);
}

void MoeBubbleGame::drawExitConfirmOverlay() const
{
    setlinecolor(RGB(115, 125, 130));
    for (int y = 0; y < GameConfig::WindowHeight; y += 9)
    {
        line(0, y, GameConfig::WindowWidth, y);
    }
    drawPanel(260, 206, 700, 506, Palette::Paper, 26, true);
    drawCenteredText(L"确定要退出游戏吗？", 480, 246, 32, Palette::Ink, true);
    drawCenteredText(L"未完成的本关进度不会保存", 480, 300, 18, Palette::Danger);
    drawButton(300, 364, 474, 424, L"继续玩", exitChoice_ == 0, Palette::Aqua);
    drawButton(492, 364, 666, 424, L"退出游戏", exitChoice_ == 1, Palette::Coral);
    drawCenteredText(L"← → 选择    Enter 确认    Esc 返回", 480, 458, 17, Palette::Ink);
}
