#include <windows.h>
#include <graphics.h>
#include <stdio.h>
#include <string>
#include <vector>
#pragma comment(lib, "MSIMG32.LIB")
#pragma comment(lib, "Winmm.LIB")
using namespace std;

const int WIN_WIDTH = 1000;				//游戏窗口宽度
const int WIN_HEIGHT = 600;				//游戏窗口高度
const int GAME_FRAME = 60;				//游戏帧数
bool clearEnemy = false;				//是否一键清除敌人

// 显示透明图片
void putimage_alpha(int x, int y, IMAGE* img)
{
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
}

//动画类
class Animation
{
public:
	//在构造函数中加载动画图片
	Animation(wstring imagesPath, int imagesNum, int interval)
	{
		interval_ms = interval;		//不同角色可以设置不同的动画时间间隔
		for (int i = 0; i < imagesNum; i++)
		{
			wstring imageFilePath = imagesPath + to_wstring(i + 1) + L".png";	// "image\\wandou\\1.png"
			IMAGE* img = new IMAGE();
			loadimage(img, imageFilePath.c_str());
			imageList.push_back(img);
		}
	}

	~Animation()
	{
		for (int i = 0; i < imageList.size(); i++)
		{
			delete imageList[i];
		}
	}

	//播放动画
	void Play(int x, int y, int delta)
	{
		timer += delta;
		if (timer > interval_ms)
		{
			imageIndex = (imageIndex + 1) % imageList.size();
			timer = 0;
		}
		putimage_alpha(x, y, imageList[imageIndex]);
	}

private:
	std::vector<IMAGE*> imageList;		//动画图片集合
	int interval_ms = 0;				//动画播放时间间隔
	int timer = 0;						//动画计时器
	int imageIndex = 0;					//每帧图片索引
};

//玩家类
class Player
{
public:
	Player()
	{
		aniPea = new Animation(_T("image\\pea\\"), 13, 45);
	}

	~Player()
	{
		delete aniPea;
	}

	//玩家消息处理
	void ProcessEvent(const ExMessage& msg) {
		//按下键盘处理
		if (msg.message == WM_KEYDOWN)
		{
			switch (msg.vkcode)
			{
			case VK_UP:
				isMoveUp = true;
				break;
			case VK_DOWN:
				isMoveDown = true;
				break;
			case VK_LEFT:
				isMoveLeft = true;
				break;
			case VK_RIGHT:
				isMoveRight = true;
				break;
			case VK_CONTROL:			//绝招：一键清敌
				clearEnemy = true;
				break;
			}
		}
		//松开按键处理
		if (msg.message == WM_KEYUP)
		{
			switch (msg.vkcode)
			{
			case VK_UP:
				isMoveUp = false;
				break;
			case VK_DOWN:
				isMoveDown = false;
				break;
			case VK_LEFT:
				isMoveLeft = false;
				break;
			case VK_RIGHT:
				isMoveRight = false;
				break;
			}
		}
	}

	//玩家移动
	void Move()
	{
		//计算不同方向(包括同时)按下时的速度增量
		int directX = isMoveRight - isMoveLeft;
		int directY = isMoveDown - isMoveUp;
		double directXY = sqrt(directX * directX + directY * directY);
		if (directXY != 0)
		{
			double factorX = directX / directXY;
			double factorY = directY / directXY;
			peaPositon.x += (int)PEA_SPEED * factorX;		//增速 * X方向的标准化分量
			peaPositon.y += (int)PEA_SPEED * factorY;		//增速 * Y方向的标准化分量
		}

		//玩家越界处理
		if (peaPositon.x < 0)								//左边界检测
			peaPositon.x = 0;
		if (peaPositon.x + PEA_IMG_WIDTH > WIN_WIDTH)	//右边界检测
			peaPositon.x = WIN_WIDTH - PEA_IMG_WIDTH;
		if (peaPositon.y < 0)								//上边界检测
			peaPositon.y = 0;
		if (peaPositon.y + PEA_IMG_HEIGHT > WIN_HEIGHT)	//下边界检测
			peaPositon.y = WIN_HEIGHT - PEA_IMG_HEIGHT;
	}

	//绘制玩家
	void Draw()
	{
		aniPea->Play(peaPositon.x, peaPositon.y, 1000 / GAME_FRAME);		// 绘制豌豆射手
	}

	POINT& GetPosition()
	{
		return peaPositon;
	}

	int GetPeaImgWidth()
	{
		return PEA_IMG_WIDTH;
	}

	int GetPeaImgHeight()
	{
		return PEA_IMG_HEIGHT;
	}

private:
	POINT peaPositon = { 500, 300 };		//豌豆初始位置
	const int PEA_SPEED = 5;				//豌豆移动速度
	const int PEA_IMG_WIDTH = 75;			//豌豆图片宽度
	const int PEA_IMG_HEIGHT = 75;			//豌豆图片高度		
	Animation* aniPea;						//豌豆动画
	bool isMoveUp = false;					//是否向上、下、左、右四个方向移动
	bool isMoveDown = false;
	bool isMoveLeft = false;
	bool isMoveRight = false;
};

//子弹类
class Bullet
{
public:
	POINT position = { 0, 0 };

	Bullet() = default;
	~Bullet() = default;

	void Draw()
	{
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(50, 255, 50));
		fillcircle(position.x, position.y, RADIUS);
	}

private:
	const int RADIUS = 10;
};

//敌人类
class Enemy
{
public:
	Enemy()
	{
		aniZombie = new Animation(_T("image\\zombie\\"), 22, 45);

		//敌人出现的边界
		enum class Edge
		{
			Up = 0,
			Down,
			Left,
			Right
		};

		//随机生成敌人出现时的边界，并设置其坐标
		Edge edge = (Edge)(rand() % 4);
		switch (edge)
		{
		case Edge::Up:
			zombiePosition.x = rand() % WIN_WIDTH;
			zombiePosition.y = -ZOMBIE_IMG_HEIGHT;
			break;
		case Edge::Down:
			zombiePosition.x = rand() % WIN_WIDTH;
			zombiePosition.y = WIN_HEIGHT;
			break;
		case Edge::Left:
			zombiePosition.x = -ZOMBIE_IMG_WIDTH;
			zombiePosition.y = rand() % WIN_HEIGHT;
			break;
		case Edge::Right:
			zombiePosition.x = WIN_WIDTH;
			zombiePosition.y = rand() % WIN_HEIGHT;
			break;
		default:
			break;
		}
	}

	~Enemy()
	{
		delete aniZombie;
	}

	//敌人移动
	void Move(Player* player)
	{
		//获取玩家位置		
		POINT& playerPosition = player->GetPosition();

		//计算不同方向(包括同时)按下时的速度增量
		int directX = playerPosition.x - zombiePosition.x;
		int directY = playerPosition.y - zombiePosition.y;
		double directXY = sqrt(directX * directX + directY * directY);
		if (directXY != 0)
		{
			double factorX = directX / directXY;
			double factorY = directY / directXY;
			zombiePosition.x += (int)ZOMBIE_SPEED * factorX;		//增速 * X方向的标准化分量
			zombiePosition.y += (int)ZOMBIE_SPEED * factorY;		//增速 * Y方向的标准化分量
		}
	}

	//绘制敌人
	void Draw()
	{
		aniZombie->Play(zombiePosition.x, zombiePosition.y, 1000 / GAME_FRAME);		// 绘制僵尸图片

	}

	//检测敌人与子弹碰撞
	bool CheckBulletCollision(Bullet& bullet)
	{
		//将子弹看做一个点，判断其是否落在敌人图片所在矩形区域
		if (bullet.position.x > zombiePosition.x && bullet.position.x < zombiePosition.x + ZOMBIE_IMG_WIDTH &&
			bullet.position.y > zombiePosition.y && bullet.position.y < zombiePosition.y + ZOMBIE_IMG_HEIGHT)
			return true;
		return false;
	}

	//检测敌人与玩家碰撞
	bool CheckPlayerCollision(Player& player)
	{
		//计算敌人中心点坐标
		int centerX = zombiePosition.x + ZOMBIE_IMG_WIDTH / 2;
		int centerY = zombiePosition.y + ZOMBIE_IMG_HEIGHT / 2;

		//将敌人看做一个点，判断其是否落在玩家图片所在矩形区域
		if (centerX > player.GetPosition().x && centerX < player.GetPosition().x + player.GetPeaImgWidth() &&
			centerY > player.GetPosition().y && centerY < player.GetPosition().y + player.GetPeaImgHeight())
			return true;
		return false;
	}

	//敌人受击处理
	void Hurt()
	{
		alive = false;
	}

	//判断敌人是否存活
	bool CheckAlive()
	{
		return alive;
	}

private:
	POINT zombiePosition = { 0, 0 };		//僵尸位置 
	const int ZOMBIE_SPEED = 2;				//僵尸移动速度
	const int ZOMBIE_IMG_WIDTH = 166;		//僵尸图片宽度
	const int ZOMBIE_IMG_HEIGHT = 144;		//僵尸图片高度
	Animation* aniZombie;					//僵尸动画
	bool alive = true;						//僵尸是否存活
};

//生成新的敌人
void GenerateEnemy(vector<Enemy*>& enemyList)
{
	const int INTERVAL = 100;
	static int counter = 0;					//计数器
	if ((++counter) % INTERVAL == 0)
		enemyList.push_back(new Enemy());	//创建并添加敌人到列表
}

//子弹随玩家位置而变化
void UpdateBullets(vector<Bullet>& bulletList, Player& player)
{
	const double RADIAL_SPEED = 0.001;								//径向波动速度
	const double TANGENT_SPEED = 0.003;								//切向波动速度
	double radianInterval = 2 * 3.1415926 / bulletList.size();		//子弹之间的弧度间隔
	double radius = 150 + 50 * sin(GetTickCount() * RADIAL_SPEED);	//子弹径向波动的半径（根据运行时间变化）
	POINT playerPosition = player.GetPosition();
	for (int i = 0; i < bulletList.size(); i++)
	{
		double radian = GetTickCount() * TANGENT_SPEED + radianInterval * i; //当前子弹所在的弧度值
		bulletList[i].position.x = playerPosition.x + player.GetPeaImgWidth() / 2 + (int)(radius * cos(radian));
		bulletList[i].position.y = playerPosition.y + player.GetPeaImgHeight() / 2 + (int)(radius * sin(radian));
	}

}

//绘制玩家得分
void DrawScore(int score)
{
	TCHAR txtScore[64];
	_stprintf_s(txtScore, _T("当前得分：%d"), score);
	setbkmode(TRANSPARENT);							//设置文字透明背景
	settextcolor(RGB(0, 0, 255));					//设置文字颜色
	settextstyle(40, 0, _T("微软雅黑"));			//设置字体
	outtextxy(WIN_WIDTH / 2 - textwidth(txtScore) / 2, 10, txtScore);	//绘制文字
}

//加载声音
void LoadAudio()
{
	mciSendString(_T("open audio\\bg.mp3 alias bgm"), NULL, 0, NULL);		//背景音乐
	mciSendString(_T("open audio\\hit.wav alias hit"), NULL, 0, NULL);		//子弹和敌人的撞击声
}

//播放背景音乐
void PlayBGM()
{
	mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
}

//播放子弹打击敌人的声音
void PlayHit()
{
	mciSendString(_T("play hit from 0"), NULL, 0, NULL);
}

int main()
{
	bool continueGame = true;						//游戏主循环控制
	bool gameOver = false;							//当前局循环控制
	ExMessage msg;									//键鼠消息
	IMAGE imgBG;									//背景图片对象
	loadimage(&imgBG, _T("image\\background.jpg"));	//加载背景图片
	Player* player = new Player();					//创建玩家
	vector<Enemy*> enemyList;						//敌人列表
	vector<Bullet> bulletList(3);					//子弹列表
	int score = 0;									//玩家得分
	LoadAudio();									//加载游戏中的声音

	initgraph(WIN_WIDTH, WIN_HEIGHT);
	BeginBatchDraw();
	PlayBGM();										//播放背景音乐				

	while (continueGame)
	{
		while (!gameOver)
		{
			ULONGLONG beginTime = GetTickCount64();

			//消息处理
			while (peekmessage(&msg))
				player->ProcessEvent(msg);

			//玩家移动
			player->Move();

			//更新子弹
			UpdateBullets(bulletList, *player);

			//生成敌人
			GenerateEnemy(enemyList);

			//敌人移动
			for (Enemy* enemy : enemyList)
				enemy->Move(player);

			//检测敌人是否与玩家发生碰撞
			for (Enemy* enemy : enemyList)
			{
				if (enemy->CheckPlayerCollision(*player))
				{
					TCHAR message[256];
					_stprintf_s(message, _T("你的得分是：%d"), score);
					MessageBox(GetHWnd(), message, _T("游戏结束"), MB_OK);		//游戏结束，显示弹窗消息
					gameOver = true;
					continueGame = false;
					break;
				}
			}

			//检测敌人是否与子弹发生碰撞
			for (Enemy* enemy : enemyList)
			{
				for (Bullet& bullet : bulletList)
				{
					if (enemy->CheckBulletCollision(bullet))
					{
						enemy->Hurt();		//修改敌人状态
						PlayHit();
						score++;			//玩家得分
					}
				}
			}

			//清除alive状态为false的敌人
			for (int i = 0; i < enemyList.size(); i++)
			{
				Enemy* enemy = enemyList[i];				//暂存当前敌人
				if (!enemyList[i]->CheckAlive())
				{
					swap(enemyList[i], enemyList.back());	//当前敌人和列表中最后一个敌人交换位置
					enemyList.pop_back();					//删除列表中最后一个敌人
					delete enemy;							//回收敌人的内存空间
				}
			}

			// 一键清敌
			if (clearEnemy)
			{
				enemyList.clear();
				clearEnemy = false;
			}

			//绘图
			cleardevice();
			putimage(0, 0, &imgBG);				//绘制背景图片
			player->Draw();						//绘制玩家
			for (Enemy* enemy : enemyList)		//绘制敌人
				enemy->Draw();
			for (Bullet& bullet : bulletList)	//绘制子弹
				bullet.Draw();
			DrawScore(score);					//绘制得分
			FlushBatchDraw();

			//帧延时
			ULONGLONG endTime = GetTickCount64();
			ULONGLONG elapsedTime = endTime - beginTime;
			if (elapsedTime < 1000 / GAME_FRAME)
				Sleep(1000 / GAME_FRAME - elapsedTime);
		}
	}
	delete player;								//删除玩家

	EndBatchDraw();
	closegraph();
	return 0;
}