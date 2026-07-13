#include <graphics.h>

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

        for (int index = 0; index < lineCount; ++index)
        {
            int offset = margin + index * cellSize;
            line(margin, offset, boardEnd, offset);
            line(offset, margin, offset, boardEnd);
        }
    }

    void drawStarPoints() const
    {
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

int main()
{
    GobangBoard board;
    board.run();
    return 0;
}
