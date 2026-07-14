#include <graphics.h>
#include <string>

// 练习一：用一个类封装棋盘尺寸、绘制步骤和窗口生命周期。
// 修改 cellSize 或 margin 即可整体调整布局，不需要逐条修改网格坐标。
class GobangBoard
{
public:
    void run()
    {
        openWindow();
        draw();
        waitForKeyPress();
        closegraph();
    }

    void captureReportScreenshot() const
    {
        // 报告模式复用正式绘制函数保存证据，不改变正常交互入口。
        openWindow();
        draw();
        saveimage(L"report_screenshot.png");
        closegraph();
    }

private:
    static constexpr int windowWidth = 600;
    static constexpr int windowHeight = 600;
    static constexpr int lineCount = 15;
    static constexpr int margin = 45;
    static constexpr int cellSize = 36;
    static constexpr int boardEnd = margin + (lineCount - 1) * cellSize;

    void openWindow() const
    {
        initgraph(windowWidth, windowHeight);
        setbkcolor(RGB(222, 176, 105));
        cleardevice();
    }

    void draw() const
    {
        drawGrid();
        drawStarPoints();
    }

    void drawGrid() const
    {
        setlinecolor(RGB(70, 45, 18));
        setlinestyle(PS_SOLID, 2);

        // 一次循环同时绘制同序号的横线和竖线，避免硬编码 30 条线段。
        for (int index = 0; index < lineCount; ++index)
        {
            int offset = margin + index * cellSize;
            line(margin, offset, boardEnd, offset);
            line(offset, margin, offset, boardEnd);
        }
    }

    void drawStarPoints() const
    {
        // 三个索引两两组合得到九个标准星位。
        static constexpr int starPoints[] = { 3, 7, 11 };
        setfillcolor(RGB(40, 25, 12));

        for (int row : starPoints)
        {
            for (int column : starPoints)
            {
                int x = margin + column * cellSize;
                int y = margin + row * cellSize;
                solidcircle(x, y, 5);
            }
        }
    }

    void waitForKeyPress() const
    {
        ExMessage message;
        do
        {
            message = getmessage(EX_KEY);
        } while (message.message != WM_KEYDOWN);
    }
};

int main(int argc, char* argv[])
{
    GobangBoard board;
    if (argc > 1 && std::string(argv[1]) == "--capture-report")
    {
        board.captureReportScreenshot();
        return 0;
    }
    board.run();
    return 0;
}
