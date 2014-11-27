/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OutputWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWin32OutputWindow.h"

#include "vtkObjectFactory.h"
#include "vtkWindows.h"

vtkStandardNewMacro(vtkWin32OutputWindow);

HWND vtkWin32OutputWindowOutputWindow = 0;

//----------------------------------------------------------------------------
LRESULT APIENTRY vtkWin32OutputWindowWndProc(HWND hWnd, UINT message,
                                             WPARAM wParam,
                                             LPARAM lParam)
{
  switch (message)
    {
    case WM_SIZE:
      {
      int w = LOWORD(lParam);  // width of client area
      int h = HIWORD(lParam); // height of client area

      MoveWindow(vtkWin32OutputWindowOutputWindow,
                 0, 0, w, h, true);
      }
      break;
    case WM_DESTROY:
      vtkWin32OutputWindowOutputWindow = NULL;
      vtkObject::GlobalWarningDisplayOff();
      break;
    case WM_CREATE:
      break;
    }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

//----------------------------------------------------------------------------
vtkWin32OutputWindow::vtkWin32OutputWindow()
{
  // Default to sending output to stderr/cerr when running a dashboard:
  //
  if(getenv("DART_TEST_FROM_DART") ||
    getenv("DASHBOARD_TEST_FROM_CTEST"))
    {
    this->SendToStdErr = true;
    }
  else
    {
    this->SendToStdErr = false;
    }
}

//----------------------------------------------------------------------------
vtkWin32OutputWindow::~vtkWin32OutputWindow()
{
}

//----------------------------------------------------------------------------
// Display text in the window, and translate the \n to \r\n.
//
void vtkWin32OutputWindow::DisplayText(const char* someText)
{
  if(!someText)
    {
    return;
    }
  if(this->PromptUser)
    {
    this->PromptText(someText);
    return;
    }

  // Create a buffer big enough to hold the entire text
  char* buffer = new char[strlen(someText)+1];
  // Start at the beginning
  const char* NewLinePos = someText;
  while(NewLinePos)
    {
    int len = 0;
    // Find the next new line in text
    NewLinePos = strchr(someText, '\n');
    // if no new line is found then just add the text
    if(NewLinePos == 0)
      {
      vtkWin32OutputWindow::AddText(someText);
      OutputDebugString(someText);

      if (this->SendToStdErr)
        {
        cerr << someText;
        }
      }
    // if a new line is found copy it to the buffer
    // and add the buffer with a control new line
    else
      {
      len = NewLinePos - someText;
      strncpy(buffer, someText, len);
      buffer[len] = 0;
      someText = NewLinePos+1;
      vtkWin32OutputWindow::AddText(buffer);
      vtkWin32OutputWindow::AddText("\r\n");
      OutputDebugString(buffer);
      OutputDebugString("\r\n");

      if (this->SendToStdErr)
        {
        cerr << buffer;
        cerr << "\r\n";
        }
      }
    }
  delete [] buffer;
}

//----------------------------------------------------------------------------
// Add some text to the EDIT control.
//
void vtkWin32OutputWindow::AddText(const char* someText)
{
  if(!Initialize()  || (strlen(someText) == 0))
    {
    return;
    }

#ifdef UNICODE
  // move to the end of the text area
  SendMessageW( vtkWin32OutputWindowOutputWindow, EM_SETSEL,
               (WPARAM)-1, (LPARAM)-1 );
  wchar_t *wmsg = new wchar_t [mbstowcs(NULL, someText, 32000)+1];
  mbstowcs(wmsg, someText, 32000);
  // Append the text to the control
  SendMessageW( vtkWin32OutputWindowOutputWindow, EM_REPLACESEL,
                0, (LPARAM)wmsg );
  delete [] wmsg;
#else
  // move to the end of the text area
  SendMessageA( vtkWin32OutputWindowOutputWindow, EM_SETSEL,
               (WPARAM)-1, (LPARAM)-1 );
  // Append the text to the control
  SendMessageA( vtkWin32OutputWindowOutputWindow, EM_REPLACESEL,
                0, (LPARAM)someText );
#endif
}

//----------------------------------------------------------------------------
// initialize the output window with an EDIT control and
// a container window.
//
int vtkWin32OutputWindow::Initialize()
{
  // check to see if it is already initialized
  if(vtkWin32OutputWindowOutputWindow)
    {
    return 1;
    }

  // Initialize the output window

  WNDCLASS wndClass;
  // has the class been registered ?
#ifdef UNICODE
  if (!GetClassInfo(GetModuleHandle(NULL),L"vtkOutputWindow",&wndClass))
#else
  if (!GetClassInfo(GetModuleHandle(NULL),"vtkOutputWindow",&wndClass))
#endif
  {
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = vtkWin32OutputWindowWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.hInstance = GetModuleHandle(NULL);
#ifndef _WIN32_WCE
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
#endif
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
#ifdef UNICODE
    wndClass.lpszClassName = L"vtkOutputWindow";
#else
    wndClass.lpszClassName = "vtkOutputWindow";
#endif
    // vtk doesn't use these extra bytes, but app writers
    // may want them, so we provide them -- big enough for
    // one run time pointer: 4 bytes on 32-bit builds, 8 bytes
    // on 64-bit builds
    wndClass.cbWndExtra = sizeof(vtkLONG);
    RegisterClass(&wndClass);
    }

  // create parent container window
#ifdef _WIN32_WCE
  HWND win = CreateWindow(
    L"vtkOutputWindow", L"vtkOutputWindow",
    WS_OVERLAPPED | WS_CLIPCHILDREN,
    0, 0, 800, 512,
    NULL, NULL, GetModuleHandle(NULL), NULL);
#elif UNICODE
  HWND win = CreateWindow(
    L"vtkOutputWindow", L"vtkOutputWindow",
    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
    0, 0, 900, 700,
    NULL, NULL, GetModuleHandle(NULL), NULL);
#else
  HWND win = CreateWindow(
    "vtkOutputWindow", "vtkOutputWindow",
    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
    0, 0, 900, 700,
    NULL, NULL, GetModuleHandle(NULL), NULL);
#endif

  // Now create child window with text display box
  CREATESTRUCT lpParam;
  lpParam.hInstance = GetModuleHandle(NULL);
  lpParam.hMenu = NULL;
  lpParam.hwndParent = win;
  lpParam.cx = 900;
  lpParam.cy = 700;
  lpParam.x = 0;
  lpParam.y = 0;
#if defined(_WIN32_WCE) || defined(UNICODE)
  lpParam.lpszName = L"Output Control";
  lpParam.lpszClass = L"EDIT"; // use the RICHEDIT control widget
#else
  lpParam.lpszName = "Output Control";
  lpParam.lpszClass = "EDIT"; // use the RICHEDIT control widget
#endif

#ifdef _WIN32_WCE
  lpParam.style = ES_MULTILINE | ES_READONLY | WS_CHILD
    | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VISIBLE
    | WS_VSCROLL | WS_HSCROLL;
#else
  lpParam.style = ES_MULTILINE | ES_READONLY | WS_CHILD
    | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VISIBLE | WS_MAXIMIZE
    | WS_VSCROLL | WS_HSCROLL;
#endif

  lpParam.dwExStyle = 0;
  // Create the EDIT window as a child of win
#if defined(_WIN32_WCE) || defined(UNICODE)
  vtkWin32OutputWindowOutputWindow = CreateWindow(
    lpParam.lpszClass,  // pointer to registered class name
    L"", // pointer to window name
    lpParam.style,        // window style
    lpParam.x,                // horizontal position of window
    lpParam.y,                // vertical position of window
    lpParam.cx,           // window width
    lpParam.cy,          // window height
    lpParam.hwndParent,      // handle to parent or owner window
    NULL,          // handle to menu or child-window identifier
    lpParam.hInstance,     // handle to application instance
    &lpParam        // pointer to window-creation data
    );
#else
  vtkWin32OutputWindowOutputWindow = CreateWindow(
    lpParam.lpszClass,  // pointer to registered class name
    "", // pointer to window name
    lpParam.style,        // window style
    lpParam.x,                // horizontal position of window
    lpParam.y,                // vertical position of window
    lpParam.cx,           // window width
    lpParam.cy,          // window height
    lpParam.hwndParent,      // handle to parent or owner window
    NULL,          // handle to menu or child-window identifier
    lpParam.hInstance,     // handle to application instance
    &lpParam        // pointer to window-creation data
    );
#endif

  const int maxsize = 5242880;

#ifdef UNICODE
  SendMessageW(vtkWin32OutputWindowOutputWindow,
               EM_LIMITTEXT, maxsize, 0L);
#else
  SendMessageA(vtkWin32OutputWindowOutputWindow,
               EM_LIMITTEXT, maxsize, 0L);
#endif


  // show the top level container window
  ShowWindow(win, SW_SHOW);
  return 1;
}

//----------------------------------------------------------------------------
void vtkWin32OutputWindow::PromptText(const char* someText)
{
  char *vtkmsg = new char [strlen(someText) + 100];
  sprintf(vtkmsg,"%s\nPress Cancel to suppress any further messages.",
          someText);
#ifdef UNICODE
  wchar_t *wmsg = new wchar_t [mbstowcs(NULL, vtkmsg, 32000)+1];
  mbstowcs(wmsg, vtkmsg, 32000);
  if (MessageBox(NULL, wmsg, L"Error",
                 MB_ICONERROR | MB_OKCANCEL) == IDCANCEL)
    {
    vtkObject::GlobalWarningDisplayOff();
    }
  delete [] wmsg;
#else
  if (MessageBox(NULL, vtkmsg, "Error",
                 MB_ICONERROR | MB_OKCANCEL) == IDCANCEL)
    {
    vtkObject::GlobalWarningDisplayOff();
    }
#endif
  delete [] vtkmsg;
}

//----------------------------------------------------------------------------
void vtkWin32OutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (vtkWin32OutputWindowOutputWindow)
    {
    os << indent << "OutputWindow: "
       << vtkWin32OutputWindowOutputWindow << "\n";
    }
  else
    {
    os << indent << "OutputWindow: (null)\n";
    }

  os << indent << "SendToStdErr: " << this->SendToStdErr << "\n";
}
