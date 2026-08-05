// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWin32OutputWindow.h"

#include "vtkObjectFactory.h"
#include "vtkStringFormatter.h"
#include "vtkWindows.h"

#include "vtksys/Encoding.hxx"

#include <condition_variable>
#include <mutex>
#include <thread>

#include <iostream>

VTK_ABI_NAMESPACE_BEGIN
#define WM_VTK_APPEND_TEXT (WM_USER + 1)

vtkStandardNewMacro(vtkWin32OutputWindow);

static std::mutex vtkWin32OutputWindowMutex;

static HWND vtkWin32OutputWindowOutputWindow = nullptr;
static std::thread vtkWin32OutputWindowUIThread;
static bool vtkWin32OutputWindowUIThreadReady = false;
static std::mutex vtkWin32OutputWindowUIThreadMutex;
static std::condition_variable vtkWin32OutputWindowUIThreadCV;

//------------------------------------------------------------------------------
LRESULT APIENTRY vtkWin32OutputWindowWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_SIZE:
    {
      int w = LOWORD(lParam); // width of client area
      int h = HIWORD(lParam); // height of client area

      if (!MoveWindow(vtkWin32OutputWindowOutputWindow, 0, 0, w, h, true))
      {
        auto errorCode = GetLastError();
        std::string errorMsg = std::system_category().message(errorCode);
        std::cerr << "MoveWindow failed in vtkWin32OutputWindowWndProc(), error(" << errorCode
                  << "): " << errorMsg << "\n";
      }
    }
    break;
    case WM_DESTROY:
      vtkWin32OutputWindowOutputWindow = nullptr;
      vtkObject::GlobalWarningDisplayOff();
      PostQuitMessage(0);
      break;
    case WM_CREATE:
      break;
    case WM_VTK_APPEND_TEXT:
    {
      wchar_t* text = (wchar_t*)lParam;
      if (vtkWin32OutputWindowOutputWindow)
      {
        SendMessageW(vtkWin32OutputWindowOutputWindow, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessageW(vtkWin32OutputWindowOutputWindow, EM_REPLACESEL, 0, (LPARAM)text);
      }
      free(text);
      return 0;
    }
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

//------------------------------------------------------------------------------
vtkWin32OutputWindow::vtkWin32OutputWindow()
{
  // Default to sending output to stderr/std::cerr when running a dashboard
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
  std::scoped_lock<std::mutex> lock(vtkWin32OutputWindowMutex);
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
          std::cout << someText;
          break;
        case StreamType::StdError:
          std::cerr << someText;
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
          std::cout << buffer;
          std::cout << "\r\n";
          break;
        case StreamType::StdError:
          std::cerr << buffer;
          std::cerr << "\r\n";
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

  // Heap-allocate the text and post to the UI thread for processing.
  // PostMessage is async and avoids cross-thread SendMessage deadlocks.
  std::wstring wmsg = vtksys::Encoding::ToWide(someText);
  wchar_t* heapText = _wcsdup(wmsg.c_str());
  if (!PostMessageW(
        GetParent(vtkWin32OutputWindowOutputWindow), WM_VTK_APPEND_TEXT, 0, (LPARAM)heapText))
  {
    free(heapText);
    auto errorCode = GetLastError();
    std::string errorMsg = std::system_category().message(errorCode);
    std::cerr << "PostMessageW failed in vtkWin32OutputWindow::AddText(), error(" << errorCode
              << "): " << errorMsg << "\n";
  }
}

//------------------------------------------------------------------------------
// UI thread entry point: creates the window and runs the message loop.
//
static void vtkWin32OutputWindowUIThreadFunc(std::string title, bool show)
{
  // Register the window class (on this thread)
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
    if (!RegisterClassA(&wndClass))
    {
      auto errorCode = GetLastError();
      std::string errorMsg = std::system_category().message(errorCode);
      std::cerr << "RegisterClassA failed in vtkWin32OutputWindowUIThreadFunc(), error("
                << errorCode << "): " << errorMsg << "\n";
      // Signal the calling thread so it doesn't wait forever
      {
        std::scoped_lock<std::mutex> lock(vtkWin32OutputWindowUIThreadMutex);
        vtkWin32OutputWindowUIThreadReady = true;
      }
      vtkWin32OutputWindowUIThreadCV.notify_one();
      return;
    }
  }

  // create parent container window
  int width = 900;
  int height = 700;
  std::wstring wtitle = vtksys::Encoding::ToWide(title);
  HWND win =
    CreateWindowW(L"vtkOutputWindow", wtitle.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0, 0,
      width, height, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
  if (!win)
  {
    auto errorCode = GetLastError();
    std::string errorMsg = std::system_category().message(errorCode);
    std::cerr << "CreateWindowW failed in vtkWin32OutputWindowUIThreadFunc(), error(" << errorCode
              << "): " << errorMsg << "\n";
    // Signal the calling thread so it doesn't wait forever
    {
      std::scoped_lock<std::mutex> lock(vtkWin32OutputWindowUIThreadMutex);
      vtkWin32OutputWindowUIThreadReady = true;
    }
    vtkWin32OutputWindowUIThreadCV.notify_one();
    return;
  }

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
  if (!vtkWin32OutputWindowOutputWindow)
  {
    auto errorCode = GetLastError();
    std::string errorMsg = std::system_category().message(errorCode);
    std::cerr << "CreateWindowA failed in vtkWin32OutputWindowUIThreadFunc(), error(" << errorCode
              << "): " << errorMsg << "\n";
    DestroyWindow(win);
    // Signal the calling thread so it doesn't wait forever
    {
      std::scoped_lock<std::mutex> lock(vtkWin32OutputWindowUIThreadMutex);
      vtkWin32OutputWindowUIThreadReady = true;
    }
    vtkWin32OutputWindowUIThreadCV.notify_one();
    return;
  }

  const int maxsize = 5242880;
  SendMessageA(vtkWin32OutputWindowOutputWindow, EM_LIMITTEXT, maxsize, 0L);

  ShowWindow(win, show ? SW_SHOW : SW_HIDE);

  // Signal the calling thread that we're ready
  {
    std::scoped_lock<std::mutex> lock(vtkWin32OutputWindowUIThreadMutex);
    vtkWin32OutputWindowUIThreadReady = true;
  }
  vtkWin32OutputWindowUIThreadCV.notify_one();

  // Run the message loop
  MSG msg;
  BOOL bRet;
  while ((bRet = GetMessage(&msg, nullptr, 0, 0)) != 0)
  {
    if (bRet == -1)
    {
      auto errorCode = GetLastError();
      std::string errorMsg = std::system_category().message(errorCode);
      std::cerr << "GetMessage failed in vtkWin32OutputWindowUIThreadFunc(), error(" << errorCode
                << "): " << errorMsg << "\n";
      break;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

//------------------------------------------------------------------------------
// initialize the output window by spawning a UI thread.
//
int vtkWin32OutputWindow::Initialize()
{
  // check to see if it is already initialized
  if (vtkWin32OutputWindowOutputWindow)
  {
    return 1;
  }

  // Spawn a dedicated UI thread that creates the window and runs a message loop.
  vtkWin32OutputWindowUIThreadReady = false;
  vtkWin32OutputWindowUIThread =
    std::thread(vtkWin32OutputWindowUIThreadFunc, this->GetWindowTitle(), this->ShowWindow);
  vtkWin32OutputWindowUIThread.detach();

  // Wait for the UI thread to finish creating the window
  {
    std::unique_lock<std::mutex> lock(vtkWin32OutputWindowUIThreadMutex);
    vtkWin32OutputWindowUIThreadCV.wait(lock, [] { return vtkWin32OutputWindowUIThreadReady; });
  }

  return vtkWin32OutputWindowOutputWindow != nullptr ? 1 : 0;
}

//------------------------------------------------------------------------------
void vtkWin32OutputWindow::PromptText(const char* someText)
{
  auto vtkmsg = vtk::format("{}\nPress Cancel to suppress any further messages.", someText);
  std::wstring wmsg = vtksys::Encoding::ToWide(vtkmsg);
  const auto messageType = this->GetCurrentMessageType();
  if (messageType == MESSAGE_TYPE_ERROR)
  {
    if (MessageBoxW(nullptr, wmsg.c_str(), L"Error", MB_ICONERROR | MB_OKCANCEL) == IDCANCEL)
    {
      vtkObject::GlobalWarningDisplayOff();
    }
  }
  else if (messageType == MESSAGE_TYPE_TEXT || messageType == MESSAGE_TYPE_DEBUG)
  {
    if (MessageBoxW(nullptr, wmsg.c_str(), L"Information", MB_ICONINFORMATION | MB_OKCANCEL) ==
      IDCANCEL)
    {
      vtkObject::GlobalWarningDisplayOff();
    }
  }
  else if (messageType == MESSAGE_TYPE_GENERIC_WARNING || messageType == MESSAGE_TYPE_WARNING)
  {
    if (MessageBoxW(nullptr, wmsg.c_str(), L"Warning", MB_ICONWARNING | MB_OKCANCEL) == IDCANCEL)
    {
      vtkObject::GlobalWarningDisplayOff();
    }
  }
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
VTK_ABI_NAMESPACE_END
