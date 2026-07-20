#include "MoeBubbleGame.h"

#include <imm.h>
#include <mmsystem.h>

#include <iomanip>
#include <sstream>

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "winmm.lib")

namespace
{
    constexpr PlayerControls SinglePlayerControls{
        'W', 'S', 'A', 'D', VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT
    };
    constexpr PlayerControls PlayerOneControls{ 'W', 'S', 'A', 'D' };
    constexpr PlayerControls PlayerTwoControls{ VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT };
    constexpr int MainMenuItemCount = 5;
    constexpr int MainMenuButtonTop = 214;
    constexpr int MainMenuButtonStep = 68;
    constexpr int MainMenuButtonHeight = 54;

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

    void drawSmallIcon(const ItemIconAtlas& icons, int x, int y, PowerUpType type, int value)
    {
        // 角色卡和说明页空间较紧，保留图标与角标叠放的紧凑版本；
        // 游戏 HUD 使用下方的状态卡版本，避免两种使用场景互相牵制尺寸。
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
        setfillcolor(RGB(255, 251, 239));
        fillroundrect(x, y, x + 52, y + 48, 12, 12);
        if (icons.loaded())
        {
            icons.draw(type, x + 22, y + 22, 40);
        }
        else
        {
            setfillcolor(color);
            solidcircle(x + 21, y + 22, 16);
            drawCenteredText(symbol, x + 21, y + 12, 16, Palette::Ink, true);
        }
        const std::wstring valueText = std::to_wstring(value);
        setlinecolor(Palette::Ink);
        setfillcolor(Palette::White);
        fillroundrect(x + 26, y + 27, x + 52, y + 48, 8, 8);
        drawCenteredText(valueText, x + 39, y + (value >= 100 ? 31 : 29),
            value >= 100 ? 11 : 14, Palette::Ink, true);
    }

    // HUD 使用独立的等宽状态卡。图标、中文名称和数值各占一个垂直区域，
    // 四张卡可以稳定排在 228 像素侧栏内，属性值变化时不会挤压相邻内容。
    void drawHudStatusCard(const ItemIconAtlas& icons, int x, int y,
        PowerUpType type, int value)
    {
        constexpr int CardWidth = 42;
        constexpr int CardHeight = 88;
        constexpr int IconSize = 34;

        const wchar_t* label = L"容量";
        wchar_t fallbackSymbol[2] = L"B";
        COLORREF accent = Palette::Aqua;
        switch (type)
        {
        case PowerUpType::BlastRange:
            label = L"范围";
            fallbackSymbol[0] = L'R';
            accent = Palette::Coral;
            break;
        case PowerUpType::BubbleCapacity:
            label = L"容量";
            fallbackSymbol[0] = L'B';
            accent = Palette::Aqua;
            break;
        case PowerUpType::Speed:
            label = L"速度";
            fallbackSymbol[0] = L'S';
            accent = Palette::Honey;
            break;
        case PowerUpType::Shield:
            label = L"护盾";
            fallbackSymbol[0] = L'盾';
            accent = Palette::Mint;
            break;
        }

        // 边框沿用属性主题色，浅色底板保证透明水彩图标在不同场景下都有对比度。
        setlinecolor(accent);
        setfillcolor(RGB(255, 251, 239));
        fillroundrect(x, y, x + CardWidth, y + CardHeight, 10, 10);

        if (icons.loaded())
        {
            icons.draw(type, x + CardWidth / 2, y + 22, IconSize);
        }
        else
        {
            setfillcolor(accent);
            solidcircle(x + CardWidth / 2, y + 22, 15);
            drawCenteredText(fallbackSymbol, x + CardWidth / 2, y + 13,
                15, Palette::Ink, true);
        }

        drawCenteredText(label, x + CardWidth / 2, y + 43, 13, Palette::Ink, true);

        // 数值固定在底部胶囊中；三位数只缩小字体，不改变卡片几何尺寸，
        // 因而容量、范围、速度和护盾始终保持相同的视觉重量。
        setlinecolor(accent);
        setfillcolor(RGB(244, 249, 246));
        fillroundrect(x + 4, y + 64, x + CardWidth - 4, y + 84, 7, 7);
        drawCenteredText(std::to_wstring(value), x + CardWidth / 2,
            y + (value >= 100 ? 67 : 65), value >= 100 ? 11 : 14,
            Palette::Ink, true);
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
    // 三种尺寸分别在加载阶段完成缩放；这里仅返回对应缓存，调用方无需
    // 了解 IMAGE 的存储方式，也不会在绘制循环中临时创建图片。
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
    // 同一原图加载成三种目标尺寸，绘制时只做裁切，避免每帧重复缩放。
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
    // 四名角色在图集中横向等宽排列，枚举值可以直接换算为源矩形左边界。
    const int sourceX = static_cast<int>(style) * portraitWidth;
    putimage(x, y, portraitWidth, portraitHeight, atlas, sourceX, 0, SRCCOPY);
}

AudioManager::AudioManager()
{
    // 以 EXE 所在目录为基准拼接资源，工程移动到其他电脑仍能找到音频。
    wchar_t executablePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    audioDirectory_ = std::filesystem::path(executablePath).parent_path() / L"assets" / L"audio";
}

AudioManager::~AudioManager()
{
    // 显式关闭 MCI 别名和异步音效，防止窗口退出后系统仍占用 wav 文件。
    stopMusic();
    PlaySoundW(nullptr, nullptr, 0);
}

std::filesystem::path AudioManager::assetPath(const wchar_t* filename) const
{
    return audioDirectory_ / filename;
}

void AudioManager::playLoop(const wchar_t* filename, MusicTrack track)
{
    // 已在播放同一首音乐时不重复 open；MCI 重复打开同一别名会返回错误，
    // 也会让菜单与暂停界面切换时出现短暂断音。
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
    // 固定别名 moe_bgm 让暂停、恢复和关闭操作不必重复保存完整文件路径。
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
    // 短音效异步播放，不阻塞约 60 FPS 的游戏主循环；资源缺失时静默跳过。
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

    // 游戏只保留一个主循环。场景之间通过 scene_ 切换，不递归进入新的循环，
    // 因此暂停、重开和返回菜单都不会积累额外调用栈。
    while (running_)
    {
        const auto frameStart = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(frameStart - previousFrame).count();
        previousFrame = frameStart;
        // 窗口拖动或断点会产生过大的帧间隔，限幅可防止对象一步穿墙。
        deltaTime = std::clamp(deltaTime, 0.0f, 0.05f);

        // 每帧严格执行：窗口消息 → 键盘状态 → 场景输入 → 更新 → 绘制。
        processWindowMessages();
        input_.poll();
        processInput();
        update(deltaTime);
        render();
        FlushBatchDraw();

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - frameStart).count();
        // 计算本帧实际耗时后补足 16 ms，降低空闲 CPU 占用并稳定动画速度。
        if (elapsed < 16)
        {
            Sleep(static_cast<DWORD>(16 - elapsed));
        }
    }
    return 0;
}

int MoeBubbleGame::captureReportScreenshots(const std::filesystem::path& outputDirectory)
{
    // 确定性布置几个代表场景，仍调用正式 render()，保证报告图与提交程序一致。
    std::filesystem::create_directories(outputDirectory);
    openWindow();

    auto capture = [&](const wchar_t* filename)
    {
        // 截图前强制刷新双缓冲，确保 saveimage 取得的是完整的一帧而非上帧残留。
        render();
        FlushBatchDraw();
        const std::filesystem::path path = outputDirectory / filename;
        saveimage(path.c_str());
    };

    scene_ = SceneState::MainMenu;
    sceneTime_ = 0.8f;
    capture(L"game_main_menu.png");

    scene_ = SceneState::CharacterSelect;
    characterIndex_ = static_cast<int>(CharacterStyle::Cat);
    capture(L"game_character_select.png");

    selectedStyle_ = CharacterStyle::Bear;
    startNewGame();
    player_.update(1.3f);
    powerUps_.clear();
    powerUps_.emplace_back(GridPos{ 1, 3 }, PowerUpType::BlastRange, &itemIcons_);
    powerUps_.emplace_back(GridPos{ 1, 5 }, PowerUpType::BubbleCapacity, &itemIcons_);
    powerUps_.emplace_back(GridPos{ 1, 7 }, PowerUpType::Speed, &itemIcons_);
    powerUps_.emplace_back(GridPos{ 1, 9 }, PowerUpType::Shield, &itemIcons_);
    capture(L"game_level1_items.png");

    gameMode_ = GameMode::TwoPlayer;
    selectedStyle_ = CharacterStyle::Bear;
    selectedStyle2_ = CharacterStyle::Rabbit;
    startNewGame();
    player_.update(1.3f);
    player2_.update(1.3f);
    capture(L"game_coop_mode.png");

    gameMode_ = GameMode::SinglePlayer;
    player2_.deactivate();
    setupLevel(3);
    player_.update(1.3f);
    scene_ = SceneState::Playing;
    capture(L"game_level3_boss.png");

    scene_ = SceneState::Victory;
    score_ = 8650;
    statistics_.cratesDestroyed = 37;
    statistics_.powerUpsCollected = 11;
    statistics_.enemiesDefeated = 9;
    statistics_.elapsedTime = 287.0f;
    capture(L"game_victory.png");
    return 0;
}

void MoeBubbleGame::openWindow()
{
    // EW_NOCLOSE 让关闭请求也进入统一消息处理，避免系统直接销毁窗口导致资源未释放。
    initgraph(GameConfig::WindowWidth, GameConfig::WindowHeight, EW_NOCLOSE);
    windowOpened_ = true;
    SetWindowTextW(GetHWnd(), L"萌泡大作战 - 单人 / 本地双人合作");
    ImmAssociateContext(GetHWnd(), HIMC{});
    setbkcolor(Palette::Paper);
    setbkmode(TRANSPARENT);
    // 图片与音频都从 EXE 目录向下定位；复制整个输出目录后仍能直接运行，
    // 也避免开发机盘符或工程位置进入资源路径。
    wchar_t executablePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    const std::filesystem::path portraitPath = std::filesystem::path(executablePath).parent_path()
        / L"assets" / L"concepts" / L"chibi_character_concepts.png";
    portraits_.load(portraitPath);
    const std::filesystem::path spritePath = std::filesystem::path(executablePath).parent_path()
        / L"assets" / L"sprites" / L"player_walk_sheet.png";
    player_.loadSpriteSheet(spritePath);
    player2_.loadSpriteSheet(spritePath);
    const std::filesystem::path itemIconPath = std::filesystem::path(executablePath).parent_path()
        / L"assets" / L"ui" / L"item_icons_v1.png";
    itemIcons_.load(itemIconPath);
    const std::filesystem::path enemySpritePath = std::filesystem::path(executablePath).parent_path()
        / L"assets" / L"sprites" / L"enemy_sprites_v1.png";
    enemySprites_.load(enemySpritePath);
    // 后续所有场景先画到后台缓冲，再由主循环一次性提交，减少 EasyX 闪烁。
    BeginBatchDraw();
}

void MoeBubbleGame::processWindowMessages()
{
    // 鼠标点击是本帧边沿信号，每帧先清零；悬停坐标则保留最近位置。
    mouseLeftPressed_ = false;
    ExMessage message{};
    // 一次取空本帧积压的鼠标和窗口消息，避免快速移动鼠标时坐标明显滞后。
    while (peekmessage(&message, EX_MOUSE | EX_WINDOW))
    {
        if (message.message >= WM_MOUSEFIRST && message.message <= WM_MOUSELAST)
        {
            mouseX_ = message.x;
            mouseY_ = message.y;
            if (message.message == WM_LBUTTONDOWN)
            {
                mouseLeftPressed_ = true;
            }
        }
        if (message.message == WM_CLOSE)
        {
            running_ = false;
        }
    }
}

void MoeBubbleGame::processInput()
{
    // 状态机将输入分派到当前场景，未激活界面不会误响应按键或鼠标。
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
    // sceneTime_ 为界面入场动画和提示闪烁提供统一时间基准；爆炸音效冷却
    // 则限制连锁水泡在同一瞬间反复启动同一个异步声音。
    sceneTime_ += deltaTime;
    effectCooldown_ = std::max(0.0f, effectCooldown_ - deltaTime);

    if (scene_ == SceneState::Playing)
    {
        updatePlaying(deltaTime);
    }
    else
    {
        // 非战斗场景停止游戏对象，只保留短暂结算粒子继续播放。
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
    // 所有场景切换统一处理音乐与场景计时，避免各按钮重复维护状态。
    const SceneState previous = scene_;
    scene_ = state;
    sceneTime_ = 0.0f;

    // 菜单类场景共用轻音乐，战斗场景使用关卡音乐，覆盖层只暂停当前曲目。
    // 统一在此处处理可保证鼠标、键盘和自动过关触发的切换行为一致。
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
    // 加上 itemCount 后再取模，使向上越过第 0 项时可以安全循环到最后一项。
    index = (index + direction + itemCount) % itemCount;
    audio_.playEffect(L"sfx_menu_move.wav");
}

bool MoeBubbleGame::mouseInside(int left, int top, int right, int bottom) const
{
    return mouseX_ >= left && mouseX_ <= right && mouseY_ >= top && mouseY_ <= bottom;
}

bool MoeBubbleGame::mouseSelects(int& index, int candidate,
    int left, int top, int right, int bottom)
{
    if (!mouseInside(left, top, right, bottom))
    {
        return false;
    }
    if (index != candidate)
    {
        // 悬停只改变当前高亮并播放移动音，真正执行按钮仍要求本帧左键按下。
        index = candidate;
        audio_.playEffect(L"sfx_menu_move.wav");
    }
    return mouseLeftPressed_;
}

void MoeBubbleGame::handleMainMenuInput()
{
    // 鼠标热区与绘制按钮使用同一组纵向间距，clicked 只记录是否点击了任意一项。
    bool clicked = false;
    for (int index = 0; index < MainMenuItemCount; ++index)
    {
        const int top = MainMenuButtonTop + index * MainMenuButtonStep;
        if (mouseSelects(menuIndex_, index, 558, top, 868, top + MainMenuButtonHeight))
        {
            clicked = true;
        }
    }
    if (input_.pressed(VK_UP) || input_.pressed('W'))
    {
        moveMenuSelection(menuIndex_, MainMenuItemCount, -1);
    }
    if (input_.pressed(VK_DOWN) || input_.pressed('S'))
    {
        moveMenuSelection(menuIndex_, MainMenuItemCount, 1);
    }
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE) || clicked)
    {
        audio_.playEffect(L"sfx_menu_confirm.wav");
        if (menuIndex_ == 0)
        {
            gameMode_ = GameMode::SinglePlayer;
            startNewGame();
        }
        else if (menuIndex_ == 1)
        {
            // 双人模式依次选择两名角色，第二名确认后直接进入合作关卡。
            gameMode_ = GameMode::TwoPlayer;
            coopCharacterSelection_ = true;
            selectingPlayer_ = 1;
            characterIndex_ = static_cast<int>(selectedStyle_);
            transitionTo(SceneState::CharacterSelect);
        }
        else if (menuIndex_ == 2)
        {
            coopCharacterSelection_ = false;
            selectingPlayer_ = 1;
            characterIndex_ = static_cast<int>(selectedStyle_);
            transitionTo(SceneState::CharacterSelect);
        }
        else if (menuIndex_ == 3)
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
    // 移入角色卡即可预览属性；确认后才写入 selectedStyle_，取消不会改变原选择。
    for (int index = 0; index < 4; ++index)
    {
        const int left = 30 + index * 232;
        mouseSelects(characterIndex_, index, left, 148, left + 204, 600);
    }
    if (input_.pressed(VK_LEFT) || input_.pressed('A'))
    {
        moveMenuSelection(characterIndex_, 4, -1);
    }
    if (input_.pressed(VK_RIGHT) || input_.pressed('D'))
    {
        moveMenuSelection(characterIndex_, 4, 1);
    }
    const bool confirmClicked = mouseLeftPressed_ && mouseInside(282, 618, 678, 674);
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE) || confirmClicked)
    {
        audio_.playEffect(L"sfx_menu_confirm.wav");
        if (!coopCharacterSelection_)
        {
            selectedStyle_ = styleFromIndex(characterIndex_);
            transitionTo(SceneState::MainMenu);
        }
        else if (selectingPlayer_ == 1)
        {
            selectedStyle_ = styleFromIndex(characterIndex_);
            selectingPlayer_ = 2;
            characterIndex_ = static_cast<int>(selectedStyle2_);
        }
        else
        {
            selectedStyle2_ = styleFromIndex(characterIndex_);
            coopCharacterSelection_ = false;
            startNewGame();
        }
    }
    if (input_.pressed(VK_ESCAPE))
    {
        transitionTo(SceneState::MainMenu);
    }
}

void MoeBubbleGame::handleInstructionsInput()
{
    // 说明页既可从主菜单进入，也可从暂停菜单进入，返回目标由进入前保存的状态决定。
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE) || input_.pressed(VK_ESCAPE)
        || (mouseLeftPressed_ && mouseInside(330, 612, 630, 666)))
    {
        transitionTo(instructionReturnScene_);
    }
}

void MoeBubbleGame::handlePlayingInput()
{
    // P1 使用 WASD + Space，P2 使用方向键 + Enter；放泡按键只读取一次性边沿。
    if (input_.pressed(VK_SPACE))
    {
        tryPlaceBubble(player_);
    }
    if (twoPlayerMode() && input_.pressed(VK_RETURN))
    {
        tryPlaceBubble(player2_);
    }
    if (input_.pressed('P') || input_.pressed(VK_ESCAPE))
    {
        pauseIndex_ = 0;
        transitionTo(SceneState::Paused);
    }
}

void MoeBubbleGame::handlePausedInput()
{
    // P/Esc 是快速恢复通道；下方菜单同时支持键盘循环选择和鼠标悬停点击。
    if (input_.pressed('P') || input_.pressed(VK_ESCAPE))
    {
        transitionTo(SceneState::Playing);
        return;
    }
    bool clicked = false;
    for (int index = 0; index < 4; ++index)
    {
        if (mouseSelects(pauseIndex_, index, 330, 260 + index * 72, 630, 314 + index * 72))
        {
            clicked = true;
        }
    }
    if (input_.pressed(VK_UP) || input_.pressed('W'))
    {
        moveMenuSelection(pauseIndex_, 4, -1);
    }
    if (input_.pressed(VK_DOWN) || input_.pressed('S'))
    {
        moveMenuSelection(pauseIndex_, 4, 1);
    }
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE) || clicked)
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
    // 胜利页采用横排按钮，过关/失败页采用竖排按钮，因此鼠标热区分别计算。
    bool clicked = false;
    if (scene_ == SceneState::Victory)
    {
        clicked = mouseSelects(resultIndex_, 0, 248, 580, 468, 638);
        if (mouseSelects(resultIndex_, 1, 492, 580, 712, 638)) clicked = true;
    }
    else
    {
        const int firstTop = scene_ == SceneState::LevelClear ? 458 : 468;
        clicked = mouseSelects(resultIndex_, 0, 292, firstTop, 668, firstTop + 56);
        const int secondTop = scene_ == SceneState::LevelClear ? 528 : 538;
        if (mouseSelects(resultIndex_, 1, 292, secondTop, 668, secondTop + 56)) clicked = true;
    }
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
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE) || clicked)
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
    // exitReturnScene_ 记录弹窗下方的原场景，选择“取消”时可原路返回。
    bool clicked = mouseSelects(exitChoice_, 0, 300, 364, 474, 424);
    if (mouseSelects(exitChoice_, 1, 492, 364, 666, 424)) clicked = true;
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
    if (input_.pressed(VK_RETURN) || input_.pressed(VK_SPACE) || clicked)
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
    // 新游戏清空跨关累计值；角色配置只在这里装载，进入下一关不会重置强化与生命。
    score_ = 0;
    level_ = 1;
    statistics_.reset();
    player_.resetForNewGame(selectedStyle_);
    if (twoPlayerMode())
    {
        player2_.resetForNewGame(selectedStyle2_);
    }
    else
    {
        player2_.deactivate();
    }
    setupLevel(level_);
    transitionTo(SceneState::Playing);
}

void MoeBubbleGame::setupLevel(int level, bool restoreLevelScore)
{
    // 重开本关时回滚分数与统计；进入新关时保留累计结果并刷新对象容器。
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
    if (twoPlayerMode())
    {
        player2_.prepareForLevel({ 1, 2 });
    }

    // 敌人编号从 100 开始，与玩家编号 1 分离；水泡据此判断其所有者能否离开出生格。
    int enemyId = 100;
    const float patrolSpeed = 66.0f + level_ * 8.0f;
    const float hunterSpeed = 72.0f + level_ * 9.0f;
    // 不同派生敌人统一放入 vector<unique_ptr<Enemy>>，由虚函数决定实际行为。
    enemies_.push_back(std::make_unique<PatrolEnemy>(
        enemyId++, cellCenter({ 11, 13 }), patrolSpeed, &enemySprites_));
    enemies_.push_back(std::make_unique<PatrolEnemy>(
        enemyId++, cellCenter({ 1, 13 }), patrolSpeed, &enemySprites_));
    if (level_ >= 2)
    {
        enemies_.push_back(std::make_unique<HunterEnemy>(
            enemyId++, cellCenter({ 11, 1 }), hunterSpeed, &enemySprites_));
    }
    if (level_ >= 3)
    {
        enemies_.push_back(std::make_unique<BossEnemy>(enemyId++, cellCenter({ 9, 13 }),
            hunterSpeed - 8.0f, GameConfig::BossMaxHealth, &enemySprites_));
    }

    if (!restoreLevelScore)
    {
        // 记录关卡入口快照。暂停菜单选择重开时恢复这些累计值，避免重复刷分。
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

void MoeBubbleGame::tryPlaceBubble(Player& player)
{
    if (!player.active() || !player.canPlaceBubble())
    {
        return;
    }
    const GridPos cell = player.cell();
    if (twoPlayerMode())
    {
        const Player& teammate = player.id() == player_.id() ? player2_ : player_;
        // 两人站在同一格时禁止放泡，避免非所有者被新水泡立即困在格子内部。
        if (teammate.active() && teammate.cell() == cell)
        {
            return;
        }
    }
    // 同一网格只能存在一个有效水泡；检查使用 STL 算法，不暴露容器内部索引。
    const bool occupied = std::any_of(bubbles_.begin(), bubbles_.end(), [&cell](const WaterBubble& bubble)
    {
        return bubble.active() && bubble.cell() == cell;
    });
    if (!map_.isWalkable(cell) || occupied)
    {
        return;
    }
    bubbles_.emplace_back(cell, player.id(), player.blastRange());
    player.onBubblePlaced();
    audio_.playEffect(L"sfx_bubble_place.wav");
}

void MoeBubbleGame::updatePlaying(float deltaTime)
{
    // 更新顺序刻意固定：先移动玩家，再推进定时对象和 AI，最后统一碰撞与清理。
    // 这样本帧新产生的水浪可以立即命中，而失效对象不会在遍历中被删除。
    statistics_.elapsedTime += deltaTime;
    if (player_.active())
    {
        player_.update(deltaTime);
        player_.handleMovement(input_, deltaTime, map_, bubbles_,
            twoPlayerMode() ? PlayerOneControls : SinglePlayerControls);
    }
    if (twoPlayerMode() && player2_.active())
    {
        player2_.update(deltaTime);
        player2_.handleMovement(input_, deltaTime, map_, bubbles_, PlayerTwoControls);
    }

    for (WaterBubble& bubble : bubbles_)
    {
        // 只有所有者需要更新“离开后不可穿回”的一次性通行状态。
        if (bubble.active() && bubble.ownerId() == player_.id())
        {
            bubble.updateOwnerPassage(player_.bounds());
        }
        else if (twoPlayerMode() && bubble.active() && bubble.ownerId() == player2_.id())
        {
            bubble.updateOwnerPassage(player2_.bounds());
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
    // 第一遍只推进倒计时，不直接擦除对象，后面的连锁检测仍能访问完整容器。
    for (WaterBubble& bubble : bubbles_)
    {
        if (bubble.active())
        {
            bubble.update(deltaTime);
        }
    }

    // 持续处理本帧新触发的水泡，保证任意长度连锁不会拖到下一帧。
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
    // 水泡失效时归还玩家容量；连锁引爆和自然到时共用同一条回收路径。
    if (ownerId == player_.id())
    {
        player_.onBubbleExpired();
    }
    else if (twoPlayerMode() && ownerId == player2_.id())
    {
        player2_.onBubbleExpired();
    }

    waves_.emplace_back(origin);
    createParticles(cellCenter(origin), Palette::Aqua, 7);
    constexpr std::array<GridPos, 4> directions = {
        GridPos{ 0, 1 }, GridPos{ 0, -1 }, GridPos{ 1, 0 }, GridPos{ -1, 0 }
    };

    // 四方向逐格传播：固定墙立即阻断；木箱先受击再终止；水泡被置为立即触发。
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
            // 命中另一个水泡只把其计时器归零，外层 while 会在本帧继续处理它。
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
        // 多个水泡同时爆炸时只播放一次声音，粒子和伤害仍按各自格子完整结算。
        audio_.playEffect(L"sfx_bubble_explode.wav");
        effectCooldown_ = 0.08f;
    }
}

std::vector<GridPos> MoeBubbleGame::collectDangerousCells() const
{
    // 把现有水浪和即将爆炸水泡的覆盖格合并，作为所有 AI 的共享避险数据。
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
        // 只预测 0.85 秒内即将引爆的水泡，避免 AI 长时间绕开暂时安全的通道。
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
    // 基类智能指针触发运行时多态，无需按 Patrol/Hunter/Boss 分支判断。
    for (const std::unique_ptr<Enemy>& enemy : enemies_)
    {
        if (enemy->active())
        {
            // 合作模式下每个敌人分别选择距离自己更近的存活玩家，避免始终只追 P1。
            Vec2 target = player_.position();
            if (!player_.active() && twoPlayerMode())
            {
                target = player2_.position();
            }
            else if (twoPlayerMode() && player2_.active()
                && lengthSquared(player2_.position() - enemy->position())
                    < lengthSquared(player_.position() - enemy->position()))
            {
                target = player2_.position();
            }
            enemy->updateAI(deltaTime, map_, bubbles_, danger, target, randomEngine_);
        }
    }
}

void MoeBubbleGame::updateCollisions()
{
    // 两名玩家共用伤害流程，但分别维护生命、护盾和无敌时间；只有全部阵亡才失败。
    auto damagePlayer = [&](Player& player, const RectF& damageBounds, int particleCount)
    {
        if (!player.active() || !intersects(damageBounds, player.bounds()) || !player.takeDamage())
        {
            return false;
        }
        audio_.playEffect(L"sfx_hit.wav");
        createParticles(player.position(), Palette::Danger, particleCount);
        if (allPlayersDefeated())
        {
            finishGame(false);
            return true;
        }
        return false;
    };

    // 先处理水浪，再处理敌人接触和道具拾取；全部玩家死亡后立即结束本帧。
    for (const WaterWave& wave : waves_)
    {
        if (!wave.active())
        {
            continue;
        }
        if (damagePlayer(player_, wave.bounds(), 10))
        {
            return;
        }
        if (twoPlayerMode() && damagePlayer(player2_, wave.bounds(), 10))
        {
            return;
        }

        for (const std::unique_ptr<Enemy>& enemy : enemies_)
        {
            // 普通敌人一次淘汰；Boss 通过重写 takeWaveHit 返回 Damaged 或 Defeated。
            if (enemy->active() && intersects(wave.bounds(), enemy->bounds()))
            {
                const EnemyHitResult result = enemy->takeWaveHit();
                if (result == EnemyHitResult::Damaged)
                {
                    audio_.playEffect(L"sfx_hit.wav");
                    createParticles(enemy->position(), Palette::Coral, 7);
                }
                else if (result == EnemyHitResult::Defeated)
                {
                    score_ += enemy->scoreValue();
                    ++statistics_.enemiesDefeated;
                    createParticles(enemy->position(), Palette::Purple, enemy->isBoss() ? 24 : 12);
                }
            }
        }
    }

    for (const std::unique_ptr<Enemy>& enemy : enemies_)
    {
        // 敌人与玩家接触和水浪伤害共用 Player 的无敌计时，不会每帧连续扣血。
        if (!enemy->active())
        {
            continue;
        }
        if (damagePlayer(player_, enemy->bounds(), 8))
        {
            return;
        }
        if (twoPlayerMode() && damagePlayer(player2_, enemy->bounds(), 8))
        {
            return;
        }
    }

    auto collectPowerUp = [&](PowerUp& powerUp, Player& player)
    {
        if (!player.active() || !intersects(powerUp.bounds(), player.bounds()))
        {
            return false;
        }
        player.applyPowerUp(powerUp.type());
        powerUp.deactivate();
        score_ += 50;
        ++statistics_.powerUpsCollected;
        audio_.playEffect(L"sfx_power_up.wav");
        createParticles(player.position(), Palette::Mint, 10);
        return true;
    };

    for (PowerUp& powerUp : powerUps_)
    {
        // 拾取后只标记失效，稍后的 cleanupInactiveObjects 负责安全移出 vector。
        if (!powerUp.active())
        {
            continue;
        }
        if (collectPowerUp(powerUp, player_))
        {
            continue;
        }
        if (twoPlayerMode())
        {
            collectPowerUp(powerUp, player2_);
        }
    }
}

void MoeBubbleGame::createPowerUp(const GridPos& cell)
{
    // 木箱破坏后有 34% 掉落概率，四类道具等概率选择。
    std::uniform_int_distribution<int> chance(0, 99);
    if (chance(randomEngine_) >= 34)
    {
        return;
    }
    std::uniform_int_distribution<int> type(0, 3);
    powerUps_.emplace_back(cell, static_cast<PowerUpType>(type(randomEngine_)), &itemIcons_);
}

void MoeBubbleGame::createParticles(const Vec2& position, COLORREF color, int count)
{
    // 极坐标随机生成全方向速度，随后由 StarParticle 的重力形成自然下落轨迹。
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
    // 过关奖励同时考虑关卡序号和剩余生命，随后再决定进入下一关或总胜利页。
    score_ += 500 * level_ + totalRemainingLives() * 100;
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
    // 所有结束路径在这里同步最高分和默认按钮，保证结算界面初始状态一致。
    highScore_ = std::max(highScore_, score_);
    resultIndex_ = 0;
    transitionTo(victory ? SceneState::Victory : SceneState::GameOver);
}

bool MoeBubbleGame::allPlayersDefeated() const
{
    if (!twoPlayerMode())
    {
        return !player_.active();
    }
    return !player_.active() && !player2_.active();
}

int MoeBubbleGame::totalRemainingLives() const
{
    return player_.lives() + (twoPlayerMode() ? player2_.lives() : 0);
}

void MoeBubbleGame::cleanupInactiveObjects()
{
    // 更新/碰撞阶段只标记 active=false，统一在帧末使用擦除-移除，
    // 避免遍历中删除导致迭代器失效；unique_ptr 同时自动释放派生敌人。
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
    // 绘制阶段只读取场景状态；暂停/确认框先画底层游戏，再叠加遮罩。
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
    drawPanel(24, 20, 214, 58, Palette::White, 16, false);
    drawCenteredText(L"单人 / 本地双人", 119, 28, 18, Palette::Ink, true);

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
    drawCenteredText(L"玩家 1 当前搭档", 276, 598, 17, Palette::Ink);

    constexpr std::array<const wchar_t*, MainMenuItemCount> labels = {
        L"单人闯关", L"双人合作", L"角色选择", L"操作说明", L"退出游戏"
    };
    for (int index = 0; index < static_cast<int>(labels.size()); ++index)
    {
        const int top = MainMenuButtonTop + index * MainMenuButtonStep;
        const COLORREF accent = index == 4 ? Palette::Coral
            : (index == 1 ? Palette::Mint : Palette::Aqua);
        drawButton(558, top, 868, top + MainMenuButtonHeight,
            labels[index], index == menuIndex_, accent);
    }
    drawCenteredText(L"鼠标点击 / ↑ ↓ 选择 / Enter 确认", 713, 576, 17, Palette::Ink);
    drawCenteredText(L"C++ · Visual Studio · EasyX", 480, 680, 16, RGB(108, 127, 137));
}

void MoeBubbleGame::drawCharacterSelect() const
{
    drawBackground();
    const std::wstring title = coopCharacterSelection_
        ? L"玩家 " + std::to_wstring(selectingPlayer_) + L" 选择泡泡搭档"
        : L"选择你的泡泡搭档";
    drawCenteredText(title, 480, 46, 42, Palette::Ink, true);
    drawCenteredText(L"四位搭档拥有不同生命、移速、水泡数量和爆破范围", 480, 100, 18, RGB(92, 113, 124));

    for (int index = 0; index < 4; ++index)
    {
        const int left = 30 + index * 232;
        const int right = left + 204;
        const bool selected = index == characterIndex_;
        const CharacterStyle style = styleFromIndex(index);
        const CharacterProfile profile = characterProfile(style);
        if (selected)
        {
            setlinecolor(styleColor(style));
            setfillcolor(styleColor(style));
            fillroundrect(left - 5, 142, right + 5, 606, 26, 26);
        }
        drawPanel(left, 148, right, 600, Palette::White, 22, true);
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
        drawCenteredText(styleName(style), (left + right) / 2, 456, 19,
            selected ? styleColor(style) : Palette::Ink, true);
        drawCenteredText(profile.role, (left + right) / 2, 484, 16, Palette::AquaDark, true);
        drawCenteredText(L"生命 " + std::to_wstring(profile.lives)
            + L"  移速 " + std::to_wstring(static_cast<int>(profile.moveSpeed)),
            (left + right) / 2, 508, 14, Palette::Ink);
        drawSmallIcon(itemIcons_, left + 18, 538, PowerUpType::BubbleCapacity, profile.bubbleCapacity);
        drawSmallIcon(itemIcons_, left + 76, 538, PowerUpType::BlastRange, profile.blastRange);
        drawSmallIcon(itemIcons_, left + 134, 538, PowerUpType::Shield, profile.shieldCharges);
    }

    const wchar_t* confirmLabel = !coopCharacterSelection_ ? L"确认选择"
        : (selectingPlayer_ == 1 ? L"玩家 1 确认，选择玩家 2" : L"玩家 2 确认并开始");
    drawButton(282, 618, 678, 674, confirmLabel, true,
        styleColor(styleFromIndex(characterIndex_)));
    drawCenteredText(L"鼠标选择并确认 / ← → 切换 / Esc 返回", 480, 687, 17, Palette::Ink);
}

void MoeBubbleGame::drawInstructions() const
{
    drawBackground();
    drawPanel(54, 38, 906, 682, Palette::White, 28, true);
    drawCenteredText(L"操作说明", 480, 68, 42, Palette::AquaDark, true);
    drawCenteredText(L"单人或本地双人合作：淘汰本关所有 AI 敌人即可前进", 480, 122, 19, Palette::Ink);

    drawTextAt(L"键盘操作", 104, 174, 26, Palette::Ink, true);
    drawPanel(68, 220, 438, 278, RGB(239, 249, 250), 14, false);
    drawTextAt(L"P1  WASD 移动 / Space 放泡", 88, 238, 18, Palette::Ink, true);
    drawPanel(68, 294, 438, 352, RGB(242, 249, 238), 14, false);
    drawTextAt(L"P2  方向键移动 / Enter 放泡", 88, 312, 18, Palette::Ink, true);
    drawPanel(68, 368, 438, 426, RGB(249, 242, 235), 14, false);
    drawTextAt(L"P / Esc  暂停游戏", 88, 386, 18, Palette::Ink, true);

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

    drawSmallIcon(itemIcons_, 190, 526, PowerUpType::BubbleCapacity, 1);
    drawSmallIcon(itemIcons_, 286, 526, PowerUpType::BlastRange, 2);
    drawSmallIcon(itemIcons_, 382, 526, PowerUpType::Speed, 0);
    drawSmallIcon(itemIcons_, 478, 526, PowerUpType::Shield, 0);
    drawTextAt(L"容量", 188, 580, 17, Palette::Ink);
    drawTextAt(L"范围", 284, 580, 17, Palette::Ink);
    drawTextAt(L"速度", 380, 580, 17, Palette::Ink);
    drawTextAt(L"护盾", 476, 580, 17, Palette::Ink);
    drawButton(330, 612, 630, 666, L"返回", true, Palette::Aqua);
}

void MoeBubbleGame::drawGameplay() const
{
    cleardevice();
    setfillcolor(Palette::Paper);
    solidrectangle(0, 0, GameConfig::WindowWidth, GameConfig::WindowHeight);

    drawPanel(24, 16, 684, 64, Palette::White, 16, true);
    drawTextAt(L"第 " + std::to_wstring(level_) + L" / 3 关", 44, 27, 20, Palette::Ink, true);
    const Enemy* boss = nullptr;
    for (const std::unique_ptr<Enemy>& enemy : enemies_)
    {
        if (enemy->active() && enemy->isBoss())
        {
            boss = enemy.get();
            break;
        }
    }
    const std::wstring enemyStatus = boss != nullptr
        ? L"首领 HP  " + std::to_wstring(boss->health()) + L" / " + std::to_wstring(boss->maxHealth())
        : L"剩余敌人  " + std::to_wstring(enemies_.size());
    drawCenteredText(enemyStatus, 352, 27, 20, Palette::PurpleDark, true);
    const std::wstring scoreText = L"分数  " + std::to_wstring(score_);
    setUiFont(20, true);
    drawTextAt(scoreText, 664 - textwidth(scoreText.c_str()), 27, 20, Palette::Honey, true);

    map_.draw();
    for (const PowerUp& powerUp : powerUps_) powerUp.draw();
    for (const WaterBubble& bubble : bubbles_) bubble.draw();
    for (const std::unique_ptr<Enemy>& enemy : enemies_) enemy->draw();
    if (player_.active()) player_.draw();
    if (twoPlayerMode() && player2_.active()) player2_.draw();
    for (const WaterWave& wave : waves_) wave.draw();
    for (const StarParticle& particle : particles_) particle.draw();

    drawHud();
    drawPanel(24, 660, 684, 702, Palette::White, 12, false);
    const wchar_t* controls = twoPlayerMode()
        ? L"P1 WASD+Space    P2 方向键+Enter    P / Esc 暂停"
        : L"WASD / 方向键移动     Space 放泡     P / Esc 暂停";
    drawCenteredText(controls, 354, 670, twoPlayerMode() ? 15 : 17, Palette::Ink);
}

void MoeBubbleGame::drawHud() const
{
    drawPanel(GameConfig::HudLeft, GameConfig::HudTop,
        GameConfig::HudLeft + GameConfig::HudWidth,
        GameConfig::HudTop + GameConfig::HudHeight, Palette::White, 24, true);

    if (twoPlayerMode())
    {
        drawCenteredText(L"双人合作", 822, 38, 22, Palette::AquaDark, true);
        auto drawPlayerCard = [&](const Player& player, const wchar_t* playerLabel,
            const wchar_t* controls, int top, COLORREF accent)
        {
            drawPanel(724, top, 920, top + 204, RGB(248, 250, 243), 16, false);
            drawTextAt(playerLabel, 738, top + 14, 17, accent, true);
            drawCenteredText(styleName(player.style()), 850, top + 14, 17,
                styleColor(player.style()), true);

            if (portraits_.loaded())
            {
                portraits_.draw(player.style(), PortraitSize::Hud, 738, top + 42);
            }
            else
            {
                drawChibiCharacter(player.style(), { 770.0f, static_cast<float>(top + 112) },
                    0.82f, false, sceneTime_);
            }

            for (int index = 0; index < GameConfig::MaxPlayerLives; ++index)
            {
                drawHeart(810 + index * 27, top + 66, index < player.lives());
            }
            drawTextAt(L"容量  " + std::to_wstring(player.bubbleCapacity()),
                808, top + 92, 14, Palette::Ink);
            drawTextAt(L"范围  " + std::to_wstring(player.blastRange()),
                808, top + 116, 14, Palette::Ink);
            drawTextAt(L"速度  " + std::to_wstring(player.movementSpeed()),
                808, top + 140, 14, Palette::Ink);
            drawTextAt(L"护盾  " + std::to_wstring(player.shieldCharges()),
                808, top + 164, 14, Palette::Ink);
            drawCenteredText(player.active() ? controls : L"已阵亡 · 等待队友",
                822, top + 181, 13, player.active() ? Palette::Ink : Palette::Danger, true);
        };

        drawPlayerCard(player_, L"P1", L"WASD + Space", 68, Palette::AquaDark);
        drawPlayerCard(player2_, L"P2", L"方向键 + Enter", 282, Palette::PurpleDark);

        drawPanel(730, 500, 914, 548, RGB(239, 249, 250), 12, false);
        drawCenteredText(L"合作目标：淘汰全部敌人", 822, 514, 15, Palette::AquaDark, true);
        drawTextAt(L"木箱 " + std::to_wstring(statistics_.cratesDestroyed)
            + L"  道具 " + std::to_wstring(statistics_.powerUpsCollected),
            734, 566, 15, Palette::Ink);
        drawTextAt(L"敌人 " + std::to_wstring(statistics_.enemiesDefeated)
            + L"  用时 " + formatTime(statistics_.elapsedTime),
            734, 596, 15, Palette::Ink);
        drawCenteredText(L"最高分  " + std::to_wstring(std::max(highScore_, score_)),
            822, 650, 16, Palette::Honey, true);
        return;
    }

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
    for (int index = 0; index < GameConfig::MaxPlayerLives; ++index)
    {
        drawHeart(786 + index * 34, 231, index < player_.lives());
    }

    drawTextAt(L"强化状态", 730, 274, 18, Palette::Ink, true);
    constexpr int StatusCardLeft = 732;
    constexpr int StatusCardTop = 304;
    constexpr int StatusCardStep = 46;
    drawHudStatusCard(itemIcons_, StatusCardLeft, StatusCardTop,
        PowerUpType::BubbleCapacity, player_.bubbleCapacity());
    drawHudStatusCard(itemIcons_, StatusCardLeft + StatusCardStep, StatusCardTop,
        PowerUpType::BlastRange, player_.blastRange());
    drawHudStatusCard(itemIcons_, StatusCardLeft + StatusCardStep * 2, StatusCardTop,
        PowerUpType::Speed, player_.movementSpeed());
    drawHudStatusCard(itemIcons_, StatusCardLeft + StatusCardStep * 3, StatusCardTop,
        PowerUpType::Shield, player_.shieldCharges());

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
        const COLORREF starColor = index < std::min(3, totalRemainingLives())
            ? Palette::Honey : Palette::Shadow;
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
    const std::wstring lifeText = twoPlayerMode()
        ? L"P1 生命 " + std::to_wstring(player_.lives())
            + L"    P2 生命 " + std::to_wstring(player2_.lives())
        : L"剩余生命  " + std::to_wstring(player_.lives());
    drawCenteredText(lifeText + L"    累计道具  "
        + std::to_wstring(statistics_.powerUpsCollected),
        480, 392, 18, Palette::Ink);
    drawButton(292, 458, 668, 514, L"进入下一关", resultIndex_ == 0, Palette::Aqua);
    drawButton(292, 528, 668, 584, L"返回主菜单", resultIndex_ == 1, Palette::Coral);
}

void MoeBubbleGame::drawGameOverOverlay() const
{
    drawPanel(220, 110, 740, 612, Palette::Paper, 30, true);
    drawCenteredText(L"挑战失败", 480, 146, 46, Palette::Coral, true);
    if (twoPlayerMode() && portraits_.loaded())
    {
        portraits_.draw(player_.style(), PortraitSize::Hud,
            380 - portraits_.width(PortraitSize::Hud) / 2, 216);
        portraits_.draw(player2_.style(), PortraitSize::Hud,
            520 - portraits_.width(PortraitSize::Hud) / 2, 216);
    }
    else if (portraits_.loaded())
    {
        portraits_.draw(player_.style(), PortraitSize::Hud,
            400 - portraits_.width(PortraitSize::Hud) / 2, 216);
    }
    else
    {
        drawChibiCharacter(player_.style(), { 400.0f, 290.0f }, 1.45f, false, sceneTime_);
    }
    if (!twoPlayerMode())
    {
        drawBubbleDecoration(566, 288, 34, Palette::Danger);
    }
    drawCenteredText(twoPlayerMode() ? L"两名玩家都阵亡了，重新配合再挑战！"
        : L"别灰心，调整路线再来一次吧！", 480, 374, 20, Palette::Ink);
    drawCenteredText(L"最终分数  " + std::to_wstring(score_)
        + L"    到达第 " + std::to_wstring(level_) + L" 关", 480, 416, 20, Palette::Honey, true);
    drawButton(292, 468, 668, 524, L"重新挑战", resultIndex_ == 0, Palette::Aqua);
    drawButton(292, 538, 668, 594, L"返回主菜单", resultIndex_ == 1, Palette::Coral);
}

void MoeBubbleGame::drawVictoryOverlay() const
{
    for (const StarParticle& particle : particles_) particle.draw();
    drawCenteredText(L"全部通关！", 480, 62, 58, Palette::AquaDark, true);
    drawCenteredText(twoPlayerMode() ? L"默契配合完成三关挑战"
        : L"你已经成为真正的泡泡达人", 480, 132, 21, Palette::Ink);
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
