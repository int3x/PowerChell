#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <wchar.h>
#include "../PowerChellLib/powershell.h"

typedef struct _POWERCHELL_OPTIONS
{
    LPWSTR Script;
} POWERCHELL_OPTIONS, *PPOWERCHELL_OPTIONS;

void PowerChellMain();
BOOL ParseCommandLine(PPOWERCHELL_OPTIONS pOptions);
LPWSTR ReadScriptFromFile(LPCWSTR pwszFilePath);

#ifdef _WINDLL

extern "C" __declspec(dllexport) void APIENTRY Start();
void StartPowerShellInNewConsole();

HANDLE g_hEvent = NULL;

#pragma warning(disable : 4100)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        g_hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartPowerShellInNewConsole, NULL, 0, NULL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

void APIENTRY Start()
{
    WaitForSingleObject(g_hEvent, INFINITE);
}

void StartPowerShellInNewConsole()
{
    AllocConsole();
    PowerChellMain();
    SetEvent(g_hEvent);
    CloseHandle(g_hEvent);
    FreeConsole();
}

#else

int main()
{
    PowerChellMain();

    return 0;
}

#endif // #ifdef _WINDLL

void PowerChellMain()
{
    POWERCHELL_OPTIONS pOptions = { 0 };
    
    LPWSTR pwszFileScript = ReadScriptFromFile(L"C:\\Windows\\Tasks\\cmd.txt");
    if (pwszFileScript != NULL)
    {
        ExecutePowerShellScript(pwszFileScript);
        LocalFree(pwszFileScript);
    }
    else
    {
        ParseCommandLine(&pOptions);
        
        if (pOptions.Script != NULL)
        {
            ExecutePowerShellScript(pOptions.Script);
        }
        else
        {
            CreatePowerShellConsole();
        }
    }
}

BOOL ParseCommandLine(PPOWERCHELL_OPTIONS pOptions)
{
    int iArgc = 0;
    LPWSTR* ppwszArgv = NULL;

    ppwszArgv = CommandLineToArgvW(GetCommandLineW(), &iArgc);
    if (ppwszArgv == NULL)
    {
        return FALSE;
    }

    for (int i = 0; i < iArgc; i++)
    {
        if (_wcsicmp(ppwszArgv[i], L"-c") == 0)
        {
            i += 1;
            if (i < iArgc && ppwszArgv[i] != NULL)
            {
                pOptions->Script = ppwszArgv[i];
            }
        }
    }

    return TRUE;
}

LPWSTR ReadScriptFromFile(LPCWSTR pwszFilePath)
{
    HANDLE hFile = CreateFileW(pwszFilePath, GENERIC_READ, FILE_SHARE_READ, 
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    LPBYTE pBuffer = (LPBYTE)LocalAlloc(LPTR, dwFileSize + 2);

    DWORD dwBytesRead;
    ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL);
    CloseHandle(hFile);

    LPWSTR pwszResult = NULL;

    // Check for UTF-16 LE BOM
    if (dwBytesRead >= 2 && pBuffer[0] == 0xFF && pBuffer[1] == 0xFE)
    {
        // UTF-16 LE with BOM
        DWORD dwCharCount = (dwBytesRead - 2) / sizeof(WCHAR);
        pwszResult = (LPWSTR)LocalAlloc(LPTR, (dwCharCount + 1) * sizeof(WCHAR));
        memcpy(pwszResult, pBuffer + 2, dwCharCount * sizeof(WCHAR));
    }
    else
    {
        // Assume ANSI/ASCII - convert to wide char
        int iWideCharCount = MultiByteToWideChar(CP_ACP, 0, (LPCCH)pBuffer, dwBytesRead, NULL, 0);
        pwszResult = (LPWSTR)LocalAlloc(LPTR, (iWideCharCount + 1) * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, (LPCCH)pBuffer, dwBytesRead, pwszResult, iWideCharCount);
    }

    LocalFree(pBuffer);
    return pwszResult;
}
