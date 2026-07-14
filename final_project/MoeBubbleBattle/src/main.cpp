#include "MoeBubbleGame.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int)
{
    MoeBubbleGame game;
    // 报告截图是显式命令行模式，正常从 Visual Studio 启动仍进入交互主循环。
    if (commandLine != nullptr && wcsstr(commandLine, L"--capture-report") != nullptr)
    {
        return game.captureReportScreenshots(
            std::filesystem::current_path() / L"report_screenshots");
    }
    return game.run();
}
