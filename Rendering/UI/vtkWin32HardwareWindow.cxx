/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHardwareWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWin32HardwareWindow.h"

#include <assert.h>

#include "vtkObjectFactory.h"

//============================================================================
vtkStandardNewMacro(vtkWin32HardwareWindow);

//----------------------------------------------------------------------------
vtkWin32HardwareWindow::vtkWin32HardwareWindow()
  : ApplicationInstance(0)
  , ParentId(0)
  , WindowId(0)
{
}

//----------------------------------------------------------------------------
vtkWin32HardwareWindow::~vtkWin32HardwareWindow() {}

//----------------------------------------------------------------------------
void vtkWin32HardwareWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

HINSTANCE vtkWin32HardwareWindow::GetApplicationInstance()
{
  return this->ApplicationInstance;
}

HWND vtkWin32HardwareWindow::GetWindowId()
{
  return this->WindowId;
}

void vtkWin32HardwareWindow::SetDisplayId(void* arg)
{
  this->ApplicationInstance = (HINSTANCE)(arg);
}

void vtkWin32HardwareWindow::SetWindowId(void* arg)
{
  this->WindowId = (HWND)(arg);
}

void vtkWin32HardwareWindow::SetParentId(void* arg)
{
  this->ParentId = (HWND)(arg);
}

void* vtkWin32HardwareWindow::GetGenericDisplayId()
{
  return this->ApplicationInstance;
}

void* vtkWin32HardwareWindow::GetGenericWindowId()
{
  return this->WindowId;
}

void* vtkWin32HardwareWindow::GetGenericParentId()
{
  return this->ParentId;
}

// ----------------------------------------------------------------------------
namespace
{
void AdjustWindowRectForBorders(
  HWND hwnd, DWORD style, const int x, const int y, const int width, const int height, RECT& r)
{
  if (!style && hwnd)
  {
    style = GetWindowLong(hwnd, GWL_STYLE);
  }
  r.left = x;
  r.top = y;
  r.right = r.left + width;
  r.bottom = r.top + height;
  BOOL result = AdjustWindowRect(&r, style, FALSE);
  if (!result)
  {
    vtkGenericWarningMacro("AdjustWindowRect failed, error: " << GetLastError());
  }
}
}

void vtkWin32HardwareWindow::Create()
{
  // get the application instance if we don't have one already
  if (!this->ApplicationInstance)
  {
    // if we have a parent window get the app instance from it
    if (this->ParentId)
    {
      this->ApplicationInstance = (HINSTANCE)vtkGetWindowLong(this->ParentId, vtkGWL_HINSTANCE);
    }
    else
    {
      this->ApplicationInstance = GetModuleHandle(nullptr); /*AfxGetInstanceHandle();*/
    }
  }

  // has the class been registered ?
  WNDCLASS wndClass;
#ifdef UNICODE
  if (!GetClassInfo(this->ApplicationInstance, L"vtkOpenGL", &wndClass))
#else
  if (!GetClassInfo(this->ApplicationInstance, "vtkOpenGL", &wndClass))
#endif
  {
    wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wndClass.lpfnWndProc = DefWindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.hInstance = this->ApplicationInstance;
    wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = nullptr;
#ifdef UNICODE
    wndClass.lpszClassName = L"vtkOpenGL";
#else
    wndClass.lpszClassName = "vtkOpenGL";
#endif
    // vtk doesn't use the first extra vtkLONG's worth of bytes,
    // but app writers may want them, so we provide them. VTK
    // does use the second vtkLONG's worth of bytes of extra space.
    wndClass.cbWndExtra = 2 * sizeof(vtkLONG);
    RegisterClass(&wndClass);
  }

  if (!this->WindowId)
  {
    int x = ((this->Position[0] >= 0) ? this->Position[0] : 5);
    int y = ((this->Position[1] >= 0) ? this->Position[1] : 5);
    int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
    int width = ((this->Size[0] > 0) ? this->Size[0] : 300);

    /* create window */
    if (this->ParentId)
    {
#ifdef UNICODE
      this->WindowId =
        CreateWindow(L"vtkVulkan", wname, WS_CHILD | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/, x, y,
          width, height, this->ParentId, nullptr, this->ApplicationInstance, nullptr);
#else
      this->WindowId =
        CreateWindow("vtkVulkan", "VTK - Vulkan", WS_CHILD | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/,
          x, y, width, height, this->ParentId, nullptr, this->ApplicationInstance, nullptr);
#endif
    }
    else
    {
      DWORD style;
      if (this->Borders)
      {
        style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/;
      }
      else
      {
        style = WS_POPUP | WS_CLIPCHILDREN /*| WS_CLIPSIBLINGS*/;
      }
      RECT r;
      AdjustWindowRectForBorders(0, style, x, y, width, height, r);
#ifdef UNICODE
      this->WindowId = CreateWindow(L"vtkOpenGL", wname, style, x, y, r.right - r.left,
        r.bottom - r.top, nullptr, nullptr, this->ApplicationInstance, nullptr);
#else
      this->WindowId = CreateWindow("vtkOpenGL", "VTK - Vulkan", style, x, y, r.right - r.left,
        r.bottom - r.top, nullptr, nullptr, this->ApplicationInstance, nullptr);
#endif
    }
#ifdef UNICODE
    delete[] wname;
#endif

    if (!this->WindowId)
    {
      vtkGenericWarningMacro("Could not create window, error:  " << GetLastError());
      return;
    }
    // extract the create info

    /* display window */
    if (this->ShowWindow)
    {
      ::ShowWindow(this->WindowId, SW_SHOW);
    }
    // UpdateWindow(this->WindowId);
    // vtkSetWindowLong(this->WindowId, sizeof(vtkLONG), (intptr_t)this);
  }
}

void vtkWin32HardwareWindow::Destroy()
{
  ::DestroyWindow(this->WindowId); // windows api
  this->WindowId = 0;
}
