#include <graphics.h>
#include <imm.h>
#include <string>
#include <tchar.h>

#pragma comment(lib, "imm32.lib")

class Shape
{
public:
    virtual ~Shape() = default;
    virtual void draw(int centerX, int centerY, int size, COLORREF color) const = 0;
};

class Square final : public Shape
{
public:
    void draw(int centerX, int centerY, int size, COLORREF color) const override
    {
        int halfSize = size / 2;
        setlinecolor(color);
        setfillcolor(color);
        solidrectangle(centerX - halfSize, centerY - halfSize,
            centerX + halfSize, centerY + halfSize);
    }
};

class Circle final : public Shape
{
public:
    void draw(int centerX, int centerY, int radius, COLORREF color) const override
    {
        setlinecolor(color);
        setfillcolor(color);
        solidcircle(centerX, centerY, radius);
    }
};

class DrawingApp
{
public:
    void run()
    {
        openWindow();
        processMessages();
        EndBatchDraw();
        closegraph();
    }

    void captureReportScreenshot() const
    {
        openWindow();
        square_.draw(150, 150, 42, RGB(102, 213, 232));
        circle_.draw(280, 210, 34, RGB(255, 143, 130));
        square_.draw(430, 330, 68, RGB(145, 216, 191));
        circle_.draw(620, 400, 48, RGB(245, 199, 104));
        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        settextstyle(24, 0, _T("Microsoft YaHei UI"));
        outtextxy(28, 28, _T("左键：正方形   右键：圆形   R/G/B/W：切换颜色   Ctrl：放大"));
        FlushBatchDraw();
        saveimage(L"report_screenshot.png");
        EndBatchDraw();
        closegraph();
    }

private:
    static constexpr int windowWidth = 800;
    static constexpr int windowHeight = 600;
    static constexpr int normalSize = 10;
    static constexpr int controlSize = 20;

    Square square_;
    Circle circle_;
    COLORREF drawColor_ = WHITE;
    bool running_ = true;

    void openWindow() const
    {
        initgraph(windowWidth, windowHeight);
        ImmAssociateContext(GetHWnd(), HIMC{});
        setbkcolor(BLACK);
        cleardevice();
        BeginBatchDraw();
    }

    void processMessages()
    {
        ExMessage message;
        while (running_)
        {
            while (peekmessage(&message, EX_MOUSE | EX_KEY))
            {
                handleMessage(message);
            }
            Sleep(10);
        }
    }

    void handleMessage(const ExMessage& message)
    {
        if (message.message == WM_LBUTTONDOWN)
        {
            drawShape(square_, message);
        }
        else if (message.message == WM_RBUTTONDOWN)
        {
            drawShape(circle_, message);
        }
        else if (message.message == WM_KEYDOWN)
        {
            handleKey(message.vkcode);
        }
    }

    void drawShape(const Shape& shape, const ExMessage& message) const
    {
        int size = message.ctrl ? controlSize : normalSize;
        shape.draw(message.x, message.y, size, drawColor_);
        FlushBatchDraw();
    }

    void handleKey(BYTE keyCode)
    {
        switch (keyCode)
        {
        case 'C':
            clearCanvas();
            break;
        case 'R':
            drawColor_ = RED;
            break;
        case 'G':
            drawColor_ = GREEN;
            break;
        case 'B':
            drawColor_ = BLUE;
            break;
        case 'W':
            drawColor_ = WHITE;
            break;
        case VK_ESCAPE:
            running_ = false;
            break;
        }
    }

    void clearCanvas() const
    {
        cleardevice();
        FlushBatchDraw();
    }
};

int main(int argc, char* argv[])
{
    DrawingApp app;
    if (argc > 1 && std::string(argv[1]) == "--capture-report")
    {
        app.captureReportScreenshot();
        return 0;
    }
    app.run();
    return 0;
}
