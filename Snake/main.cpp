#define NOMINMAX

#include <vector>
#include <limits>
#include <random>
#include <ctime>
#include <memory>
#include <algorithm>
#include <Windows.h>
#include <tchar.h>
#include "resource.h"

enum Error 
{ 
	ClassRegErr,
	CreateWndErr,
	AlreadyExistErr,
};

enum UserMessage
{
	GameOver	= WM_APP + 1,
	MaxSnake	= WM_APP + 2,
};

constexpr uint32_t BlockSize = 32;

inline bool operator == (const POINT& p1, const POINT& p2) { return p1.x == p2.x && p1.y == p2.y; }
inline bool operator != (const POINT& p1, const POINT& p2) { return !(p1 == p2); }

template<typename T>
class RandGen
{
public:
	RandGen(T minVal = 0, T maxVal = std::numeric_limits<T>::max()) : defEngine((uint32_t)time(0)), intDistr(minVal, maxVal) {}
	T operator () () { return intDistr(defEngine); }
private:
	std::default_random_engine defEngine;
	std::uniform_int_distribution<T> intDistr;
};

class Timer
{
public:
	Timer();
	void Tick();
	void Reset();
	double Elapsed() const;
private:
	double freq;
	LARGE_INTEGER startPoint;
	LARGE_INTEGER curr;
};

class Snake
{
public:
	enum Direction 
	{
		LEFT = VK_LEFT, 
		UP = VK_UP, 
		RIGHT = VK_RIGHT,
		DOWN = VK_DOWN, 
	};

	Snake(uint32_t fieldSize);
	~Snake();

	Direction GetDirection() const;
	void SetDirection(Direction d);
	POINT GetHead() const;
	uint32_t BodySize() const;
	void Eat();
	bool Move(uint32_t fieldSize);
	bool IsBody(POINT testPoint) const;
	void Draw(HDC hdc) const;
	const std::vector<POINT>& Body() const;
private:
	std::vector<POINT> body;
	uint32_t headInd;
	Direction dir;
	HBITMAP hBitMap;
	bool foodEaten;
};

class Food
{
public:
	Food();
	~Food();
	POINT GetPos() const;
	void SetPos(POINT p);
	void Draw(HDC hdc) const;
private:
	HBITMAP hBitMap;
	POINT pos;
};

class App
{
public:
	static App* GetApp();
	static HINSTANCE AppInstance();
	App(HINSTANCE hInstance, int showCmd);
	~App();
	int Run();
private:
	static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	ATOM RegisterWindowClass();
	void CreateMainWindow(int showCmd);
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnPaint();
	void SpawnFood();
	void Update();
private:
	static const uint32_t FieldSize = 10;

	static App* pApp;
	static HINSTANCE hInst;

	HWND hMainWnd;

	std::unique_ptr<Snake> snake;
	std::unique_ptr<Food> food;
	std::unique_ptr<Timer> timer;

	double timeStep;
	bool running;
};
App* App::pApp = nullptr;
HINSTANCE App::hInst = nullptr;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int nShowCmd)
{
	App app(hInstance, nShowCmd);
	return app.Run();
}

// Timer class methods ------------------------------------------------------------------------------------------------
Timer::Timer()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	freq = static_cast<double>(li.QuadPart);
	Reset();
}
inline void Timer::Tick() 
{
	QueryPerformanceCounter(&curr);
}
inline void Timer::Reset() 
{
	QueryPerformanceCounter(&startPoint);
	curr.QuadPart = startPoint.QuadPart;
}
inline double Timer::Elapsed() const { return static_cast<double>(curr.QuadPart - startPoint.QuadPart) / freq; }


// Snake class methods ------------------------------------------------------------------------------------------------
Snake::Snake(uint32_t fieldSize) : headInd(3), dir(Direction::UP), foodEaten(false)
{
	hBitMap = LoadBitmap(App::AppInstance(), MAKEINTRESOURCE(IDB_BITMAP1));
	POINT p = { (LONG)fieldSize / 2, (LONG)fieldSize / 2 + 2 };
	for(int i = 0; i < 4; ++i, --p.y)
		body.emplace_back(p);
}
Snake::~Snake() { DeleteObject(hBitMap); }
inline Snake::Direction Snake::GetDirection() const { return dir; }
inline void Snake::SetDirection(Direction d) { dir = d; }
inline POINT Snake::GetHead() const { return body[headInd]; }
inline const std::vector<POINT>& Snake::Body() const { return body; }
inline uint32_t Snake::BodySize() const { return body.size(); }
inline void Snake::Eat() { foodEaten = true; }
bool Snake::Move(uint32_t fieldSize)
{
	POINT nextHead = body[headInd];
	switch(dir)
	{
		case Snake::UP: --nextHead.y; break;
		case Snake::DOWN: ++nextHead.y; break;
		case Snake::LEFT: --nextHead.x; break;
		case Snake::RIGHT: ++nextHead.x; break;
	}

	if(nextHead.x < 0 || nextHead.x == fieldSize 
		|| nextHead.y < 0 || nextHead.y == fieldSize 
		|| IsBody(nextHead))
		return false;

	if(!foodEaten)
	{
		headInd = (headInd + 1) % body.size();
		body[headInd] = nextHead;
	}
	else
	{
		++headInd;
		body.insert(body.begin() + headInd, nextHead);
		foodEaten = false;
	}
	return true;
}
bool Snake::IsBody(POINT testPoint) const
{
	for(const auto& p : body)
		if(p == testPoint)
			return true;
	return false;
}
void Snake::Draw(HDC hdc) const
{
	HDC dc = CreateCompatibleDC(hdc);
	HBITMAP old = (HBITMAP)SelectObject(dc, hBitMap);

	for(auto& p : body)
	{
		StretchBlt(hdc, p.x * BlockSize, p.y * BlockSize, BlockSize, BlockSize, dc, 0, 0, 32, 32, SRCCOPY);
	}

	SelectObject(dc, old);
	DeleteDC(dc);
}

// Food class methods ------------------------------------------------------------------------------------------------
Food::Food()
{
	hBitMap = LoadBitmap(App::AppInstance(), MAKEINTRESOURCE(IDB_BITMAP2));
	pos.x = pos.y = 0;
}
Food::~Food() { DeleteObject(hBitMap); }
inline POINT Food::GetPos() const { return pos; }
inline void Food::SetPos(POINT p) { pos = p; }
void Food::Draw(HDC hdc) const
{
	HDC dc = CreateCompatibleDC(hdc);
	HBITMAP old = (HBITMAP)SelectObject(dc, hBitMap);

	StretchBlt(hdc, pos.x * BlockSize, pos.y * BlockSize, BlockSize, BlockSize, dc, 0, 0, 32, 32, SRCCOPY);

	SelectObject(dc, old);
	DeleteDC(dc);
}

// App class methods ------------------------------------------------------------------------------------------------
App::App(HINSTANCE hInstance, int showCmd) : timeStep(0.3), running(false)
{
	if(pApp)
		throw Error::AlreadyExistErr;

	pApp = this;
	hInst = hInstance;
	snake = std::make_unique<Snake>(FieldSize);
	food = std::make_unique<Food>();
	timer = std::make_unique<Timer>();
	SpawnFood();

	if(!RegisterWindowClass())
		throw Error::ClassRegErr;

	CreateMainWindow(showCmd);
}
App::~App() 
{ 
	UnregisterClass(_T("SnakeMainWndClass"), hInst);
	pApp = nullptr; 
}
inline App* App::GetApp() { return App::pApp; }
inline HINSTANCE App::AppInstance() { return hInst; }
int App::Run()
{
	MSG msg = { 0 };
	running = true;
	timer->Reset();
	try
	{
		while(msg.message != WM_QUIT)
		{
			if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else if(running)
			{
				timer->Tick();
				if(timeStep < timer->Elapsed())
				{
					Update();
					timer->Reset();
				}
			}
		}
	}
	catch(Error err)
	{
		const TCHAR* errMsg = nullptr;
		switch(err)
		{
			case ClassRegErr: errMsg = _T("Window class registration failed."); break;
			case CreateWndErr: errMsg = _T("Window creation failed."); break;
			case AlreadyExistErr: errMsg = _T("App class object already exists."); break;
			default: errMsg = _T("WTF?"); break;
		}
		MessageBox(0, errMsg, _T("Error"), MB_OK);
	}
	catch(std::bad_alloc&)
	{
		MessageBox(0, _T("Not enough memory."), _T("Error"), MB_OK);
	}
	return (int)msg.wParam;
}
LRESULT CALLBACK App::MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return App::GetApp()->WndProc(hwnd, msg, wParam, lParam);
}
ATOM App::RegisterWindowClass()
{
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
	wcex.lpfnWndProc = App::MainProc;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(hInst, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wcex.lpszClassName = _T("SnakeMainWndClass");

	return RegisterClassEx(&wcex);
}
void App::CreateMainWindow(int showCmd)
{
	RECT rc = { 0, 0, FieldSize * BlockSize, FieldSize * BlockSize };
	AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0);
	hMainWnd = CreateWindowEx(0, _T("SnakeMainWndClass"), _T("Snake game"), WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
		0, 0, hInst, nullptr);

	if(!hMainWnd)
		throw Error::CreateWndErr;

	ShowWindow(hMainWnd, showCmd);
	UpdateWindow(hMainWnd);
}
LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_KEYDOWN:
			if(VK_LEFT - 1 < wParam && wParam < VK_DOWN + 1)
				snake->SetDirection(static_cast<Snake::Direction>(wParam));
			break;
		case WM_PAINT: OnPaint(); break;
		case WM_DESTROY: PostQuitMessage(0); break;
		default: return DefWindowProc(hwnd, msg, wParam, lParam); break;
	}
	return 0;
}
void App::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hMainWnd, &ps);

	snake->Draw(hdc);
	food->Draw(hdc);

	EndPaint(hMainWnd, &ps);
}
void App::SpawnFood()
{
	std::vector<uint32_t> freeInd(FieldSize * FieldSize);
	uint32_t val = 0;
	for(auto& i : freeInd)
		i = val++;
	for(const auto& p : snake->Body())
		freeInd[FieldSize * p.y + p.x] = -1;
	freeInd.erase(std::remove(freeInd.begin(), freeInd.end(), (uint32_t)-1), freeInd.end());

	RandGen<uint32_t> rgen(0, freeInd.size() - 1);
	val = freeInd[rgen()];
	POINT p = { (LONG)(val % FieldSize), (LONG)(val / FieldSize) };
	food->SetPos(p);
}
void App::Update()
{
	if(!snake->Move(FieldSize))
	{
		MessageBox(hMainWnd, _T("GAME OVER!"), _T("Message"), MB_OK);
		running = false;
		return;
	}
	if(snake->BodySize() == FieldSize * FieldSize)
	{
		MessageBox(hMainWnd, _T("You reached maximum snake length!"), _T("Message"), MB_OK);
		running = false;
		return;
	}
	if(snake->GetHead() == food->GetPos())
	{
		snake->Eat();
		SpawnFood();
	}
	RedrawWindow(hMainWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
	//PostMessage(hMainWnd, WM_PAINT, 0, 0);
}