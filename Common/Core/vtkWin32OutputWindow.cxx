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

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkWindows.h"

#include "vtksys/Encoding.hxx"

vtkStandardNewMacro(vtkWin32OutputWindow);

HWND vtkWin32OutputWindowOutputWindow = 0;

//------------------------------------------------------------------------------
LRESULT APIENTRY vtkWin32OutputWindowWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_SIZE:
    {
      int w = LOWORD(lParam); // width of client area
      int h = HIWORD(lParam); // height of client area

      MoveWindow(vtkWin32OutputWindowOutputWindow, 0, 0, w, h, true);
    }
    break;
    case WM_DESTROY:
      vtkWin32OutputWindowOutputWindow = nullptr;
      vtkObject::GlobalWarningDisplayOff();
      break;
    case WM_CREATE:
      break;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

//------------------------------------------------------------------------------
vtkWin32OutputWindow::vtkWin32OutputWindow()
{
  // Default to sending output to stderr/cerr when running a dashboard
  // and logging is not enabled.
  if (getenv("DART_TEST_FROM_DART") || getenv("DASHBOARD_TEST_FROM_CTEST"))
  {
    this->SetDisplayModeToDefault();
  }
  else
  {
    this->SetDisplayModeToNever();
  }
}

//------------------------------------------------------------------------------
vtkWin32OutputWindow::~vtkWin32OutputWindow() {}

//------------------------------------------------------------------------------
// Display text in the window, and translate the \n to \r\n.
//
void vtkWin32OutputWindow::DisplayText(const char* someText)
{
  if (!someText)
  {
    return;
  }
  if (this->PromptUser)
  {
    this->PromptText(someText);
    return;
  }

  const auto streamtype = this->GetDisplayStream(this->GetCurrentMessageType());

  // Create a buffer big enough to hold the entire text
  char* buffer = new char[strlen(someText) + 1];
  // Start at the beginning
  const char* NewLinePos = someText;
  while (NewLinePos)
  {
    int len = 0;
    // Find the next new line in text
    NewLinePos = strchr(someText, '\n');
    // if no new line is found then just add the text
    if (NewLinePos == 0)
    {
      this->AddText(someText);
      OutputDebugString(someText);
      switch (streamtype)
      {
        case StreamType::StdOutput:
          cout << someText;
          break;
        case StreamType::StdError:
          cerr << someText;
          break;
        default:
          break;
      }
    }
    // if a new line is found copy it to the buffer
    // and add the buffer with a control new line
    else
    {
      len = NewLinePos - someText;
      strncpy(buffer, someText, len);
      buffer[len] = 0;
      someText = NewLinePos + 1;
      this->AddText(buffer);
      this->AddText("\r\n");
      OutputDebugString(buffer);
      OutputDebugString("\r\n");
      switch (streamtype)
      {
        case StreamType::StdOutput:
          cout << buffer;
          cout << "\r\n";
          break;
        case StreamType::StdError:
          cerr << buffer;
          cerr << "\r\n";
          break;
        default:
          break;
      }
    }
  }
  delete[] buffer;
}

//------------------------------------------------------------------------------
// Add some text to the EDIT control.
//
void vtkWin32OutputWindow::AddText(const char* someText)
{
  if (!this->Initialize() || (strlen(someText) == 0))
  {
    return;
  }

  // move to the end of the text area
  SendMessageA(vtkWin32OutputWindowOutputWindow, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
  // Append the text to the control
  std::wstring wmsg = vtksys::Encoding::ToWide(someText);
  SendMessageW(vtkWin32OutputWindowOutputWindow, EM_REPLACESEL, 0, (LPARAM)wmsg.c_str());
}

//------------------------------------------------------------------------------
// initialize the output window with an EDIT control and
// a container window.
//
int vtkWin32OutputWindow::Initialize()
{
  // check to see if it is already initialized
  if (vtkWin32OutputWindowOutputWindow)
  {
    return 1;
  }

  // Initialize the output window
  // has the class been registered ?
  WNDCLASSA wndClass;
  if (!GetClassInfoA(GetModuleHandle(nullptr), "vtkOutputWindow", &wndClass))
  {
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = vtkWin32OutputWindowWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.hInstance = GetModuleHandle(nullptr);
    wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = nullptr;
    wndClass.lpszClassName = "vtkOutputWindow";
    // vtk doesn't use these extra bytes, but app writers
    // may want them, so we provide them -- big enough for
    // one run time pointer: 4 bytes on 32-bit builds, 8 bytes
    // on 64-bit builds
    wndClass.cbWndExtra = sizeof(vtkLONG);
    RegisterClassA(&wndClass);
  }

  // create parent container window
  int width = 900;
  int height = 700;
  std::wstring wtitle = vtksys::Encoding::ToWide(this->GetWindowTitle());
  HWND win =
    CreateWindowW(L"vtkOutputWindow", wtitle.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0, 0,
      width, height, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

  // Now create child window with text display box
  CREATESTRUCTA lpParam;
  lpParam.hInstance = GetModuleHandle(nullptr);
  lpParam.hMenu = nullptr;
  lpParam.hwndParent = win;
  lpParam.cx = width;
  lpParam.cy = height;
  lpParam.x = 0;
  lpParam.y = 0;
  lpParam.lpszName = "Output Control";
  lpParam.lpszClass = "EDIT"; // use the RICHEDIT control widget
  lpParam.style = ES_MULTILINE | ES_READONLY | WS_CHILD | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
    WS_VISIBLE | WS_MAXIMIZE | WS_VSCROLL | WS_HSCROLL;
  lpParam.dwExStyle = 0;

  // Create the EDIT window as a child of win
  vtkWin32OutputWindowOutputWindow =
    CreateWindowA(lpParam.lpszClass, // pointer to registered class name
      "",                            // pointer to window name
      lpParam.style,                 // window style
      lpParam.x,                     // horizontal position of window
      lpParam.y,                     // vertical position of window
      lpParam.cx,                    // window width
      lpParam.cy,                    // window height
      lpParam.hwndParent,            // handle to parent or owner window
      nullptr,                       // handle to menu or child-window identifier
      lpParam.hInstance,             // handle to application instance
      &lpParam                       // pointer to window-creation data
    );

  const int maxsize = 5242880;
  SendMessageA(vtkWin32OutputWindowOutputWindow, EM_LIMITTEXT, maxsize, 0L);

  // show the top level container window
  ShowWindow(win, SW_SHOW);
  return 1;
}

//------------------------------------------------------------------------------
void vtkWin32OutputWindow::PromptText(const char* someText)
{
  size_t vtkmsgsize = strlen(someText) + 100;
  char* vtkmsg = new char[vtkmsgsize];
  snprintf(vtkmsg, vtkmsgsize, "%s\nPress Cancel to suppress any further messages.", someText);
  std::wstring wmsg = vtksys::Encoding::ToWide(vtkmsg);
  if (MessageBoxW(nullptr, wmsg.c_str(), L"Error", MB_ICONERROR | MB_OKCANCEL) == IDCANCEL)
  {
    vtkObject::GlobalWarningDisplayOff();
  }
  delete[] vtkmsg;
}

//------------------------------------------------------------------------------
void vtkWin32OutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (vtkWin32OutputWindowOutputWindow)
  {
    os << indent << "OutputWindow: " << vtkWin32OutputWindowOutputWindow << "\n";
  }
  else
  {
    os << indent << "OutputWindow: (null)\n";
  }
}
