/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHardwareWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWin32HardwareWindow
 * @brief   represents a window in a windows GUI
 */

#ifndef vtkWin32HardwareWindow_h
#define vtkWin32HardwareWindow_h

#include "vtkHardwareWindow.h"
#include "vtkRenderingUIModule.h" // For export macro
#include "vtkWindows.h"           // For windows API

class VTKRENDERINGUI_EXPORT vtkWin32HardwareWindow : public vtkHardwareWindow
{
public:
  static vtkWin32HardwareWindow* New();
  vtkTypeMacro(vtkWin32HardwareWindow, vtkHardwareWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  HINSTANCE GetApplicationInstance();

  HWND GetWindowId();

  void Create() override;
  void Destroy() override;

  //@{
  /**
   * These are window system independent methods that are used
   * to help interface vtkWindow to native windowing systems.
   */
  void SetDisplayId(void*) override;
  void SetWindowId(void*) override;
  void SetParentId(void*) override;
  void* GetGenericDisplayId() override;
  void* GetGenericWindowId() override;
  void* GetGenericParentId() override;
  //@}

protected:
  vtkWin32HardwareWindow();
  ~vtkWin32HardwareWindow() override;

  HWND ParentId;
  HWND WindowId;
  HINSTANCE ApplicationInstance;

private:
  vtkWin32HardwareWindow(const vtkWin32HardwareWindow&) = delete;
  void operator=(const vtkWin32HardwareWindow&) = delete;
};

#endif
