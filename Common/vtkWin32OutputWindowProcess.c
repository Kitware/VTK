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
#include <windows.h>
#include <tchar.h>

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
