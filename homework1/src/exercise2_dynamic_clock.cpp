#include <graphics.h>
#include <cmath>
#include <cstdio>
#include <tchar.h>

class ClockHand
{
public:
    ClockHand(int length, int width, COLORREF color)
        : length_(length), width_(width), color_(color)
    {
    }

    void draw(int centerX, int centerY, double degree) const
    {
        double radian = (degree - 90.0) * piValue / 180.0;
        int endX = centerX + static_cast<int>(std::cos(radian) * length_);
        int endY = centerY + static_cast<int>(std::sin(radian) * length_);

        setlinecolor(color_);
        setlinestyle(PS_SOLID, width_);
        line(centerX, centerY, endX, endY);
    }

private:
    static constexpr double piValue = 3.14159265358979323846;
    int length_;
    int width_;
    COLORREF color_;
};

class DynamicClock
{
public:
    DynamicClock()
        : hourHand_(110, 8, RGB(35, 35, 35)),
          minuteHand_(160, 5, RGB(35, 35, 35)),
          secondHand_(185, 2, RGB(210, 50, 45))
    {
    }

    void run()
    {
        initgraph(windowWidth, windowHeight);
        BeginBatchDraw();

        while (running_)
        {
            processInput();
            if (running_)
            {
                drawCurrentTime();
                FlushBatchDraw();
                Sleep(16);
            }
        }

        EndBatchDraw();
        closegraph();
    }

private:
    static constexpr double piValue = 3.14159265358979323846;
    static constexpr int windowWidth = 600;
    static constexpr int windowHeight = 600;
    static constexpr int centerX = windowWidth / 2;
    static constexpr int centerY = windowHeight / 2;
    static constexpr int clockRadius = 230;

    ClockHand hourHand_;
    ClockHand minuteHand_;
    ClockHand secondHand_;
    bool running_ = true;

    int pointX(double degree, int radius) const
    {
        double radian = (degree - 90.0) * piValue / 180.0;
        return centerX + static_cast<int>(std::cos(radian) * radius);
    }

    int pointY(double degree, int radius) const
    {
        double radian = (degree - 90.0) * piValue / 180.0;
        return centerY + static_cast<int>(std::sin(radian) * radius);
    }

    void processInput()
    {
        ExMessage message;
        while (peekmessage(&message, EX_KEY))
        {
            if (message.message == WM_KEYDOWN && message.vkcode == VK_ESCAPE)
            {
                running_ = false;
            }
        }
    }

    void drawCurrentTime() const
    {
        SYSTEMTIME currentTime;
        GetLocalTime(&currentTime);

        double secondDegree = currentTime.wSecond * 6.0 + currentTime.wMilliseconds * 0.006;
        double minuteDegree = currentTime.wMinute * 6.0 + currentTime.wSecond * 0.1;
        double hourDegree = (currentTime.wHour % 12) * 30.0 + currentTime.wMinute * 0.5;

        drawClockFace();
        hourHand_.draw(centerX, centerY, hourDegree);
        minuteHand_.draw(centerX, centerY, minuteDegree);
        secondHand_.draw(centerX, centerY, secondDegree);
        drawCenterPin();
    }

    void drawClockFace() const
    {
        setbkcolor(RGB(24, 28, 36));
        cleardevice();

        setfillcolor(RGB(246, 241, 228));
        setlinecolor(RGB(200, 170, 95));
        setlinestyle(PS_SOLID, 6);
        fillcircle(centerX, centerY, clockRadius);

        for (int index = 0; index < 60; ++index)
        {
            int outerRadius = clockRadius - 14;
            int innerRadius = index % 5 == 0 ? clockRadius - 42 : clockRadius - 28;
            setlinecolor(index % 5 == 0 ? RGB(50, 50, 50) : RGB(120, 120, 120));
            setlinestyle(PS_SOLID, index % 5 == 0 ? 4 : 2);
            line(pointX(index * 6.0, innerRadius), pointY(index * 6.0, innerRadius),
                pointX(index * 6.0, outerRadius), pointY(index * 6.0, outerRadius));
        }

        drawHourNumbers();
    }

    void drawHourNumbers() const
    {
        setbkmode(TRANSPARENT);
        settextcolor(RGB(45, 45, 45));
        settextstyle(30, 0, _T("Consolas"));

        for (int number = 1; number <= 12; ++number)
        {
            TCHAR text[4];
            _stprintf_s(text, sizeof(text) / sizeof(text[0]), _T("%d"), number);
            double degree = number * 30.0;
            int x = pointX(degree, clockRadius - 72) - textwidth(text) / 2;
            int y = pointY(degree, clockRadius - 72) - textheight(text) / 2;
            outtextxy(x, y, text);
        }
    }

    void drawCenterPin() const
    {
        setfillcolor(RGB(210, 50, 45));
        setlinecolor(RGB(210, 50, 45));
        solidcircle(centerX, centerY, 8);
    }
};

int main()
{
    DynamicClock clock;
    clock.run();
    return 0;
}
