/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OutputWindowProcess.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifdef _MSC_VER
/* Handle MSVC compiler warning messages, etc.  */
# pragma warning ( disable : 4115 )
#endif

/* Function to delete executable calling it.  */
static int SelfDelete();

#include <windows.h>

#undef _T
#ifdef UNICODE
# define _T(x) L x
#else
# define _T(x) x
#endif

static HWND MainWindow = 0;
static HWND EditWindow = 0;
static LPCTSTR MainWindowClass = _T("vtkOutputWindowProcess");
static LPCTSTR EditWindowClass = _T("EDIT");
static LONG MainWindowStyle = (WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW);
static LONG EditWindowStyle = (ES_MULTILINE | ES_READONLY | WS_CHILD |
                               ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VISIBLE |
                               WS_VSCROLL | WS_HSCROLL | WS_MAXIMIZE);

static LRESULT APIENTRY MainWindowProc(HWND hWnd, UINT m, WPARAM w, LPARAM l)
{
  switch (m)
    {
    case WM_SIZE:
      MoveWindow(EditWindow, 0, 0, LOWORD(l), HIWORD(l), TRUE);
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    }
  return DefWindowProc(hWnd, m, w, l);
}

static void RegisterWindowClass()
{
  WNDCLASS wndClass;
  if(!GetClassInfo(GetModuleHandle(0), MainWindowClass, &wndClass))
    {
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = MainWindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.hInstance = GetModuleHandle(0);
    wndClass.hIcon = LoadIcon(0, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(0, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = MainWindowClass;
    wndClass.cbWndExtra = 0;
    RegisterClass(&wndClass);
    }
}

static DWORD WINAPI ReadThreadProc(LPVOID p)
{
  char buffer[1024];
  DWORD nRead = 0;
  while(ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, 1024, &nRead, 0))
    {
    buffer[nRead] = 0;
    SendMessage(EditWindow, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    SendMessage(EditWindow, EM_REPLACESEL, (WPARAM)0, (LPARAM)buffer);
    }
  return (DWORD)p;
}

void MainEventLoop()
{
  BOOL b;
  MSG msg;
  while((b = GetMessage(&msg, 0, 0, 0)) != 0 && b != -1)
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   PSTR szCmdLine, int iCmdShow)
{
  (void)hInst; (void)hPrev; (void)szCmdLine; (void)iCmdShow;

  /* Setup a child process to delete this executable when it exits.  */
  SelfDelete();

  /* Create a simple GUI.  */
  RegisterWindowClass();
  MainWindow = CreateWindow(MainWindowClass, MainWindowClass, MainWindowStyle,
                            0,0,512,512, 0, 0, GetModuleHandle(0), 0);
  EditWindow = CreateWindow(EditWindowClass, "", EditWindowStyle,
                            0,0,512,512, MainWindow, 0, GetModuleHandle(0), 0);
  ShowWindow(MainWindow, SW_SHOW);
  UpdateWindow(MainWindow);

  /* Create a thread to read from standard input and write to the window.  */
  {DWORD threadId; CreateThread(0, 1024, ReadThreadProc, 0, 0, &threadId);}

  /* Run the event loop until the window is closed.  */
  MainEventLoop();

  return 0;
}

/*--------------------------------------------------------------------------*/
/* Code to delete executable calling it.  Based on code from James
   Brown at http://www.catch22.org.uk/tuts/selfdel.asp.  */

#pragma pack(push, 1)

#define SELF_DELETE_CODESIZE 0x200

/* Data to inject into remote process.  */
typedef struct SelfDeleteRemoteCode_s SelfDeleteRemoteCode;
typedef struct SelfDeleteRemoteCode_s
{
  SelfDeleteRemoteCode *Arg0;
  BYTE  opCodes[SELF_DELETE_CODESIZE];
  HANDLE  hParent;

  DWORD (__stdcall *fnWaitForSingleObject)(HANDLE hHandle, DWORD dwMilliseconds);
  BOOL (__stdcall *fnCloseHandle)(HANDLE hObject);
  BOOL (__stdcall *fnDeleteFile)(LPCTSTR lpFileName);
  void (__stdcall *fnSleep)(DWORD dwMilliseconds);
  void (__stdcall *fnExitProcess)(UINT uExitCode);
  DWORD (__stdcall *fnGetLastError)(void);

  TCHAR szFileName[MAX_PATH];
} SelfDeleteRemoteCode;

#pragma pack(pop)

#ifdef _DEBUG
# error "vtkWin32OutputWindowProcess must be compiled Release"
#endif

/* Function to execute in remote process.  It may only call windows
   kernel functions through pointers in the SelfDeleteRemoteCode
   structure.  */
static void SelfDeleteRemoteThread(SelfDeleteRemoteCode* remote)
{
  /* Block until parent process terminates.  */
  remote->fnWaitForSingleObject(remote->hParent, INFINITE);
  remote->fnCloseHandle(remote->hParent);

  /* Delete the executable file.  */
  while(!remote->fnDeleteFile(remote->szFileName))
    {
    remote->fnSleep(1000);
    }

  /* Exit so that we do not execute garbage code.  */
  remote->fnExitProcess(0);
}

/* Function to setup remote process that waits for this process to
   exit and then deletes its executable.  */
static int SelfDelete()
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);

  /* Create a process using the explorer executable but suspend it
     immediately.  */
  if(CreateProcess(0, _T("explorer.exe"), 0, 0, 0,
                   (CREATE_SUSPENDED | IDLE_PRIORITY_CLASS),
                   0, 0, &si, &pi))
    {
    /* Structure to store code and data to copy to remote process.  */
    SelfDeleteRemoteCode code;

    DWORD oldProtect;
    CONTEXT context;
    DWORD entryPoint;

    /* Setup pointers to kernel functions for the remote code to call.  */
    code.fnWaitForSingleObject  = WaitForSingleObject;
    code.fnCloseHandle          = CloseHandle;
    code.fnDeleteFile           = DeleteFile;
    code.fnSleep                = Sleep;
    code.fnExitProcess          = ExitProcess;
    code.fnGetLastError         = GetLastError;

    /* Give the remote process a copy of our own process handle.  */
    DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),
                    pi.hProcess, &code.hParent, 0, FALSE, 0);

    /* Store the file name of this process's executable.  */
    GetModuleFileName(0, code.szFileName, MAX_PATH);

    /* Store the binary code to execute remotely.  */
    memcpy(code.opCodes, SelfDeleteRemoteThread, SELF_DELETE_CODESIZE);

    /* Allocate some space on process's stack and place our
       SelfDeleteRemoteCode structure there.  Then set the instruction
       pointer to this location and let the process resume.  */
    context.ContextFlags = (CONTEXT_INTEGER | CONTEXT_CONTROL);
    GetThreadContext(pi.hThread, &context);

    /* Allocate space on stack that is aligned to cache-line boundary.  */
    entryPoint = (context.Esp - sizeof(SelfDeleteRemoteCode)) & ~0x1F;

    /* Place a pointer to the structure at the bottom-of-stack.  This
       pointer is located in such a way that it becomes the
       SelfDeleteRemoteThread's first argument.  */
    code.Arg0 = (SelfDeleteRemoteCode*)entryPoint;

    /* Set dummy return address for remote thread.  It will never return.  */
    context.Esp = entryPoint - 4;

    /* Set remote thread to execute the opCodes we copy to the process.  */
    context.Eip = entryPoint + (((char*)&code.opCodes) - ((char*)&code));

    /* Copy the code and data to the remote process entry point.  */
    VirtualProtectEx(pi.hProcess, (PVOID)entryPoint, sizeof(code),
                     PAGE_EXECUTE_READWRITE, &oldProtect);
    WriteProcessMemory(pi.hProcess, (PVOID)entryPoint, &code, sizeof(code), 0);

    /* Make sure the new code will be loaded.  */
    FlushInstructionCache(pi.hProcess, (PVOID)entryPoint, sizeof(code));

    /* Set the remote thread to execute at our entry point.  */
    SetThreadContext(pi.hThread, &context);

    /* Let the remote process continue.  It will block until this
       process exits.  */
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 1;
    }

  return 0;
}
