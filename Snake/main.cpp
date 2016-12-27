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
#define _SCL_SECURE_NO_WARNINGS

#include <Windows.h>
#include <CommCtrl.h>
#include <tchar.h>
#include <vector>
#include <limits>
#include <random>
#include <ctime>
#include <memory>
#include <algorithm>
#include <numeric>
#include "resource.h"

enum class Error 
{ 
    ClassRegErr,
    CreateWndErr,
    AlreadyExistErr,
};

class HandleManager
{
public:
    HandleManager(HANDLE handle = nullptr) : hHandle(handle == INVALID_HANDLE_VALUE ? nullptr : handle) {}
    HandleManager(const HandleManager&) = delete;
    HandleManager(HandleManager&& right) noexcept : hHandle(right.release()) {}
    HandleManager& operator = (HANDLE handle) 
    {
        reset(handle); 
        return *this;
    }
    HandleManager& operator = (const HandleManager&) = delete;
    HandleManager& operator = (HandleManager&& right) noexcept
    {
        reset(right.release());
        return *this;
    }
    ~HandleManager() { reset(); }

    HANDLE get() const { return hHandle; }
    HANDLE release()
    {
        HANDLE ret = hHandle;
        hHandle = nullptr;
        return ret;
    }
    void reset(HANDLE hNewHandle = nullptr)
    {
        if(hHandle)
            CloseHandle(hHandle);
        hHandle = (hNewHandle == INVALID_HANDLE_VALUE) ? nullptr : hNewHandle;
    }
    bool operator !() const { return hHandle == nullptr; }
    explicit operator bool() const { return hHandle != nullptr; }
private:
    HANDLE hHandle;
};

struct AppGuard
{
    AppGuard(LPCTSTR mutexName) : mutex(CreateMutex(nullptr, FALSE, mutexName)) 
    {
        if(GetLastError() == ERROR_ALREADY_EXISTS)
            mutex.reset();
    }
    HandleManager mutex;
};

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
    enum Direction { UP, DOWN, RIGHT, LEFT };

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
    void Draw(HDC hdc) const;
    const std::vector<POINT>& Body() const;

private:
    void DrawBlock(HDC hdc, POINT p, int imgInd) const;
    Direction GetDirection(POINT p1, POINT p2) const;

private:
    std::vector<POINT> body;
    HIMAGELIST hImgList = nullptr;
    uint32_t headInd = (uint32_t)-1;
    Direction dir = Direction::UP;
    bool foodEaten = false;
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
    HBITMAP hBitMap = nullptr;
    POINT pos;
};

struct ToolBar
{
    enum ImgInd { OptImg, NewGameImg, UnpauseImg, PauseImg, CountImg };
    ToolBar() = default;
    ToolBar(DWORD style, int x, int y, int w, int h, HWND parent, UINT id, HINSTANCE hInst, UINT imgId);
    void Destroy();

    HWND hToolBar = nullptr;
    HWND hStaticScore = nullptr;
    HIMAGELIST hImgList = nullptr;
};

class App
{
private:
    static const uint32_t MaxRecordsCount = 10;
    struct ScoresData
    {
        struct Record
        {
            static const uint32_t MaxRecordStrLen = 40;
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t score = 0;
            uint32_t nameLength = 0;
            uint32_t recordLength = 0;
            std::unique_ptr<TCHAR[]> name;
            std::unique_ptr<TCHAR[]> recordStr;

            void  BuildRecordStr(bool bEmpty);
            void* ReadRecord(void* pMem);
            void* WriteRecord(void* pMem);
        };
        typedef std::unique_ptr<Record> RecordPtr;
        uint32_t nRecords;
        RecordPtr records[MaxRecordsCount];
    };

public:
    static HINSTANCE AppInstance();
    static App* GetApp();

    App(HINSTANCE hInstance, LPTSTR lpCmdLine, int showCmd);
    App(const App&) = delete;
    App(App&&) = default;
    App& operator = (const App&) = delete;
    App& operator = (App&&) = default;
    ~App();

    int Run();

private:
    static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK OptionsDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ScoresDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK PlayerNameInputDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    BOOL SaveScoresData();
    BOOL CreateScoresSaverExe();
    BOOL LaunchScoresSaverExe();
    BOOL SendDataToScoresSaver(HandleManager& pipe);
    BOOL DeleteScoresSaverExe(LPCTSTR cmdLine);

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateMainWindow(int showCmd);
    void EndGame();
    bool LoadScoresData();
    void NewGame();
    void Options();
    void OutScore() const;
    void Pause();
    void Records(bool bPushRecord);
    ATOM RegisterWindowClass();
    void ResizeGameArea(uint32_t w, uint32_t h);
    void SpawnFood();
    void Update();

    int  KeyPressed(int vKey) const;
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT code);
    void OnKeyboardInput();
    void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
    void OnNotify(HWND hwnd, int id, LPNMHDR phdr);
    void OnPaint();
    
private:
    static App* pApp;
    static HINSTANCE hInst;

    HWND hMainWnd = nullptr;
    ToolBar toolBar;

    uint32_t vertIndent = 0;
    uint32_t width = 10;
    uint32_t height = 10;
    uint32_t score = 0;

    std::unique_ptr<Snake> snake;
    std::unique_ptr<Food> food;
    std::unique_ptr<Timer> timer;
    std::unique_ptr<ScoresData> scoresData;

    bool running = false;
    bool paused = false;
    bool scoresChanged = false;

    double speed = 0.3;
    double timeStep = 0.3;
};

App* App::pApp = nullptr;
HINSTANCE App::hInst = nullptr;

constexpr LPCTSTR PipeName = _T("\\\\.\\pipe\\SnakeGamePipe");
constexpr LPCTSTR SnakeGameMutexName = _T("SnakeGameGuardMutex");
constexpr LPCTSTR ScoresSaverExeName = _T("_SnakeGameEmbeddedExecutable.exe");

constexpr DWORD MainWindowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
constexpr int nButtons = 3;
constexpr int BtnSize = 24;
constexpr uint32_t BlockSize = 24;
constexpr uint32_t MinWidth = 8;
constexpr uint32_t MinHeight = 8;
constexpr uint32_t MaxWidth = 32;
constexpr uint32_t MaxHeight = 32;
constexpr COLORREF BkColor = RGB(192, 192, 192);

AppGuard appGuard(SnakeGameMutexName);

//########################################################################################################################

inline bool operator == (const POINT& p1, const POINT& p2) { return p1.x == p2.x && p1.y == p2.y; }
inline bool operator != (const POINT& p1, const POINT& p2) { return !(p1 == p2); }

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpCmdLine, int nShowCmd)
{
    if(!appGuard.mutex)
        return 0;
    App app(hInstance, lpCmdLine, nShowCmd);
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
Snake::Snake()
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
inline void Snake::DrawBlock(HDC hdc, POINT p, int imgInd) const
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
void Snake::Draw(HDC hdc) const
{
    uint32_t tailInd = (headInd + 1) % body.size();

    for(uint32_t i = 0; i < body.size(); ++i)
    {
        if(i != headInd && i != tailInd)
            DrawBlock(hdc, body[i], 8);
    }
    Direction tailDir = GetDirection(body[(tailInd + 1) % body.size()], body[tailInd]);
    DrawBlock(hdc, body[headInd], (int)dir);
    DrawBlock(hdc, body[tailInd], (int)tailDir + 4);
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
void Food::Draw(HDC hdc) const
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
        case WM_ERASEBKGND: return 1;
        case WM_KEYDOWN: OnKeyDown(hwnd, (UINT)wParam, TRUE, (int)LOWORD(lParam), (UINT)HIWORD(lParam)); break;
        case WM_KEYUP:
            if(wParam == VK_SPACE)
                timeStep = speed;
            break;
        case WM_NOTIFY: OnNotify(hwnd, (int)wParam, (LPNMHDR)lParam); break;
        case WM_COMMAND: OnCommand(hwnd, LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam)); break;
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
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            SetDlgItemInt(hwnd, IDC_WIDTH_EDIT, GetApp()->width, FALSE);
            SetDlgItemInt(hwnd, IDC_HEIGHT_EDIT, GetApp()->height, FALSE);
            break;
        }
        case WM_COMMAND:
        {
            if(wParam == IDOK)
            {
                uint32_t newWidth = 0, newHeight = 0;
                newWidth = GetDlgItemInt(hwnd, IDC_WIDTH_EDIT, nullptr, FALSE);
                if(newWidth < MinWidth || newWidth > MaxWidth)
                {
                    TCHAR buf[64] = { 0 };
                    _stprintf_s(buf, _T("Width must be in range [%d, %d]"), MinWidth, MaxWidth);
                    MessageBox(hwnd, buf, _T("Error"), MB_OK);
                    break;
                }

                newHeight = GetDlgItemInt(hwnd, IDC_HEIGHT_EDIT, nullptr, FALSE);
                if(newHeight < MinHeight || newHeight > MaxHeight)
                {
                    TCHAR buf[64] = { 0 };
                    _stprintf_s(buf, _T("Height must be in range [%d, %d]"), MinHeight, MaxHeight);
                    MessageBox(hwnd, buf, _T("Error"), MB_OK);
                    break;
                }
                if(newWidth != GetApp()->width || newHeight != GetApp()->height)
                {
                    GetApp()->width = newWidth;
                    GetApp()->height = newHeight;
                    EndDialog(hwnd, 1);
                }
                else
                    EndDialog(hwnd, 0);
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
INT_PTR CALLBACK App::ScoresDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    constexpr uint32_t BufferSize = 64;
    static uint32_t newScore;

    switch(msg)
    {
        case WM_INITDIALOG:
        {
            newScore = (uint32_t)lParam;
            if(!newScore)
                break;

            uint32_t& nRecords = GetApp()->scoresData->nRecords;
            auto& records = GetApp()->scoresData->records;

            // create new record
            auto newRecord = std::make_unique<ScoresData::Record>();
            newRecord->score = newScore;
            newRecord->width = GetApp()->width;
            newRecord->height = GetApp()->height;
            newRecord->nameLength = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_NAME_INPUT_DIALOG), hwnd, PlayerNameInputDialogProc, (LPARAM)&newRecord->name);
            newRecord->BuildRecordStr(false);

            // insert new record
            auto it1 = (nRecords == 0) ? std::begin(records) :
                std::find_if(std::begin(records), std::begin(records) + nRecords,
                    [score = newRecord->score](const ScoresData::RecordPtr& rec){ return rec->score <= score; });
            if(it1 != std::begin(records) + nRecords)
                std::move_backward(it1, std::begin(records) + (MaxRecordsCount - 1), std::begin(records) + MaxRecordsCount);
            *it1 = std::move(newRecord);

            if(nRecords < MaxRecordsCount)
                ++nRecords;                  
            break;
        }
        case WM_PAINT:
        {         
            constexpr TCHAR header[] = _T(" N  Name        Field(WxH)  Score\n");
            constexpr uint32_t headerSize = _countof(header);

            auto& records = GetApp()->scoresData->records;
            uint32_t recordsTextLen = std::accumulate(std::cbegin(records), std::cend(records), headerSize,
                [](uint32_t sum, const ScoresData::RecordPtr& rec) {return sum + rec->recordLength; });
            auto recordsText = std::make_unique<TCHAR[]>(recordsTextLen);

            memcpy(recordsText.get(), header, headerSize * sizeof(TCHAR));
            std::for_each(std::cbegin(records), std::cend(records),
                [it = recordsText.get() + headerSize, i = uint32_t(0), end = recordsText.get() + recordsTextLen](const ScoresData::RecordPtr& rec) mutable
                {
                    TCHAR buf[16];
                    _stprintf_s(buf, _T("%2u"), ++i);
                    memcpy(it, rec->recordStr.get(), rec->recordLength * sizeof(TCHAR));
                    memcpy(it + 1, buf, 2 * sizeof(TCHAR));
                    it += rec->recordLength;
                });

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SetBkColor(hdc, GetSysColor(COLOR_MENU));
            HFONT hFont = CreateFont(-12, 0, 0, 0, 0, 0, 0, 0, RUSSIAN_CHARSET, 0, 0, 0, FIXED_PITCH | FF_MODERN, _T("Consolas"));
            hFont = (HFONT)SelectObject(hdc, hFont);
            RECT rc = { 7, 7, 350, 350 };
            DrawText(hdc, recordsText.get(), recordsTextLen, &rc, DT_NOPREFIX);

            DeleteObject(SelectObject(hdc, hFont));
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_COMMAND:
        {
            if(wParam == IDOK)
            {
                if(newScore)
                {
                    GetApp()->scoresChanged = true;
                    EndDialog(hwnd, 1);
                }
                else
                    EndDialog(hwnd, 0);
            }
            break;
        }
        default:
            return FALSE;
    }
    return TRUE;
}
INT_PTR CALLBACK App::PlayerNameInputDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static std::unique_ptr<TCHAR[]> *namePtr = nullptr;
    switch(msg)
    {
        case WM_INITDIALOG:
            namePtr = (std::unique_ptr<TCHAR[]>*)lParam;          
            PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_NAME_EDIT), MAKELPARAM(TRUE, 0));
            break;
        case WM_COMMAND:
            if(wParam == IDOK)
            {
                TCHAR buf[16];
                uint32_t nCh = GetDlgItemText(hwnd, IDC_NAME_EDIT, buf, 16);
                if(nCh == 0)
                    goto m;
                TCHAR *itBeg, *itEnd;
                for(itBeg = buf; *itBeg && !_istgraph(*itBeg); ++itBeg);
                if(!*itBeg)
                    goto m;
                for(itEnd = buf + nCh - 1; itBeg < itEnd && !_istgraph(*itEnd); --itEnd);
                nCh = ++itEnd - itBeg;
                *namePtr = std::make_unique<TCHAR[]>(nCh + 1);
                memcpy(namePtr->get(), itBeg, nCh * sizeof(TCHAR));
                EndDialog(hwnd, nCh);
                break;
m:
                MessageBox(hwnd, _T("Incorrect name!"), _T("Error"), MB_OK);
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

App::App(HINSTANCE hInstance, LPTSTR lpCmdLine, int showCmd)
{
    if(_tcsstr(lpCmdLine, _T("-d")))
    {
        DeleteScoresSaverExe(lpCmdLine);
        return;
    }

    if(pApp)
        throw Error::AlreadyExistErr;

    pApp = this;
    hInst = hInstance;

    snake = std::make_unique<Snake>();
    food = std::make_unique<Food>();
    timer = std::make_unique<Timer>();

    LoadScoresData();

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

BOOL App::CreateScoresSaverExe()
{
    HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(IDR_SCORES_SAVER_EXE), RT_RCDATA);
    HGLOBAL hGlRes = LoadResource(hInst, hRes);
    LPVOID pRes = LockResource(hGlRes);
    DWORD resSize = SizeofResource(hInst, hRes);

    HandleManager hExecFile = CreateFile(ScoresSaverExeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr);
    if(!hExecFile)
        return FALSE;
    if(!WriteFile(hExecFile.get(), pRes, resSize, nullptr, nullptr))
    {
        DeleteFile(ScoresSaverExeName);
        return FALSE;
    }
    return TRUE;
}
BOOL App::DeleteScoresSaverExe(LPCTSTR cmdLine)
{
    size_t tmp = 0;
    if(_stscanf_s(cmdLine, _T(" -d %zu"), &tmp) != 1)
        return FALSE;
    HandleManager hSaverProcess((HANDLE)tmp);
    WaitForSingleObject(hSaverProcess.get(), 1000);
    return DeleteFile(ScoresSaverExeName);
}
BOOL App::LaunchScoresSaverExe()
{
    HandleManager hCurrProc = OpenProcess(PROCESS_ALL_ACCESS, TRUE, GetCurrentProcessId());
    if(!hCurrProc)
        return FALSE;
    TCHAR currExeName[MAX_PATH];
    GetModuleFileName(hInst, currExeName, MAX_PATH);

    auto cmdLine = std::make_unique<TCHAR[]>(2 * MAX_PATH + 32);
    _stprintf_s(cmdLine.get(), 2 * MAX_PATH + 32, _T("\"%s\" -n \"%s\" -h %zu"), ScoresSaverExeName, currExeName, (size_t)hCurrProc.get());

    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi = {};
    BOOL ret = CreateProcess(nullptr, cmdLine.get(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
    if(ret)
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return ret;
}
BOOL App::SaveScoresData()
{ 
    HandleManager pipe = CreateNamedPipe(PipeName, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, 2, 4096, 4096, 0, nullptr);
    if( !(pipe && CreateScoresSaverExe() && LaunchScoresSaverExe() && SendDataToScoresSaver(pipe)) )
        return FALSE;
    return TRUE;
}
BOOL App::SendDataToScoresSaver(HandleManager& pipe)
{
    uint32_t dataSize = std::accumulate(std::cbegin(scoresData->records), std::cbegin(scoresData->records) + scoresData->nRecords, uint32_t(sizeof(uint32_t)),
        [](uint32_t sum, const ScoresData::RecordPtr& rec) { return sum + 4 * sizeof(uint32_t) + rec->nameLength * sizeof(TCHAR); });

    auto data = std::make_unique<char[]>(dataSize + sizeof(uint32_t));
    uint32_t *it = (uint32_t*)data.get();
    *it++ = dataSize;
    *it++ = scoresData->nRecords;
    std::for_each(std::cbegin(scoresData->records), std::cbegin(scoresData->records) + scoresData->nRecords,
        [&it](const ScoresData::RecordPtr& rec)
        {
            it = (uint32_t*)rec->WriteRecord(it);
        });

    ConnectNamedPipe(pipe.get(), nullptr);
    return WriteFile(pipe.get(), data.get(), dataSize + sizeof(uint32_t), nullptr, nullptr);
}

inline HINSTANCE App::AppInstance() { return hInst; }
inline void App::EndGame()
{
    running = false; 
    SendMessage(toolBar.hToolBar, TB_ENABLEBUTTON, (WPARAM)ID_PAUSE_BTN, MAKELPARAM(FALSE, 0));
    Records(true);
}
inline App* App::GetApp() { return App::pApp; }
inline int  App::KeyPressed(int vKey) const { return 0x8000 & GetAsyncKeyState(vKey); }
inline void App::Options()
{
    if(DialogBox(hInst, MAKEINTRESOURCE(IDD_OPTIONS_DIALOG), hMainWnd, OptionsDialogProc))
    {
        ResizeGameArea(width, height);
        NewGame();
    }
}
inline void App::OutScore() const
{
    TCHAR buf[16] = { 0 };
    _stprintf_s(buf, _T("SCORE: %d"), score);
    SetWindowText(toolBar.hStaticScore, buf);
}
inline void App::Pause()
{
    if(running)
    {
        SendMessage(toolBar.hToolBar, TB_CHANGEBITMAP, (WPARAM)ID_PAUSE_BTN, (LPARAM)(paused ? toolBar.PauseImg : toolBar.UnpauseImg));
        paused = !paused;
    }
}
inline void App::Records(bool bPushRecord)
{
    if(bPushRecord && (score == 0 || scoresData->nRecords == MaxRecordsCount && score < scoresData->records[MaxRecordsCount - 1]->score))
        return;
    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SCORE_TABLE_DIALOG), hMainWnd, ScoresDialogProc, (LPARAM)(bPushRecord ? score : 0));
}

void App::CreateMainWindow(int showCmd)
{
    hMainWnd = CreateWindowEx(0, _T("SnakeMainWndClass"), _T("Snake game"), MainWindowStyle, 
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,    0, 0, hInst, nullptr);

    if(!hMainWnd)
        throw Error::CreateWndErr;

    ResizeGameArea(width, height);
    ShowWindow(hMainWnd, showCmd);
    UpdateWindow(hMainWnd);
}
bool App::LoadScoresData()
{
    HRSRC hScoreRes = FindResource(hInst, MAKEINTRESOURCE(IDR_SCOREDATA), RT_RCDATA);
    HGLOBAL hLoadScoreRes = LoadResource(hInst, hScoreRes);
    uint32_t *scoresResAddr = (uint32_t*)LockResource(hLoadScoreRes);

    if(!scoresResAddr)
        return false;

    scoresData = std::make_unique<ScoresData>();
    scoresData->nRecords = *scoresResAddr++;

    for(uint32_t i = 0; i < MaxRecordsCount; ++i)
    {
        auto& record = scoresData->records[i];
        record = std::make_unique<ScoresData::Record>();
        if(i < scoresData->nRecords)
            scoresResAddr = (uint32_t*)record->ReadRecord(scoresResAddr);
        else
            record->ReadRecord(nullptr);
    }
    return true;
}
void App::NewGame()
{
    running = true;
    paused = true;
    score = 0;
    timeStep = speed;
    snake->Reset(width, height);
    timer->Reset();
    SpawnFood();
    SendMessage(toolBar.hToolBar, TB_CHANGEBITMAP, ID_PAUSE_BTN, (LPARAM)toolBar.UnpauseImg);
    SendMessage(toolBar.hToolBar, TB_ENABLEBUTTON, (WPARAM)ID_PAUSE_BTN, MAKELPARAM(TRUE, 0));
    OutScore();
    RedrawWindow(hMainWnd, nullptr, nullptr, RDW_INVALIDATE);
}
ATOM App::RegisterWindowClass()
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = App::MainProc;
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(hInst, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_GAME_MENU);
    wcex.lpszClassName = _T("SnakeMainWndClass");

    return RegisterClassEx(&wcex);
}
void App::ResizeGameArea(uint32_t w, uint32_t h)
{
    width = std::max(MinWidth, std::min(w, MaxWidth));
    height = std::max(MinHeight, std::min(h, MaxHeight));
    RECT wndRect, adjRect = { 0, 0, (LONG)(width * BlockSize), (LONG)(height * BlockSize + vertIndent + GetSystemMetrics(SM_CYMENU)) };
    AdjustWindowRectEx(&adjRect, MainWindowStyle, FALSE, 0);
    GetWindowRect(hMainWnd, &wndRect);
    wndRect.right = adjRect.right - adjRect.left;
    wndRect.bottom = adjRect.bottom - adjRect.top;
    MoveWindow(hMainWnd, wndRect.left, wndRect.top, wndRect.right, wndRect.bottom, TRUE);
    SendMessage(toolBar.hToolBar, TB_AUTOSIZE, 0, 0);
}
int  App::Run()
{
    if(!pApp)
        return 0;
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
        if(scoresChanged)
            SaveScoresData();
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
    RECT rc = { 0, (LONG)vertIndent, (LONG)(width * BlockSize), (LONG)(height * BlockSize + vertIndent) };
    if(snake->GetHead() == food->GetPos())
    {
        snake->Eat();
        ++score;
        OutScore();        
        if(snake->BodySize() == width * height)
        {
            EndGame();
            InvalidateRect(hMainWnd, &rc, FALSE);
            MessageBox(hMainWnd, _T("You reached maximum snake length!"), _T("Message"), MB_OK);
            return;
        }
        else
            SpawnFood();

    }
    InvalidateRect(hMainWnd, &rc, FALSE);
}

void App::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT code)
{
    if(code == 0)
    {
        switch(id)
        {
            case ID_GAME_OPTIONS: Options(); break;
            case ID_GAME_CHAMPIONS: Records(false); break;
            case ID_GAME_EXIT: PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
    }
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

    if(vk == VK_SPACE)
        timeStep = std::max(0.1, speed / 3.0);
    else if(vk == 'P')
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

    snake->Draw(hMemDC);
    if(running)
        food->Draw(hMemDC);

    // Copy bitmap to device
    BitBlt(hdc, 0, vertIndent, pw, ph, hMemDC, 0, 0, SRCCOPY);

    DeleteObject(SelectObject(hMemDC, hBitMap));
    DeleteDC(hMemDC);

    EndPaint(hMainWnd, &ps);
}

void  App::ScoresData::Record::BuildRecordStr(bool bEmpty)
{
    if(!recordStr)
        recordStr = std::make_unique<TCHAR[]>(MaxRecordStrLen + 1);
    recordLength = (bEmpty) ?
        _stprintf_s(recordStr.get(), MaxRecordStrLen, _T("\n    Empty             0x0       0")) :
        _stprintf_s(recordStr.get(), MaxRecordStrLen, _T("\n    %-10.10s  %7dx%-2d  %5d"), name.get(), width, height, score);    
}
void* App::ScoresData::Record::ReadRecord(void* pMem)
{
    if(pMem)
    {
        uint32_t *it = (uint32_t*)pMem;
        width = *it++;
        height = *it++;
        score = *it++;
        nameLength = *it++;
        name = std::make_unique<TCHAR[]>(nameLength + 1);
        memcpy(name.get(), it, nameLength * sizeof(TCHAR));

        BuildRecordStr(false);
        return (TCHAR*)it + nameLength;
    }
    else
    {
        BuildRecordStr(true);
        return nullptr;
    }
}
void* App::ScoresData::Record::WriteRecord(void* pMem)
{
    uint32_t *it = (uint32_t*)pMem;
    *it++ = width;
    *it++ = height;
    *it++ = score;
    *it++ = nameLength;
    memcpy(it, name.get(), nameLength * sizeof(TCHAR));
    return (TCHAR*)it + nameLength;
}
