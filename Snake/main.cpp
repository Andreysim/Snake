#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define NOMINMAX

#include <vector>
#include <tuple>
#include <limits>
#include <random>
#include <ctime>
#include <memory>
#include <algorithm>
#include <tchar.h>
#include <Windows.h>
#include <CommCtrl.h>
#include "resource.h"

typedef std::tuple<uint32_t, uint32_t> OptDlgParam;

enum class Error 
{ 
    ClassRegErr,
    CreateWndErr,
    AlreadyExistErr,
};

constexpr int nButtons = 3;
constexpr int BtnSize = 24;
constexpr uint32_t BlockSize = 24;
constexpr uint32_t MinWidth = 8;
constexpr uint32_t MinHeight = 8;
constexpr uint32_t MaxWidth = 32;
constexpr uint32_t MaxHeight = 32;
constexpr COLORREF BkColor = RGB(192, 192, 192);


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
        UP = 0, 
        DOWN = 1, 
        RIGHT = 2,
        LEFT = 3, 
    };

    Snake();
    ~Snake();

    void Reset(uint32_t fieldWidth, uint32_t fieldHeight);
    Direction GetDirection() const;
    void SetDirection(Direction d);
    POINT GetHead() const;
    uint32_t BodySize() const;
    void Eat();
    bool Move(uint32_t fieldWidth, uint32_t fieldHeight);
    bool IsValidDirection(Direction dir) const;
    bool IsBody(POINT testPoint) const;
    void Draw(HDC hdc, uint32_t indent) const;
    const std::vector<POINT>& Body() const;

private:
    void DrawBlock(HDC hdc, POINT p, uint32_t indent, int imgInd) const;
    Direction GetDirection(POINT p1, POINT p2) const;

    std::vector<POINT> body;
    uint32_t headInd;
    Direction dir;
    HIMAGELIST hImgList;
    bool foodEaten;
};

class Food
{
public:
    Food();
    ~Food();
    POINT GetPos() const;
    void SetPos(POINT p);
    void Draw(HDC hdc, uint32_t indent) const;

private:
    HBITMAP hBitMap;
    POINT pos;
};

struct ToolBar
{
    enum ImgInd { OptImg, NewGameImg, UnpauseImg, PauseImg, CountImg };
    ToolBar();
    ToolBar(DWORD style, int x, int y, int w, int h, HWND parent, UINT id, HINSTANCE hInst, UINT imgId);
    void Destroy();

    HWND hToolBar;
    HWND hStaticScore;
    HIMAGELIST hImgList;
};

class App
{
public:
    static App* GetApp();
    static HINSTANCE AppInstance();

    App(HINSTANCE hInstance, int showCmd);

    App(const App&) = delete;
    App(App&&) = default;

    App& operator = (const App&) = delete;
    App& operator = (App&&) = default;

    ~App();
    int Run();

private:
    static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK OptionsDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    ATOM RegisterWindowClass();
    void CreateMainWindow(int showCmd);
    void EndGame();
    void ResizeGameArea(uint32_t w, uint32_t h);
    int KeyPressed(int vKey) const;
    void OnKeyboardInput();
    void OnNotify(HWND hwnd, int id, LPNMHDR phdr);
    void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
    void OnPaint();
    void Options();
    void NewGame();
    void Pause();
    void SpawnFood();
    void Update();
    void OutScore() const;

private:
    static App* pApp;
    static HINSTANCE hInst;

    HWND hMainWnd;
    ToolBar toolBar;

    uint32_t vertIndent;
    uint32_t width;
    uint32_t height;
    uint32_t score;

    std::unique_ptr<Snake> snake;
    std::unique_ptr<Food> food;
    std::unique_ptr<Timer> timer;

    bool running;
    bool paused;
    double timeStep;
};
App* App::pApp = nullptr;
HINSTANCE App::hInst = nullptr;

//########################################################################################################################

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
inline double Timer::Elapsed() const 
{
    return static_cast<double>(curr.QuadPart - startPoint.QuadPart) / freq; 
}

// Toolbar struct methods ------------------------------------------------------------------------------------------------
ToolBar::ToolBar() : hToolBar(nullptr) {}
ToolBar::ToolBar(DWORD style, int x, int y, int w, int h, HWND parent, UINT id, HINSTANCE hInst, UINT imgId)
{
    hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, nullptr, style, x, y, w, h, parent, (HMENU)id, hInst, nullptr);
    hImgList = ImageList_Create(BtnSize, BtnSize, ILC_COLOR32 | ILC_MASK, ImgInd::CountImg, 0);

    HBITMAP hTBBM = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(imgId), IMAGE_BITMAP, ImgInd::CountImg * BtnSize, BtnSize, 0);
    HBITMAP hMaskBM = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(imgId), IMAGE_BITMAP, ImgInd::CountImg * BtnSize, BtnSize, LR_MONOCHROME);
    ImageList_SetBkColor(hImgList, RGB(255, 255, 255));
    ImageList_Add(hImgList, hTBBM, hMaskBM);
    DeleteObject(hMaskBM);
    DeleteObject(hTBBM);
    
    TBBUTTON tbBtns[nButtons] =
    {
        { 0, ID_OPT_BTN,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_NOPREFIX | BTNS_AUTOSIZE, {0}, 0, 0 },
        { 1, ID_NEW_GAME_BTN, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_NOPREFIX | BTNS_AUTOSIZE, {0}, 0, 0 },
        { 2, ID_PAUSE_BTN,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_NOPREFIX | BTNS_AUTOSIZE, {0}, 0, 0 }
    };

    SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM)hImgList);
    SendMessage(hToolBar, TB_ADDBUTTONS, (WPARAM)nButtons, (LPARAM)&tbBtns);
    SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);

    RECT rc;
    SendMessage(hToolBar, TB_GETRECT, ID_PAUSE_BTN, (LPARAM)&rc);

    hStaticScore = CreateWindowEx(0, WC_STATIC, _T("SCORE: 0"),
        WS_CHILD | WS_VISIBLE |  SS_LEFTNOWORDWRAP | SS_CENTERIMAGE, 
        rc.right + 10, rc.top, 100, rc.bottom - rc.top,
        hToolBar, (HMENU)ID_SCORE, hInst, nullptr);

    ShowWindow(hToolBar, SW_SHOW);
}
inline void ToolBar::Destroy() 
{
    DestroyWindow(hStaticScore);
    DestroyWindow(hToolBar);
    ImageList_Destroy(hImgList);
}

// Snake class methods ------------------------------------------------------------------------------------------------
Snake::Snake() : headInd(~0), dir(Direction::UP), foodEaten(false)
{
    constexpr int ImgCnt = 9;

    HBITMAP hSnakeBM = (HBITMAP)LoadImage(App::AppInstance(), MAKEINTRESOURCE(IDB_SNAKE_FULL), IMAGE_BITMAP, BlockSize * ImgCnt, BlockSize, 0);
    HBITMAP hSnakeBMMask = (HBITMAP)LoadImage(App::AppInstance(), MAKEINTRESOURCE(IDB_SNAKE_FULL), IMAGE_BITMAP, BlockSize * ImgCnt, BlockSize, LR_MONOCHROME);

    hImgList = ImageList_Create(BlockSize, BlockSize, ILC_COLOR32 | ILC_MASK, ImgCnt, 0);
    ImageList_SetBkColor(hImgList, BkColor);
    ImageList_Add(hImgList, hSnakeBM, hSnakeBMMask);

    DeleteObject(hSnakeBM);
    DeleteObject(hSnakeBMMask);
}
Snake::~Snake() { ImageList_Destroy(hImgList); }
inline Snake::Direction Snake::GetDirection() const { return dir; }
inline void Snake::SetDirection(Direction d) { dir = d; }
inline POINT Snake::GetHead() const { return body[headInd]; }
inline const std::vector<POINT>& Snake::Body() const { return body; }
inline uint32_t Snake::BodySize() const { return body.size(); }
inline void Snake::Eat() { foodEaten = true; }
inline void Snake::DrawBlock(HDC hdc, POINT p, uint32_t indent, int imgInd) const
{
    ImageList_Draw(hImgList, imgInd, hdc, p.x * BlockSize, p.y * BlockSize, ILD_NORMAL);
}
void Snake::Reset(uint32_t fieldWidth, uint32_t fieldHeight)
{
    body.clear();
    POINT p = { (LONG)fieldWidth / 2, (LONG)fieldHeight / 2 + 2 };
    for(int i = 0; i < 4; ++i, --p.y)
        body.emplace_back(p);

    headInd = body.size() - 1;
    dir = Direction::UP;
    foodEaten = false;
}
bool Snake::Move(uint32_t fieldWidth, uint32_t fieldHeight)
{
    POINT nextHead = body[headInd];
    switch(dir)
    {
        case Snake::UP: --nextHead.y; break;
        case Snake::DOWN: ++nextHead.y; break;
        case Snake::LEFT: --nextHead.x; break;
        case Snake::RIGHT: ++nextHead.x; break;
    }

    if(nextHead.x < 0 || nextHead.x == fieldWidth 
        || nextHead.y < 0 || nextHead.y == fieldHeight 
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
Snake::Direction Snake::GetDirection(POINT p1, POINT p2) const
{
    if(p1.x == p2.x)
        return p1.y < p2.y ? Direction::UP : Direction::DOWN;
    else
        return p1.x < p2.x ? Direction::LEFT : Direction::RIGHT;
}
bool Snake::IsValidDirection(Direction testDir) const
{
    POINT prevHead = body[headInd == 0 ? body.size() - 1 : headInd - 1];
    return (GetDirection(prevHead, body[headInd]) != testDir);
}
bool Snake::IsBody(POINT testPoint) const
{
    for(const auto& p : body)
        if(p == testPoint)
            return true;
    return false;
}
void Snake::Draw(HDC hdc, uint32_t indent) const
{
    uint32_t tailInd = (headInd + 1) % body.size();

    for(uint32_t i = 0; i < body.size(); ++i)
    {
        if(i != headInd && i != tailInd)
            DrawBlock(hdc, body[i], indent, 8);
    }
    Direction tailDir = GetDirection(body[(tailInd + 1) % body.size()], body[tailInd]);
    DrawBlock(hdc, body[headInd], indent, (int)dir);
    DrawBlock(hdc, body[tailInd], indent, (int)tailDir + 4);
}

// Food class methods ------------------------------------------------------------------------------------------------
Food::Food()
{
    hBitMap = LoadBitmap(App::AppInstance(), MAKEINTRESOURCE(IDB_FOOD));
    pos.x = pos.y = 0;
}
Food::~Food() { DeleteObject(hBitMap); }
inline POINT Food::GetPos() const { return pos; }
inline void Food::SetPos(POINT p) { pos = p; }
void Food::Draw(HDC hdc, uint32_t indent) const
{
    HDC dc = CreateCompatibleDC(hdc);
    HBITMAP old = (HBITMAP)SelectObject(dc, hBitMap);

    TransparentBlt(hdc, pos.x * BlockSize, pos.y * BlockSize, BlockSize, BlockSize, dc, 0, 0, 32, 32, RGB(255, 255, 255));

    SelectObject(dc, old);
    DeleteDC(dc);
}

// App class methods ------------------------------------------------------------------------------------------------
LRESULT CALLBACK App::MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return App::GetApp()->WndProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_PAINT: OnPaint(); break;
        case WM_KEYDOWN: OnKeyDown(hwnd, (UINT)wParam, TRUE, (int)LOWORD(lParam), (UINT)HIWORD(lParam)); break;
        case WM_NOTIFY: OnNotify(hwnd, (int)wParam, (LPNMHDR)lParam); break;
        case WM_CREATE:
        {
            toolBar = ToolBar(WS_CHILD | WS_BORDER | TBSTYLE_TOOLTIPS, 0, 0, width * BlockSize, BtnSize, hwnd, ID_TOOLBAR, hInst, IDB_TOOLBAR);
            RECT rc;
            GetWindowRect(toolBar.hToolBar, &rc);
            vertIndent = rc.bottom - rc.top - 1;
            NewGame();
            break;
        }
        case WM_CLOSE:
            toolBar.Destroy();
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
INT_PTR CALLBACK App::OptionsDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static OptDlgParam* pParam = nullptr;
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            pParam = (OptDlgParam*)lParam;
            SetDlgItemInt(hwnd, IDC_WIDTH_EDIT, std::get<0>(*pParam), FALSE);
            SetDlgItemInt(hwnd, IDC_HEIGHT_EDIT, std::get<1>(*pParam), FALSE);
        }
        case WM_COMMAND:
        {
            if(wParam == IDOK)
            {
                uint32_t val = 0;
                val = GetDlgItemInt(hwnd, IDC_WIDTH_EDIT, nullptr, FALSE);
                if(val < MinWidth || val > MaxWidth)
                {
                    TCHAR buf[64] = { 0 };
                    _stprintf_s(buf, _T("Width must be in range [%d, %d]"), MinWidth, MaxWidth);
                    MessageBox(hwnd, buf, _T("Error"), MB_OK);
                    break;
                }
                std::get<0>(*pParam) = val;

                val = GetDlgItemInt(hwnd, IDC_HEIGHT_EDIT, nullptr, FALSE);
                if(val < MinHeight || val > MaxHeight)
                {
                    TCHAR buf[64] = { 0 };
                    _stprintf_s(buf, _T("Height must be in range [%d, %d]"), MinHeight, MaxHeight);
                    MessageBox(hwnd, buf, _T("Error"), MB_OK);
                    break;
                }
                std::get<1>(*pParam) = val;

                EndDialog(hwnd, 1);
                break;
            }
            else if(wParam == IDCANCEL)
                EndDialog(hwnd, 0);
            break;
        }
        default:
            return FALSE;
    }
    return TRUE;
}
App::App(HINSTANCE hInstance, int showCmd) : vertIndent(0), width(10), height(10), score(0), timeStep(0.3), running(false), paused(false)
{
    if(pApp)
        throw Error::AlreadyExistErr;

    pApp = this;
    hInst = hInstance;

    snake = std::make_unique<Snake>();
    food = std::make_unique<Food>();
    timer = std::make_unique<Timer>();

    INITCOMMONCONTROLSEX iccex = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES };
    InitCommonControlsEx(&iccex);

    if(!RegisterWindowClass())
        throw Error::ClassRegErr;
    CreateMainWindow(showCmd);
}
App::~App() 
{ 
    UnregisterClass(_T("SnakeMainWndClass"), hInst);
    pApp = nullptr; 
}

inline App* App::GetApp() {    return App::pApp; }
inline HINSTANCE App::AppInstance() { return hInst; }
inline int App::KeyPressed(int vKey) const { return 0x8000 & GetAsyncKeyState(vKey); }
inline void App::OutScore() const
{
    TCHAR buf[16] = { 0 };
    _stprintf_s(buf, _T("SCORE: %d"), score);
    SetWindowText(toolBar.hStaticScore, buf);
}
inline void App::EndGame() { running = false; }
inline void App::Pause()
{
    if(running)
    {
        SendMessage(toolBar.hToolBar, TB_CHANGEBITMAP, (WPARAM)ID_PAUSE_BTN, (LPARAM)(paused ? toolBar.PauseImg : toolBar.UnpauseImg));
        paused = !paused;
    }
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
void App::NewGame()
{
    running = true;
    paused = true;
    score = 0;
    snake->Reset(width, height);
    timer->Reset();
    SpawnFood();
    SendMessage(toolBar.hToolBar, TB_CHANGEBITMAP, ID_PAUSE_BTN, (LPARAM)toolBar.UnpauseImg);
    OutScore();
    RedrawWindow(hMainWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
}
void App::Options()
{
    OptDlgParam param(width, height);
    if(DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_OPTIONS_DIALOG), hMainWnd, OptionsDialogProc, (LPARAM)&param))
    {
        if(std::get<0>(param) != width || std::get<1>(param) != height)
        {
            ResizeGameArea(std::get<0>(param), std::get<1>(param));
            NewGame();
        }
    }
}
void App::CreateMainWindow(int showCmd)
{
    hMainWnd = CreateWindowEx(0, _T("SnakeMainWndClass"), _T("Snake game"), WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,    0, 0, hInst, nullptr);

    if(!hMainWnd)
        throw Error::CreateWndErr;

    ResizeGameArea(width, height);
    ShowWindow(hMainWnd, showCmd);
    UpdateWindow(hMainWnd);
}
void App::ResizeGameArea(uint32_t w, uint32_t h)
{
    width = std::max(MinWidth, std::min(w, MaxWidth));
    height = std::max(MinHeight, std::min(h, MaxHeight));
    RECT wndRect, adjRect = { 0, 0, (LONG)(width * BlockSize), (LONG)(height * BlockSize + vertIndent) };
    AdjustWindowRectEx(&adjRect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    GetWindowRect(hMainWnd, &wndRect);
    wndRect.right = adjRect.right - adjRect.left;
    wndRect.bottom = adjRect.bottom - adjRect.top;
    MoveWindow(hMainWnd, wndRect.left, wndRect.top, wndRect.right, wndRect.bottom, TRUE);
    SendMessage(toolBar.hToolBar, TB_AUTOSIZE, 0, 0);
}
void App::OnKeyboardInput()
{
    if(paused)
        return;
    if(KeyPressed(VK_LEFT) && snake->IsValidDirection(Snake::Direction::LEFT))
        snake->SetDirection(Snake::Direction::LEFT);
    else if(KeyPressed(VK_RIGHT) && snake->IsValidDirection(Snake::Direction::RIGHT))
        snake->SetDirection(Snake::Direction::RIGHT);
    else if(KeyPressed(VK_UP) && snake->IsValidDirection(Snake::Direction::UP))
        snake->SetDirection(Snake::Direction::UP);
    else if(KeyPressed(VK_DOWN) && snake->IsValidDirection(Snake::Direction::DOWN))
        snake->SetDirection(Snake::Direction::DOWN);
}
void App::OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    _CRT_UNUSED(hwnd);
    _CRT_UNUSED(fDown);
    _CRT_UNUSED(cRepeat);
    _CRT_UNUSED(flags);

    if(vk == 'P')
        Pause();
    else if(vk == 'O')
        Options();
    else if(vk == 'N')
        NewGame();
}
void App::OnNotify(HWND hwnd, int id, LPNMHDR phdr)
{
    _CRT_UNUSED(hwnd);
    _CRT_UNUSED(id);

    if(phdr->idFrom == ID_TOOLBAR)
    {
        if(phdr->code == TBN_GETINFOTIP)
        {
            LPNMTBGETINFOTIP ptbit = (LPNMTBGETINFOTIP)phdr;
            switch(ptbit->iItem)
            {
                case ID_OPT_BTN:
                    ptbit->pszText = _T("Options (O)");
                    break;
                case ID_NEW_GAME_BTN:
                    ptbit->pszText = _T("Start new game (N)");
                    break;
                case ID_PAUSE_BTN:
                    ptbit->pszText = paused ? _T("Resume (P)") : _T("Pause (P)");
                    break;
            }
            ptbit->cchTextMax = _tcslen(ptbit->pszText) + 1;
        }
        else if(phdr->code == NM_CLICK)
        {
            LPNMMOUSE pnmm = (LPNMMOUSE)phdr;
            switch(pnmm->dwItemSpec)
            {
                case ID_OPT_BTN: Options(); break;
                case ID_NEW_GAME_BTN: NewGame(); break;
                case ID_PAUSE_BTN: Pause(); break;
            }
        }
    }
}
void App::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hMainWnd, &ps);
    HDC hMemDC = CreateCompatibleDC(hdc);

    uint32_t pw = width * BlockSize, ph = height * BlockSize;

    HBITMAP hBitMap = CreateCompatibleBitmap(hdc, pw, ph);
    hBitMap = (HBITMAP)SelectObject(hMemDC, hBitMap);

    //Fill bitmap with BkColor color
    {
        HBRUSH hBr = CreateSolidBrush(BkColor);
        RECT rc = { 0, 0, (LONG)pw, (LONG)ph };
        FillRect(hMemDC, &rc, hBr);
        DeleteObject(hBr);
    }

    snake->Draw(hMemDC, vertIndent);
    if(running)
        food->Draw(hMemDC, vertIndent);

    // Copy bitmap to device
    BitBlt(hdc, 0, vertIndent, pw, ph, hMemDC, 0, 0, SRCCOPY);

    DeleteObject(SelectObject(hMemDC, hBitMap));
    DeleteDC(hMemDC);

    EndPaint(hMainWnd, &ps);
}
void App::SpawnFood()
{
    std::vector<uint32_t> freeInd(width * height);
    uint32_t val = 0;
    for(auto& i : freeInd)
        i = val++;
    for(const auto& p : snake->Body())
        freeInd[width * p.y + p.x] = std::numeric_limits<uint32_t>::max();
    freeInd.erase(std::remove(freeInd.begin(), freeInd.end(), (uint32_t)-1), freeInd.end());

    RandGen<uint32_t> rgen(0, freeInd.size() - 1);
    val = freeInd[rgen()];
    POINT p = { (LONG)(val % width), (LONG)(val / width) };
    food->SetPos(p);
}
void App::Update()
{
    if(!snake->Move(width, height))
    {
        MessageBox(hMainWnd, _T("GAME OVER!"), _T("Message"), MB_OK);
        EndGame();
        return;
    }
    if(snake->GetHead() == food->GetPos())
    {
        snake->Eat();
        ++score;
        OutScore();        
        if(snake->BodySize() == width * height)
        {
            EndGame();
            RedrawWindow(hMainWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
            MessageBox(hMainWnd, _T("You reached maximum snake length!"), _T("Message"), MB_OK);
            return;
        }
        else
            SpawnFood();

    }
    RedrawWindow(hMainWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
}
int App::Run()
{
    MSG msg = { 0 };
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

            timer->Tick();
            OnKeyboardInput();
            if(timeStep < timer->Elapsed())
            {
                if(running && !paused)
                    Update();
                timer->Reset();
            }
        }
    }
    catch(Error err)
    {
        const TCHAR* errMsg = nullptr;
        switch(err)
        {
            case Error::ClassRegErr: errMsg = _T("Window class registration failed."); break;
            case Error::CreateWndErr: errMsg = _T("Window creation failed."); break;
            case Error::AlreadyExistErr: errMsg = _T("App class object already exists."); break;
            default: errMsg = _T("WTF?"); break;
        }
        MessageBox(0, errMsg, _T("Error"), MB_OK | MB_ICONERROR);
    }
    catch(std::bad_alloc&)
    {
        MessageBox(0, _T("Not enough memory."), _T("Error"), MB_OK | MB_ICONERROR);
    }
    return (int)msg.wParam;
}
