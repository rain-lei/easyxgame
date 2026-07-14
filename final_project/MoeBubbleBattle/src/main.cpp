#include "MoeBubbleGame.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int)
{
    MoeBubbleGame game;
    if (commandLine != nullptr && wcsstr(commandLine, L"--capture-report") != nullptr)
    {
        return game.captureReportScreenshots(
            std::filesystem::current_path() / L"report_screenshots");
    }
    return game.run();
}
