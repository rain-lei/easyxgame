#include <graphics.h>
#include <algorithm>
#include <cmath>
#include <imm.h>
#include <tchar.h>
#include <string>

#pragma comment(lib, "imm32.lib")

// 挡板封装左右按键状态、边界约束和绘制；坐标不暴露给游戏主循环修改。
class Paddle
{
public:
    Paddle(double x, int y, int width, int height, int speed)
        : x_(x), y_(y), width_(width), height_(height), speed_(speed)
    {
    }

    void handleKeyDown(BYTE keyCode)
    {
        if (keyCode == 'A' || keyCode == VK_LEFT)
        {
            movingLeft_ = true;
        }
        else if (keyCode == 'D' || keyCode == VK_RIGHT)
        {
            movingRight_ = true;
        }
    }

    void handleKeyUp(BYTE keyCode)
    {
        if (keyCode == 'A' || keyCode == VK_LEFT)
        {
            movingLeft_ = false;
        }
        else if (keyCode == 'D' || keyCode == VK_RIGHT)
        {
            movingRight_ = false;
        }
    }

    void update(int windowWidth)
    {
        if (movingLeft_)
        {
            x_ -= speed_;
        }
        if (movingRight_)
        {
            x_ += speed_;
        }

        x_ = std::clamp(x_, 0.0, static_cast<double>(windowWidth - width_));
    }

    void draw() const
    {
        setlinecolor(RGB(60, 200, 120));
        setfillcolor(RGB(60, 200, 120));
        solidrectangle(static_cast<int>(x_), y_,
            static_cast<int>(x_) + width_, y_ + height_);
    }

    double left() const { return x_; }
    double right() const { return x_ + width_; }
    double centerX() const { return x_ + width_ / 2.0; }
    int top() const { return y_; }
    int width() const { return width_; }

private:
    double x_;
    int y_;
    int width_;
    int height_;
    int speed_;
    bool movingLeft_ = false;
    bool movingRight_ = false;
};

// 小球封装位置、速度、墙面/挡板碰撞和反弹，不负责读取键盘。
class Ball
{
public:
    Ball(double x, double y, double velocityX, double velocityY, int radius)
        : x_(x), y_(y), velocityX_(velocityX), velocityY_(velocityY), radius_(radius)
    {
    }

    bool update(const Paddle& paddle, int windowWidth, int windowHeight)
    {
        // 保存上一帧 y 值，用“穿过挡板上边缘”判断避免侧面重复反弹。
        double previousY = y_;
        x_ += velocityX_;
        y_ += velocityY_;

        collideWithWalls(windowWidth);
        collideWithPaddle(paddle, previousY);
        return y_ + radius_ < windowHeight;
    }

    void draw() const
    {
        setlinecolor(YELLOW);
        setfillcolor(YELLOW);
        solidcircle(static_cast<int>(x_), static_cast<int>(y_), radius_);
    }

private:
    double x_;
    double y_;
    double velocityX_;
    double velocityY_;
    int radius_;

    void collideWithWalls(int windowWidth)
    {
        if (x_ - radius_ <= 0)
        {
            x_ = radius_;
            velocityX_ = std::fabs(velocityX_);
        }
        else if (x_ + radius_ >= windowWidth)
        {
            x_ = windowWidth - radius_;
            velocityX_ = -std::fabs(velocityX_);
        }

        if (y_ - radius_ <= 0)
        {
            y_ = radius_;
            velocityY_ = std::fabs(velocityY_);
        }
    }

    void collideWithPaddle(const Paddle& paddle, double previousY)
    {
        bool overlapsPaddleX = x_ + radius_ >= paddle.left()
            && x_ - radius_ <= paddle.right();
        bool crossesPaddleTop = previousY + radius_ <= paddle.top()
            && y_ + radius_ >= paddle.top();

        if (velocityY_ > 0 && overlapsPaddleX && crossesPaddleTop)
        {
            y_ = paddle.top() - radius_;
            velocityY_ = -std::fabs(velocityY_);

            // 命中点相对挡板中心的偏移决定水平速度，给玩家可控的反弹角度。
            double hitOffset = (x_ - paddle.centerX()) / (paddle.width() / 2.0);
            velocityX_ = std::clamp(hitOffset * 7.0, -7.0, 7.0);
        }
    }
};

// 游戏组合根拥有 Ball 与 Paddle，统一安排输入、更新、绘制和帧率限制。
class BouncingBallGame
{
public:
    BouncingBallGame()
        : ball_(400.0, 120.0, 4.0, 5.0, 12),
          paddle_(340.0, 540, 120, 16, 9)
    {
    }

    void run()
    {
        openWindow();
        while (running_)
        {
            runFrame();
        }
        EndBatchDraw();
        closegraph();
    }

    void captureReportScreenshot() const
    {
        openWindow();
        drawScene();
        FlushBatchDraw();
        saveimage(L"report_screenshot.png");
        EndBatchDraw();
        closegraph();
    }

private:
    static constexpr int windowWidth = 800;
    static constexpr int windowHeight = 600;
    static constexpr int targetFrameMs = 16;

    Ball ball_;
    Paddle paddle_;
    bool running_ = true;
    bool paused_ = false;

    void openWindow() const
    {
        initgraph(windowWidth, windowHeight);
        ImmAssociateContext(GetHWnd(), HIMC{});
        setbkcolor(BLACK);
        BeginBatchDraw();
    }

    void runFrame()
    {
        // 固定帧阶段顺序，保证输入先影响对象，再绘制同一帧的新状态。
        DWORD frameStart = GetTickCount();
        processInput();

        if (!paused_ && running_)
        {
            paddle_.update(windowWidth);
            running_ = ball_.update(paddle_, windowWidth, windowHeight);
        }

        drawScene();
        FlushBatchDraw();
        limitFrameRate(frameStart);
    }

    void processInput()
    {
        ExMessage message;
        while (peekmessage(&message, EX_KEY))
        {
            if (message.message == WM_KEYDOWN)
            {
                handleKeyDown(message);
            }
            else if (message.message == WM_KEYUP)
            {
                paddle_.handleKeyUp(message.vkcode);
            }
        }
    }

    void handleKeyDown(const ExMessage& message)
    {
        if (message.vkcode == 'P' && !message.prevdown)
        {
            paused_ = !paused_;
        }
        else if (message.vkcode == VK_ESCAPE)
        {
            running_ = false;
        }
        else
        {
            paddle_.handleKeyDown(message.vkcode);
        }
    }

    void drawScene() const
    {
        cleardevice();

        setlinecolor(RGB(80, 80, 80));
        line(0, windowHeight - 1, windowWidth, windowHeight - 1);

        ball_.draw();
        paddle_.draw();

        if (paused_)
        {
            setbkmode(TRANSPARENT);
            settextcolor(WHITE);
            settextstyle(36, 0, _T("Consolas"));
            outtextxy(330, 260, _T("PAUSED"));
        }
    }

    void limitFrameRate(DWORD frameStart) const
    {
        // 目标约 60 FPS，避免不同电脑运行速度影响移动和碰撞手感。
        DWORD frameElapsed = GetTickCount() - frameStart;
        if (frameElapsed < targetFrameMs)
        {
            Sleep(targetFrameMs - frameElapsed);
        }
    }
};

int main(int argc, char* argv[])
{
    BouncingBallGame game;
    if (argc > 1 && std::string(argv[1]) == "--capture-report")
    {
        game.captureReportScreenshot();
        return 0;
    }
    game.run();
    return 0;
}
