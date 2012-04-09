/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxWinDevice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTDxWinDevice - Implementation of vtkTDxDevice on Windows
// .SECTION Description
// vtkTDxWinDevice is a concrete implementation of vtkTDxDevice on Windows
// It uses the COM API.
// .SECTION See Also
// vtkTDxDevice, vtkTDxWinDevice

#ifndef __vtkTDxWinDevice_h
#define __vtkTDxWinDevice_h

#include "vtkTDxDevice.h"

class vtkRenderWindowInteractor;

// including <WinDef.h> directly leads to the following error:
// "C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\winnt.h(81) :
// fatal error C1189: #error :  "No Target Architecture" "
// so we need to include <windows.h> instead.
#include <windows.h> // we need HWND from <WinDef.h>

class vtkTDxWinDevicePrivate;

class VTK_RENDERING_EXPORT vtkTDxWinDevice : public vtkTDxDevice
{
public:
  static vtkTDxWinDevice *New();
  vtkTypeMacro(vtkTDxWinDevice,vtkTDxDevice);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the Handle of the Window. Initial value is 0.
  HWND GetWindowHandle() const;
  
  // Description:
  // Set the handle of the Window.
  // \pre not_yet_initialized: !GetInitialized()
  void SetWindowHandle(HWND hWnd);

  // Description:
  // Initialize the device with the current display and window ids.
  // It updates the value of GetInitialized().
  // Initialization can fail (if the device is not present or the driver is
  // not running). You must look for the value of
  // GetInitialized() before processing further.
  // If the case initialization is successful, GetIsListening() is false. 
  // \pre not_yet_initialized: !GetInitialized()
  void Initialize();
  
  // Description:
  // See description in the superclass. Implementation for Windows.
  virtual void Close();

  // Description:
  // Tells if we are listening events on the device.
  bool GetIsListening() const;

  // Description:
  // Call it when the window has or get the focus
  // \pre initialized: GetInitialized()
  // \pre not_yet: !GetIsListening()
  void StartListening();
  
  // Description:
  // Call it when the window lose the focus.
  // \pre initialized: GetInitialized()
  // \pre is_listening: GetIsListening()
  void StopListening();
  
  // Description:
  // Process the 3DConnexion event.
  // Called internally by the timer.
  void ProcessEvent();

protected:
  // Description:
  // Default constructor.
  vtkTDxWinDevice();
  
  // Description:
  // Destructor. If the device is not initialized, do nothing. If the device
  // is initialized, close the device.
  virtual ~vtkTDxWinDevice();
  
  HWND WindowHandle;
  
  vtkTDxWinDevicePrivate *Private;
  bool IsListening;  
  
private:
  vtkTDxWinDevice(const vtkTDxWinDevice&);  // Not implemented.
  void operator=(const vtkTDxWinDevice&);  // Not implemented.
};

#endif
