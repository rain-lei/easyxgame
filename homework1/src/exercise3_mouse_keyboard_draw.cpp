#include <graphics.h>
#include <imm.h>

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

int main()
{
    DrawingApp app;
    app.run();
    return 0;
}
