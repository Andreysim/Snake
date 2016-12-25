#include <Windows.h>
#include <tchar.h>
#include <cinttypes>
#include <memory>

#define IDR_SCOREDATA 120

#if defined(DEBUG) || defined(_DEBUG)

#include <string>

#if defined(UNICODE) || defined(_UNICODE)
#define String(a) std::wstring(a)
#define ToStr(a) std::to_wstring(a)
#else
#define String(a) std::string(a)
#define ToStr(a) std::to_string(a)
#endif

#define OutStr(str) MessageBox(0, (str).c_str(), 0, MB_OK)

#else
#define String(a)
#define ToStr(a) 
#define OutStr(str) 
#endif

typedef std::unique_ptr<char[]> Data;
typedef std::unique_ptr<TCHAR[]> MemStr;

class HandleManager
{
public:
    HandleManager(HANDLE handle = nullptr) : hHandle(handle == INVALID_HANDLE_VALUE ? nullptr : handle) {}
    HandleManager(const HandleManager&) = delete;
    HandleManager& operator = (const HandleManager&) = delete;
    HandleManager(HandleManager&&) = default;
    HandleManager& operator = (HandleManager&&) = default;
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
        hHandle = hNewHandle;
    }
    bool operator !() const { return hHandle == nullptr; }
    explicit operator bool() const { return !*this; }
private:
    HANDLE hHandle;
};

constexpr LPCTSTR PipeName = _T("\\\\.\\pipe\\SnakeGamePipe");

uint32_t ReadData(Data& data, MemStr& snakeExeName);
BOOL UpdateSnakeScoresTable(LPCTSTR snakeExeName, Data& data, uint32_t dataSize);
int LaunchDeletionProcess(LPCTSTR snakeExeName);


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR cmdLine, int nShowCmd)
{
    OutStr(String(cmdLine));
    MemStr snakeExeName = std::make_unique<TCHAR[]>(MAX_PATH);
    size_t tmp;
    if(_stscanf_s(cmdLine, _T(" -n \"%[^\"]\" -h %zu"), snakeExeName.get(), MAX_PATH, &tmp) == 2)
    {
        HandleManager hSnakeProcess((HANDLE)tmp);
        Data data;
        uint32_t dataSize = ReadData(data, snakeExeName);
        if(dataSize)
        {
            WaitForSingleObject(hSnakeProcess.get(), 2000);
            UpdateSnakeScoresTable(snakeExeName.get(), data, dataSize);
        }
        return LaunchDeletionProcess(snakeExeName.get());
    }
    return 1;
}

uint32_t ReadData(Data& data, MemStr& snakeExeName)
{
    HandleManager pipe = CreateFile(PipeName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if(!pipe)
    {
        OutStr(String(_T("Client pipe not created")));
        return 0;
    }
    uint32_t dataSize = 0;
    if(!ReadFile(pipe.get(), &dataSize, sizeof(uint32_t), nullptr, nullptr) || dataSize == 0)
    {
        OutStr(String(_T("Data size not readed")));
        return 0;
    }
    DWORD readed = 0;
    data = std::make_unique<char[]>(dataSize);
    if(!ReadFile(pipe.get(), data.get(), dataSize, &readed, nullptr) || dataSize != readed)
    {
        data.reset();
        dataSize = 0;
    }
    OutStr(String(_T("Recieved: ")) + ToStr(dataSize));
    return dataSize;
}

BOOL UpdateSnakeScoresTable(LPCTSTR snakeExeName, Data& data, uint32_t dataSize)
{
    HANDLE hRes = BeginUpdateResource(snakeExeName, FALSE);
    BOOL ret = UpdateResource(hRes, RT_RCDATA, MAKEINTRESOURCE(IDR_SCOREDATA), MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT), data.get(), dataSize);
    ret = EndUpdateResource(hRes, !ret);
    OutStr(String(_T("Update: ")) + ToStr(ret));
    return ret;
}

int LaunchDeletionProcess(LPCTSTR snakeExeName)
{
    HandleManager hCurrProc = OpenProcess(PROCESS_ALL_ACCESS, TRUE, GetCurrentProcessId());
    if(!hCurrProc)
        return 0;

    // Launch game with cmd parameter '-d' for deletion of this executable
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi = {};
    TCHAR cmdLine[MAX_PATH + 32];
    _stprintf_s(cmdLine, _T("\"%s\" -d %zu"), snakeExeName, (size_t)hCurrProc.get());
    BOOL ret = CreateProcess(nullptr, cmdLine, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
    if(ret)
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return !ret;
}
