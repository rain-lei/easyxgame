#include <windows.h>
#include <graphics.h>
#include <time.h>
#define GRID_NUMS 4										// 每行和每列的格子数
#define GRID_WIDTH 100									// 格子的宽度
#define GRID_HEIGHT 100									// 格子的高度
#define GRID_GAP 15										// 格子的间距
#define GRID_BORDER_RADIUS 20							// 格子的圆角半径
#define WIN_WIDTH GRID_WIDTH * 4 + GRID_GAP * 5			// 绘图窗口的宽度
#define WIN_HEIGHT GRID_HEIGHT * 4 + GRID_GAP * 5		// 绘图窗口的高度
#define TARGET_NUM 2048									// 游戏目标2048
#define GAME_FRAME 60									// 游戏帧数
#define ANIM_FRAME 10									// 动画帧数
#define ANIM_INTERVEL 45;								// 动画帧间隔（ms）

// 定义颜色
enum Colors
{
	// 背景色
	BACKGROUND = RGB(187, 173, 160),
	// 格子颜色
	GRID_0 = RGB(205, 193, 180),
	GRID_2 = RGB(238, 228, 218),
	GRID_4 = RGB(237, 224, 200),
	GRID_8 = RGB(242, 177, 121),
	GRID_16 = RGB(245, 149, 99),
	GRID_32 = RGB(246, 124, 95),
	GRID_64 = RGB(246, 94, 59),
	GRID_128 = RGB(237, 207, 114),
	GRID_256 = RGB(237, 204, 97),
	GRID_512 = RGB(237, 200, 80),
	GRID_1024 = RGB(237, 197, 63),
	GRID_2048 = RGB(237, 194, 46),
	// 数字颜色
	NUM_DEEP = RGB(119, 110, 101),
	NUM_LIGHT = RGB(249, 246, 242)
};

// 定义按键方向
enum MoveDirect
{
	MOVEUP = 1,
	MOVEDOWN = 2,
	MOVELEFT = 3,
	MOVERIGHT = 4
};

int numbers[GRID_NUMS][GRID_NUMS];			// 格子上的数值，初始所有元素为0
int animation[GRID_NUMS][GRID_NUMS];		// 格子的动画类型，0:无动画（默认）、1:随机生成的数、2:合并后的数
int animationIndex[GRID_NUMS][GRID_NUMS];	// 格子和数字动画的帧索引，1:从第1帧开始播放，10：最终显示的帧
int gridX1, gridY1, gridX2, gridY2;			// 格子的左上角和右下角坐标
int numX, numY;								// 数字的左上角坐标
int gridOffset = 0;							// 格子坐标的偏移量
int numHeightDelta = 0;						// 数字高度的增量
TCHAR chars[5];								// 转换后的数字（长度要比数字多1位）

// 在空白的位置上随机生成一个数字2或4
void randnum()
{
	srand(time(NULL));									// 设置随机数种子
	while (true)
	{
		int r = rand() % GRID_NUMS;						// 随机生成数组的一维下标
		int c = rand() % GRID_NUMS;						// 随机生成数组的二维下标
		if (numbers[r][c] == 0)							// 判断该位置是否为空
		{
			numbers[r][c] = rand() % 10 == 0 ? 4 : 2;	// 将空位置设置为数字2或4（概率）
			animation[r][c] = 1;						// 标记生成随机数的格子的动画类型
			animationIndex[r][c] = 0;					// 从第1帧播放动画
			break;
		}
	}
}

// 初始化游戏
void initGameData()
{
	// 初始化格子中的数字和动画效果
	for (int r = 0; r < GRID_NUMS; r++)
	{
		for (int c = 0; c < GRID_NUMS; c++)
		{
			numbers[r][c] = 0;
			animation[r][c] = 0;
		}
	}
	// 生成两个随机数
	randnum();
	randnum();
}

// 将0元素移动到数组最后，将非0元素移到数组前面：参数direct表示横向或纵向移动，参数n表示行号或列号
int* moveZeroes(int* nums, int numsSize, int direct, int n) {
	int* newNums = (int*)malloc(numsSize * sizeof(int));		// 分配新数组空间
	int i = 0;			// 原数组下标
	int j = 0;			// 新数组下标

	// 1. 遍历原数组，将所有非0元素存入新数组中
	for (i = 0; i < numsSize; i++)
	{
		if (nums[i] != 0) {
			newNums[j] = nums[i];
			// 根据按键方向，设置合并数的动画类型
			switch (direct)
			{
			case MOVEUP:
				animation[j][n] = animation[i][n];
				if (i != j)
					animation[i][n] = 0;
				break;
			case MOVEDOWN:
				animation[3 - j][n] = animation[3 - i][n];
				if (i != j)
					animation[3 - i][n] = 0;
				break;
			case MOVELEFT:
				animation[n][j] = animation[n][i];
				if (i != j)
					animation[n][i] = 0;
				break;
			case MOVERIGHT:
				animation[n][3 - j] = animation[n][3 - i];
				if (i != j)
					animation[n][3 - i] = 0;
				break;
			}
			j++;
		}
	}
	// 2. 将新数组剩余的空位填充为0
	while (j < numsSize) {
		newNums[j++] = 0;
	}

	return newNums;
}

// 合并数组中相邻的相同元素：参数direct表示横向或纵向移动，参数n表示行号或列号
int* merge(int* nums, int numsSize, int direct, int n)
{
	int* newNums = (int*)malloc(numsSize * sizeof(int));			// 分配一维数组空间

	// 1. 合并之前，先将0元素移动到数组最后
	newNums = moveZeroes(nums, numsSize, direct, n);

	// 2. 遍历数组，合并相邻的两个相同元素
	for (int i = 0; i < numsSize - 1; i++)
	{
		// 如果当前元素不为0，且与右侧元素相同，则进行合并
		if (newNums[i] != 0 && newNums[i] == newNums[i + 1])
		{
			newNums[i] += newNums[i + 1];
			newNums[i + 1] = 0;
			// 根据按键方向，设置合并数的动画类型
			switch (direct)
			{
			case MOVEUP:
				animation[i][n] = 2;
				animationIndex[i][n] = 0;
				break;
			case MOVEDOWN:
				animation[3 - i][n] = 2;
				animationIndex[3 - i][n] = 0;
				break;
			case MOVELEFT:
				animation[n][i] = 2;
				animationIndex[n][i] = 0;
				break;
			case MOVERIGHT:
				animation[n][3 - i] = 2;
				animationIndex[n][3 - i] = 0;
				break;
			}
		}
	}

	// 3. 合并之后，再将0元素移动到数组最后
	newNums = moveZeroes(newNums, numsSize, direct, n);

	return newNums;
}

// 处理上键
void moveup()
{
	int originalNums[GRID_NUMS][GRID_NUMS];						// 二位数组用于记录原数组内容	
	memcpy(originalNums, numbers, sizeof(numbers));				// 复制原始数组内容
	int* movedNums = (int*)malloc(GRID_NUMS * sizeof(int));		// 分配内存空间用于保存一行或一列待合并的数据
	// 对每一列进行处理
	for (int c = 0; c < GRID_NUMS; c++)
	{
		// 获取一列格子的数据
		for (int i = 0, r = 0; r < GRID_NUMS; r++)
			movedNums[i++] = numbers[r][c];

		// 合并数字
		movedNums = merge(movedNums, GRID_NUMS, MOVEUP, c);

		// 重新为一列格子赋值
		for (int i = 0, r = 0; r < GRID_NUMS; r++)
			numbers[r][c] = movedNums[i++];
	}

	// 对比原始数组和修改后的数组，如果数组内容发生改变，则新生成一个随机数
	if (memcmp(originalNums, numbers, sizeof(numbers)) != 0) {
		randnum();
	}
}

// 处理下键
void movedown()
{
	int originalNums[GRID_NUMS][GRID_NUMS];						// 二位数组用于记录原数组内容	
	memcpy(originalNums, numbers, sizeof(numbers));				// 复制原始数组内容
	int* movedNums = (int*)malloc(GRID_NUMS * sizeof(int));		// 分配内存空间用于保存一行或一列待合并的数据
	// 对每一列进行处理
	for (int c = 0; c < GRID_NUMS; c++)
	{
		// 获取一列格子的数据
		for (int i = 0, r = GRID_NUMS - 1; r >= 0; r--)
			movedNums[i++] = numbers[r][c];
		// 合并数字
		movedNums = merge(movedNums, GRID_NUMS, MOVEDOWN, c);
		// 重新为一列格子赋值
		for (int i = 0, r = GRID_NUMS - 1; r >= 0; r--)
			numbers[r][c] = movedNums[i++];
	}
	// 对比原始数组和修改后的数组，如果数组内容发生改变，则新生成一个随机数
	if (memcmp(originalNums, numbers, sizeof(numbers)) != 0) {
		randnum();
	}
}

// 处理左键
void moveleft()
{
	int originalNums[GRID_NUMS][GRID_NUMS];						// 二位数组用于记录原数组内容	
	memcpy(originalNums, numbers, sizeof(numbers));				// 复制原始数组内容
	int* movedNums = (int*)malloc(GRID_NUMS * sizeof(int));		// 分配内存空间用于保存一行或一列待合并的数据
	// 对每一行进行处理
	for (int r = 0; r < GRID_NUMS; r++)
	{
		// 获取一行格子的数据
		for (int i = 0, c = 0; c < GRID_NUMS; c++)
			movedNums[i++] = numbers[r][c];
		// 合并数字
		movedNums = merge(movedNums, GRID_NUMS, MOVELEFT, r);
		// 重新为一列格子赋值
		for (int i = 0, c = 0; c < GRID_NUMS; c++)
			numbers[r][c] = movedNums[i++];
	}
	// 对比原始数组和修改后的数组，如果数组内容发生改变，则新生成一个随机数
	if (memcmp(originalNums, numbers, sizeof(numbers)) != 0) {
		randnum();
	}
}

// 处理右键
void moveright()
{
	int originalNums[GRID_NUMS][GRID_NUMS];						// 二位数组用于记录原数组内容	
	memcpy(originalNums, numbers, sizeof(numbers));				// 复制原始数组内容
	int* movedNums = (int*)malloc(GRID_NUMS * sizeof(int));		// 分配内存空间用于保存一行或一列待合并的数据
	// 对每一行进行处理
	for (int r = 0; r < GRID_NUMS; r++) {
		// 获取一行格子的数据
		for (int i = 0, c = GRID_NUMS - 1; c >= 0; c--)
			movedNums[i++] = numbers[r][c];
		// 合并数字
		movedNums = merge(movedNums, GRID_NUMS, MOVERIGHT, r);
		// 重新为一列格子赋值
		for (int i = 0, c = GRID_NUMS - 1; c >= 0; c--)
			numbers[r][c] = movedNums[i++];
	}
	// 对比原始数组和修改后的数组，如果数组内容发生改变，则新生成一个随机数
	if (memcmp(originalNums, numbers, sizeof(numbers)) != 0) {
		randnum();
	}
}

// 通过数值获取格子的颜色
COLORREF getColorByNum(int number)
{
	COLORREF color = NULL;
	switch (number)
	{
	case 0:
		color = GRID_0;
		break;
	case 2:
		color = GRID_2;
		break;
	case 4:
		color = GRID_4;
		break;
	case 8:
		color = GRID_8;
		break;
	case 16:
		color = GRID_16;
		break;
	case 32:
		color = GRID_32;
		break;
	case 64:
		color = GRID_64;
		break;
	case 128:
		color = GRID_128;
		break;
	case 256:
		color = GRID_256;
		break;
	case 512:
		color = GRID_512;
		break;
	case 1024:
		color = GRID_1024;
		break;
	case 2048:
		color = GRID_2048;
		break;
	}
	return color;
}

// 判断数组中是否有2048
bool isTargetExist()
{
	bool found = false;							// 标记是否找到2048
	for (int r = 0; r < GRID_NUMS; r++)
	{
		for (int c = 0; c < GRID_NUMS; c++)
		{
			if (numbers[r][c] == TARGET_NUM)
			{
				found = true;					// 标记为找到
				break;
			}
		}
	}
	return found;
}

// 判断游戏是否结束：无法移动
bool isGameOver()
{
	// 遍历数组，当所有数字都不为0且相邻的格子数字不相同时游戏结束
	for (int r = 0; r < GRID_NUMS; r++) {
		for (int c = 0; c < GRID_NUMS; c++) {
			// 如果存在空格子，则游戏未结束
			if (numbers[r][c] == 0) {
				return false;
			}
			// 如果上下格子数字相同，则游戏未结束
			if (r < 3 && numbers[r][c] == numbers[r + 1][c]) {
				return false;
			}
			// 如果左右格子数字相同，则游戏未结束
			if (c < 3 && numbers[r][c] == numbers[r][c + 1]) {
				return false;
			}
		}
	}
	return true;
}

int main()
{
	// 变量定义与初始化
	bool continueGame = true;						// 是否继续游戏
	bool gameOver;									// 游戏是否结束
	ExMessage msg;									// 消息处理对象
	int mbChoice;									// 消息框的选择结果

	// 窗口初始化
	initgraph(WIN_WIDTH, WIN_HEIGHT);				// 初始化绘图窗口
	SetWindowText(GetHWnd(), _T("2048小游戏"));		// 设置窗口标题
	setbkcolor(BACKGROUND);							// 设置窗口背景色
	BeginBatchDraw();								// 开启批量绘图

	// 游戏主循环
	while (continueGame)
	{
		initGameData();								// 初始化数据
		gameOver = false;							// 本局游戏结束控制
		while (!gameOver)							// 当前一局游戏
		{
			ULONGLONG beginTime = GetTickCount64();

			// 消息处理
			while (peekmessage(&msg))
			{
				if (msg.message == WM_KEYDOWN)
				{
					switch (msg.vkcode)
					{
					case 'w':
					case 'W':
					case VK_UP:						// 上键
						moveup();
						break;
					case 's':
					case 'S':
					case VK_DOWN:					// 下键
						movedown();
						break;
					case 'a':
					case 'A':
					case VK_LEFT:					// 左键
						moveleft();
						break;
					case 'd':
					case 'D':
					case VK_RIGHT:					// 右键
						moveright();
						break;
					case VK_ESCAPE:					// ESC键
						mbChoice = MessageBox(GetHWnd(), _T("重新开始？还是退出游戏？"), _T("请选择"), MB_YESNOCANCEL | MB_DEFBUTTON1);
						if (mbChoice == IDYES)				// 重新开始游戏
						{
							gameOver = true;
							break;
						}
						else if (mbChoice == IDNO)			// 选择退出游戏
						{
							gameOver = true;
							continueGame = false;
							break;
						}
						else if (mbChoice == IDCANCEL)		// 关闭消息框
							break;
					}
				}
			}

			// 绘图
			cleardevice();							// 用背景色清屏
			for (int r = 0; r < GRID_NUMS; r++)
			{
				for (int c = 0; c < GRID_NUMS; c++)
				{
					// 每个格子帧索引自增，最大10帧
					animationIndex[r][c]++;
					if (animationIndex[r][c] > ANIM_FRAME)
						animationIndex[r][c] = ANIM_FRAME;

					// 计算每个格子左上角和右下角的坐标，根据数字设置格子颜色
					gridX1 = (c + 1) * GRID_GAP + c * GRID_WIDTH;
					gridY1 = (r + 1) * GRID_GAP + r * GRID_HEIGHT;
					gridX2 = gridX1 + GRID_WIDTH;
					gridY2 = gridY1 + GRID_HEIGHT;

					// 如果数字为0，则只绘制格子
					if (numbers[r][c] == 0)
					{
						setfillcolor(getColorByNum(numbers[r][c]));
						solidroundrect(gridX1, gridY1, gridX2, gridY2, GRID_BORDER_RADIUS, GRID_BORDER_RADIUS);
					}
					else
					{
						// 根据动画类型计算每个格子和文字的坐标偏移量
						if (animation[r][c] == 0)			// 无动画
						{
							gridOffset = 0;
							numHeightDelta = 0;
						}
						else if (animation[r][c] == 1)		// 随机数动画：格子和数字从小变大
						{
							gridOffset = (ANIM_FRAME - animationIndex[r][c]) * 3;
							numHeightDelta = (ANIM_FRAME - animationIndex[r][c]) * 2;
						}
						else if (animation[r][c] == 2)		// 合并数动画：格子和数字从大变小
						{
							gridOffset = (animationIndex[r][c] - ANIM_FRAME) * 1;
							numHeightDelta = (animationIndex[r][c] - ANIM_FRAME) * 2;
						}

						// 设置格子的颜色
						setfillcolor(getColorByNum(numbers[r][c]));
						// 绘制格子
						solidroundrect(gridX1 + gridOffset, gridY1 + gridOffset, gridX2 - gridOffset, gridY2 - gridOffset, GRID_BORDER_RADIUS, GRID_BORDER_RADIUS);

						// 设置数字的样式
						if (numbers[r][c] <= 4)				// 数字2和4为深色，其他数字为浅色
							settextcolor(NUM_DEEP);
						else
							settextcolor(NUM_LIGHT);
						if (numbers[r][c] <= 64)			// 1-2位的数字较大，3-4位的数字较小
							settextstyle(60 - numHeightDelta, 0, _T("黑体"));
						else
							settextstyle(44 - numHeightDelta, 0, _T("黑体"));
						setbkmode(TRANSPARENT);										// 设置文字背景透明
						_stprintf_s(chars, _T("%d"), numbers[r][c]);				// 将数字转换为字符串
						numX = gridX1 + GRID_WIDTH / 2 - textwidth(chars) / 2;		// 计算数字的坐标
						numY = gridY1 + GRID_HEIGHT / 2 - textheight(chars) / 2;
						// 绘制数字
						outtextxy(numX, numY, chars);
					}
				}
			}
			FlushBatchDraw();						// 刷新绘图		

			// 胜负判定
			if (isTargetExist())					// 获胜
			{
				mbChoice = MessageBox(GetHWnd(), _T("恭喜你过关啦！再来一次？还是退出游戏？"), _T("请选择"), MB_YESNO | MB_DEFBUTTON1);
				if (mbChoice == IDYES)				// 选择再来一次
					break;
				else if (mbChoice == IDNO)			// 选择退出游戏
				{
					continueGame = false;
					break;
				}
			}
			if (isGameOver())						// 失败
			{
				mbChoice = MessageBox(GetHWnd(), _T("很遗憾你失败了~ 再来一次？还是退出游戏？"), _T("请选择"), MB_YESNO | MB_DEFBUTTON1);
				if (mbChoice == IDYES)
					gameOver = true;				// 结束当前局，重新开始游戏
				else if (mbChoice == IDNO)
				{
					gameOver = true;				// 结束当前局游戏
					continueGame = false;			// 退出游戏
				}
			}

			// 游戏帧补时
			ULONGLONG endTime = GetTickCount64();
			ULONGLONG elapsedTime = endTime - beginTime;
			if (elapsedTime < 1000 / GAME_FRAME)
				Sleep(1000 / GAME_FRAME - elapsedTime);
		}
	}

	EndBatchDraw();				// 结束批量绘图
	closegraph();				// 关闭绘图窗口
	return 0;
}